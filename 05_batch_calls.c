/*
 * Pillar 5: Avoid Repeated Server Calls
 *
 * Scenario: A naming-convention pass needs to check and, where necessary,
 * correct the object_name of every duct component in a BOM against a
 * naming standard (e.g. "DUCT-<size>-<gauge>").
 *
 * Every ITK call is a server round-trip. This example shows the same
 * task done as N+N calls (read one, write one, repeat) vs batched reads
 * and writes.
 */

/* ---------------------------------------------------------------------
 * SLOW: read and write inside the same per-object loop
 * --------------------------------------------------------------------- */
int fix_duct_names_slow(tag_t *duct_components, int count)
{
    char *name = NULL;
    char corrected[256];

    for (int i = 0; i < count; i++)
    {
        AOM_ask_value_string(duct_components[i], "object_name", &name); /* round-trip 1 */

        if (!matches_naming_standard(name))
        {
            build_corrected_name(name, corrected);
            AOM_lock(duct_components[i]);                                       /* round-trip 2 */
            AOM_set_value_string(duct_components[i], "object_name", corrected); /* round-trip 3 */
            AOM_save(duct_components[i]);                                       /* round-trip 4 */
            AOM_unlock(duct_components[i]);                                     /* round-trip 5 */
        }
        MEM_free(name);
    }
    return ITK_ok; /* up to 5 round-trips per object */
}

/* ---------------------------------------------------------------------
 * OPTIMIZED: batch the reads, only touch objects that actually need it,
 * batch the writes for that reduced set
 * --------------------------------------------------------------------- */
int fix_duct_names_optimized(tag_t *duct_components, int count)
{
    char **names = NULL;
    int status;

    /* one call for every current name */
    status = AOM_ask_values(count, duct_components, "object_name", &names);
    if (status != ITK_ok)
    {
        return status;
    }

    /* build the reduced set of objects that actually need correction —
       no server calls in this pass, it's all local comparison */
    tag_t *to_fix = (tag_t *)MEM_alloc(count * sizeof(tag_t));
    char **corrected_names = (char **)MEM_alloc(count * sizeof(char *));
    int fix_count = 0;

    for (int i = 0; i < count; i++)
    {
        if (!matches_naming_standard(names[i]))
        {
            to_fix[fix_count] = duct_components[i];
            corrected_names[fix_count] = build_corrected_name_alloc(names[i]);
            fix_count++;
        }
    }

    if (fix_count > 0)
    {
        AOM_lock_list(fix_count, to_fix);                                  /* one batched lock */
        AOM_set_values(fix_count, to_fix, "object_name", corrected_names); /* one batched write */
        AOM_save_list(fix_count, to_fix);                                  /* one batched save */
        AOM_unlock_list(fix_count, to_fix);                                /* one batched unlock */
    }

    MEM_free(names);
    MEM_free(to_fix);
    MEM_free(corrected_names);
    return ITK_ok; /* 5 total round-trips for the whole batch, not per object */
}

/*
 * For a 1,000-component duct BOM with 40 names out of standard, the slow
 * version makes roughly 1,000 read calls plus up to 200 write-side calls
 * for the corrections. The optimized version makes 5 calls, period.
 *
 * Rule: a slow handler is usually a handler making too many calls, not
 * one doing heavy computation.
 */
