#include <ucontext.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "timeheap.h"

// TODO: make arrays dynamic
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
	size_t maxCapacity;// allocated size of arr, awake, sleeping, and fds
	size_t lazyCapacity;// stacks are only available for this much contexts
	size_t maxInAdvanceStackBookings;// how many new stacks will be allocated when previous ones have ended
	int* awake;// we will Round-Robbin on the indices of this array
	size_t awakeSize;
	int* sleeping;
	struct pollfd* fds;
	nfds_t nfds;// size of sleeping array == size of fds array == nfds
	int* dead;
	size_t deadSize;
	size_t currAwake;
	ucontext_t* manageEndingContext;
	TimeHeap timeSleepers;
} Contexts = {
	NULL,
	0,
	1,
	2,
	2,
	NULL,
	0,
	NULL,
	NULL,
	0,
	NULL,
	0,
	0,
	NULL,
	{0}
};
int _FromEnd = 0;
void _wake(size_t i) {
	assert(i < Contexts.nfds);
	int idx = Contexts.sleeping[i];
	for (size_t x = i; x < -1 + Contexts.nfds; x++) {
		Contexts.sleeping[x] = Contexts.sleeping[x+1];
		Contexts.fds[x] = Contexts.fds[x+1];
	}
	Contexts.nfds--;
	Contexts.awake[Contexts.awakeSize++] = idx;
}
void _try_to_wake_up_sleeping_coroutines() {
	// right now, it only works for waking those that sleep_yield on reading from a fd
	int state = poll(Contexts.fds, Contexts.nfds, 0);
	for (size_t i = 0; i < Contexts.nfds; i++) {
		if (Contexts.fds[i].revents & Contexts.fds[i].events) {
			_wake(i);
		}
	}
}
void _try_to_wake_up_timeupped() {
	if (timeheap_isempty(&Contexts.timeSleepers)) return;
	while (time(NULL) >= timeheap_toptime(&Contexts.timeSleepers)) {
		int id = timeheap_pop(&Contexts.timeSleepers, NULL);
		Contexts.awake[Contexts.awakeSize++] = id;
		if (timeheap_isempty(&Contexts.timeSleepers)) break;// TODO: make life simpler by checking this in timeheap_toptime and returning ((time_t) -1) in this case
	}
}
void coroutine_yield() {
	_try_to_wake_up_sleeping_coroutines();
	_try_to_wake_up_timeupped();
	if (_FromEnd == 0) {
		int oldcurr = Contexts.currAwake;
		Contexts.currAwake = (oldcurr+1) % Contexts.awakeSize;
		swapcontext(Contexts.arr[Contexts.awake[oldcurr]], Contexts.arr[Contexts.awake[Contexts.currAwake]]);
		return;
	}
	_FromEnd = 0;
	setcontext(Contexts.arr[Contexts.awake[Contexts.currAwake]]);
}
void _endyield() {
	Contexts.dead[Contexts.deadSize++] = Contexts.awake[Contexts.currAwake];
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	if (Contexts.awakeSize == Contexts.currAwake) {
		// This is the case where we are end yielding the right-most element of the awake array
		Contexts.currAwake--;
	}
	_FromEnd = 1;
	coroutine_yield();
}
void _enlarge_Contexts_arrays() {
	printf("_enlarge_Contexts_arrays got called!\n");
	assert(Contexts.manageEndingContext != NULL);
	// if Contexts.manageEndingContext is null, initialize hasn't been called or Contexts has been corrupted at some point
	const size_t newMaxCapacity = 2 * Contexts.maxCapacity;
	const size_t oldMaxCapacity = Contexts.maxCapacity;
	Contexts.maxCapacity = newMaxCapacity;
	ucontext_t** newarr = (ucontext_t**) malloc(newMaxCapacity * sizeof(ucontext_t*));
	if (newarr == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.arr\n", newMaxCapacity * sizeof(ucontext_t*));
		exit(1);
	}
	for (size_t i = 0; i < oldMaxCapacity; i++) {
		newarr[i] = Contexts.arr[i];
	}
	free(Contexts.arr);
	Contexts.arr = newarr;
	int* newAwake = (int*) malloc(newMaxCapacity * sizeof(int));
	if (newAwake == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.awake\n", newMaxCapacity * sizeof(int));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.awakeSize; i++) {
		newAwake[i] = Contexts.awake[i];
	}
	free(Contexts.awake);
	Contexts.awake = newAwake;
	int* newSleeping = (int*) malloc(newMaxCapacity * sizeof(int));
	if (newSleeping == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.sleeping\n", newMaxCapacity * sizeof(int));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.nfds; i++) {
		newSleeping[i] = Contexts.sleeping[i];
	}
	free(Contexts.sleeping);
	Contexts.sleeping = newSleeping;
	struct pollfd* newFds = (struct pollfd*) malloc(newMaxCapacity * sizeof(struct pollfd));
	if (newFds == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.fds\n", newMaxCapacity * sizeof(struct pollfd));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.nfds; i++) {
		newFds[i] = Contexts.fds[i];
	}
	free(Contexts.fds);
	Contexts.fds = newFds;
	int* newDead = (int*) malloc(newMaxCapacity * sizeof(int));
	if (newDead == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.dead\n", newMaxCapacity * sizeof(int));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.deadSize; i++) {
		newDead[i] = Contexts.dead[i];
	}
	free(Contexts.dead);
	Contexts.dead = newDead;
}
void _increase_lazy_capacity() {
	printf("_increase_lazy_capacity got called!\n");
	assert(Contexts.deadSize == 0 && Contexts.lazyCapacity == Contexts.arrSize);
	if (!(Contexts.lazyCapacity + Contexts.maxInAdvanceStackBookings <= Contexts.maxCapacity)) _enlarge_Contexts_arrays();
	for (size_t i = 0; i < Contexts.maxInAdvanceStackBookings; i++) {
		size_t idx = i + Contexts.lazyCapacity;
		Contexts.arr[idx] = (ucontext_t*) malloc(sizeof(ucontext_t));
		if (Contexts.arr[idx] == NULL) {
			printf("malloc failed to allocate %zu bytes for Contexts.arr[%zu]\n", sizeof(ucontext_t), idx);
			exit(1);
		}
		getcontext(Contexts.arr[idx]);// probably for setting uc_mcontext and uc_sigmask
		Contexts.arr[idx]->uc_stack.ss_sp = mmap(NULL, (size_t) 1024 * getpagesize(), PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); //TODO: add MAP_GROWS_DOWN so that stack can grow when guard page is hit

		if (MAP_FAILED == Contexts.arr[idx]->uc_stack.ss_sp) {
			printf("mmap failed to allocate %d bytes for Contexts.arr[%zu]->uc_stack.ss_sp. Error is:\"%s\"", 1024 * getpagesize(), idx, strerror(errno));
			exit(1);
		}
		Contexts.arr[idx]->uc_stack.ss_size = 1024 * getpagesize();
		Contexts.arr[idx]->uc_stack.ss_flags = 0;
		Contexts.arr[idx]->uc_link = Contexts.manageEndingContext;
	}
	Contexts.lazyCapacity += Contexts.maxInAdvanceStackBookings;
}
int _setup_for_coroutine_add() {
	if (Contexts.deadSize == 0 && Contexts.arrSize == Contexts.lazyCapacity) {
		if (Contexts.lazyCapacity == Contexts.maxCapacity) _enlarge_Contexts_arrays();
		_increase_lazy_capacity();
	}
	if (Contexts.deadSize > 0) {
		int idx = Contexts.dead[Contexts.deadSize-1];
		Contexts.awake[Contexts.awakeSize++] = idx;
		Contexts.deadSize--;
		return idx;
	}
	int idx = Contexts.arrSize;
	Contexts.awake[Contexts.awakeSize++] = Contexts.arrSize++;
	errno = 0;
	return idx;
}
void coroutine_add(void (*func) (void), const int argcount, void* arg) {
	int idx = _setup_for_coroutine_add();
	assert(argcount == 0 || argcount == 1);
	if (argcount == 0) makecontext(Contexts.arr[idx], (void (*) (void)) func, argcount);
	else makecontext(Contexts.arr[idx], (void (*) (void)) func, argcount, (intptr_t) arg);
}
void coroutines_initialize() {
	assert(Contexts.lazyCapacity == Contexts.maxInAdvanceStackBookings);
	if (Contexts.maxCapacity < Contexts.lazyCapacity) Contexts.maxCapacity = Contexts.lazyCapacity;
	Contexts.arr = (ucontext_t**) malloc(Contexts.maxCapacity * sizeof(ucontext_t*));
	if (Contexts.arr == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.arr\n", Contexts.maxCapacity * sizeof(ucontext_t*));
		exit(1);
	}
	Contexts.awake = (int*) malloc(Contexts.maxCapacity * sizeof(int));
	if (Contexts.awake == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.awake\n", Contexts.maxCapacity * sizeof(int));
		exit(1);
	}
	Contexts.sleeping = (int*) malloc(Contexts.maxCapacity * sizeof(int));
	if (Contexts.sleeping == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.sleeping\n", Contexts.maxCapacity * sizeof(int));
		exit(1);
	}
	Contexts.fds = (struct pollfd*) malloc(Contexts.maxCapacity * sizeof(struct pollfd));
	if (Contexts.fds == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.fds\n", Contexts.maxCapacity * sizeof(struct pollfd));
		exit(1);
	}
	Contexts.dead = (int*) malloc(Contexts.maxCapacity * sizeof(int));
	if (Contexts.dead == NULL) {
		printf("malloc failed to allocate %zu bytes for Contexts.dead\n", Contexts.maxCapacity * sizeof(int));
		exit(1);
	}
	for (size_t i = 0; i < Contexts.maxInAdvanceStackBookings; i++) {
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
			Contexts.arr[i]->uc_stack.ss_flags = 0;
		}
		/*
		if (i > 0) {
			Contexts.arr[i]->uc_link = (ucontext_t*) malloc(sizeof(ucontext_t));
			if (Contexts.arr[i]->uc_link == NULL) {
				printf("malloc failed to allocate %zu bytes for Contexts.arr[%zu]->uc_link\n", sizeof(ucontext_t), i);
				exit(1);
			}
			getcontext(Contexts.arr[i]->uc_link);
			Contexts.arr[i]->uc_link->uc_stack.ss_sp = Contexts.arr[i]->uc_stack.ss_sp;
			Contexts.arr[i]->uc_link->uc_stack.ss_size = Contexts.arr[i]->uc_stack.ss_size;
			Contexts.arr[i]->uc_link->uc_stack.ss_flags = Contexts.arr[i]->uc_stack.ss_flags;
			Contexts.arr[i]->uc_link->uc_link = 0;
			makecontext(Contexts.arr[i]->uc_link, (void (*) (void)) _endyield, 0);
		}*/
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
	makecontext(Contexts.manageEndingContext, (void (*) (void)) _endyield, 0);
	for (size_t i = 1; i < Contexts.maxInAdvanceStackBookings; i++) {
		Contexts.arr[i]->uc_link = Contexts.manageEndingContext;
	}
	Contexts.arrSize++;
	Contexts.awake[0] = 0;
	Contexts.awakeSize++;
}
void sleep_yield(struct pollfd pfd) {
	int idx = Contexts.awake[Contexts.currAwake];
	/*
	   {
	   printf("Contexts.awake: {");
	   for (size_t i = 0; i < Contexts.awakeSize; i++) {
	   printf("%d%s", Contexts.awake[i], i+1 == Contexts.awakeSize ? "" : ", ");
	   }
	   printf("}\n");
	   }*/
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	if (Contexts.awakeSize == Contexts.currAwake) {
		// This is the case where we are sleep yielding the right-most element of the awake array
		Contexts.currAwake--;
	}
	assert(Contexts.awakeSize > 0); //atleast the main loop must be awake so that it can yield again and again, where each yield will try to wake up asleep coroutines
	/*	{
		printf("Contexts.awake: {");
		for (size_t i = 0; i < Contexts.awakeSize; i++) {
		printf("%d%s", Contexts.awake[i], i+1 == Contexts.awakeSize ? "" : ", ");
		}
		printf("}\n");
		}
		*/
	Contexts.sleeping[Contexts.nfds] = idx;
	Contexts.fds[Contexts.nfds] = pfd;
	Contexts.nfds++;
	swapcontext(Contexts.arr[idx], Contexts.arr[Contexts.awake[Contexts.currAwake]]);
}
ssize_t coroutine_write(int fd, const char* buf, size_t bufSize) {
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLOUT
	};
	sleep_yield(pfd);
	return write(fd, buf, bufSize);
}
ssize_t coroutine_read(int fd, void* buf, size_t bufSize) {
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLIN
	};
	sleep_yield(pfd);
	return read(fd, buf, bufSize);
}
void coroutine_sleep(unsigned int seconds) {
	int idx = Contexts.awake[Contexts.currAwake];
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	if (Contexts.awakeSize == Contexts.currAwake) {
		// This is the case where we are time-sleeping the right-most element of the awake array
		Contexts.currAwake--;
	}
	assert(Contexts.awakeSize > 0); //atleast the main loop must be awake so that it can yield again and again, where each yield will try to wake up asleep coroutines
	timeheap_insert(&Contexts.timeSleepers, idx, seconds);
	swapcontext(Contexts.arr[idx], Contexts.arr[Contexts.awake[Contexts.currAwake]]);
}
void coroutines_gather() {
	while (Contexts.arrSize - Contexts.deadSize > 1) {
		coroutine_yield();
	}
}

/*int main() {
	test1();

	return 0;
}*/
