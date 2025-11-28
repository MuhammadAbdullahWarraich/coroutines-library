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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdbool.h>
#include <time.h>
#include "timeheap.h"

// TODO: use variadic arguments to make a generic coroutine_add function
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
	TimeHeap timeSleepers;
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
	NULL,
	{0}
};
int _FromEnd = 0;
void wake(size_t i) {
	assert(i < Contexts.nfds);
	int idx = Contexts.sleeping[i];
	for (size_t x = i; x < -1 + Contexts.nfds; x++) {
		Contexts.sleeping[x] = Contexts.sleeping[x+1];
		Contexts.fds[x] = Contexts.fds[x+1];
	}
	Contexts.nfds--;
	assert(Contexts.awakeSize < Contexts.awakeCapacity);
	Contexts.awake[Contexts.awakeSize++] = idx;
}
void _try_to_wake_up_sleeping_coroutines() {
	// right now, it only works for waking those that sleep_yield on reading from a fd
	int state = poll(Contexts.fds, Contexts.nfds, 0);
	for (size_t i = 0; i < Contexts.nfds; i++) {
		if (Contexts.fds[i].revents & Contexts.fds[i].events) {
			wake(i);
		}
	}
}
void _try_to_wake_up_timeupped() {
	if (timeheap_isempty(&Contexts.timeSleepers)) return;
	while (time(NULL) >= timeheap_toptime(&Contexts.timeSleepers)) {
		int id = timeheap_pop(&Contexts.timeSleepers, NULL);
		assert(Contexts.awakeSize < Contexts.awakeCapacity);
		Contexts.awake[Contexts.awakeSize++] = id;
		if (timeheap_isempty(&Contexts.timeSleepers)) break;// TODO: make life simpler by checking this in timeheap_toptime and returning ((time_t) -1) in this case
	}
}
void yield() {
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
void endyield() {
	for (size_t i = Contexts.awake[Contexts.currAwake]; i < Contexts.arrSize - 1; i++) {
		Contexts.arr[i] = Contexts.arr[i+1];
	}
	Contexts.arrSize--;
	for (size_t i = Contexts.currAwake; i < -1 + Contexts.awakeSize; i++) {
		Contexts.awake[i] = Contexts.awake[i+1];
	}
	Contexts.awakeSize--;
	if (Contexts.awakeSize == Contexts.currAwake) {
		// This is the case where we are end yielding the right-most element of the awake array
		Contexts.currAwake--;
	}
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
ssize_t coroutine_write(int fd, char* buf, size_t bufSize) {
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLOUT
	};
	sleep_yield(pfd);
	return write(fd, buf, bufSize);
}
ssize_t coroutine_read(int fd, char* buf, size_t bufSize) {
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLIN
	};
	sleep_yield(pfd);
	return read(fd, buf, bufSize);
}
void counter(int id, int n) {
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		yield();
		/*	if (i == n - id) {
			printf("gonna sleep_yield!\n");
			struct pollfd pfd = {0};
			sleep_yield(pfd);
			printf("WOW! Guess I learnt something new, because this was supposed to never be printed!\n");
			}
			else yield();*/
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
void sleep_for(unsigned int seconds) {
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
void timesleeping_counter(int id, int n) {
	for (int i = 0; i < n; i++) {
		printf("id: %d ; i: %d\n", id, i);
		sleep_for(1);
	}
}
void coroutines_gather() {
	while (Contexts.arrSize > 1) {
		yield();
	}
}
void test3() {
	initialize();
	{
		int oldsize = setup_for_coroutine_add();
		makecontext(Contexts.arr[oldsize], (void (*) (void)) timesleeping_counter, 2, 1, 10);
	}
	coroutines_gather();
}
void test1() {
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
#define BUF_SIZE 500
void tcp_client() {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	size_t len;
	ssize_t nread;
	char buf[BUF_SIZE];


	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

	s = getaddrinfo("127.0.0.1", "42069", &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	   Try each address until we successfully connect(2).
	   If socket(2) (or connect(2)) fails, we (close the socket
	   and) try the next address. */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

	freeaddrinfo(result);           /* No longer needed */

	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	/* Send remaining command-line arguments as separate
	   datagrams, and read responses from server */
	char* messages[] = {
		"assalamualaikum sir, I am the first message",
		"hello, i am the second message!",
		"bye, I am the third message",
		"what is there left to say, other than that I am the fourth message"
	};
	size_t messagesSize = 4;
	for (int j = 0; j < messagesSize; j++) {
		len = strlen(messages[j]) + 1;
		/* +1 for terminating null byte */

		if (len > BUF_SIZE) {
			fprintf(stderr,
					"Ignoring long message in argument %d\n", j);
			continue;
		}
		if (coroutine_write(sfd, messages[j], len) != len) {
			fprintf(stderr, "partial/failed write\n");
			printf("partial/failed write\n");
			exit(EXIT_FAILURE);
		}
		printf("(id: %d)wrote %zd bytes: \"%s\"\n", Contexts.awake[Contexts.currAwake], len, messages[j]);

		nread = coroutine_read(sfd, buf, BUF_SIZE);
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		printf("(id: %d)read %zd bytes: \"%s\"\n", Contexts.awake[Contexts.currAwake], nread, buf);
	}
	printf("ending client\n");
	//exit(EXIT_SUCCESS);
}
void test2() {
	initialize();
	{
		int oldsize = setup_for_coroutine_add();
		makecontext(Contexts.arr[oldsize], (void (*) (void)) tcp_client, 0);
	}
	{
		int oldsize = setup_for_coroutine_add();
		makecontext(Contexts.arr[oldsize], (void (*) (void)) tcp_client, 0);
	}
	assert(Contexts.awakeSize == 3);
	while (Contexts.arrSize > 1) {
		yield();
	}
}

int main() {
	test3();

	return 0;
}
