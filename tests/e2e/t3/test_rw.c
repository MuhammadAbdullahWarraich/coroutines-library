#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "coroutines.h"
#define BUFSIZE 1024
struct RWArgs {
	int id;
	int fd;
};
typedef struct RWArgs RWArgs;
const char* messages[] = {
	"assalamualaikum sir, I am the first message",
	"hello, i am the second message!",
	"bye, I am the third message",
	"what is there left to say, other than that I am the fourth message"
};
const size_t messagesSize = 4;
void reader(void* args) {
	RWArgs* cargs = (RWArgs*) args;
	int id = cargs->id;
	int fd = cargs->fd;
	char buf[BUFSIZE];
	size_t messagesSize = 4;
	for (int j = 0; j < messagesSize; j++) {
		size_t len = strlen(messages[j]) + 1;
		/* +1 for terminating null byte */

		if (len > BUFSIZE) {
			fprintf(stderr, "Ignoring long message in argument %d\n", j);
			continue;
		}
		ssize_t nread = -1;
		nread = coroutine_read(fd, buf, BUFSIZE);
		if (nread == -1 || nread != len) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		buf[nread] = '\0';// isn't needed, because we know what we expect, but anyways
		assert(strlen(messages[j])+1 == len);
		assert(0 == strncmp(buf, messages[j], len));
		printf("(id: %d)read %zd bytes: \"%s\"\n", id, nread, buf);
	}
}
void writer(void* args) {
	RWArgs* cargs = (RWArgs*) args;
	int id = cargs->id;
	int fd = cargs->fd;
	for (int j = 0; j < messagesSize; j++) {
		size_t len = strlen(messages[j]) + 1;
		/* +1 for terminating null byte */
		ssize_t n = -1;
		unsigned int idx = 0;
		while (n != 0) {
// if pipe gets full, we will sleep until reader reads some stuff. given the small length of each message, we don't really need to think about this stuff, but it looks cooler this way
			n = coroutine_write(fd, idx+messages[j], len-idx);
			if (len == idx+n) break;
			if (n == -1) {
				perror("write");
				exit(EXIT_FAILURE);
			}
			idx += n;
		}
		coroutine_sleep(1);
	}
}
void _test_pipe_size_limit() {
	// TODO: complete this function and run it, just for learning
	int fds[2];
	int res = pipe(fds);
	assert(res == 0 || res == 1);// if this fails, were the docs wrong?!
	if (res == -1) {
		printf("pipe failed to make a pipe. Error is: \"%s\"\n", strerror(errno));
		exit(1);
	}
}
void test() {
	int fds[2];
        int res = pipe(fds);
	assert(res == 0 || res == 1);// if this fails, were the docs wrong?!
	if (res == -1) {
		printf("pipe failed to make a pipe. Error is: \"%s\"\n", strerror(errno));
		exit(1);
	}
	//printf("size of long is: %zu\n", sizeof(long));
	coroutines_initialize();
	RWArgs reader_args = {
		.id = 1,
		.fd = fds[0]
	};
	RWArgs writer_args = {
		.id = 2,
		.fd = fds[1]
	};
	coroutine_add((void (*) (void)) reader, (void *) &reader_args);
	coroutine_yield();
	coroutine_add((void (*) (void)) writer, (void *) &writer_args);
	coroutines_gather();
}
int main() {
	test();
	return 0;
}
