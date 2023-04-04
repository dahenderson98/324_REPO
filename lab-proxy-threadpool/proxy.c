#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BIG_BUFF_SIZE 50000
#define MAX_LINE 256
#define BUF_SIZE 500
#define MAXLINE 8192
#define SO_REUSEPORT 15
#define NTHREADS 8
#define SBUFSIZE 5

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int all_headers_received(char *);
int parse_request(char *, char *, char *, char *, char *, char *);
int open_sfd();
void handle_client(int);
void *handle_clients(void *); 
void test_parser();
void print_bytes(unsigned char *, int);

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

// Threadpool data structures

/* $begin sbuft */
typedef struct {
    int *buf;          /* Buffer array */         
    int n;             /* Maximum number of slots */
    int front;         /* buf[(front+1)%n] is first item */
    int rear;          /* buf[rear%n] is last item */
    sem_t mutex;       /* Protects accesses to buf */
    sem_t slots;       /* Counts available slots */
    sem_t items;       /* Counts available items */
} sbuf_t;
/* $end sbuft */

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

/* Create an empty, bounded, shared FIFO buffer with n slots */
/* $begin sbuf_init */
void sbuf_init(sbuf_t *sp, int n) {
    sp->buf = calloc(n, sizeof(int)); 
    sp->n = n;                       /* Buffer holds max of n items */
    sp->front = sp->rear = 0;        /* Empty buffer iff front == rear */
    sem_init(&sp->mutex, 0, 1);      /* Binary semaphore for locking */
    sem_init(&sp->slots, 0, n);      /* Initially, buf has n empty slots */
    sem_init(&sp->items, 0, 0);      /* Initially, buf has zero data items */
}
/* $end sbuf_init */

/* Clean up buffer sp */
/* $begin sbuf_deinit */
void sbuf_deinit(sbuf_t *sp) {
    free(sp->buf);
}
/* $end sbuf_deinit */

/* Insert item onto the rear of shared buffer sp */
/* $begin sbuf_insert */
void sbuf_insert(sbuf_t *sp, int item) {
    // printf("before sem_wait(&sp->slots)\n"); fflush(stdout);
    sem_wait(&sp->slots);                          /* Wait for available slot */
    // printf("after sem_wait(&sp->slots)\n"); fflush(stdout);
    sem_wait(&sp->mutex);                          /* Lock the buffer */
    sp->buf[(++sp->rear)%(sp->n)] = item;   /* Insert the item */
    sem_post(&sp->mutex);                          /* Unlock the buffer */
    // printf("before sem_post(&sp->items)\n"); fflush(stdout);
    sem_post(&sp->items);                          /* Announce available item */
    // printf("after sem_post(&sp->items)\n"); fflush(stdout);
}
/* $end sbuf_insert */

/* Remove and return the first item from buffer sp */
/* $begin sbuf_remove */
int sbuf_remove(sbuf_t *sp) {
    int item;
    // printf("before sem_wait(&sp->items)\n"); fflush(stdout);
    sem_wait(&sp->items);                          /* Wait for available item */
    // printf("after sem_wait(&sp->items)\n"); fflush(stdout);
    sem_wait(&sp->mutex);                          /* Lock the buffer */
    item = sp->buf[(++sp->front)%(sp->n)];  /* Remove the item */
    sem_post(&sp->mutex);                          /* Unlock the buffer */
    // printf("before sem_post(&sp->slots)\n"); fflush(stdout);
    sem_post(&sp->slots);                          /* Announce available slot */
    // printf("after sem_post(&sp->slots)\n"); fflush(stdout);
    return item;
}
/* $end sbuf_remove */
/* $end sbufc */

sbuf_t sbuf; /* Shared buffer of connected descriptors */

int main(int argc, char *argv[])
{
	pthread_t tid;

	if((sfd = open_sfd(argc, argv)) < 0) {
		perror("opening file server socket");
		return -1;
	}

	// Create worker threads
	sbuf_init(&sbuf, SBUFSIZE);
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&tid, NULL, handle_clients, NULL);

	while(1) {
		// accept() a client connection
		connfd = accept(sfd, remote_addr, &addr_len);

		// Insert connfd in buffer
		sbuf_insert(&sbuf, connfd);
	}

	return 0;
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

int open_sfd(int argc, char *argv[]) {
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

	int optval = 1;
	setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	if(bind(sfd, local_addr, local_addr_len) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

	listen(sfd, 100);

	return sfd;
}

void handle_client(int connfd) {
	char request[BIG_BUFF_SIZE], fwd[BIG_BUFF_SIZE];
	memset(&request[0], 0, BIG_BUFF_SIZE);
	memset(&fwd[0], 0, BIG_BUFF_SIZE);
	int rtotal_read = 0;
	int rbytes_read = 0;
	int debug = 0;
	do {
		rbytes_read = recv(connfd, request + rtotal_read, 512, 0);
		rtotal_read += rbytes_read;
		if(rbytes_read <= 0){
			break;
		}
	} while(!all_headers_received(request) && rbytes_read > 0 && rtotal_read <= BIG_BUFF_SIZE);
	request[rtotal_read] = 00;
	
	// Test parsing
	if(debug) printf("Request Received: \n%s\n",request);
	char *method, *hostname, *port, *path, *headers;
	method = (char *) malloc(16);
	hostname = (char *) malloc(64);
	port = (char *) malloc(8);
	path = (char *) malloc(64);
	headers = (char *) malloc(1024);
	if(parse_request(request, method, hostname, port, path, headers)) {
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
	strcat(fwd, method);
	strcat(fwd, " ");

	// Add path to new request
	strcat(fwd, path);
	strcat(fwd, " ");

	// Add HTTP v1.0
	strcat(fwd, "HTTP/1.0\r\n");

	// Add Host header
	char host_header[100] = "Host: ";
	strcat(host_header, hostname);
	if(strcmp(port, "80") != 0) {
		strcat(host_header, ":");
		strcat(host_header, port);
	}
	strcat(host_header, "\r\n");
	strcat(fwd, host_header);

	// Add User-Agent header
	strcat(fwd, user_agent_hdr);
	strcat(fwd, "\r\n");

	// Add Connection header
	strcat(fwd, "Connection: close\r\n");

	// Add Proxy-Connection header
	strcat(fwd, "Proxy-Connection: close\r\n");

	// End headers
	strcat(fwd, "\r\n");

	if(debug) printf("New Request: \n%s\n", fwd);

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
		remote_addr2 = (struct sockaddr *)&remote_addr_in2;
		local_addr2 = (struct sockaddr *)&local_addr_in2;

		// If connect is successful, then break out of the loop; we will use the current address
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

	// Send new http request to server
	int total_sent = 0;
	int bytes_sent = 0;
	do {
		if(strlen(fwd) - total_sent < 512) {
			bytes_sent = write(server_sfd2, fwd + total_sent, strlen(fwd) - total_sent);
		}
		else {
			bytes_sent = write(server_sfd2, fwd + total_sent, 512);
		}
		if(bytes_sent < 0) {
			break;
		}
		total_sent += bytes_sent;
	} while(bytes_sent == 512 && total_sent <= strlen(fwd));

	if (debug) printf("before server response\n");

	// Read response from server
	char response[BIG_BUFF_SIZE];
	memset(&response[0], 0, BIG_BUFF_SIZE);
	int restotal_read = 0;
	int resbytes_read = 0;
	do {
		resbytes_read = recv(server_sfd2, response + restotal_read, 512, 0);
		if(resbytes_read < 0){
			perror("reading from server");
			break;
		} 
		restotal_read += resbytes_read;
	} while(resbytes_read > 0 && restotal_read <= BIG_BUFF_SIZE);

	close(server_sfd2);

	if (debug) printf("Bytes read: %d\nResponse from server:\n%s\n", restotal_read, response);

	// Forward server's response to client
	if(write(connfd, response, restotal_read) < 0) {
		perror("returning response to client");
	}

	free(hostname);
	free(port);
	free(path);
	free(headers);

	sleep(2);
}

void *handle_clients(void *vargp) {
	pthread_detach(pthread_self());

	while(1) {
		int connfd = sbuf_remove(&sbuf);
		handle_client(connfd);
		close(connfd);
	}
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
