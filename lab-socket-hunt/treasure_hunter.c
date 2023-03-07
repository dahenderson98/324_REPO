// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823701059
// #define USERID 12345

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	// int server = atoi(argv[1]);
	// int port = atoi(argv[2]);
	int level = atoi(argv[3]);
	int seed = atoi(argv[4]);
	char* serverstr = argv[1];
	// char* portstr = argv[2];

	// Create first request
	unsigned char first_req[8];
	unsigned long user_id = htonl(USERID);
	unsigned short short_seed = htons(seed);

	first_req[0] = 0;
	first_req[1] = level;
	memcpy(&first_req[2],&user_id,4);
	memcpy(&first_req[6],&short_seed,2);
	
	//print_bytes(first_req,8);

	// Set up socket
	int addr_fam = AF_INET;
	int ai_socktype = SOCK_DGRAM;
	socklen_t addr_len;

	int sfd, s;

	struct sockaddr_in serv_addr;

	struct sockaddr_in remote_addr_in;
	struct sockaddr_in6 remote_addr_in6;
	struct sockaddr *remote_addr;
	
	struct sockaddr_in local_addr_in;
	struct sockaddr_in6 local_addr_in6;
	struct addrinfo hints;
	struct addrinfo *result;
	struct sockaddr *local_addr;

	unsigned short remote_port;
	unsigned short local_port;

	char remote_addr_str[INET6_ADDRSTRLEN];
	char local_addr_str[INET6_ADDRSTRLEN];
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = addr_fam;    /* Allow IPv4, IPv6, or both, depending on
				    what was specified on the command line. */
	hints.ai_socktype = ai_socktype; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;  /* Any protocol */

	/* SECTION A - pre-socket setup; getaddrinfo() */

	s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	addr_fam = result->ai_family;
	addr_len = result->ai_addrlen;
	if (addr_fam == AF_INET) {
		remote_addr_in = *(struct sockaddr_in *)result->ai_addr;
		inet_ntop(addr_fam, &remote_addr_in.sin_addr, serverstr, addr_len);
		remote_port = ntohs(remote_addr_in.sin_port);
		remote_addr = (struct sockaddr *)&remote_addr_in;
		local_addr = (struct sockaddr *)&local_addr_in;
	} else {
		remote_addr_in6 = *(struct sockaddr_in6 *)result->ai_addr;
		inet_ntop(addr_fam, &remote_addr_in6.sin6_addr,
				remote_addr_str, addr_len);
		remote_port = ntohs(remote_addr_in6.sin6_port);
		remote_addr = (struct sockaddr *)&remote_addr_in6;
		local_addr = (struct sockaddr *)&local_addr_in6;
	}
	
	if (result == NULL) {   /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);   /* No longer needed */

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

	/**** Level 0 *****/

	char treasure[1024];
	int t_ptr = 0;

	// Send request to server
	sendto(sfd, first_req, 8, 0, remote_addr, addr_len);

	// Read response from server
	unsigned char response_0[256];
	int r = recvfrom(sfd, response_0, 256, 0, NULL, NULL);
	if (r < 0){
		perror("Read server response");
		exit(-1);
	}

	// Print server's response
	// print_bytes(response_0, r);

	// Extract data from response
	int chunk_l_0 = response_0[0];
	if (chunk_l_0 == 0){
		printf("No more Treasure!\n");
		return 0;
	} 
	else if (chunk_l_0 > 127){
		printf("Error getting respnse... Error: %d\n",chunk_l_0);
		return -1;
	}
	// printf("%d\n", chunk_l_0);
	
	// char chunk_0[chunk_l_0 + 1];
	memcpy(&treasure[t_ptr], &response_0[1], chunk_l_0);
	treasure[t_ptr + chunk_l_0] = 0;
	t_ptr += chunk_l_0;
	// printf("%s\n", treasure);

	int op_code_0 = response_0[chunk_l_0 + 1];
	// printf("%d\n", op_code_0);

	unsigned short op_param_0;
	memcpy(&op_param_0, &response_0[chunk_l_0 + 2], 2);
	unsigned short op_param_rev_0 = ntohs(op_param_0);
	// printf("%x\n", op_param_0);

	// Op-Code 1 -> update remote port
	if (op_code_0 == 1) {
		remote_addr_in.sin_port = htons(op_param_rev_0);
	}

	unsigned int nonce_0;
	memcpy(&nonce_0, &response_0[chunk_l_0 + 4], 4);
	// printf("%x\n", nonce_0);
	
	/**** Level 2-n *****/
	char request[4];
	unsigned int nonce_0_h = ntohl(nonce_0);
	nonce_0_h += 1;
	unsigned int nonce_rev_0 = htonl(nonce_0_h);
	// printf("%x\n",nonce_rev_0);
	memcpy(&request, &nonce_rev_0, 4);

	int chunk_l = 0;

	do {
		// printf("Starting 2nd\n");
		// break;
		// Send request to server
		sendto(sfd, request, 4, 0, remote_addr, addr_len);

		// Read response from server
		unsigned char response[256];
		int r = recvfrom(sfd, response, 256, 0, NULL, NULL);
		if (r < 0){
			perror("Read server response");
			exit(-1);
		}

		// Print server's response
		//print_bytes(response, r);

		// Extract data from response
		chunk_l = response[0];
		if (chunk_l > 127){
			printf("Error getting response... Error: %d\n",chunk_l);
			return -1;
		}
		// printf("%d\n", chunk_l_0);
		
		memcpy(&treasure[t_ptr], &response[1], chunk_l);
		treasure[t_ptr + chunk_l] = 0;
		t_ptr += chunk_l;
		// printf("%s", chunk_0);

		int op_code = response[chunk_l + 1];
		// printf("%d\n", op_code_0);

		unsigned short op_param;
		memcpy(&op_param, &response[chunk_l + 2], 2);
		// printf("%x\n", op_param_0);

		unsigned short op_param_rev = ntohs(op_param);
		// printf("%x\n", op_param);

		// Op-Code 1 -> update remote port
		if (op_code == 1) {
			remote_addr_in.sin_port = htons(op_param_rev);
		}

		unsigned int nonce;
		memcpy(&nonce, &response[chunk_l + 4], 4);
		// printf("%x\n", nonce_0);
		
		/**** Next req *****/
		unsigned int nonce_h = ntohl(nonce);
		nonce_h += 1;
		unsigned int nonce_rev = htonl(nonce_h);
		// printf("%x\n",nonce_rev);
		memcpy(&request, &nonce_rev, 4);

		// printf("Got another response\n");
	} while (chunk_l > 0);

	// Print final treasure
	printf("%s\n", treasure);

	return 0;
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
