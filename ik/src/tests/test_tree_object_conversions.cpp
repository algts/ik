#include "gmock/gmock.h"
#include "ik/bone.h"
#include "ik/node.h"
#include "ik/cpputils.hpp"

#define NAME tree_object_conversions

using namespace ::testing;

class NAME : public Test
{
public:
    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }
};

TEST_F(NAME, single_bone_to_node)
{
    ik::Ref<ik_bone> b1 = ik_bone_create();
}
