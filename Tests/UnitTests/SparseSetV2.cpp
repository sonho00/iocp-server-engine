#include "Network/Common/SparseSetV2.hpp"

#include <gtest/gtest.h>

TEST(DataStructureTest, SparseSetV2) {
	SparseSetV2<10, 3> set;

	uint64_t h1 = set.Pop(0);
	EXPECT_NE(h1, (SparseSetV2<10, 3>::kInvalidHandle));
	set.MoveToState(h1, 1);

	uint64_t h2 = set.Pop(0);
	EXPECT_NE(h2, (SparseSetV2<10, 3>::kInvalidHandle));
	set.MoveToState(h2, 2);

	EXPECT_TRUE(set.IsValid(h1));
	EXPECT_EQ(set.GetIndicesInState(1).size(), 1);
	EXPECT_EQ(set.GetIndicesInState(2).size(), 1);

	LOG_INFO("Test 1 Passed: Acquire");

	set.MoveToState(h1, 2);
	EXPECT_TRUE(set.GetIndicesInState(1).empty());
	EXPECT_EQ(set.GetIndicesInState(2).size(), 2);

	LOG_INFO("Test 2 Passed: Transition");

	uint64_t old_h1 = h1;
	set.Push(h1);
	LOG_INFO("Checking invalidity of handle after release...");
	EXPECT_FALSE(set.IsValid(old_h1));

	LOG_INFO("Test 3 Passed: Release & Reuse");

	uint64_t h1_new = set.Pop(0);
	EXPECT_NE(h1_new, old_h1);

	LOG_INFO("Test 4 Passed: Handle Reuse");

	for (int i = 0; i < 9; ++i) {
		uint64_t h = set.Pop(0);
		EXPECT_NE(h, (SparseSetV2<10, 3>::kInvalidHandle));
		set.MoveToState(h, 1);
	}

	LOG_INFO("Checking capacity limit...");
	EXPECT_EQ(set.Pop(0), (SparseSetV2<10, 3>::kInvalidHandle));

	LOG_INFO("Test 5 Passed: Capacity check");
}
