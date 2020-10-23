#include "ik/algorithm.h"
#include "ik/bone.h"
#include "ik/meta_solvers.h"
#include "ik/solver.h"
#include "ik/subtree.h"
#include "ik/log.h"
#include "cstructures/btree.h"
#include "cstructures/vector.h"
#include <string.h>

static struct cs_vector g_solvers;

/* ------------------------------------------------------------------------- */
static void
deinit_solver(struct ik_solver* solver)
{
    solver->impl.deinit((struct ik_solver*)solver);
    IK_DECREF(solver->algorithm);
    IK_DECREF(solver->root_bone);
}

/* ------------------------------------------------------------------------- */
static void
destroy_solver(struct ik_solver* solver)
{
    deinit_solver(solver);
    ik_refcounted_obj_free((struct ik_refcounted*)solver);
}

/* ------------------------------------------------------------------------- */
static struct ik_solver*
create_solver(struct ik_algorithm* algorithm, struct ik_subtree* subtree, struct ik_bone* root)
{
    VECTOR_FOR_EACH(&g_solvers, struct ik_solver_interface*, p_iface)
        if (strcmp((*p_iface)->name, algorithm->type) == 0)
        {
            struct ik_solver_interface* iface = *p_iface;
            struct ik_solver* solver = (struct ik_solver*)
                ik_refcounted_alloc(iface->size, (ik_deinit_func)deinit_solver);
            if (solver == NULL)
            {
                ik_log_out_of_memory("create_solver()");
                return NULL;
            }

            solver->impl = *iface;
            solver->algorithm = algorithm;
            solver->root_bone = root;

            if (solver->impl.init((struct ik_solver*)solver, subtree) != 0)
            {
                ik_refcounted_obj_free((struct ik_refcounted*)solver);
                return NULL;
            }

            IK_INCREF(algorithm);
            IK_INCREF(root);

            return solver;
        }
    VECTOR_END_EACH

    ik_log_printf(IK_ERROR, "Unknown algorithm \"%s\". failed to allocate solver", algorithm->type);
    return NULL;
}

/* ------------------------------------------------------------------------- */
int
ik_solver_init_builtin_interfaces()
{
#define X(name) extern const struct ik_solver_interface ik_solver_##name;
    IK_ALGORITHM_LIST
#undef X

    vector_init(&g_solvers, sizeof(struct ik_solver_interface*));

#define X(name) \
    if (ik_solver_register(&ik_solver_##name) != 0) \
        goto register_solver_failed;
    IK_ALGORITHM_LIST
#undef X

    return 0;

    register_solver_failed : vector_deinit(&g_solvers);
    return -1;
}

/* ------------------------------------------------------------------------- */
void
ik_solver_deinit_builtin_interfaces(void)
{
    vector_deinit(&g_solvers);
}

/* ------------------------------------------------------------------------- */
int
ik_solver_register(const struct ik_solver_interface* interface)
{
    VECTOR_FOR_EACH(&g_solvers, struct ik_solver_interface*, p_iface)
        if (strcmp((*p_iface)->name, interface->name) == 0)
        {
            ik_log_printf(IK_ERROR, "Solver with name `%s` already registered!", interface->name);
            return -1;
        }
    VECTOR_END_EACH

    if (vector_push(&g_solvers, &interface) != 0)
    {
        ik_log_out_of_memory("ik_solver_register()");
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */
int
ik_solver_unregister(const struct ik_solver_interface* interface)
{
    VECTOR_FOR_EACH(&g_solvers, struct ik_solver_interface*, p_iface)
        if (strcmp((*p_iface)->name, interface->name) == 0)
        {
            vector_erase_element(&g_solvers, p_iface);
            return 0;
        }
    VECTOR_END_EACH

    ik_log_printf(IK_ERROR, "Solver with name `%s` not found!", interface->name);
    return -1;
}

/* ------------------------------------------------------------------------- */
static int
find_all_effector_bones(struct cs_vector* result, const struct ik_bone* bone)
{
    BONE_FOR_EACH_CHILD(bone, child)
        if (find_all_effector_bones(result, child) != 0)
            return -1;
    BONE_END_EACH

    if (bone->effector != NULL)
        if (vector_push(result, &bone) != 0)
            return -1;

    return 0;
}

/* ------------------------------------------------------------------------- */
static int
color_chain_recurse(struct cs_btree* colors,
                    const struct ik_bone* root,
                    const struct ik_bone* bone,
                    int chain_length,
                    int* color_counter,
                    int* chain_color)
{
    /* If chain joins up with any colored part of the tree, adopt that color */
    if (*chain_color == -1)
    {
        int* existing_color = btree_find(colors, (cs_btree_key)bone);
        if (existing_color)
            *chain_color = *existing_color;
    }

#define is_chain_end() (                                                      \
    (ik_bone_get_parent(bone) == root) ||                                     \
    (chain_length == 1)                ||                                     \
    (bone->parent->effector != NULL))

    /* Recurse */
    if (!is_chain_end())
    {
        if (color_chain_recurse(colors, root, ik_bone_get_parent(bone), chain_length - 1, color_counter, chain_color) != 0)
            return -1;
    }

    /*
     * End of chain reached. If it did not join with anything, create a new
     * color
     */
    if (*chain_color == -1)
    {
        *chain_color = *color_counter;
        (*color_counter)++;
    }

    switch (btree_insert_new(colors, (cs_btree_key)bone, chain_color))
    {
        case BTREE_EXISTS:
        case BTREE_OK:
            break;

        default:
            ik_log_out_of_memory("color_chain_recurse()");
            return -1;
    }

    return 0;
}
static int
color_chain(struct cs_btree* colors,
            const struct ik_bone* root,
            const struct ik_bone* bone,
            int chain_length,
            int* color_counter)
{
    int chain_color = -1;

    /* Have to handle this special case here because recursive function shits
     * itself otherwise */
    if (bone == root)
        return 0;
    return color_chain_recurse(colors, root, bone, chain_length, color_counter, &chain_color);
}
static int
color_reachable_segments(struct cs_btree* colors,
                         const struct cs_vector* effector_bones,
                         const struct ik_bone* root)
{
    int color_counter = 0;

    /*
     * Iterate the chain of bones starting at each effector bone and ending
     * at the specified chain length of the effector, mark every bone on the
     * way.
     */
    VECTOR_FOR_EACH(effector_bones, const struct ik_bone*, p_effector_bone)
        const struct ik_bone* bone = *p_effector_bone;
        const struct ik_effector* effector = bone->effector;
        if (color_chain(colors, root, bone, (int)effector->chain_length, &color_counter) != 0)
            return -1;
    VECTOR_END_EACH

    return 0;
}

/* ------------------------------------------------------------------------- */
static struct ik_solver*
find_algorithm_and_create_solver(struct ik_subtree* subtree, struct ik_bone* root)
{
    const struct ik_bone* bone;
    struct ik_algorithm* algorithm;

    algorithm = NULL;
    for (bone = subtree->root; bone != ik_bone_get_parent(bone); bone = ik_bone_get_parent(bone))
        if (bone->algorithm != NULL)
        {
            algorithm = bone->algorithm;
            break;
        }
    if (algorithm == NULL)
    {
        ik_log_printf(IK_WARN, "Found bones that are influenced by end-effectors, but couldn't find an attached algorithm. This subtree will be ignored.");
        return NULL;
    }

    return create_solver(algorithm, subtree, root);
}

/* ------------------------------------------------------------------------- */
static int
create_solver_for_each_subtree_recurse(struct cs_vector* solver_list,
                                       struct ik_bone* root,
                                       struct ik_bone* bone,
                                       struct ik_subtree* current_subtree,
                                       int parent_color,
                                       const struct cs_btree* segment_colors)
{
    struct cs_vector combined_solvers;

    vector_init(&combined_solvers, sizeof(struct ik_solver*));

    /* If the parent subtree exists, and there is an effector on this
     * segment, then this is a leaf bone of the parent subtree */
    if (parent_color != -1 && bone->effector != NULL)
    {
        assert(current_subtree != NULL);
        if (subtree_add_leaf(current_subtree, bone) != 0)
            goto create_solvers_failed;
    }

    BONE_FOR_EACH_CHILD(bone, child)
        const int* segment_color = btree_find(segment_colors, (cs_btree_key)child);

        /* Sanity check: If bone was added as leaf, then the first if-statement
         * (recursing on an existing subtree) must not be true */
        if (parent_color != -1 && bone->effector != NULL)
        {
            assert(segment_color == NULL || *segment_color != parent_color);
        }

        if (segment_color && *segment_color == parent_color)
        {
            assert(current_subtree);
            if (create_solver_for_each_subtree_recurse(solver_list, root, child, current_subtree, *segment_color, segment_colors) == -1)
                goto create_solvers_failed;
        }
        else if (segment_color)
        {
            struct ik_solver* solver;
            struct ik_subtree subtree;

            /* Begin new subtree */
            subtree_init(&subtree);
            subtree_set_root(&subtree, bone);
            if (create_solver_for_each_subtree_recurse(solver_list, root, child, &subtree, *segment_color, segment_colors) != 0)
                goto recurse_failed;

            if (subtree_leaves(&subtree) == 0)
                goto subtree_empty;

            solver = find_algorithm_and_create_solver(&subtree, root);
            if (solver == NULL)
                goto new_solver_failed;

            if (vector_push(&combined_solvers, &solver) != 0)
                goto push_new_solver_failed;

            new_solver_failed :
            subtree_empty :
            subtree_deinit(&subtree);
            continue;

            push_new_solver_failed : destroy_solver(solver);
            recurse_failed         : subtree_deinit(&subtree);
            goto create_solvers_failed;
        }
        else
        {
            if (create_solver_for_each_subtree_recurse(solver_list, root, child, NULL, -1, segment_colors) != 0)
                goto create_solvers_failed;
        }
    BONE_END_EACH

    if (vector_count(&combined_solvers) > 0)
    {
        if (vector_count(&combined_solvers) > 1 || ik_bone_child_count(bone) > 1)
        {
            struct ik_solver* solver = NULL; /*ik_solver_combine_create(&combined_solvers, bone);*/
            if (solver == NULL)
                goto create_solvers_failed;
            if (vector_push(solver_list, &solver) != 0)
            {
                destroy_solver(solver);
                goto create_solvers_failed;
            }
        }
        else
        {
            struct ik_solver* solver = *(struct ik_solver**)vector_back(&combined_solvers);
            if (vector_push(solver_list, &solver) != 0)
                goto create_solvers_failed;
        }
    }

    vector_deinit(&combined_solvers);
    return 0;

    create_solvers_failed : VECTOR_FOR_EACH(&combined_solvers, struct ik_solver*, psolver)
                            destroy_solver(*psolver);
                            VECTOR_END_EACH
                            vector_deinit(&combined_solvers);
                            return -1;
}
static int
create_solver_for_each_subtree(struct cs_vector* solver_list,
                               struct ik_bone* root,
                               const struct cs_btree* segment_colors)
{
    return create_solver_for_each_subtree_recurse(solver_list, root, root, NULL, -1, segment_colors);
}

/* ------------------------------------------------------------------------- */
struct ik_solver*
ik_solver_build(struct ik_bone* root)
{
    struct cs_vector effector_bones;
    struct cs_btree bone_colors;
    struct cs_vector solver_list;
    struct ik_solver* solver;

    /* Create a list of all bones that have effectors attached */
    vector_init(&effector_bones, sizeof(struct ik_bone*));
    if (find_all_effector_bones(&effector_bones, root) != 0)
        goto find_effectors_failed;

    /* May not have to do anything if none were found */
    if (vector_count(&effector_bones) == 0)
    {
        ik_log_printf(IK_WARN, "No effectors were found in the tree. No solvers were created.");
        goto find_effectors_failed;
    }

    /* Mark all bones that the effectors can reach */
    btree_init(&bone_colors, sizeof(int));
    if (color_reachable_segments(&bone_colors, &effector_bones, root) != 0)
        goto color_reachable_segments_failed;

    /*
     * It's possible that chain length limits end up isolating parts of the
     * tree, splitting it into a list of "sub-trees" which must be solved
     * in a particular order (root to leaf).
     */
    vector_init(&solver_list, sizeof(struct ik_solver*));
    if (create_solver_for_each_subtree(&solver_list, root, &bone_colors) != 0)
        goto split_into_subtrees_failed;

    if (vector_count(&solver_list) == 0)
    {
        solver = NULL;
        vector_deinit(&solver_list);
    }
    else if (vector_count(&solver_list) == 1)
    {
        solver = *(struct ik_solver**)vector_back(&solver_list);
        vector_deinit(&solver_list);
    }
    else
    {
        vector_reverse(&solver_list);
        solver = ik_solver_group_create(&solver_list);
        if (solver == NULL)
            goto create_meta_solver_failed;
        vector_deinit(&solver_list);
    }

    btree_deinit(&bone_colors);
    vector_deinit(&effector_bones);

    return solver;

    create_meta_solver_failed      :
    split_into_subtrees_failed     : VECTOR_FOR_EACH(&solver_list, struct ik_solver*, psolver)
                                         destroy_solver(*psolver);
                                     VECTOR_END_EACH
                                     vector_deinit(&solver_list);
    color_reachable_segments_failed              : btree_deinit(&bone_colors);
    find_effectors_failed          : vector_deinit(&effector_bones);
    return NULL;
}

/* ------------------------------------------------------------------------- */
int
ik_solver_solve(struct ik_solver* solver)
{
    return solver->impl.solve(solver);
}
