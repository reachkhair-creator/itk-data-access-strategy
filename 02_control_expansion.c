/*
 * Pillar 2: Control Object Expansion
 *
 * Scenario: A handler needs to check the material spec of duct components
 * directly under an AHU (Air Handling Unit) sub-assembly. It does NOT need
 * the full multi-level structure down to fasteners and gaskets.
 *
 * Unbounded recursive GRM traversal on an AHU assembly can pull in
 * thousands of relation lines — most of which are irrelevant to this task.
 */

/* ---------------------------------------------------------------------
 * SLOW: recursive traversal with no depth limit
 * --------------------------------------------------------------------- */
int check_duct_material_slow(tag_t ahu_assembly)
{
    tag_t *related = NULL;
    int related_count = 0;
    int status;

    /* Pulls the ENTIRE downstream structure — sheet metal, fasteners,
       gaskets, insulation, everything — regardless of what's needed. */
    status = GRM_list_all_secondary_objects(ahu_assembly, "IMAN_BOMView", &related_count, &related);
    if (status != ITK_ok)
    {
        return status;
    }

    for (int i = 0; i < related_count; i++)
    {
        /* recurses into every child, every level, no cutoff */
        check_duct_material_slow(related[i]);
    }

    MEM_free(related);
    return ITK_ok;
}

/* ---------------------------------------------------------------------
 * OPTIMIZED: bounded traversal, stop at the level the task needs
 * --------------------------------------------------------------------- */
int check_duct_material_optimized(tag_t ahu_assembly, int max_depth)
{
    tag_t *related = NULL;
    int related_count = 0;
    int status;

    if (max_depth <= 0)
    {
        return ITK_ok; /* depth cap reached — stop expanding */
    }

    status = GRM_list_secondary_objects(ahu_assembly, "IMAN_BOMView", &related_count, &related);
    if (status != ITK_ok)
    {
        return status;
    }

    for (int i = 0; i < related_count; i++)
    {
        char *item_type = NULL;
        AOM_ask_value_string(related[i], "object_type", &item_type);

        /* Only descend into duct-relevant subtypes, skip the rest */
        if (item_type && strcmp(item_type, "DuctComponent") == 0)
        {
            check_duct_material_optimized(related[i], max_depth - 1);
        }
        MEM_free(item_type);
    }

    MEM_free(related);
    return ITK_ok;
}

/*
 * Even better: if the real goal is "find all duct components with a given
 * material spec," a saved query against the classification/material
 * attribute replaces manual traversal entirely — see
 * 04_right_api.c for that pattern.
 *
 * Rule: expanding a BOM in ITK is expensive — expand only what you need.
 */
