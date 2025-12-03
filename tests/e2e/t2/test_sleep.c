#include <stdio.h>
#include "coroutines.h"
struct CounterArgs {
	int id;
	int n;
	int sleep_time;
};
typedef struct CounterArgs CounterArgs;
void sleeping_counter(void* args) {
	CounterArgs* cargs = (CounterArgs*) args;
	int id = cargs->id;
	int n = cargs->n;
	int sleep_time = cargs->sleep_time;
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		coroutine_sleep(sleep_time);
	}
}
void test() {
	CounterArgs args1 = {
		.id = 1,
		.n = 5,
		.sleep_time = 1
	};
	coroutines_initialize();
	coroutine_add((void (*) (void)) sleeping_counter, (void*) &args1);
	coroutines_gather();
	coroutines_cleanup();// unnecessary, because the memory will be freed with the end of the process anyways and we are almost at the end of the process
}
int main() {
	test();
	return 0;
}
