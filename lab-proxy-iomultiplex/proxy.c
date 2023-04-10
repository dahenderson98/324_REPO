#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BIG_BUFF_SIZE 102400
#define MAXEVENTS 64
#define MAX_LINE 2048
#define BUF_SIZE 500
#define MAXLINE 8192
#define SO_REUSEPORT 15

/* Client request states */
#define READ_REQUEST 0
#define SEND_REQUEST 1
#define READ_RESPONSE 2
#define SEND_RESPONSE 3

struct client_info {
	int fd;				   /* Client file descriptor */
	int sfd;			   /* Server file descriptor */
	int state;	 		   /* Current state of the request */
	int in_req_offset;	   /* Last location written to inbound request buffer */
	int out_req_offset;	   /* Last location written to outbound request buffer */ 
	int in_res_offset;	   /* Last location written to inbound response buffer */
	int out_res_offset;    /* Last location written to outbound response buffer */
	char *in_buf_ptr;	   /* Pointer to inbound request buffer */
	char *out_buf_ptr;	   /* Pointer to outbound request buffer */
	char *res_ptr;         /* Pointer to response buffer */
    int n_read_from_c;     /* Total number of bytes read from the client */
};

// Socket data structures
struct addrinfo hints;
struct addrinfo *result, *rp;
int sfd, connfd, s, j, portindex;
size_t len;
ssize_t nread;
char buf[BUF_SIZE];
int hostindex = 2;
int af = AF_INET;
int addr_fam;
socklen_t addr_len = sizeof(struct sockaddr_storage);
struct sockaddr_in remote_addr_in;
struct sockaddr *remote_addr;
unsigned short remote_port;
struct sockaddr_in local_addr_in;
struct sockaddr *local_addr;
unsigned short local_port;

// EPOLL data structures
struct epoll_event event;
struct epoll_event events[MAXEVENTS];
int i;

struct client_info *new_client;
struct client_info *listener;
struct client_info *active_client;

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";
int debug = 0;

int parse_request(char *, char *, char *, char *, char *, char *);
int open_sfd();
void handle_new_clients(int sfd, int _efd);
void handle_client(struct client_info *active_client, int _efd); 
int all_headers_received(char *);
void test_parser();
void print_bytes(unsigned char *, int);


int main(int argc, char *argv[])
{
	
	int efd;
	// Create an epoll instance
	if ((efd = epoll_create1(0)) < 0) {
		perror("Error with epoll_create1");
		exit(EXIT_FAILURE);
	}

	// Get listening socket
	int sfd;
	if((sfd = open_sfd(argc, argv)) < 0) {
		perror("opening file server socket");
		return -1;
	}

	// Allocate memory for a new struct client_info, and populate it with
	// info for the listening socket
	listener = malloc(sizeof(struct client_info));
	listener->fd = sfd;

	// Register your listening proxy server socket with the epoll instance that you created, 
	// for reading and for edge-triggered monitoring
	event.data.ptr = listener;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event) < 0) {
		fprintf(stderr, "error adding event\n");
		exit(EXIT_FAILURE);
	}

	while (1) {
		int n;
		if ((n = epoll_wait(efd, events, MAXEVENTS, 1000)) < 0) {
			perror("epoll_wait");
			exit(1);
		}

		// Loop through all events and handle each appropriately
		for (i = 0; i < n; i++) {

			// Grab the data structure from the event, and cast it (appropriately) to a struct client_info *.
			active_client = (struct client_info *)(events[i].data.ptr);

			if (debug) printf("New event for fd %d\n", active_client->fd);

			if ((events[i].events & EPOLLERR) ||
					(events[i].events & EPOLLHUP) ||
					(events[i].events & EPOLLRDHUP)) {
				// An error has occured on this fd 
				fprintf(stderr, "epoll error on fd %d\n", events[i].data.fd);
				close(events[i].data.fd);
				free(active_client);
				continue;
			}

			int _efd = efd;
			if (sfd == active_client->fd) {
				handle_new_clients(active_client->fd, _efd);
			}
			else {
				handle_client(active_client, _efd);
			}
		}
	}
	free(listener);

	return 0;
}

int open_sfd (int argc, char *argv[]) {
	int portindex;
	unsigned short port;
	int address_family;
	int sock_type;
	struct sockaddr_in ipv4addr;

	int sfd;
	struct sockaddr *local_addr;
	socklen_t local_addr_len;

	portindex = 1;
	port = atoi(argv[portindex]);
	address_family = AF_INET;
	sock_type = SOCK_STREAM;

	/* SECTION A - populate address structures */
	ipv4addr.sin_family = address_family;
	ipv4addr.sin_addr.s_addr = INADDR_ANY; // listen on any/all IPv4 addresses
	ipv4addr.sin_port = htons(port);       // specify port explicitly, in network byte order

	// Assign local_addr and local_addr_len to ipv4addr
	local_addr = (struct sockaddr *)&ipv4addr;
	local_addr_len = sizeof(ipv4addr);

	/* SECTION B - setup socket */
	if((sfd = socket(address_family, sock_type, 0)) < -1) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

	// Allow server to reconnect to an existing port on failure
	int optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	// set listening file descriptor non-blocking
	if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

	if(bind(sfd, local_addr, local_addr_len) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	if (listen(sfd, 100) < 0) {
		perror("Could not listen");
		exit(EXIT_FAILURE);
	}

	return sfd;
}

void handle_new_clients (int fd, int _efd) {
	int connfd;
	socklen_t remote_addr_len = sizeof(struct sockaddr_storage);

	// Loop to accept any and all client connections
	while (1) {
		remote_addr_len = sizeof(struct sockaddr_storage);
		connfd = accept(fd, (struct sockaddr *)&remote_addr, &remote_addr_len);

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

		// Allocate memory for a new struct client_info, and populate it with info for the new client
		struct client_info *_new_client = (struct client_info *)malloc(sizeof(struct client_info));
		_new_client->fd = connfd;
		_new_client->in_req_offset = 0;
		_new_client->out_req_offset = 0;
		_new_client->in_res_offset = 0;
		_new_client->out_res_offset = 0;

		// Set up client's request buffers

		char *request = (char *)malloc(BIG_BUFF_SIZE);
		char *fwd = (char *)malloc(BIG_BUFF_SIZE);
		memset(&request[0], 0, BIG_BUFF_SIZE);
		memset(&fwd[0], 0, BIG_BUFF_SIZE);
		_new_client->in_buf_ptr = &request[0];
		_new_client->out_buf_ptr = &fwd[0];

		// Set up client's response buffer
		char *response = (char *)malloc(BIG_BUFF_SIZE);
		memset(&response[0], 0, BIG_BUFF_SIZE);
		_new_client->res_ptr = &response[0];

		_new_client->state = READ_REQUEST;

		// Register the client file descriptor for incoming events using edge-triggered monitoring
		event.data.ptr = _new_client;
		event.events = EPOLLIN | EPOLLET;
		
		if (epoll_ctl(_efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(1);
		}

		if (debug) printf("New connfd: %d\n", connfd);
	}
}

void handle_client(struct client_info *client, int efd) {
	if (client == NULL) return;

	if (debug) printf("Entering handle_client, FD: %d, State: %d, ",client->fd, client->state);

	if (client->state == READ_REQUEST) {
		// START READ_REQUEST
		if (debug) printf("Entering READ_REQUEST\n");
		
		// Read request from client
		// int rtotal_read = 0;
		int rbytes_read = 0;
		int debug = 1;
		do {
			rbytes_read = recv(client->fd, client->in_buf_ptr + client->in_req_offset, 512, 0);
			if (debug) printf("read loop -> rbytes_read=%d\n",rbytes_read);
			if (rbytes_read == 0){
				break;
			}
			else if (rbytes_read < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					// set offset
					// client->in_req_offset += rtotal_read;
					if (debug) printf("returning, errno is %d\n",errno);
					return;
				}
				else {
					perror("recv");
					close(client->fd);
					free(client);
					return;
				}
			}
			client->in_req_offset += rbytes_read;
		} while(!all_headers_received(client->in_buf_ptr) && (client->in_req_offset) < BIG_BUFF_SIZE);
		client->in_buf_ptr[client->in_req_offset] = 00;

		// Test parsing
		//if(debug) printf("Request Received: \n%s\n",client->in_buf_ptr);
		char method[16], hostname[64], port[8], path[64], headers[1024];
		if(parse_request(client->in_buf_ptr, method, hostname, port, path, headers)) {
			if(debug){
				printf("Parsed: \n");
				printf("METHOD: %s\n", method);
				printf("HOSTNAME: %s\n", hostname);
				printf("PORT: %s\n", port);
				printf("PATH: %s\n", path);
				printf("HEADERS: %s\n", headers);
			}
		} else {
			if(debug) printf("REQUEST INCOMPLETE\n");
		}
		/// Copy client's request to send to server
		// Add method to new request
		strcat(client->out_buf_ptr, method);
		strcat(client->out_buf_ptr, " ");

		// Add path to new request
		strcat(client->out_buf_ptr, path);
		strcat(client->out_buf_ptr, " ");

		// Add HTTP v1.0
		strcat(client->out_buf_ptr, "HTTP/1.0\r\n");

		// Add Host header
		char host_header[100] = "Host: ";
		strcat(host_header, hostname);
		if(strcmp(port, "80") != 0) {
			strcat(host_header, ":");
			strcat(host_header, port);
		}
		strcat(host_header, "\r\n");
		strcat(client->out_buf_ptr, host_header);
		// headers_len += strlen(host_header);

		// Add User-Agent header
		strcat(client->out_buf_ptr, user_agent_hdr);
		strcat(client->out_buf_ptr, "\r\n");
		// headers_len += strlen(user_agent_hdr) + 2;

		// Add Connection header
		strcat(client->out_buf_ptr, "Connection: close\r\n");
		// headers_len += 19;

		// Add Proxy-Connection header
		strcat(client->out_buf_ptr, "Proxy-Connection: close\r\n");
		// headers_len += 25;

		// End headers
		strcat(client->out_buf_ptr, "\r\n");

		//if(debug) printf("New Request: \n%s\n", client->out_buf_ptr);

		/// Create socket for communicating with server
		int server_sfd2;
		struct addrinfo hints2, *result2, *rp2;
		struct sockaddr_in remote_addr_in2, local_addr_in2;
		socklen_t addr_len2;
		struct sockaddr *remote_addr2, *local_addr2;
		char local_addr_str[INET6_ADDRSTRLEN];
		char remote_addr_str[INET6_ADDRSTRLEN];

		memset(&hints2, 0, sizeof(struct addrinfo));
		hints2.ai_family = af;    /* Allow IPv4, IPv6, or both, depending on
						what was specified on the command line. */
		hints2.ai_socktype = SOCK_STREAM; /* Datagram socket */
		hints2.ai_flags = 0;
		hints2.ai_protocol = 0;  /* Any protocol */

		s = getaddrinfo(hostname, port, &hints2, &result2);
		if(s != 0) {
			perror("server getaddrinfo");
			exit(EXIT_FAILURE);
		}

		for (rp2 = result2; rp2 != NULL; rp = rp2->ai_next) {
			server_sfd2 = socket(rp2->ai_family, rp2->ai_socktype, rp2->ai_protocol);
			if(server_sfd2 == -1) {
				printf("bad fd\n");
				continue;
			}

			addr_fam = rp2->ai_family;
			addr_len2 = rp2->ai_addrlen;

			remote_addr_in2 = *(struct sockaddr_in *)rp2->ai_addr;
			inet_ntop(addr_fam, &remote_addr_in2.sin_addr, remote_addr_str, addr_len2);
			// remote_port2 = ntohs(remote_addr_in2.sin_port);
			remote_addr2 = (struct sockaddr *)&remote_addr_in2;
			local_addr2 = (struct sockaddr *)&local_addr_in2;

			// fprintf(stderr, "Connecting to %s:%d (family: %d, len: %d)\n", remote_addr_str, remote_port, addr_fam, addr_len);

			/* if connect is successful, then break out of the loop; we
			* will use the current address */
			if(connect(server_sfd2, remote_addr2, addr_len2) != -1)
				break;  /* Success */

			close(server_sfd2);
		}

		if(rp2 == NULL) {   /* No address succeeded */
			fprintf(stderr,"Could not connect to server\n");
			exit(EXIT_FAILURE);
		}

		freeaddrinfo(result2);   /* No longer needed */

		s = getsockname(server_sfd2, local_addr2, &addr_len2);

		inet_ntop(addr_fam, &local_addr_in2.sin_addr, local_addr_str, addr_len2);

		// Set listening file descriptor non-blocking
		if (fcntl(server_sfd2, F_SETFL, fcntl(server_sfd2, F_GETFL, 0) | O_NONBLOCK) < 0) {
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}


		// Record server file descriptor, set client's state to "ready request to send to server"
		client->sfd = server_sfd2;
		client->state = SEND_REQUEST;

		// Deregister client file descriptor from epoll reading
		// if (epoll_ctl(efd, EPOLL_CTL_DEL, client->sfd, &event)< 0) {
		// 	perror("remove client from epoll reading");
		// 	return;
		// }
		// Register server file descriptor for writing
		event.data.ptr = client;
		event.events = EPOLLOUT | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_ADD, client->sfd, &event) < 0) {
			perror("add event for writing");
			return;
		}

		if (debug) printf("Leaving READ_REQUEST\n");
		// END READ_REQUEST
	}
	
	else if (client->state == SEND_REQUEST) {
		// START SEND_REQUEST
		if (debug) printf("Entering SEND_REQUEST\n");

		// Send new http request to server
		int total_sent = 0;
		int bytes_sent = 0;
		do {
			if(strlen(client->out_buf_ptr) - (client->out_req_offset + total_sent) < 512) {
				bytes_sent = write(client->sfd, client->out_buf_ptr + client->out_req_offset + total_sent, strlen(client->out_buf_ptr) - (client->out_req_offset + total_sent));
			}
			else {
				bytes_sent = write(client->sfd, client->out_buf_ptr + client->out_req_offset + total_sent, 512);
			}
			if(bytes_sent < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					client->out_req_offset += total_sent;
					return;
				}
				else {
					perror("send request");
					close(client->fd);
					close(client->sfd);
					free(client);
					return;
				}
			}
			total_sent += bytes_sent;
		} while(bytes_sent == 512 && (client->out_req_offset + total_sent) < strlen(client->out_buf_ptr));

		// // Deregister server file descriptor from epoll writing
		// if (epoll_ctl(efd, EPOLL_CTL_DEL, client->sfd, &event)< 0) {
		// 	perror("remove client from epoll writing");
		// 	return;
		// }
		// Register server socket for epoll reading
		event.data.ptr = client;
		event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(efd, EPOLL_CTL_MOD, client->sfd, &event) < 0) {
			fprintf(stderr, "error adding event\n");
			exit(EXIT_FAILURE);
		}

		// Set client state to "read server response"
		client->state = READ_RESPONSE;

		if (debug) printf("Leaving SEND_REQUEST\n");
		// END SEND_REQUEST
	}
	
	else if (client->state == READ_RESPONSE) {
		// START READ_RESPONSE
		if (debug) printf("Entering READ_RESPONSE\n");

		// Read response from server
		int restotal_read = 0;
		int resbytes_read = 0;
		do {
			resbytes_read = recv(client->sfd, client->res_ptr + client->in_res_offset + restotal_read, 512, 0);
			if (resbytes_read == 0) {
				// Increment inbound response offset, close server file descriptor
				client->in_res_offset += restotal_read;
				close(client->sfd);
				//printf("Response:\n%s\n",client->res_ptr);
				// Deregister client file descriptor from epoll reading
				// if (epoll_ctl(efd, EPOLL_CTL_DEL, client->fd, &event)< 0) {
				// 	perror("remove client from epoll reading");
				// 	return;
				// }
				// Register client file descriptor for writing
				event.data.ptr = client;
				event.events = EPOLLOUT | EPOLLET;
				if (epoll_ctl(efd, EPOLL_CTL_MOD, client->fd, &event) < 0) {
					perror("add event for writing");
					return;
				}

				// Update client's state to "send response to client"
				client->state = SEND_RESPONSE;
				
				if (debug) printf("Leaving READ_RESPONSE 1\n");
				return;
			}
			else if (resbytes_read < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					client->in_res_offset += restotal_read;
					return;
				}
				else {
					perror("read response");
					close(client->fd);
					close(client->sfd);
					free(client);
					return;
				}
			}
			restotal_read += resbytes_read;
		} while((client->in_res_offset + restotal_read) < BIG_BUFF_SIZE);

		if (debug) printf("Leaving READ_RESPONSE 2\n");
		// END READ_RESPONSE
	}
	
	else if (client->state == SEND_RESPONSE) {
		// START SEND_RESPONSE
		if (debug) printf("Entering SEND_RESPONSE\n");

		int rtotal_sent = 0;
		int rbytes_sent = 0;

		// Forward server's response to client
		do {
			if(client->in_res_offset - rtotal_sent < 512) {
				rbytes_sent = write(client->fd, client->res_ptr + client->out_res_offset + rtotal_sent, client->in_res_offset - (client->out_res_offset + rtotal_sent));
			}
			else {
				rbytes_sent = write(client->fd, client->res_ptr + client->out_req_offset + rtotal_sent, 512);
			}

			if (rbytes_sent < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					client->out_res_offset += rtotal_sent;
					return;
				}
				else {
					perror("send response");
					close(client->fd);
					free(client);
					return;
				}
			}
			rtotal_sent += rbytes_sent;
		} while ((client->out_res_offset + rtotal_sent) < client->in_res_offset);
		
		// Close client connection
		close(client->fd);
		free(client);

		if (debug) printf("Leaving SEND_RESPONSE\n");
		// END SEND_RESPONSE
	}
	
	else {
		// No state set for request
		fprintf(stderr, "Can't Handle Client: No state set for request\n");
		return;
	}
}

int all_headers_received(char *request) {
	return !strstr(request, "\r\n\r\n") ? 0 : 1;
}

int parse_request(char *request, char *method,
		char *hostname, char *port, char *path, char *headers) {

	// Ensure all buffers are empty
	int m_l = sizeof(method);
	memset(&method[0], 0, m_l);
	int ho_l = sizeof(hostname);
	memset(&hostname[0], 0, ho_l);
	int po_l = sizeof(port);
	memset(&port[0], 0, po_l);
	int pa_l = sizeof(path);
	memset(&path[0], 0, pa_l);
	int he_l = sizeof(headers);
	memset(&headers[0], 0, he_l);

	// Get request method
	char *first_sp = strchr(request, ' ');
	strncpy(method, request, first_sp - request);

	// Get request URL
	char url[MAX_LINE];
	char *sec_sp = strchr(first_sp + 1, ' ');
	strncpy(url, first_sp + 1, (sec_sp - 1) - first_sp);
	url[(sec_sp - 1) - first_sp] = 00;

	// From URL, get hostname, port, and path
	char *slashes = strstr(url, "//");
	char *sec_colon = strchr(slashes + 2, ':');
	char *third_fs = strchr(slashes + 2, '/');
	// If no port specified, port is set to 80 by default
	if(!sec_colon) {
		strncpy(hostname, slashes + 2, (third_fs - 1) - (slashes + 1));
		strcpy(port, "80");
	}
	// Else, parse port after hostname
	else {
		strncpy(hostname, slashes + 2, (sec_colon - 1) - (slashes + 1));
		hostname[(sec_colon - 1) - (slashes + 1)] = 00;
		strncpy(port, sec_colon + 1, third_fs - (sec_colon + 1));
		port[third_fs - (sec_colon + 1)] = 00;
	}

	// Get path from end of URL
	strcpy(path, third_fs);

	char *end_f_line = strstr(request, "\r\n");
	strcpy(headers, end_f_line + 2);
	
	return 1;
}

void test_parser() {
	int i;
	char method[16], hostname[64], port[8], path[64], headers[1024];

       	char *reqs[] = {
		"GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

		NULL
	};
	
	for (i = 0; reqs[i] != NULL; i++) {
		printf("Testing %s\n", reqs[i]);
		if (parse_request(reqs[i], method, hostname, port, path, headers)) {
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("HEADERS: %s\n", headers);
		} else {
			printf("REQUEST INCOMPLETE\n");
		}
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}
