#include "gmock/gmock.h"
#include "ik/algorithm.h"
#include "ik/chain_tree.h"
#include "ik/cpputils.hpp"
#include "ik/bone.h"
#include "ik/solver.h"

#define NAME solver_build

using namespace ::testing;

struct ik_solver_dummy
{
    IK_SOLVER_HEAD

    struct ik_chain chain_tree;
};

struct ik_solver_group
{
    IK_SOLVER_HEAD

    struct cs_vector subsolvers;
};

struct ik_solver_combine
{
    IK_SOLVER_HEAD

    struct ik_solver** subsolvers;
    struct ik_bone** child_bones;
    struct ik_bone* shared_bone;
    union ik_quat* child_rotations;

    int subsolver_count;
    int child_bone_count;
};

static int dummy_init(struct ik_solver* solver_base, const struct ik_subtree* subtree) {
    struct ik_solver_dummy* solver = (struct ik_solver_dummy*)solver_base;
    chain_tree_init(&solver->chain_tree);
    return chain_tree_build(&solver->chain_tree, subtree);
}
static void dummy_deinit(struct ik_solver* solver_base) {
    struct ik_solver_dummy* solver = (struct ik_solver_dummy*)solver_base;
    chain_tree_deinit(&solver->chain_tree);
}
static int  dummy_solve(struct ik_solver* solver_base) { return 1; }

const struct ik_solver_interface ik_solver_DUMMY1 = {
    "dummy1",
    sizeof(struct ik_solver_dummy),
    dummy_init,
    dummy_deinit,
    dummy_solve
};
const struct ik_solver_interface ik_solver_DUMMY2 = {
    "dummy2",
    sizeof(struct ik_solver_dummy),
    dummy_init,
    dummy_deinit,
    dummy_solve
};
const struct ik_solver_interface ik_solver_DUMMY3 = {
    "dummy3",
    sizeof(struct ik_solver_dummy),
    dummy_init,
    dummy_deinit,
    dummy_solve
};

class NAME : public Test
{
public:
    ik_bone* tree_without_effectors()
    {
        ik_bone* tree = ik_bone_create();          tree->user_data = (void*)0;
        ik_bone* n1 = ik_bone_create_child(tree);  n1->user_data = (void*)1;
        ik_bone* n2 = ik_bone_create_child(n1);    n2->user_data = (void*)2;
        ik_bone* n3 = ik_bone_create_child(n2);    n3->user_data = (void*)3;
        ik_bone* n4 = ik_bone_create_child(n3);    n4->user_data = (void*)4;
        ik_bone* n5 = ik_bone_create_child(n4);    n5->user_data = (void*)5;
        ik_bone* n6 = ik_bone_create_child(n5);    n6->user_data = (void*)6;
        ik_bone* n7 = ik_bone_create_child(n3);    n7->user_data = (void*)7;
        ik_bone* n8 = ik_bone_create_child(n7);    n8->user_data = (void*)8;
        ik_bone* n9 = ik_bone_create_child(n8);    n9->user_data = (void*)9;
        return tree;
    }

    ik_bone* tree_with_two_effectors()
    {
        ik_bone *tree, *n6, *n9;
        tree = tree_without_effectors();
        n6 = ik_bone_find(tree, (void*)6);
        n9 = ik_bone_find(tree, (void*)9);

        ik_bone_create_effector(n6);
        ik_bone_create_effector(n9);
        ik_bone_create_algorithm(tree, "dummy1");

        return tree;
    }

    ik_bone* tree_with_two_effectors_and_no_algorithms()
    {
        ik_bone *tree, *n6, *n9;
        tree = tree_without_effectors();
        n6 = ik_bone_find(tree, (void*)6);
        n9 = ik_bone_find(tree, (void*)9);

        ik_bone_create_effector(n6);
        ik_bone_create_effector(n9);

        return tree;
    }

    ik_bone* tree_llr()
    {

        /*
         * e1 -> 6       8 <- e2
         *        \     /
         *         5   7
         *          \ /
         *    b2 ->  4      10 <- e3
         *            \     /
         *             3   9
         *              \ /
         *               2  <- b1
         *               |
         *               1
         *               |
         *               0
         */
        ik_bone* tree = ik_bone_create();
        ik_bone* b1 = ik_bone_create_child(tree);
        b1 = ik_bone_create_child(b1);
        ik_bone* b2 = ik_bone_create_child(b1);
        b2 = ik_bone_create_child(b2);
        ik_bone* e1 = ik_bone_create_child(b2);
        e1 = ik_bone_create_child(e1);
        ik_bone* e2 = ik_bone_create_child(b2);
        e2 = ik_bone_create_child(e2);
        ik_bone* e3 = ik_bone_create_child(b1);
        e3 = ik_bone_create_child(e3);

        ik_bone_create_effector(e1);
        ik_bone_create_effector(e2);
        ik_bone_create_effector(e3);

        return tree;
    }

    ik_bone* tree_lrr()
    {
        /*
         *     e2 -> 8       10 <- e3
         *            \     /
         *             7   9
         *              \ /
         * e1 -> 4       6 <- b2
         *        \     /
         *         3   5
         *          \ /
         *           2  <- b1
         *           |
         *           1
         *           |
         *           0
         */
        ik_bone* tree = ik_bone_create();
        ik_bone* b1 = ik_bone_create_child(tree);
        b1 = ik_bone_create_child(b1);
        ik_bone* e1 = ik_bone_create_child(b1);
        e1 = ik_bone_create_child(e1);
        ik_bone* b2 = ik_bone_create_child(b1);
        b2 = ik_bone_create_child(b2);
        ik_bone* e2 = ik_bone_create_child(b2);
        e2 = ik_bone_create_child(e2);
        ik_bone* e3 = ik_bone_create_child(b2);
        e3 = ik_bone_create_child(e3);

        ik_bone_create_effector(e1);
        ik_bone_create_effector(e2);
        ik_bone_create_effector(e3);

        return tree;
    }

    virtual void SetUp() override
    {
        ik_solver_register(&ik_solver_DUMMY1);
        ik_solver_register(&ik_solver_DUMMY2);
        ik_solver_register(&ik_solver_DUMMY3);
    }

    virtual void TearDown() override
    {
        ik_solver_unregister(&ik_solver_DUMMY3);
        ik_solver_unregister(&ik_solver_DUMMY2);
        ik_solver_unregister(&ik_solver_DUMMY1);
    }
};

TEST_F(NAME, no_action_if_tree_has_no_effectors_or_algorithms)
{
    ik::Ref<ik_bone> tree = tree_without_effectors();
    ik::Ref<ik_solver> solver = ik_solver_build(tree);
    EXPECT_THAT(solver.isNull(), IsTrue());
}

TEST_F(NAME, no_action_if_tree_has_no_algorithms)
{
    ik::Ref<ik_bone> tree = tree_without_effectors();
    ik::Ref<ik_bone> n6 = ik_bone_find(tree, (void*)6);
    ik::Ref<ik_bone> n9 = ik_bone_find(tree, (void*)9);

    ik_effector* eff1 = ik_bone_create_effector(n6);
    ik_effector* eff2 = ik_bone_create_effector(n9);

    ik::Ref<ik_solver> solver = ik_solver_build(tree);
    EXPECT_THAT(solver.isNull(), IsTrue());
}

TEST_F(NAME, check_refcounts_are_correct)
{
    // TODO
    ik::Ref<ik_bone> tree = tree_with_two_effectors();
    ik::Ref<ik_solver> solver = ik_solver_build(tree);
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->algorithm, NotNull());
    EXPECT_THAT(solver->algorithm->type, StrEq("dummy1"));

    EXPECT_THAT(solver->refcount->refs, Eq(1));
}

TEST_F(NAME, always_find_root_most_algorithm)
{
    ik::Ref<ik_bone> tree = tree_with_two_effectors_and_no_algorithms();
    ik::Ref<ik_bone> n2 = ik_bone_find(tree, (void*)2);

    ik_algorithm* a1 = ik_bone_create_algorithm(tree, "dummy1");
    ik_algorithm* a2 = ik_bone_create_algorithm(n2, "dummy2");

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    // e1 -> 6           9 <- e2
    //        \         /
    //         5       8
    //          \     /
    //           4   7
    //            \ /
    //             3
    //             |
    //             2 <- a2
    //             |
    //             1
    //             |
    //             0 <- a1
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->algorithm, NotNull());
    EXPECT_THAT(solver->algorithm, Eq(a1));
}

TEST_F(NAME, choose_next_available_algorithm_after_chain_ends_exact)
{
    ik::Ref<ik_bone> tree = tree_with_two_effectors_and_no_algorithms();

    ik::Ref<ik_bone> n2 = ik_bone_find(tree, (void*)2);
    ik::Ref<ik_bone> n3 = ik_bone_find(tree, (void*)3);
    ik::Ref<ik_bone> n6 = ik_bone_find(tree, (void*)6);
    ik::Ref<ik_bone> n9 = ik_bone_find(tree, (void*)9);

    ik_algorithm* a1 = ik_bone_create_algorithm(tree, "dummy1");
    ik_algorithm* a2 = ik_bone_create_algorithm(n3, "dummy2");

    n6->effector->chain_length = 3;
    n9->effector->chain_length = 3;

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    // e1 -> 6           9 <- e2
    //        \         /
    //         5       8
    //          \     /
    //           4   7
    //            \ /
    //             3 <- a2, chains end here
    //             |
    //             2
    //             |
    //             1
    //             |
    //             0 <- a1
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->impl.name, StrEq("combine"));
    ASSERT_THAT(((ik_solver_combine*)solver.get())->subsolver_count, Eq(2));

    // We expect algorithm 2 to be chosen because it is the next available
    // one after the chains end
    ik::Ref<ik_solver> s1 = ((ik_solver_combine*)solver.get())->subsolvers[0];
    ASSERT_THAT(s1->algorithm, NotNull());
    EXPECT_THAT(s1->algorithm, Eq(a2));
    ik::Ref<ik_solver> s2 = ((ik_solver_combine*)solver.get())->subsolvers[1];
    ASSERT_THAT(s2->algorithm, NotNull());
    EXPECT_THAT(s2->algorithm, Eq(a2));
}

TEST_F(NAME, choose_next_available_algorithm_after_chain_ends)
{
    ik::Ref<ik_bone> tree = tree_with_two_effectors_and_no_algorithms();

    ik::Ref<ik_bone> n2 = ik_bone_find(tree, (void*)2);
    ik::Ref<ik_bone> n4 = ik_bone_find(tree, (void*)4);
    ik::Ref<ik_bone> n6 = ik_bone_find(tree, (void*)6);
    ik::Ref<ik_bone> n9 = ik_bone_find(tree, (void*)9);

    ik_algorithm* a1 = ik_bone_create_algorithm(tree, "dummy1");
    ik_algorithm* a2 = ik_bone_create_algorithm(n4, "dummy2");

    n6->effector->chain_length = 2;
    n9->effector->chain_length = 2;

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    // e1 -> 6           9 <- e2
    //        \         /
    //         5       8
    //          \     /
    //     a2 -> 4   7 <- chains end here
    //            \ /
    //             3
    //             |
    //             2
    //             |
    //             1
    //             |
    //             0 <- a1
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->impl.name, StrEq("group"));
    ASSERT_THAT(vector_count(&((ik_solver_group*)solver.get())->subsolvers), Eq(2));

    // We expect algorithm 1 to be chosen because it is the next available
    // one after the chains end
    ik::Ref<ik_solver> s1 = *(ik_solver**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 0);
    ASSERT_THAT(s1->impl.name, StrEq("dummy1"));
    ASSERT_THAT(s1->algorithm, NotNull());
    EXPECT_THAT(s1->algorithm, Eq(a1));

    // Other solver's chain ends on algorithm 2 so it should be chosen
    ik::Ref<ik_solver> s2 = *(ik_solver**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 1);
    ASSERT_THAT(s2->impl.name, StrEq("dummy2"));
    ASSERT_THAT(s2->algorithm, NotNull());
    EXPECT_THAT(s2->algorithm, Eq(a2));
}

TEST_F(NAME, ignore_effector_on_root_bone)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);

    ik_effector* e1 = ik_bone_create_effector(tree);
    ik_effector* e2 = ik_bone_create_effector(n2);

    ik_algorithm* a1 = ik_bone_create_algorithm(tree, "dummy1");

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    //  2 <- e2
    //  |
    //  1
    //  |
    //  0 <- e1, a1
    //
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    EXPECT_THAT(solver->algorithm, Eq(a1));
}

TEST_F(NAME, split_trees_on_effectors)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n3);
    ik::Ref<ik_bone> n5 = ik_bone_create_child(n4);
    ik::Ref<ik_bone> n6 = ik_bone_create_child(n5);

    ik_effector* e1 = ik_bone_create_effector(n2);
    ik_effector* e2 = ik_bone_create_effector(n3);
    ik_effector* e3 = ik_bone_create_effector(n5);
    ik_algorithm* a = ik_bone_create_algorithm(tree, "dummy1");
    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    //       6
    //       |
    //       5 <- e3
    //       |
    //       4
    //       |
    //       3 <- e2
    //       |
    //       2 <- e1
    //       |
    //       1
    //       |
    //       0 <- a
    //
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->impl.name, StrEq("group"));
    ASSERT_THAT(vector_count(&((ik_solver_group*)solver.get())->subsolvers), Eq(3));

    ik_solver_dummy* s1 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 0);
    ik_solver_dummy* s2 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 1);
    ik_solver_dummy* s3 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 2);

    EXPECT_THAT(s1->algorithm, Eq(a));
    EXPECT_THAT(chain_bone_count(&s1->chain_tree), Eq(3));
    EXPECT_THAT(chain_get_base_bone(&s1->chain_tree), Eq(tree));
    EXPECT_THAT(chain_get_tip_bone(&s1->chain_tree), Eq(n2));

    EXPECT_THAT(s2->algorithm, Eq(a));
    EXPECT_THAT(chain_bone_count(&s2->chain_tree), Eq(2));
    EXPECT_THAT(chain_get_base_bone(&s2->chain_tree), Eq(n2));
    EXPECT_THAT(chain_get_tip_bone(&s2->chain_tree), Eq(n3));

    EXPECT_THAT(s3->algorithm, Eq(a));
    EXPECT_THAT(chain_bone_count(&s3->chain_tree), Eq(3));
    EXPECT_THAT(chain_get_base_bone(&s3->chain_tree), Eq(n3));
    EXPECT_THAT(chain_get_tip_bone(&s3->chain_tree), Eq(n5));
}

TEST_F(NAME, split_trees_on_effectors_and_choose_new_algorithm)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n3);
    ik::Ref<ik_bone> n5 = ik_bone_create_child(n4);
    ik::Ref<ik_bone> n6 = ik_bone_create_child(n5);

    ik_effector* e1 = ik_bone_create_effector(n2);
    ik_effector* e2 = ik_bone_create_effector(n3);
    ik_effector* e3 = ik_bone_create_effector(n5);

    ik_algorithm* a1 = ik_bone_create_algorithm(n3, "dummy1");
    ik_algorithm* a2 = ik_bone_create_algorithm(n2, "dummy2");
    ik_algorithm* a3 = ik_bone_create_algorithm(tree, "dummy3");

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    //       6
    //       |
    //       5 <- e3
    //       |
    //       4
    //       |
    //       3 <- e2, a1
    //       |
    //       2 <- e1, a2
    //       |
    //       1
    //       |
    //       0 <- a3
    //
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->impl.name, StrEq("group"));
    ASSERT_THAT(vector_count(&((ik_solver_group*)solver.get())->subsolvers), Eq(3));

    ik_solver_dummy* s1 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 0);
    ik_solver_dummy* s2 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 1);
    ik_solver_dummy* s3 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 2);

    EXPECT_THAT(s1->algorithm, Eq(a3));
    EXPECT_THAT(chain_bone_count(&s1->chain_tree), Eq(3));
    EXPECT_THAT(chain_get_base_bone(&s1->chain_tree), Eq(tree));
    EXPECT_THAT(chain_get_tip_bone(&s1->chain_tree), Eq(n2));

    EXPECT_THAT(s2->algorithm, Eq(a2));
    EXPECT_THAT(chain_bone_count(&s2->chain_tree), Eq(2));
    EXPECT_THAT(chain_get_base_bone(&s2->chain_tree), Eq(n2));
    EXPECT_THAT(chain_get_tip_bone(&s2->chain_tree), Eq(n3));

    EXPECT_THAT(s3->algorithm, Eq(a1));
    EXPECT_THAT(chain_bone_count(&s3->chain_tree), Eq(3));
    EXPECT_THAT(chain_get_base_bone(&s3->chain_tree), Eq(n3));
    EXPECT_THAT(chain_get_tip_bone(&s3->chain_tree), Eq(n5));
}

TEST_F(NAME, missing_root_algorithm)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n3);
    ik::Ref<ik_bone> n5 = ik_bone_create_child(n4);
    ik::Ref<ik_bone> n6 = ik_bone_create_child(n5);

    ik_effector* e1 = ik_bone_create_effector(n2);
    ik_effector* e2 = ik_bone_create_effector(n3);
    ik_effector* e3 = ik_bone_create_effector(n5);

    ik_algorithm* a1 = ik_bone_create_algorithm(n3, "dummy1");
    ik_algorithm* a2 = ik_bone_create_algorithm(n2, "dummy2");

    ik::Ref<ik_solver> solver = ik_solver_build(tree);

    //
    //       6
    //       |
    //       5 <- e3
    //       |
    //       4
    //       |
    //       3 <- e2, a1
    //       |
    //       2 <- e1, a2
    //       |
    //       1
    //       |
    //       0
    //
    //
    ASSERT_THAT(solver.isNull(), IsFalse());
    ASSERT_THAT(solver->impl.name, StrEq("group"));
    ASSERT_THAT(vector_count(&((ik_solver_group*)solver.get())->subsolvers), Eq(2));

    ik_solver_dummy* s1 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 0);
    ik_solver_dummy* s2 = *(ik_solver_dummy**)vector_get_element(&((ik_solver_group*)solver.get())->subsolvers, 1);

    EXPECT_THAT(s1->algorithm, Eq(a2));
    EXPECT_THAT(chain_bone_count(&s1->chain_tree), Eq(2));
    EXPECT_THAT(chain_get_base_bone(&s1->chain_tree), Eq(n2));
    EXPECT_THAT(chain_get_tip_bone(&s1->chain_tree), Eq(n3));

    EXPECT_THAT(s2->algorithm, Eq(a1));
    EXPECT_THAT(chain_bone_count(&s2->chain_tree), Eq(3));
    EXPECT_THAT(chain_get_base_bone(&s2->chain_tree), Eq(n3));
    EXPECT_THAT(chain_get_tip_bone(&s2->chain_tree), Eq(n5));
}

TEST_F(NAME, ignore_parents_of_root_bone)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);

    ik_effector* e = ik_bone_create_effector(n3);
    ik_algorithm* a = ik_bone_create_algorithm(tree, "dummy1");

    ik::Ref<ik_solver> s1 = ik_solver_build(tree);
    ik::Ref<ik_solver> s2 = ik_solver_build(n1);
    ik::Ref<ik_solver> s3 = ik_solver_build(n2);

    //
    //      3 <- e
    //      |
    //      2
    //      |
    //      1
    //      |
    //      0 <- a
    //
    ASSERT_THAT(s1.isNull(), IsFalse());
    ASSERT_THAT(s1->impl.name, StrEq("dummy1"));
    ASSERT_THAT(s2.isNull(), IsTrue());
    ASSERT_THAT(s3.isNull(), IsTrue());
}

TEST_F(NAME, chain_ending_in_middle_of_second_chain_creates_two_solvers)
{
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n1);

    ik_effector* e1 = ik_bone_create_effector(n3);
    ik_effector* e2 = ik_bone_create_effector(n4);
    e2->chain_length = 1;
    ik_algorithm* a = ik_bone_create_algorithm(tree, "dummy1");

    ik::Ref<ik_solver> s = ik_solver_build(tree);

    //
    //      3 <- e1
    //      |
    //      2  4 <- e2 (chain_length=1)
    //      | /
    //      1
    //      |
    //      0 <- a
    //
    ASSERT_THAT(s.isNull(), IsFalse());
    ASSERT_THAT(s->impl.name, StrEq("group"));
    ASSERT_THAT(vector_count(&((ik_solver_group*)s.get())->subsolvers), Eq(2));

    ik_solver_dummy*   s1 = *(ik_solver_dummy**)  vector_get_element(&((ik_solver_group*)s.get())->subsolvers, 0);
    ik_solver_combine* s2 = *(ik_solver_combine**)vector_get_element(&((ik_solver_group*)s.get())->subsolvers, 1);
    ASSERT_THAT(s2->impl.name, StrEq("combine"));
    ASSERT_THAT(s2->subsolver_count, Eq(1));
    ik_solver_dummy*   s3 = (ik_solver_dummy*)s2->subsolvers[0];

    EXPECT_THAT(s1->algorithm, Eq(a));
    EXPECT_THAT(chain_bone_count(&s1->chain_tree), Eq(4));
    EXPECT_THAT(chain_get_base_bone(&s1->chain_tree), Eq(tree));
    EXPECT_THAT(chain_get_tip_bone(&s1->chain_tree), Eq(n3));
    EXPECT_THAT(chain_child_count(&s1->chain_tree), Eq(0));

    EXPECT_THAT(s2->shared_bone, Eq(n1));
    ASSERT_THAT(s2->child_bone_count, Eq(2));
    EXPECT_THAT(s2->child_bones[0], Eq(n2));
    EXPECT_THAT(s2->child_bones[1], Eq(n4));

    EXPECT_THAT(s3->algorithm, Eq(a));
    EXPECT_THAT(chain_bone_count(&s3->chain_tree), Eq(2));
    EXPECT_THAT(chain_get_base_bone(&s3->chain_tree), Eq(n1));
    EXPECT_THAT(chain_get_tip_bone(&s3->chain_tree), Eq(n4));
    EXPECT_THAT(chain_child_count(&s3->chain_tree), Eq(0));
}

/*
TEST_F(NAME, split_tree_can_be_flattened_multiple_times)
{
    ik_bone *tree, *n1, *n2, *dead3, *n4, *n5, *dead6, *dead7, *dead8, *dead9, *dead10, *dead11, *dead12, *dead13;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(    tree,  ik_guid(1));
    ik::Ref<ik_bone> n2 = ik_bone_create_child(    n1,    ik_guid(2));
    ik::Ref<ik_bone> dead3 = ik_bone_create_child( n2,    ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child(    dead3, ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_create_child(    n4,    ik_guid(5));
    ik::Ref<ik_bone> dead6 = ik_bone_create_child( n5,    ik_guid(6));
    ik::Ref<ik_bone> dead7 = ik_bone_create_child( tree,  ik_guid(7));
    ik::Ref<ik_bone> dead8 = ik_bone_create_child( n1,    ik_guid(8));
    ik::Ref<ik_bone> dead9 = ik_bone_create_child( n2,    ik_guid(9));
    ik::Ref<ik_bone> dead10 = ik_bone_create_child(dead3);
    ik::Ref<ik_bone> dead11 = ik_bone_create_child(n4,    ik_guid(11));
    ik::Ref<ik_bone> dead12 = ik_bone_create_child(n5,    ik_guid(12));
    ik::Ref<ik_bone> dead13 = ik_bone_create_child(dead6);

    ik_effector *e1, *e2;
    ik_bone_create_effector(&e1, n2);
    ik_bone_create_effector(&e2, n5);
    e2->chain_length = 1;

    //
    //   13--6 <- dead
    //       |
    //   12--5 <- e2
    //       |
    //   11--4
    //       |
    //   10--3 <- dead
    //       |
    //    9--2 <- e1
    //       |
    //    8--1
    //       |
    //    7--0
    //
    //

    ik_solver solver1;
    ik_solver_init(&solver1);
    ASSERT_THAT(ik_solver_update(&solver1, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver1.solver_list), Eq(2));
    ik::Ref<ik_solver> j1solver1 = *(ik_solver**)vector_get_element(&solver1.solver_list, 0);
    ik::Ref<ik_solver> j1solver2 = *(ik_solver**)vector_get_element(&solver1.solver_list, 1);
    EXPECT_THAT(j1solver1->ndv.bone_data->refcount->refs, Eq(7));  // bones 0,1,2,4,5 and solver's solver1->ndv,solver2->ndv are holding a ref

    ik_solver solver2;
    ik_solver_init(&solver2);
    ASSERT_THAT(ik_solver_update(&solver2, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver2.solver_list), Eq(2));
    ik::Ref<ik_solver> j2solver1 = *(ik_solver**)vector_get_element(&solver2.solver_list, 0);
    ik::Ref<ik_solver> j2solver2 = *(ik_solver**)vector_get_element(&solver2.solver_list, 1);
    EXPECT_THAT(j2solver1->ndv.bone_data->refcount->refs, Eq(7));  // bones 0,1,2,4,5 and solver's solver1->ndv,solver2->ndv are holding a ref

    ik_solver solver3;
    ik_solver_init(&solver3);
    ASSERT_THAT(ik_solver_update(&solver3, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver3.solver_list), Eq(2));
    ik::Ref<ik_solver> j3solver1 = *(ik_solver**)vector_get_element(&solver3.solver_list, 0);
    ik::Ref<ik_solver> j3solver2 = *(ik_solver**)vector_get_element(&solver3.solver_list, 1);
    EXPECT_THAT(j3solver1->ndv.bone_data->refcount->refs, Eq(7));  // bones 0,1,2,4,5 and solver's solver1->ndv,solver2->ndv are holding a ref

    // The newly created flattened bone data should be the one pointing to
    // the original tree bone data
    EXPECT_THAT(j1solver1->ndv.bone_data, Ne(tree->d));
    EXPECT_THAT(j2solver1->ndv.bone_data, Ne(tree->d));
    EXPECT_THAT(j3solver1->ndv.bone_data, Eq(tree->d));
    EXPECT_THAT(j1solver2->ndv.bone_data, Ne(tree->d));
    EXPECT_THAT(j2solver2->ndv.bone_data, Ne(tree->d));
    EXPECT_THAT(j3solver2->ndv.bone_data, Eq(tree->d));

    EXPECT_THAT(j1solver1->ndv.bone_data->refcount->refs, Eq(2));  // nvd1,nvd2 are holding refs
    EXPECT_THAT(j2solver1->ndv.bone_data->refcount->refs, Eq(2));  // nvd1,nvd2 are holding refs
    EXPECT_THAT(j3solver1->ndv.bone_data->refcount->refs, Eq(7));  // bones 0,1,2,4,5 and nvd1,nvd2 are holding refs
    ik_bone_free_recursive(tree);
    EXPECT_THAT(j1solver1->ndv.bone_data->refcount->refs, Eq(2));  // nvd1,nvd2 are holding refs
    EXPECT_THAT(j2solver1->ndv.bone_data->refcount->refs, Eq(2));  // nvd1,nvd2 are holding refs
    EXPECT_THAT(j3solver1->ndv.bone_data->refcount->refs, Eq(2));  // nvd1,nvd2 are holding refs
    ik_solver_deinit(&solver1);
    EXPECT_THAT(j2solver1->ndv.bone_data->refcount->refs, Eq(2));
    EXPECT_THAT(j3solver1->ndv.bone_data->refcount->refs, Eq(2));
    ik_solver_deinit(&solver2);
    EXPECT_THAT(j3solver1->ndv.bone_data->refcount->refs, Eq(2));
    ik_solver_deinit(&solver3);
}

TEST_F(NAME, split_trees_on_effectors_with_chain_lengths)
{
    ik_solver solver;
    ik_bone *tree, *n1, *n2, *n3, *n4, *n5, *n6;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child( n1,  ik_guid(2));
    ik::Ref<ik_bone> n3 = ik_bone_create_child( n2,  ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child( n3,  ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_create_child( n4,  ik_guid(5));
    ik::Ref<ik_bone> n6 = ik_bone_create_child( n5,  ik_guid(6));

    ik_effector *e1, *e2, *e3;
    ik_bone_create_effector(&e1, n2);
    ik_bone_create_effector(&e2, n3);
    ik_bone_create_effector(&e3, n5);
    e3->chain_length = 1;

    //
    //       6
    //       |
    //       5 <- e3 (chain length = 1)
    //       |
    //       4
    //       |
    //       3 <- e2
    //       |
    //       2 <- e1
    //       |
    //       1
    //       |
    //       0
    //
    //

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(3));

    ik_solver *solver1, *solver2, *solver3;
    solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    solver3 = *(ik_solver**)vector_get_element(&solver.solver_list, 2);

    EXPECT_THAT(solver1->ndv.bone_data, Eq(tree->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n1->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n2->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n3->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n4->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n5->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(n6->d));  // Should point to different bone data
    EXPECT_THAT(solver1->ndv.bone_data->bone_count, Eq(6));  // Nodes 0-5

    // Nodes 0,1,2 should be in first bone data view
    EXPECT_THAT(solver1->ndv.subbase_idx, Eq(0));
    EXPECT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    EXPECT_THAT(solver1->ndv.chain_end_idx, Eq(3));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));

    // Nodes 2,3 should be in the second bone data view
    EXPECT_THAT(solver2->ndv.subbase_idx, Eq(2));
    EXPECT_THAT(solver2->ndv.chain_begin_idx, Eq(3));
    EXPECT_THAT(solver2->ndv.chain_end_idx, Eq(4));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n2)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n3)));

    // Nodes 4,5 should be in the third bone data view
    EXPECT_THAT(solver3->ndv.subbase_idx, Eq(4));
    EXPECT_THAT(solver3->ndv.chain_begin_idx, Eq(5));
    EXPECT_THAT(solver3->ndv.chain_end_idx, Eq(6));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n5)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, ignore_effector_on_root_bone_with_dead_bones)
{
    ik_solver solver;
    ik_bone *tree, *n1, *n2, *n3, *n4;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1,  ik_guid(2));
    ik::Ref<ik_bone> n3 = ik_bone_create_child(tree,  ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n1,  ik_guid(4));

    ik_effector *e1, *e2;
    ik_bone_create_effector(&e1, tree);
    ik_bone_create_effector(&e2, n2);

    //
    //  4 2 <- e2
    //   \|
    //  3 1
    //   \|
    //    0 <- e1
    //
    //

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(1));

    ik::Ref<ik_solver> solver = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    EXPECT_THAT(solver->ndv.bone_data->bone_count, Eq(3));
    EXPECT_THAT(solver->ndv.subbase_idx, Eq(0));
    EXPECT_THAT(solver->ndv.chain_begin_idx, Eq(1));
    EXPECT_THAT(solver->ndv.chain_end_idx, Eq(3));
    EXPECT_THAT(solver->ndv.bone_data, Eq(tree->d));
    EXPECT_THAT(solver->ndv.bone_data, Eq(n1->d));
    EXPECT_THAT(solver->ndv.bone_data, Eq(n2->d));
    EXPECT_THAT(solver->ndv.bone_data, Ne(n3->d));
    EXPECT_THAT(solver->ndv.bone_data, Ne(n4->d));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, split_trees_on_effectors_with_dead_bones)
{
    ik_solver solver;
    ik_bone *tree, *n1, *n2, *dead3, *n4, *n5, *dead6, *dead7, *dead8, *dead9, *dead10, *dead11, *dead12, *dead13;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(    tree,  ik_guid(1));
    ik::Ref<ik_bone> n2 = ik_bone_create_child(    n1,    ik_guid(2));
    ik::Ref<ik_bone> dead3 = ik_bone_create_child( n2,    ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child(    dead3, ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_create_child(    n4,    ik_guid(5));
    ik::Ref<ik_bone> dead6 = ik_bone_create_child( n5,    ik_guid(6));
    ik::Ref<ik_bone> dead7 = ik_bone_create_child( tree,  ik_guid(7));
    ik::Ref<ik_bone> dead8 = ik_bone_create_child( n1,    ik_guid(8));
    ik::Ref<ik_bone> dead9 = ik_bone_create_child( n2,    ik_guid(9));
    ik::Ref<ik_bone> dead10 = ik_bone_create_child(dead3);
    ik::Ref<ik_bone> dead11 = ik_bone_create_child(n4,    ik_guid(11));
    ik::Ref<ik_bone> dead12 = ik_bone_create_child(n5,    ik_guid(12));
    ik::Ref<ik_bone> dead13 = ik_bone_create_child(dead6);

    ik_effector *e1, *e2;
    ik_bone_create_effector(&e1, n2);
    ik_bone_create_effector(&e2, n5);
    e2->chain_length = 1;

    //
    //   13--6 <- dead
    //       |
    //   12--5 <- e2
    //       |
    //   11--4
    //       |
    //   10--3 <- dead
    //       |
    //    9--2 <- e1
    //       |
    //    8--1
    //       |
    //    7--0
    //
    //

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(2));

    ik_solver *solver1, *solver2;
    solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);

    // Make sure bone data was created properly
    EXPECT_THAT(solver1->ndv.bone_data, Eq(tree->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n1->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n2->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead3->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n4->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n5->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead6->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead7->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead8->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead9->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead10->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead11->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead12->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead13->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(solver2->ndv.bone_data));
    EXPECT_THAT(solver1->ndv.bone_data->bone_count, Eq(5));

    // Nodes 0,1,2 should be in first bone data view
    EXPECT_THAT(solver1->ndv.subbase_idx, Eq(0));
    EXPECT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    EXPECT_THAT(solver1->ndv.chain_end_idx, Eq(3));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));

    // Nodes 4,5 should be in the second bone data view
    EXPECT_THAT(solver2->ndv.subbase_idx, Eq(3));
    EXPECT_THAT(solver2->ndv.chain_begin_idx, Eq(4));
    EXPECT_THAT(solver2->ndv.chain_end_idx, Eq(5));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n5)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, split_trees_on_effectors_with_chain_lengths_with_dead_bones)
{
    ik_solver solver;
    ik_bone *tree, *n1, *n2, *n3, *n4, *n5, *dead6, *dead7, *dead8, *dead9, *dead10, *dead11, *dead12, *dead13;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1,  ik_guid(2));
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2,  ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n3,  ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_create_child(n4,  ik_guid(5));
    ik::Ref<ik_bone> dead6 = ik_bone_create_child(n5,  ik_guid(6));
    ik::Ref<ik_bone> dead7 = ik_bone_create_child(tree,  ik_guid(7));
    ik::Ref<ik_bone> dead8 = ik_bone_create_child(n1,  ik_guid(8));
    ik::Ref<ik_bone> dead9 = ik_bone_create_child(n2,  ik_guid(9));
    ik::Ref<ik_bone> dead10 = ik_bone_create_child(n3,  ik_guid(10));
    ik::Ref<ik_bone> dead11 = ik_bone_create_child(n4,  ik_guid(11));
    ik::Ref<ik_bone> dead12 = ik_bone_create_child(n5,  ik_guid(12));
    ik::Ref<ik_bone> dead13 = ik_bone_create_child(dead6,  ik_guid(13));

    ik_effector *e1, *e2, *e3;
    ik_bone_create_effector(&e1, n2);
    ik_bone_create_effector(&e2, n3);
    ik_bone_create_effector(&e3, n5);
    e3->chain_length = 1;

    //
    //   13--6
    //       |
    //   12--5 <- e3 (chain length = 1)
    //       |
    //   11--4
    //       |
    //   10--3 <- e2
    //       |
    //    9--2 <- e1
    //       |
    //    8--1
    //       |
    //    7--0
    //
    //

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(3));

    ik_solver *solver1, *solver2, *solver3;
    solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    solver3 = *(ik_solver**)vector_get_element(&solver.solver_list, 2);

    // Make sure bone data was created properly
    EXPECT_THAT(solver1->ndv.bone_data, Eq(tree->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n1->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n2->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n3->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n4->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(n5->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead6->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead7->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead8->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead9->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead10->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead11->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead12->d));
    EXPECT_THAT(solver1->ndv.bone_data, Ne(dead13->d));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(solver2->ndv.bone_data));
    EXPECT_THAT(solver1->ndv.bone_data, Eq(solver3->ndv.bone_data));
    EXPECT_THAT(solver1->ndv.bone_data->bone_count, Eq(6));

    // Nodes 0,1,2 should be in first bone data view
    EXPECT_THAT(solver1->ndv.subbase_idx, Eq(0));
    EXPECT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    EXPECT_THAT(solver1->ndv.chain_end_idx, Eq(3));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));

    // Nodes 2,3 should be in the second bone data view
    EXPECT_THAT(solver2->ndv.subbase_idx, Eq(2));
    EXPECT_THAT(solver2->ndv.chain_begin_idx, Eq(3));
    EXPECT_THAT(solver2->ndv.chain_end_idx, Eq(4));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n2)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n3)));

    // Nodes 4,5 should be in the third bone data view
    EXPECT_THAT(solver3->ndv.subbase_idx, Eq(4));
    EXPECT_THAT(solver3->ndv.chain_begin_idx, Eq(5));
    EXPECT_THAT(solver3->ndv.chain_end_idx, Eq(6));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n5)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, split_trees_with_dead_bones)
{
    ik_bone *tree, *n1, *n2, *n3, *n4, *n5, *n6;
    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child(n1);
    ik::Ref<ik_bone> n3 = ik_bone_create_child(n2);
    ik::Ref<ik_bone> n4 = ik_bone_create_child(n3);
    ik::Ref<ik_bone> n5 = ik_bone_create_child(n4);
    ik::Ref<ik_bone> n6 = ik_bone_create_child(n5);

    ik_effector *e1, *e2;
    ik_effector_create(&e1);
    ik_effector_create(&e2);
    ik_bone_attach_effector(n3, e1);
    ik_bone_attach_effector(n6, e2);
    e2->chain_length = 1;

    ik_solver solver;
    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(2));

    //
    //                 6 <- e2, chain=1
    //                /
    // e1 -> 3       5
    //        \     /
    //         2   4 <- dead
    //          \ /
    //           1
    //           |
    //           0
    //

    struct ik::Ref<ik_solver> solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    struct ik::Ref<ik_solver> solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    ASSERT_THAT(solver1->ndv.bone_data->bone_count, Eq(6));  // excludes bone 4

    EXPECT_THAT(solver1->ndv.subbase_idx, Eq(0));
    ASSERT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    ASSERT_THAT(solver1->ndv.chain_end_idx, Eq(4));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n3)));

    EXPECT_THAT(solver2->ndv.subbase_idx, Eq(4));
    ASSERT_THAT(solver2->ndv.chain_begin_idx, Eq(5));
    ASSERT_THAT(solver2->ndv.chain_end_idx, Eq(6));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n5)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n6)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, split_tree_with_effector_at_junction)
{
    ik_solver solver;
    ik::Ref<ik_bone> tree = tree_with_two_effectors();
    ik::Ref<ik_bone> n1 = ik_bone_find(tree, ik_guid(1));
    ik::Ref<ik_bone> n2 = ik_bone_find(tree, ik_guid(2));
    ik::Ref<ik_bone> n3 = ik_bone_find(tree, ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_find(tree, ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_find(tree, ik_guid(5));
    ik::Ref<ik_bone> n6 = ik_bone_find(tree, ik_guid(6));
    ik::Ref<ik_bone> n7 = ik_bone_find(tree, ik_guid(7));
    ik::Ref<ik_bone> n8 = ik_bone_find(tree, ik_guid(8));
    ik::Ref<ik_bone> n9 = ik_bone_find(tree, ik_guid(9));

    ik_effector* e3;
    ik_bone_create_effector(&e3, n3);

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));

    //
    // e1 -> 6           9 <- e2
    //        \         /
    //         5       8
    //          \     /
    //           4   7
    //            \ /
    //             3 <- e3
    //             |
    //             2
    //             |
    //             1
    //             |
    //             0 <- a1
    //

    ASSERT_THAT(vector_count(&solver.solver_list), Eq(3));
    ik::Ref<ik_solver> solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    ik::Ref<ik_solver> solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    ik::Ref<ik_solver> solver3 = *(ik_solver**)vector_get_element(&solver.solver_list, 2);
    ASSERT_THAT(solver1->ndv.bone_data->bone_count, Eq(10));

    ASSERT_THAT(solver1->ndv.subbase_idx, Eq(0));
    ASSERT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    ASSERT_THAT(solver1->ndv.chain_end_idx, Eq(4));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n3)));

    ASSERT_THAT(solver2->ndv.subbase_idx, Eq(3));
    ASSERT_THAT(solver2->ndv.chain_begin_idx, Eq(4));
    ASSERT_THAT(solver2->ndv.chain_end_idx, Eq(7));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n3)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n5)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n6)));

    ASSERT_THAT(solver3->ndv.subbase_idx, Eq(3));
    ASSERT_THAT(solver3->ndv.chain_end_idx, Eq(7));
    ASSERT_THAT(solver3->ndv.chain_end_idx, Eq(9));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n3)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n5)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n6)));

    ik_solver_deinit(&solver);
    ik_bone_free_recursive(tree);
}

TEST_F(NAME, dd)
{
    //
    //                e2       e3
    //                |         |
    //                v         v
    //   e1 -> 9     12         18     21 <- e4
    //          \    |           |    /
    //           8   11         17   20
    //            \  |           |  /
    //             7 10         16 19
    //              \|           |/
    //               6           15
    //                \         /
    //                 5       14
    //                  \     /
    //                   4   13
    //                    \ /
    //                     3
    //                     |
    //                     2
    //                     |
    //                     1
    //                     |
    //                     0
    //
}

TEST_F(NAME, check_solver_order_for_disjoint_trees_llr)
{
    ik_solver solver;
    ik::Ref<ik_bone> tree = tree_llr();
    ik_bone *e1, *e2, *e3;

    // Need to change effector chain lengths so tree becomes disjoint
    e1 = ik_bone_find(tree, ik_guid(6));
    e2 = ik_bone_find(tree, ik_guid(8));
    e3 = ik_bone_find(tree, ik_guid(10));
    IK_NODE_EFFECTOR(e1)->chain_length = 1;
    IK_NODE_EFFECTOR(e2)->chain_length = 1;

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(3));

    //
    // The scenario here is bones 0,1,2,9,10 form a chain (call this
    // "ndv 1"), bones 5,6 form a chain ("ndv 2"), and bones 7,8 form
    // a chain ("ndv 3"). Because nd's 2 and 3 depend on the solution of
    // nd 1, nd 1 must appear before ndv's 2 and 3 in the solver.
    //
    // e1 -> 6       8 <- e2
    //        \     /
    //   b2 -> 5   7 <- b3
    //          \ /
    //           4      10 <- e3
    //            \     /
    //             3   9
    //              \ /
    //               2
    //               |
    //               1
    //               |
    //               0 <- b1
    //
    struct ik_bone *b1, *b2, *b3;
    b1 = tree;
    b2 = ik_bone_find(tree, ik_guid(5));  // expected base of second nd
    b3 = ik_bone_find(tree, ik_guid(7));  // expected base of third nd
    struct ik::Ref<ik_solver> solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    struct ik::Ref<ik_solver> solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    struct ik::Ref<ik_solver> solver3 = *(ik_solver**)vector_get_element(&solver.solver_list, 2);

    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(b1)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(b2)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(b3)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}

TEST_F(NAME, check_solver_order_for_disjoint_trees_llrr)
{
    ik_solver solver;
    ik_bone *tree, *n1, *n2, *n3, *n4, *n5, *n6, *n7, *n8, *n9, *n10, *n11, *n12, *n13, *n14, *n15;

    ik::Ref<ik_bone> tree = ik_bone_create();
    ik::Ref<ik_bone> n1 = ik_bone_create_child(tree);
    ik::Ref<ik_bone> n2 = ik_bone_create_child( n1,  ik_guid(2));
    ik::Ref<ik_bone> n3 = ik_bone_create_child( n2,  ik_guid(3));
    ik::Ref<ik_bone> n4 = ik_bone_create_child( n3,  ik_guid(4));
    ik::Ref<ik_bone> n5 = ik_bone_create_child( n4,  ik_guid(5));
    ik::Ref<ik_bone> n6 = ik_bone_create_child( n5,  ik_guid(6));
    ik::Ref<ik_bone> n7 = ik_bone_create_child( n6,  ik_guid(7));
    ik::Ref<ik_bone> n8 = ik_bone_create_child( n5,  ik_guid(8));
    ik::Ref<ik_bone> n9 = ik_bone_create_child( n8,  ik_guid(9));
    ik::Ref<ik_bone> n10 = ik_bone_create_child(n2,  ik_guid(10));
    ik::Ref<ik_bone> n11 = ik_bone_create_child(n10);
    ik::Ref<ik_bone> n12 = ik_bone_create_child(n11);
    ik::Ref<ik_bone> n13 = ik_bone_create_child(n12);
    ik::Ref<ik_bone> n14 = ik_bone_create_child(n11);
    ik::Ref<ik_bone> n15 = ik_bone_create_child(n14);

    ik_effector *e1, *e2, *e3, *e4, *e5;
    ik_bone_create_effector(&e1, n7);
    ik_bone_create_effector(&e2, n9);
    ik_bone_create_effector(&e3, n13);
    ik_bone_create_effector(&e4, n15);
    ik_bone_create_effector(&e5, n2);
    e1->chain_length = 3;
    e2->chain_length = 1;
    e3->chain_length = 1;
    e4->chain_length = 4;

    //
    // e1 : 4,5,6,7
    // e2 : 8,9
    // e3 : 12,13
    // e4 : 2,10,11,14,15
    // e5 : 0,1,2
    //
    //     e2 -> 9
    //           |
    //           8     e3 -> 13   15 <- e4
    //            \           |  /
    // e1 -> 7--6--5         12 14
    //              \         |/
    //               4        11
    //                \      /
    //    dead bone -> 3   10
    //                  \ /
    //                   2 <- e5
    //                   |
    //                   1
    //                   |
    //                   0
    //

    ik_solver_init(&solver);
    ASSERT_THAT(ik_solver_update(&solver, tree), Eq(IK_OK));
    ASSERT_THAT(vector_count(&solver.solver_list), Eq(5));
    struct ik::Ref<ik_solver> solver1 = *(ik_solver**)vector_get_element(&solver.solver_list, 0);
    struct ik::Ref<ik_solver> solver2 = *(ik_solver**)vector_get_element(&solver.solver_list, 1);
    struct ik::Ref<ik_solver> solver3 = *(ik_solver**)vector_get_element(&solver.solver_list, 2);
    struct ik::Ref<ik_solver> solver4 = *(ik_solver**)vector_get_element(&solver.solver_list, 3);
    struct ik::Ref<ik_solver> solver5 = *(ik_solver**)vector_get_element(&solver.solver_list, 4);

    EXPECT_THAT(solver1->ndv.bone_data->bone_count, Eq(15));

    // 0,1,2
    EXPECT_THAT(solver1->ndv.subbase_idx, Eq(0));
    ASSERT_THAT(solver1->ndv.chain_begin_idx, Eq(1));
    ASSERT_THAT(solver1->ndv.chain_end_idx, Eq(3));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(tree)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n1)));
    EXPECT_THAT(IK_NDV_AT(&solver1->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n2)));

    // 2,10,11,14,15
    EXPECT_THAT(solver2->ndv.subbase_idx, Eq(2));
    ASSERT_THAT(solver2->ndv.chain_begin_idx, Eq(3));
    ASSERT_THAT(solver2->ndv.chain_end_idx, Eq(7));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n2)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n10)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n11)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n14)));
    EXPECT_THAT(IK_NDV_AT(&solver2->ndv, user_data, 4), Eq(IK_NODE_USER_DATA(n15)));

    // 12,13
    EXPECT_THAT(solver3->ndv.subbase_idx, Eq(7));
    ASSERT_THAT(solver3->ndv.chain_begin_idx, Eq(8));
    ASSERT_THAT(solver3->ndv.chain_end_idx, Eq(9));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n12)));
    EXPECT_THAT(IK_NDV_AT(&solver3->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n13)));

    // 4,5,6,7
    EXPECT_THAT(solver4->ndv.subbase_idx, Eq(9));
    ASSERT_THAT(solver4->ndv.chain_begin_idx, Eq(10));
    ASSERT_THAT(solver4->ndv.chain_end_idx, Eq(13));
    EXPECT_THAT(IK_NDV_AT(&solver4->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n4)));
    EXPECT_THAT(IK_NDV_AT(&solver4->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n5)));
    EXPECT_THAT(IK_NDV_AT(&solver4->ndv, user_data, 2), Eq(IK_NODE_USER_DATA(n6)));
    EXPECT_THAT(IK_NDV_AT(&solver4->ndv, user_data, 3), Eq(IK_NODE_USER_DATA(n7)));

    // 8,9
    EXPECT_THAT(solver5->ndv.subbase_idx, Eq(13));
    ASSERT_THAT(solver5->ndv.chain_begin_idx, Eq(14));
    ASSERT_THAT(solver5->ndv.chain_end_idx, Eq(15));
    EXPECT_THAT(IK_NDV_AT(&solver5->ndv, user_data, 0), Eq(IK_NODE_USER_DATA(n8)));
    EXPECT_THAT(IK_NDV_AT(&solver5->ndv, user_data, 1), Eq(IK_NODE_USER_DATA(n9)));

    ik_bone_free_recursive(tree);
    ik_solver_deinit(&solver);
}
*/
