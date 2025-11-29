#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "coroutines.h"

#define MY_SOCK_PATH "127.0.0.1"
#define LISTEN_BACKLOG 50
#define BUF_SIZE 1024
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)
struct HCArgs {
	int cfd;
	char* client_ip;
	uint16_t client_port;
};
typedef struct HCArgs HCArgs;
void handle_client(void* args) {
	HCArgs* hc_args = (HCArgs*) args;
	int cfd = hc_args->cfd;
	char* client_ip = hc_args->client_ip;
	uint16_t client_port = hc_args->client_port;
	char buf[BUF_SIZE];
	ssize_t nread;
	while (1) {
		nread = coroutine_read(cfd, buf, BUF_SIZE);
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		if (nread == 0) break;
		printf("Received %zu bytes from %s:%hu: %s\n", nread, client_ip, client_port, buf);
		if (coroutine_write(cfd, buf, nread) != nread) {
			fprintf(stderr, "partial/failed write\n");
			printf("partial/failed write\n");
			continue;
			//exit(EXIT_FAILURE);
		}
		printf("Echoed %zu bytes back to %s:%hu: %s\n", nread, client_ip, client_port, buf);
	}
}
int main() {
	coroutines_initialize();
	char buf[BUF_SIZE];
	struct in_addr ia;
	int ret_val = inet_aton(MY_SOCK_PATH, &ia);
	if (ret_val == 0)
		handle_error("inet_aton");
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(42069),
		.sin_addr = ia
	};
	int sfd, cfd;
	struct sockaddr_in peer_addr;
	socklen_t peer_addr_size;
	sfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sfd == -1)
		handle_error("socket");

	if (bind(sfd, (struct sockaddr *) &addr,
				sizeof(addr)) == -1)
		handle_error("bind");

	if (listen(sfd, LISTEN_BACKLOG) == -1)
		handle_error("listen");

	/* Now we can accept incoming connections one
	   at a time using accept(2) */
	while (1) {
		peer_addr_size = sizeof(peer_addr);
		while (1) {
			
			cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_size);
			if (-1 != cfd) break; 
			if (errno == EAGAIN || errno == EWOULDBLOCK) coroutine_yield();
			else {
				printf("accept4 failed. Error is: \"%s\"\n", strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		if (cfd == -1) handle_error("accept");
		char *client_ip = inet_ntoa(peer_addr.sin_addr);
		uint16_t client_port = ntohs(peer_addr.sin_port);
		printf("connected to client %s:%hu\n", client_ip, client_port);
		ssize_t nread;
		HCArgs hc_args = {
			.cfd = cfd,
			.client_ip = client_ip,
			.client_port = client_port
		};
		coroutine_add((void (*) (void))handle_client, 1, (void*) &hc_args);
		coroutine_yield();
	}
	return 0;
}
