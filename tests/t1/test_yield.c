#include "coroutines.h"
#include <stdio.h>
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
}
void test() {
	coroutines_initialize();
	CounterArgs c1_args = {
		.id = 1,
		.n = 10
	};
	CounterArgs c2_args = {
		.id = 2,
		.n = 10
	};
	coroutine_add((void (*) (void)) counter, 1, (void *) &c1_args);
	coroutine_yield();
	coroutine_add((void (*) (void)) counter, 1, (void *) &c2_args);
	coroutines_gather();
}
int main() {
	test();
	return 0;
}
