/*
 * Pillar 3: Write Safely — Lock Less, Validate More
 *
 * Scenario: An ECO handler needs to update the release status on a filtered
 * set of FCU revisions (only the ones that passed validation) — not the
 * entire selected set.
 *
 * The unsafe version locks everything up front, validates after locking,
 * and leaves locks held on objects that failed validation. Under load,
 * this is exactly the pattern that causes deadlocks and stuck workflows.
 */

/* ---------------------------------------------------------------------
 * UNSAFE: lock first, validate after, locks left hanging on failures
 * --------------------------------------------------------------------- */
int update_fcu_status_unsafe(tag_t *fcu_revisions, int count, char *new_status)
{
    int status;

    for (int i = 0; i < count; i++)
    {
        /* locks every object before knowing if it should even be touched */
        AOM_lock(fcu_revisions[i]);

        if (!validate_fcu_ready(fcu_revisions[i]))
        {
            continue; /* validation failed — lock never released */
        }

        AOM_set_value_string(fcu_revisions[i], "release_status", new_status);
        AOM_save(fcu_revisions[i]);
        /* unlock is missing here in the failure path above,
           and only reached for objects that passed */
        AOM_unlock(fcu_revisions[i]);
    }

    return ITK_ok;
}

/* ---------------------------------------------------------------------
 * SAFE: validate first, lock only what will actually be written,
 * unlock immediately after save, on every path
 * --------------------------------------------------------------------- */
int update_fcu_status_safe(tag_t *fcu_revisions, int count, char *new_status)
{
    int status;

    for (int i = 0; i < count; i++)
    {
        /* validate BEFORE acquiring any lock */
        if (!validate_fcu_ready(fcu_revisions[i]))
        {
            continue; /* nothing was locked, nothing to clean up */
        }

        status = AOM_lock(fcu_revisions[i]);
        if (status != ITK_ok)
        {
            continue; /* couldn't lock — move on, don't retry in a loop */
        }

        status = AOM_set_value_string(fcu_revisions[i], "release_status", new_status);
        if (status == ITK_ok)
        {
            AOM_save(fcu_revisions[i]);
        }

        AOM_unlock(fcu_revisions[i]); /* always unlock, pass or fail */
    }

    return ITK_ok;
}

/*
 * Rule: locking too many objects causes deadlocks and workflow freezes.
 * Validate before you lock. Lock only what you're actually changing.
 * Unlock on every code path, including the failure paths.
 */
