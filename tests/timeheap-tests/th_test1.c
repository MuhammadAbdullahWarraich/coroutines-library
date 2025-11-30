#include "timeheap.h"
void print_heap(TimeHeap* th) {
	for (size_t i = 0; i < th->size; i++) printf("(%d:%zu)%s", th->id[i], th->expected[i], i%2 == 0 ? "   " : " ");
	printf("\n");
}
void test() {
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
int main() {
	test();
	return 0;
}
