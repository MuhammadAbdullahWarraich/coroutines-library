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

/* CODING CONVENTIONS
 * 1. function names of functions defined by me will be in snake case
 * 2. attributes of structs & local variables will have camel case
 * 3. global variables will be in camel case but start with capital
 * 4. macros will be in all-caps
 * 5. anything prefixed by a underscore(_) should not be used outside the library(you can do this by not adding to header file of library or just add static in case of globals
 * */
struct {
	ucontext_t** arr;
	size_t size;
	size_t capacity;
	size_t curr;
	ucontext_t* manageEndingContext;
} Contexts = {
	NULL,
	0,
	10,
	0,
	NULL,
};
int _PrepFlag = 1;
int _FromEnd = 0;
void yield() {
	assert(!(Contexts.size == 0 && Contexts.curr == 0));// yield should only be called after a call to initialize
	int oldcurr = Contexts.curr;
	Contexts.curr = (oldcurr+1) % Contexts.size;
	if (_FromEnd == 0) swapcontext(Contexts.arr[oldcurr], Contexts.arr[Contexts.curr]);
	else { _FromEnd = 0; setcontext(Contexts.arr[Contexts.curr]); }
}
void endyield() {
	for (size_t i = Contexts.curr; i < Contexts.size - 1; i++) {
		Contexts.arr[i] = Contexts.arr[i+1];
	}
	Contexts.size--;
	_FromEnd = 1;
	yield();
}
void manage_ending() {
	getcontext(Contexts.manageEndingContext);
	if (_PrepFlag == 1) return;
	//TODO: if you had MAP_GROWSDOWN set in mmap, maybe continue to munmap until you touch the guard page for zero memory leaks
	if (0 != Contexts.curr && -1 == munmap(Contexts.arr[Contexts.curr]->uc_stack.ss_sp, getpagesize())) {
		printf("failed to munmap, error code is: \"%s\"\n", strerror(errno));
		exit(1);
	}
	for (size_t i = Contexts.curr; i < Contexts.size - 1; i++) {
		Contexts.arr[i] = Contexts.arr[i+1];
	}
	//printf("got till here in manage ending\n");
	while (1) yield();
	//printf("got after here in manage ending\n");
}
void initialize() {
	Contexts.arr = (ucontext_t**) malloc(10 * sizeof(ucontext_t*));
	if (Contexts.arr == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.arr\n", 10 * sizeof(ucontext_t*));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.capacity; i++) {
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
	//manage_ending();
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
	if (_PrepFlag == 0) return;
	for (size_t i = 1; i < 10; i++) {
		Contexts.arr[i]->uc_link = Contexts.manageEndingContext;
	}
	_PrepFlag = 0;
	Contexts.size++;
	getcontext(Contexts.arr[0]);
	yield();
}
void counter(int id, int n) {
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		yield();
	}
	//printf("returning from counter with id: %d\n", id);
}
int main() {
	pid_t process_id = getpid();
	//printf("process_id: %d\n", process_id);
	initialize();
	printf("Got till here\n");
	int oldsize = Contexts.size;
	Contexts.size++;
	errno = 0;
	makecontext(Contexts.arr[oldsize], (void (*) (void)) counter, 2, 1, 10);
	yield();
	printf("back in main\n");
	oldsize = Contexts.size;
	Contexts.size++;
	makecontext(Contexts.arr[oldsize], (void (*) (void)) counter, 2, 2, 10);
	while (Contexts.size > 1) yield();
	printf("main ended successfully\n");
}

