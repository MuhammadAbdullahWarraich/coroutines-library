#include "coroutines.h"
struct LevelArgs {
	int id;
};
typedef struct LevelArgs LevelArgs;
void level2(void* args) {
	coroutine_yield();
	printf("hi from level 2 to id: %d\n", ((LevelArgs*)args)->id);
}
void level1(void* args) {
	coroutine_await((void (*) (void)) level2, args);
	printf("hi from level 1 to id: %d\n", ((LevelArgs*)args)->id);
}
void level0(void* args) {
	printf("await level1...\n");
	coroutine_await((void (*) (void)) level1, args);
	printf("after await, hi from level 0 to id: %d\n", ((LevelArgs*)args)->id);
	printf("await level1 again...\n");
	coroutine_await((void (*) (void)) level1, args);
	printf("after second await, bye from level 0 to id: %d\n", ((LevelArgs*)args)->id);
}
void test() {
	LevelArgs arg1 = { .id = 1 };
	LevelArgs arg2 = { .id = 2 };

	coroutines_initialize();
	coroutine_add((void (*) (void)) level0, (void*) &arg1);
	coroutine_add((void (*) (void)) level0, (void*) &arg2);
	coroutines_gather();
}
int main() {
	test();
	return 0;
}
