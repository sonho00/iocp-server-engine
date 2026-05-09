#include <cassert>

#include "Network/Common/SpraseSetV2.hpp"

int main() {
	SparseSetV2<10, 3> set;

	uint64_t h1 = set.Pop(1);
	uint64_t h2 = set.Pop(2);

	assert(set.IsValid(h1));
	assert(set.GetIndicesInState(1).size() == 1);
	assert(set.GetIndicesInState(2).size() == 1);

	LOG_INFO("Test 1 Passed: Acquire");

	set.MoveToState(h1, 2);
	assert(set.GetIndicesInState(1).empty());
	assert(set.GetIndicesInState(2).size() == 2);

	LOG_INFO("Test 2 Passed: Transition");

	set.Push(h1);
	assert(!set.IsValid(h1));

	LOG_INFO("Test 3 Passed: Release & Reuse");

	uint64_t h1_new = set.Pop(2);
	assert(h1_new != h1);

	for (int i = 0; i < 8; ++i) {
		uint64_t h = set.Pop(1);
		assert((h != SparseSetV2<10, 3>::kInvalidHandle));
	}

	assert((set.Pop(1) == SparseSetV2<10, 3>::kInvalidHandle));

	LOG_INFO("Test 4 Passed: Capacity check");

	return 0;
}
