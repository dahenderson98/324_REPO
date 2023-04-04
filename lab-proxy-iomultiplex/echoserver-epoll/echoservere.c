#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/epoll.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>

#define MAXEVENTS 64
#define MAXLINE 2048


struct client_info {
	int fd;
	int total_length;
	char desc[1024];
};

int main(int argc, char **argv) 
{
	int portindex;
	unsigned short port;
	int address_family;
	int sock_type;
	struct sockaddr_in local_addr_in;
	struct sockaddr_in6 local_addr_in6;

	int sfd;
	struct sockaddr_storage *remote_addr;
	struct sockaddr *local_addr;
	socklen_t local_addr_len, remote_addr_len;

	ssize_t nread;

	int efd;
	struct epoll_event event;
	struct epoll_event events[MAXEVENTS];
	int i;
	int len;

	struct client_info *new_client;
	struct client_info *listener;
	struct client_info *active_client;

	int connfd;

	size_t n; 
	char buf[MAXLINE]; 

	/* Check usage */
	if (!(argc == 2 || (argc == 3 &&
			(strcmp(argv[1], "-4") == 0 || strcmp(argv[1], "-6") == 0)))) {
		fprintf(stderr, "Usage: %s [ -4 | -6 ] port\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc == 2) {
		portindex = 1;
	} else {
		portindex = 2;
	}

	/* Use IPv4 by default (or if -4 is specified);
	 * If -6 is specified, then use IPv6 instead. */
	if (argc == 2 || strcmp(argv[portindex], "-4") == 0) {
		address_family = AF_INET;
	} else {
		address_family = AF_INET6;
	}

	port = atoi(argv[portindex]);
	sock_type = SOCK_STREAM;

	if (address_family == AF_INET) {
		local_addr_in.sin_family = address_family;
		local_addr_in.sin_addr.s_addr = INADDR_ANY; // listen on any/all IPv4 addresses
		local_addr_in.sin_port = htons(port);       // specify port explicitly, in network byte order

		// Assign local_addr and local_addr_len to ipv4addr
		local_addr = (struct sockaddr *)&local_addr_in;
		local_addr_len = sizeof(local_addr_in);
	} else { // address_family == AF_INET6
		local_addr_in6.sin6_family = address_family;
		local_addr_in6.sin6_addr = in6addr_any;     // listen on any/all IPv6 addresses
		local_addr_in6.sin6_port = htons(port);     // specify port explicitly, in network byte order

		// Assign local_addr and local_addr_len to ipv6addr
		local_addr = (struct sockaddr *)&local_addr_in6;
		local_addr_len = sizeof(local_addr_in6);
	}


	if ((sfd = socket(address_family, sock_type, 0)) < 0) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	if (bind(sfd, local_addr, local_addr_len) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sfd, 100) < 0) {
		perror("Could not listen");
		exit(EXIT_FAILURE);
	}

	// set listening file descriptor non-blocking
	if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

	if ((efd = epoll_create1(0)) < 0) {
		perror("Error with epoll_create1");
		exit(EXIT_FAILURE);
	}

	// allocate memory for a new struct client_info, and populate it with
	// info for the listening socket
	listener = malloc(sizeof(struct client_info));
	listener->fd = sfd;
	sprintf(listener->desc, "Listen file descriptor (accepts new clients)");

	// register the listening file descriptor for incoming events using
	// edge-triggered monitoring
	event.data.ptr = listener;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		// wait for event to happen (-1 == no timeout)
		n = epoll_wait(efd, events, MAXEVENTS, -1);

		for (i = 0; i < n; i++) {
			// grab the data structure from the event, and cast it
			// (appropriately) to a struct client_info *.
			active_client = (struct client_info *)(events[i].data.ptr);

			printf("New event for fd %d (%s)\n", active_client->fd, active_client->desc);

			if ((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(events[i].events & EPOLLRDHUP)) {
				/* An error has occured on this fd */
				fprintf(stderr, "epoll error on %s\n", active_client->desc);
				close(active_client->fd);
				free(active_client);
				continue;
			}

			if (sfd == active_client->fd) {
				// loop until all pending clients have been accepted
				while (1) {
					remote_addr_len = sizeof(struct sockaddr_storage);
					connfd = accept(active_client->fd, (struct sockaddr *)&remote_addr, &remote_addr_len);

					if (connfd < 0) {
						if (errno == EWOULDBLOCK ||
								errno == EAGAIN) {
							// no more clients ready to accept
							break;
						} else {
							perror("accept");
							exit(EXIT_FAILURE);
						}
					}

					// set client file descriptor non-blocking
					if (fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
						fprintf(stderr, "error setting socket option\n");
						exit(1);
					}

					// allocate memory for a new struct
					// client_info, and populate it with
					// info for the new client
					new_client = (struct client_info *)malloc(sizeof(struct client_info));
					new_client->fd = connfd;
					new_client->total_length = 0;
					sprintf(new_client->desc, "Client with file descriptor %d", connfd);

					// register the client file descriptor
					// for incoming events using
					// edge-triggered monitoring
					event.data.ptr = new_client;
					event.events = EPOLLIN | EPOLLET;
					if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
						fprintf(stderr, "error adding event\n");
						exit(1);
					}
				}
			} else {
				// read from socket until (1) the remote side
				// has closed the connection or (2) there is no
				// data left to be read.
				while (1) {
					len = recv(active_client->fd, buf, MAXLINE, 0);
					if (len == 0) { // EOF received
						// closing the fd will automatically
						// unregister the fd from the efd
						close(active_client->fd);
						free(active_client);
						break;
					} else if (len < 0) {
						if (errno == EWOULDBLOCK ||
								errno == EAGAIN) {
							// no more data to be read
						} else {
							perror("client recv");
							close(active_client->fd);
							free(active_client);
						}
						break;
					} else {
						active_client->total_length += len;
						printf("Received %d bytes (total: %d)\n", len, active_client->total_length);
						send(active_client->fd, buf, len, 0);
					}
				}
			}
		}
	}
	free(listener);
}
