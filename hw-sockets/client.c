#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 500

int main(int argc, char *argv[]) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s, j;
	size_t len;
	ssize_t nread;
	char buf[BUF_SIZE];
	int hostindex;
	int af;

	int addr_fam;
	socklen_t addr_len;

	struct sockaddr_in remote_addr_in;
	struct sockaddr_in6 remote_addr_in6;
	struct sockaddr *remote_addr;
	char remote_addr_str[INET6_ADDRSTRLEN];
	unsigned short remote_port;

	struct sockaddr_in local_addr_in;
	struct sockaddr_in6 local_addr_in6;
	struct sockaddr *local_addr;
	char local_addr_str[INET6_ADDRSTRLEN];
	unsigned short local_port;

	if (argc < 3 ||
		((strcmp(argv[1], "-4") == 0 || strcmp(argv[1], "-6") == 0) &&
			argc < 4)) {
		fprintf(stderr, "Usage: %s [ -4 | -6 ] host port msg...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Use only IPv4 (AF_INET) if -4 is specified;
	 * Use only IPv6 (AF_INET6) if -6 is specified;
	 * Try both if neither is specified. */
	af = AF_UNSPEC;
	if (strcmp(argv[1], "-4") == 0 ||
			strcmp(argv[1], "-6") == 0) {
		if (strcmp(argv[1], "-6") == 0) {
			af = AF_INET6;
		} else { // (strcmp(argv[1], "-4") == 0) {
			af = AF_INET;
		}
		hostindex = 2;
	} else {
		hostindex = 1;
	}

	/* Obtain address(es) matching host/port */

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = af;    /* Allow IPv4, IPv6, or both, depending on
				    what was specified on the command line. */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;  /* Any protocol */

	/* SECTION A - pre-socket setup; getaddrinfo() */

	s = getaddrinfo(argv[hostindex], argv[hostindex + 1], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	/* getaddrinfo() returns a list of address structures.
	   Try each address until we successfully connect(2).
	   If socket(2) (or connect(2)) fails, we (close the socket
	   and) try the next address. */

	/* SECTION B - pre-socket setup; getaddrinfo() */

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		/* Retrieve the values for the following, from the current
		 * result (rp) of getaddrinfo():
		 *
		 * addr_fam: the address family in use (applies to both local
		 *         and remote address).  This is either AF_INET (IPv4)
		 *         or AF_INET6 (IPv6).
		 * addr_len: length of the structure used to hold the address
		 *         (different for IPv4 and IPv6)
		 * remote_addr_in or remote_addr_in6: remote address and remote
		 *         port for IPv4 (addr_fam == AF_INET) or
		 *         IPv6 (addr_fam == AF_INET6).  The address is stored
		 *         in sin_addr (IPv4) or sin6_addr (IPv6), and the
		 *         port is stored in sin_port (IPv4) or sin6_port
		 *         (IPv6).
		 * remote_addr_str: string representation of remote IP address
		 * remote_addr: a struct sockaddr * that points to
		 *         remote_addr_in (where addr_fam == AF_INET) or
		 *         remote_addr_in6 (where addr_fam == AF_INET6)
		 * local_addr: a struct sockaddr * that points to
		 *         local_addr_in (where addr_fam == AF_INET) or
		 *         local_addr_in6 (where addr_fam == AF_INET6)
		 *         (Note that the *value* of local_addr_in or
		 *         local_addr_in6 will be populated later; for now
		 *         local_addr is simply assigned to point to one of
		 *         them.)
		 * remote_port: the remote port
		 * */
		addr_fam = rp->ai_family;
		addr_len = rp->ai_addrlen;
		if (addr_fam == AF_INET) {
			remote_addr_in = *(struct sockaddr_in *)rp->ai_addr;
			inet_ntop(addr_fam, &remote_addr_in.sin_addr,
					remote_addr_str, addr_len);
			remote_port = ntohs(remote_addr_in.sin_port);
			remote_addr = (struct sockaddr *)&remote_addr_in;
			local_addr = (struct sockaddr *)&local_addr_in;
		} else {
			remote_addr_in6 = *(struct sockaddr_in6 *)rp->ai_addr;
			inet_ntop(addr_fam, &remote_addr_in6.sin6_addr,
					remote_addr_str, addr_len);
			remote_port = ntohs(remote_addr_in6.sin6_port);
			remote_addr = (struct sockaddr *)&remote_addr_in6;
			local_addr = (struct sockaddr *)&local_addr_in6;
		}
		fprintf(stderr, "Connecting to %s:%d (family: %d, len: %d)\n",
				remote_addr_str, remote_port, addr_fam,
				addr_len);

		/* if connect is successful, then break out of the loop; we
		 * will use the current address */
		if (connect(sfd, remote_addr, addr_len) != -1)
			break;  /* Success */

		close(sfd);
	}

	if (rp == NULL) {   /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);   /* No longer needed */

	/* Retrieve the following values using getsockname()
	 *
	 * local_addr_in or local_addr_in6: local address and local
	 *         port for IPv4 (addr_fam == AF_INET) or
	 *         IPv6 (addr_fam == AF_INET6).  The address is stored
	 *         in sin_addr (IPv4) or sin6_addr (IPv6), and the
	 *         port is stored in sin_port (IPv4) or sin6_port
	 *         (IPv6).  Note that these are populated with the call
	 *         to getsockname() because local_addr points to
	 *         struct corresponding to the address family.
	 * local_addr_str: string representation of local IP address
	 * local_port: the local port
	 * */
	s = getsockname(sfd, local_addr, &addr_len);
	if (addr_fam == AF_INET) {
		inet_ntop(addr_fam, &local_addr_in.sin_addr,
				local_addr_str, addr_len);
		local_port = ntohs(local_addr_in.sin_port);
	} else {
		inet_ntop(addr_fam, &local_addr_in6.sin6_addr,
				local_addr_str, addr_len);
		local_port = ntohs(local_addr_in6.sin6_port);
	}
	fprintf(stderr, "Local socket info: %s:%d (family: %d, len: %d)\n",
			local_addr_str, local_port, addr_fam,
			addr_len);

	/* SECTION C - interact with server; send, receive, print messages */

	/* Send remaining command-line arguments as separate
	   datagrams, and read responses from server */

	// 1. Buffer stuff from stdin
	char buffer[4096];
	int total_read = 0;
	int bytes_read = 0;
	do {
		bytes_read = fread(buffer + total_read, 1, 512, stdin);
		total_read += bytes_read;
	} while(bytes_read == 512 && total_read <= 4096);

	// 2. Send buffered stuff from stdin to server
	int total_sent = 0;
	int bytes_sent = 0;
	do {
		if (total_read - total_sent < 512) {
			bytes_sent = write(sfd, buffer + total_sent, total_read - total_sent);
		}
		else {
			bytes_sent = write(sfd, buffer + total_sent, 512);
		}
		if (bytes_sent < 0) {
			break;
		}
		total_sent += bytes_sent;
	} while(bytes_sent == 512 && total_sent <= total_read);

	// 3. Read response from remote host
	char rbuffer[16384];
	int rtotal_read = 0;
	int rbytes_read = 0;
	do {
		rbytes_read = recv(sfd, rbuffer + rtotal_read, 512, 0);
		rtotal_read += rbytes_read;
		if (rbytes_read <= 0){
			break;
		}
	} while(rbytes_read > 0 && rtotal_read <= 16384);

	// 4. Write response to stdout
	int error = write(STDOUT_FILENO, rbuffer, rtotal_read);
	if (error < 0) {
		perror("Write http response");
		exit(-1);
	}

/*
	for (j = hostindex + 2; j < argc; j++) {
		len = strlen(argv[j]) + 1;
		/* +1 for terminating null byte * /

		if (len + 1 > BUF_SIZE) {
			fprintf(stderr,
					"Ignoring long message in argument %d\n", j);
			continue;
		}
		
		if (write(sfd, argv[j], len) != len) {
			fprintf(stderr, "partial/failed write\n");
			exit(EXIT_FAILURE);
		}
		printf("sent 1\n");

		/*
		nread = read(sfd, buf, BUF_SIZE);
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		printf("Received %zd bytes: %s\n", nread, buf);
		* /

	}
	*/

	exit(EXIT_SUCCESS);
}
