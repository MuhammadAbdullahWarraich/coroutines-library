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


#define MY_SOCK_PATH "127.0.0.1"
#define LISTEN_BACKLOG 50
#define BUF_SIZE 1024
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
	printf("hello?\n");
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

	sfd = socket(AF_INET, SOCK_STREAM, 0);
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
		printf("reached loop\n");
		peer_addr_size = sizeof(peer_addr);
		cfd = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_size);
		printf("accepted?\n");
		if (cfd == -1)
			handle_error("accept");

		printf("got till here\n");
		char *client_ip = inet_ntoa(peer_addr.sin_addr);
		printf("connected to client %s:%d\n", client_ip, ntohs(peer_addr.sin_port));
		ssize_t nread;
		pid_t is_parent = fork();
		if (-1 == is_parent) {
			printf("can't handle client %s:%d because of fork failure\n", client_ip, peer_addr.sin_port);
			continue;
		}
		if (0 == is_parent) {
			printf("right now i am in child process\n");
			while (1) {
				nread = read(cfd, buf, BUF_SIZE);
				if (nread == -1) {
					perror("read");
					exit(EXIT_FAILURE);
				}
				if (nread == 0) break;
				printf("Received %zu bytes from %s:%d: %s\n", nread, client_ip, ntohs(peer_addr.sin_port), buf);
				if (write(cfd, buf, nread) != nread) {
					fprintf(stderr, "partial/failed write\n");
					printf("partial/failed write\n");
					continue;
					//exit(EXIT_FAILURE);
				}
				printf("Echoed %zu bytes back to %s:%d: %s\n", nread, client_ip, ntohs(peer_addr.sin_port), buf);
			}
			return 0;
		}
	}
}
