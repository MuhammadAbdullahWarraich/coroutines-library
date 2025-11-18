#include <ucontext.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
// TODO: make arrays dynamic
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
/* CODING CONVENTIONS
 * 1. function names of functions defined by me will be in snake case
 * 2. attributes of structs & local variables will have camel case
 * 3. global variables will be in camel case but start with capital
 * 4. macros will be in all-caps
 * 5. anything prefixed by a underscore(_) should not be used outside the library(you can do this by not adding to header file of library or just add static in case of globals
 * */

struct {
	ucontext_t** arr;
	size_t arrSize;
	size_t arrCapacity;
	int* awake;// now we will Round-Robbin on the indices of this array
	size_t awakeSize;
	size_t awakeCapacity;
	int* sleeping;
	struct pollfd* fds;
	nfds_t nfds;// size of sleeping array == size of fds array == nfds
	size_t sleepingCapacity;
	size_t currAwake;
	ucontext_t* manageEndingContext;
} Contexts = {
	NULL,
	0,
	10,
	NULL,
	0,
	10,
	NULL,
	NULL,
	0,
	10,
	0,
	NULL
};
int _FromEnd = 0;
void yield() {
	if (_FromEnd == 0) {
		int oldcurr = Contexts.currAwake;
		Contexts.currAwake = (oldcurr+1) % Contexts.awakeSize;
		swapcontext(Contexts.arr[Contexts.awake[oldcurr]], Contexts.arr[Contexts.awake[Contexts.currAwake]]);
		return;
	}
	_FromEnd = 0;
	setcontext(Contexts.arr[Contexts.awake[Contexts.currAwake]]);
}
void endyield() {
	for (size_t i = Contexts.awake[Contexts.currAwake]; i < Contexts.arrSize - 1; i++) {
		Contexts.arr[i] = Contexts.arr[i+1];
	}
	Contexts.arrSize--;
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	_FromEnd = 1;
	yield();
}
void initialize() {
	Contexts.arr = (ucontext_t**) malloc(Contexts.arrCapacity * sizeof(ucontext_t*));
	if (Contexts.arr == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.arr\n", Contexts.arrCapacity * sizeof(ucontext_t*));
		exit(1);
	}
	Contexts.awake = (int*) malloc(Contexts.awakeCapacity * sizeof(int));
	if (Contexts.awake == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.awake\n", Contexts.awakeCapacity * sizeof(int));
		exit(1);
	}
	Contexts.sleeping = (int*) malloc(Contexts.sleepingCapacity * sizeof(int));
	if (Contexts.sleeping == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.sleeping\n", Contexts.sleepingCapacity * sizeof(int));
		exit(1);
	}
	Contexts.fds = (struct pollfd*) malloc(Contexts.sleepingCapacity * sizeof(struct pollfd));
	if (Contexts.fds == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.fds\n", Contexts.sleepingCapacity * sizeof(struct pollfd));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.arrCapacity; i++) {
		Contexts.arr[i] = (ucontext_t*) malloc(sizeof(ucontext_t));
		if (Contexts.arr[i] == NULL) {
			printf("malloc failed to allocate %zu bytes for Contexts.arr[%zu]\n", sizeof(ucontext_t), i);
			exit(1);
		}
		getcontext(Contexts.arr[i]);// probably for setting uc_mcontext and uc_sigmask
		if (i > 0) {
			Contexts.arr[i]->uc_stack.ss_sp = mmap(NULL, (size_t) 1024 * getpagesize(), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); //TODO: add MAP_GROWS_DOWN so that stack can grow when guard page is hit

			if (MAP_FAILED == Contexts.arr[i]->uc_stack.ss_sp) {
				printf("mmap failed to allocate %d bytes for Contexts.arr[%zu]->uc_stack.ss_sp. Error is:\"%s\"", 1024 * getpagesize(), i, strerror(errno));
				exit(1);
			}
			Contexts.arr[i]->uc_stack.ss_size = 1024 * getpagesize();
			//Contexts.arr[i]->uc_stack.ss_sp += 1023 * getpagesize();
			Contexts.arr[i]->uc_stack.ss_flags = 0;
		}
		Contexts.arr[i]->uc_link = 0;
	}
	Contexts.manageEndingContext = (ucontext_t*) malloc(sizeof(ucontext_t));
	if (Contexts.manageEndingContext == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.manageEndingContext\n", sizeof(ucontext_t));
		exit(1);
	}
	getcontext(Contexts.manageEndingContext);
	Contexts.manageEndingContext->uc_stack.ss_sp = mmap(NULL, (size_t) 1024 * getpagesize(), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
	if (MAP_FAILED == Contexts.manageEndingContext->uc_stack.ss_sp) {
		printf("mmap failed to allocate %d bytes for Contexts.manageEndingContext->uc_stack.ss_sp. Error is:\"%s\"", 1024 * getpagesize(), strerror(errno));
		exit(1);
	}
	Contexts.manageEndingContext->uc_stack.ss_size = 1024 * getpagesize();
	Contexts.manageEndingContext->uc_stack.ss_flags = 0;
	Contexts.manageEndingContext->uc_link = 0;
	makecontext(Contexts.manageEndingContext, (void (*) (void)) endyield, 0);
	for (size_t i = 1; i < 10; i++) {
		Contexts.arr[i]->uc_link = Contexts.manageEndingContext;
	}
	Contexts.arrSize++;
	Contexts.awake[0] = 0;
	Contexts.awakeSize++;
}
void sleep_yield() {
	int idx = Contexts.awake[Contexts.currAwake];
	{
		printf("Contexts.awake: {");
		for (size_t i = 0; i < Contexts.awakeSize; i++) {
			printf("%d%s", Contexts.awake[i], i+1 == Contexts.awakeSize ? "" : ", ");
		}
		printf("}\n");
	}
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	if (Contexts.awakeSize == Contexts.currAwake) {
		// This is the case where we are sleep yielding the right-most element of the awake array
		Contexts.currAwake--;
	}
	assert(Contexts.awakeSize > 0); //atleast the main loop must be awake so that it can yield again and again, where each yield will try to wake up asleep coroutines
	{
		printf("Contexts.awake: {");
		for (size_t i = 0; i < Contexts.awakeSize; i++) {
			printf("%d%s", Contexts.awake[i], i+1 == Contexts.awakeSize ? "" : ", ");
		}
		printf("}\n");
	}
	Contexts.sleeping[Contexts.nfds++] = idx;
	// _FromEnd = 1; yield();// TODO: replace this hacky way with something else for true round-robbin -> FIXED (see below)
	if (Contexts.awakeSize > 0) {
		swapcontext(Contexts.arr[idx], Contexts.arr[Contexts.awake[Contexts.currAwake]]);
		return;
	}
	setcontext(Contexts.arr[Contexts.awake[Contexts.currAwake]]);
}
void counter(int id, int n) {
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		if (i == n - id) { printf("gonna sleep_yield!\n"); sleep_yield(); printf("WOW! Guess I learnt something new!\n"); }
		else yield();
	}
	//printf("returning from counter with id: %d\n", id);
}
int setup_for_coroutine_add() {
	assert(Contexts.arrSize < Contexts.arrCapacity);// TODO: please get dynamic arrays
	int oldsize = Contexts.arrSize;
	Contexts.awake[Contexts.awakeSize++] = Contexts.arrSize++;
	errno = 0;
	return oldsize;
}

int main() {
	pid_t process_id = getpid();
	//printf("process_id: %d\n", process_id);
	initialize();
	printf("Got till here\n");
	{
		int oldsize = setup_for_coroutine_add();
		makecontext(Contexts.arr[oldsize], (void (*) (void)) counter, 2, 1, 10);
	}
	yield();
	printf("back in main\n");
	{
		int oldsize = setup_for_coroutine_add();
		makecontext(Contexts.arr[oldsize], (void (*) (void)) counter, 2, 2, 10);
	}
	int i = 0;
	while (i++ < 20 && Contexts.arrSize > 1) { yield(); printf("I knew it!\n"); }
	printf("main ended successfully\n");
}
