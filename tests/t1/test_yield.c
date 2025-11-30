#include <stdio.h>
#include "coroutines.h"
struct CounterArgs {
	int id;
	int n;
};
typedef struct CounterArgs CounterArgs;
void counter(void* args) {
	CounterArgs* cargs = (CounterArgs*) args;
	int id = cargs->id;
	int n = cargs->n;
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		coroutine_yield();
	}
	printf("returning from %d\n", id);
}
void test() {
	coroutines_initialize();
	CounterArgs c1_args = {
		.id = 1,
		.n = 2
	};
	CounterArgs c2_args = {
		.id = 2,
		.n = 5
	};
	CounterArgs c3_args = {
		.id = 3,
		.n = 5
	};
	coroutine_add((void (*) (void)) counter, 1, (void *) &c1_args);
	coroutines_gather();
	coroutine_add((void (*) (void)) counter, 1, (void *) &c2_args);
	printf("added id=2\n");
	coroutine_add((void (*) (void)) counter, 1, (void *) &c3_args);
	coroutines_gather();
}
int main() {
	test();
	printf("back in main\n");
	return 0;
}
