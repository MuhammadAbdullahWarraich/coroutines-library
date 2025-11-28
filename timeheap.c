#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include "timeheap.h"

void _timeheap_heapify(TimeHeap* th, unsigned int i, const bool recur_back) {
	bool recur = true;
	if (recur_back && i == 0) recur = false;
	assert(i < th->size);
	unsigned int idx = i, lc_idx = 2*(i+1)-1, rc_idx = 2*(i+1);
	unsigned int next;
	if (recur_back) next = (i % 2 == 0 ? i : i+1)/2 - 1;
	time_t me, lc = -1, rc = -1;
	me = th->expected[idx];
	if (lc_idx < th->size) lc = th->expected[lc_idx];
	if (rc_idx < th->size) rc = th->expected[rc_idx];
	if (lc == -1 && lc == rc) {// no children
		if (recur_back && recur) _timeheap_heapify(th, next, recur_back);
		return;
	}
	assert(lc != -1);
	if (rc == -1) {// one child node
		if (lc < me) {
			th->expected[idx] = lc;
			th->expected[lc_idx] = me;
			{
				int temp = th->id[idx];
				th->id[idx] = th->id[lc_idx];
				th->id[lc_idx] = temp;
			}
			if (!recur_back) next = lc_idx;
			if (recur) _timeheap_heapify(th, next, recur_back);
			return;
		}
		return;
	}
	// two child nodes
	if (rc <= lc && rc < me) {
		th->expected[idx] = rc;
		th->expected[rc_idx] = me;
		{
			int temp = th->id[idx];
			th->id[idx] = th->id[rc_idx];
			th->id[rc_idx] = temp;
		}
		if (!recur_back) next = rc_idx;
		if (recur) _timeheap_heapify(th, next, recur_back);
		return;
	}
	if (lc <= rc && lc < me) {
		th->expected[idx] = lc;
		th->expected[lc_idx] = me;
		{
			int temp = th->id[idx];
			th->id[idx] = th->id[lc_idx];
			th->id[lc_idx] = temp;
		}
		if (!recur_back) next = lc_idx;
		if (recur) _timeheap_heapify(th, next, recur_back);
		return;
	}
	if (recur && recur_back) {
		_timeheap_heapify(th, next, recur_back);
		return;
	}
	assert(lc == rc && (lc == -1 || lc == me));
	// will fail if we are heapifying forward(i.e. after popping), and heap before popping wasn't a heap
}
void timeheap_insert(TimeHeap* th, const int id, const unsigned int sleep_for) {
	if (th->capacity == 0) {
		assert(th->expected == NULL && th->size == 0 && th->capacity == 0);
		unsigned int alloc_size = 10 * sizeof(time_t);
		th->expected = (time_t*) malloc(alloc_size);
		if (th->expected == NULL) {
			printf("malloc failed to allocate %u bytes for Timeheap.expected!\n", alloc_size);
			exit(1);
		}
		th->id = (int*) malloc(alloc_size);
		if (th->id == NULL) {
			printf("malloc failed to allocate %u bytes for Timeheap.id!\n", alloc_size);
			exit(1);
		}
		th->capacity = alloc_size;
	}
	if (th->size == th->capacity) {
		time_t* temp_expected = NULL;
		unsigned int alloc_size = 2 * th->capacity;
		temp_expected = (time_t*) malloc(alloc_size);
		if (temp_expected == NULL) {
			printf("malloc failed to allocate %u bytes for Timeheap.expected!\n", alloc_size);
			exit(1);
		}
		int* temp_id = (int*) malloc(alloc_size);
		if (temp_id == NULL) {
			printf("malloc failed to allocate %u bytes for Timeheap.id!\n", alloc_size);
			exit(1);
		}
		for (size_t i = 0; i < th->capacity; i++) {
			temp_expected[i] = th->expected[i];
			temp_id[i] = th->id[i];
		}
		th->capacity = alloc_size;
		free(th->expected);
		free(th->id);
		th->expected = temp_expected;
		th->id = temp_id;
	}
	assert(th->size < th->capacity);// remove if runs
	if ((time_t) -1 == time(&th->expected[th->size])) {
		printf("time failed to get current time. Error is:\"%s\"", strerror(errno));
		printf("this error is only possible if th->expected[th->size] points to an inaccessible address. So fix your code\n");
		exit(1);
	}
	th->expected[th->size] += sleep_for;
	th->id[th->size] = id;
	_timeheap_heapify(th, th->size++, true);
}
bool timeheap_isempty(TimeHeap* th) {
	return th->size == 0;
}
time_t timeheap_toptime(TimeHeap* th) {
	assert(th->size > 0);
	return th->expected[0];
}
int timeheap_pop(TimeHeap* th, time_t *const expected) {
	assert(th->size > 0);
	if (NULL != expected) *expected = th->expected[0];
	const int ret_val = th->id[0];
	size_t oldsize = th->size--;
	if (oldsize == 1) return ret_val;
	th->expected[0] = th->expected[th->size];
	th->id[0] = th->id[th->size];
	_timeheap_heapify(th, 0, false);
	return ret_val;
}
void print_heap(TimeHeap* th) {
	for (size_t i = 0; i < th->size; i++) printf("(%d:%zu)%s", th->id[i], th->expected[i], i%2 == 0 ? "   " : " ");
	printf("\n");
}
void __test_timeheap() {
	struct TimeHeap th = {
		.size = 0,
		.capacity = 0,
		.expected = NULL,
		.id = NULL
	};
	for (size_t i = 10; i >= 1; i--) {
		timeheap_insert(&th, 10-i, i);
		print_heap(&th);
	}
	for (size_t i = th.size; i > 0; i--) {
		time_t expected = 100;
		int ret_id = timeheap_pop(&th, &expected);
		printf("popped id:%d - expected_time:%zu\n", ret_id, expected);
		print_heap(&th);
	}
}
void __test2_timeheap() {
	struct TimeHeap th = {
		.size = 0,
		.capacity = 0,
		.expected = NULL,
		.id = NULL
	};
	for (size_t i = 10; i >= 1; i--) {
		timeheap_insert(&th, 10-i, i);
		print_heap(&th);
	}
	for (size_t i = th.size; i > 0; i--) {
		time_t expected = 100;
		while (true) 
			if (time(NULL) >= timeheap_toptime(&th)) 
				break;
		int ret_id = timeheap_pop(&th, &expected);
		printf("popped id:%d - expected_time:%zu\n", ret_id, expected);
		print_heap(&th);
	}
}
int main() {
	__test2_timeheap();
	return 0;
}
