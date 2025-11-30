#ifndef TimeHeap
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
struct TimeHeap {
	size_t size;
	size_t capacity;
	time_t* expected;
	int* id;
};
typedef struct TimeHeap TimeHeap;
#endif
void timeheap_insert(TimeHeap* th, const int id, const unsigned int sleep_for);
bool timeheap_isempty(TimeHeap* th);
time_t timeheap_toptime(TimeHeap* th);
int timeheap_pop(TimeHeap* th, time_t *const expected);
void timeheap_cleanup(TimeHeap* th);
