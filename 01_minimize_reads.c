/*
 * Pillar 1: Minimize Reads — Never Fetch the World
 *
 * Scenario: A handler needs the object_name of every FCU (Fan Coil Unit)
 * revision under a selected product line, to build a quick reference list
 * for a release report.
 *
 * The slow version reads one property at a time, per object — every call
 * is a server round-trip. The optimized version asks for the same property
 * across the whole list in one call.
 */

/* ---------------------------------------------------------------------
 * SLOW: one AOM_ask_value_string call per FCU revision (N round-trips)
 * --------------------------------------------------------------------- */
int report_fcu_names_slow(tag_t *fcu_revisions, int count)
{
    char *name = NULL;
    int status;

    for (int i = 0; i < count; i++)
    {
        status = AOM_ask_value_string(fcu_revisions[i], "object_name", &name);
        if (status != ITK_ok)
        {
            continue; /* skip on error, but still N calls made */
        }
        printf("FCU: %s\n", name);
        MEM_free(name);
    }
    return ITK_ok;
}

/* ---------------------------------------------------------------------
 * OPTIMIZED: one AOM_ask_values call for the entire list (1 round-trip)
 * --------------------------------------------------------------------- */
int report_fcu_names_optimized(tag_t *fcu_revisions, int count)
{
    char **names = NULL;
    int status;

    status = AOM_ask_values(count, fcu_revisions, "object_name", &names);
    if (status != ITK_ok)
    {
        return status;
    }

    for (int i = 0; i < count; i++)
    {
        printf("FCU: %s\n", names[i]);
    }

    MEM_free(names); /* free the batch result once */
    return ITK_ok;
}

/*
 * Result: for a 500-unit FCU product line, this is the difference between
 * 500 server round-trips and 1. On a loaded server with 200 concurrent
 * users, that difference is the whole ballgame — it's the gap between a
 * report that runs in under a second and one that times out.
 *
 * Rule: if you read 200 properties but use 12, your code is the problem.
 * Ask for exactly what the downstream logic consumes, and ask for it once.
 */
