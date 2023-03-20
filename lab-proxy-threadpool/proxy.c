#include <stdio.h>
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define MAX_LINE 256

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int all_headers_received(char *);
int parse_request(char *, char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);


int main()
{
	// test_parser();
	printf("%s\n", user_agent_hdr);
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
	if (!sec_colon) {
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
