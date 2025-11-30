#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "coroutines.h"

struct TCPClientArgs {

	int id;
};
typedef struct TCPClientArgs TCPClientArgs;
#define BUF_SIZE 500
void tcp_client(void* args) {
	TCPClientArgs* tcpargs = (TCPClientArgs*) args;
	int id = tcpargs->id;
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
		printf("(id: %d)wrote %zd bytes: \"%s\"\n", id, len, messages[j]);

		nread = coroutine_read(sfd, buf, BUF_SIZE);
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		printf("(id: %d)read %zd bytes: \"%s\"\n", id, nread, buf);
	}
	printf("ending client\n");
	//exit(EXIT_SUCCESS);
}
int main() {
	TCPClientArgs args1 = { .id = 1 };
	TCPClientArgs args2 = { .id = 2 };
	coroutines_initialize();
	coroutine_add((void (*) (void))tcp_client, 1, (void*) &args1);
	coroutine_add((void (*) (void))tcp_client, 1, (void*) &args2);
	coroutines_gather();
	coroutines_cleanup();// unnecessary here, because the memory will be freed with the end of the process anyways and we are almost at the end of the process
	return 0;
}
