/*
 * Pillar 4: Use the Right API for the Right Job
 *
 * Scenario: Find every released AHU (Air Handling Unit) item in the
 * database for an audit report.
 *
 * Manual traversal — walking every AHU assembly, every child, checking
 * status one object at a time — does not scale past a few hundred items.
 * A query does the filtering on the server side, in one call.
 */

/* ---------------------------------------------------------------------
 * SLOW: manual iteration over every AHU item, checking status one by one
 * --------------------------------------------------------------------- */
int find_released_ahus_slow(tag_t *all_ahu_items, int count, tag_t **released_out, int *released_count)
{
    tag_t *released = (tag_t *)MEM_alloc(count * sizeof(tag_t));
    int found = 0;
    char *status_val = NULL;

    for (int i = 0; i < count; i++)
    {
        AOM_ask_value_string(all_ahu_items[i], "release_status_list", &status_val);
        if (status_val && strcmp(status_val, "Released") == 0)
        {
            released[found++] = all_ahu_items[i];
        }
        MEM_free(status_val);
    }

    *released_out = released;
    *released_count = found;
    return ITK_ok;
}

/* ---------------------------------------------------------------------
 * OPTIMIZED: a saved query does the filtering server-side, one call
 * --------------------------------------------------------------------- */
int find_released_ahus_optimized(tag_t **released_out, int *released_count)
{
    tag_t query_tag;
    tag_t *entries = NULL;
    int entry_count = 0;
    int status;

    status = QRY_find2("AHU_Released_Items"); /* saved query, pre-built */
    if (status != ITK_ok)
    {
        return status;
    }

    /* bind query parameters, e.g. item_type = "AHU", status = "Released" */
    status = QRY_execute(query_tag, &entry_count, &entries);
    if (status != ITK_ok)
    {
        return status;
    }

    *released_out = entries;
    *released_count = entry_count;
    return ITK_ok;
}

/*
 * API selection guide used in this repo:
 *   AOM   -> properties, save, lock/unlock
 *   GRM   -> relations (BOM lines, references)
 *   WSO   -> workspace object-level operations
 *   Query -> searching / filtering — replaces manual looping
 *   POM   -> low-level access, use sparingly, last resort only
 *
 * Rule: if you're looping through 10,000 objects, you probably needed
 * a query.
 */
