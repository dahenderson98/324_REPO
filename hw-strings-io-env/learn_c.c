#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUFSIZE 30

void memprint(char *, char *, int);
void part0(void);
int part1(void);
void part2(int);
void part3();
void part4();
void part5(char *);
void part6();

int main(int argc, char *argv[]) {
	int len;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		exit(1);
	}
	part0();
	len = part1();
	part2(len);
	part3();
	part4();
	part5(argv[1]);
	part6();
}

void memprint(char *s, char *fmt, int len) {
	// iterate through each byte/character of s, and print each out
	int i;
	char fmt_with_space[8];

	sprintf(fmt_with_space, "%s ", fmt);
	for (i = 0; i < len; i++) {
		printf(fmt_with_space, s[i]);
	}
	printf("\n");
}


void part0() {
	printf("===== Intro =====\n");
	char s1[] = { 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x0a };
	char s2[] = { 0xe5, 0x8f, 0xb0, 0xe7, 0x81, 0xa3, 0x0a };
	char s3[] = { 0xf0, 0x9f, 0x98, 0x82, 0x0a };

	write(STDOUT_FILENO, s1, 6);
	write(STDOUT_FILENO, s2, 7);
	write(STDOUT_FILENO, s3, 5);
}

int part1() {
	// Do not change any of the variable declarations here.
	char s1[] = "hello";
	char s2[10];

	int s1_len = 0;
	int s2_len = 0;

	// Make any other variable declarations here.

	s1_len = sizeof(s1);
	s2_len = sizeof(s2);

	printf("===== Question 1 =====\n");
	printf("%u\n", s1_len);
	printf("===== Question 2 =====\n");
	printf("%u\n", s2_len);
	printf("===== Question 3 =====\n");
	
	return s1_len;
}

void part2(int len) {
	char s1[] = "hello";
	char s2[1024];
	char *s3 = s1;

	// Copy len bytes of s1 to s2.
	memcpy(s2, s1, len);

	printf("===== Question 4 =====\n");
	printf("%lu\n",(long unsigned int)&s1);
	printf("%lu\n",(long unsigned int)&s2);
	printf("%lu\n",(long unsigned int)&s3);
	printf("===== Question 5 =====\n");
	printf("%lu\n",(long unsigned int)s1);
	printf("%lu\n",(long unsigned int)s2);
	printf("%lu\n",(long unsigned int)s3);
	printf("===== Question 6 =====\n");
	printf("%s\n",s1);
	printf("%s\n",s2);
	printf("%s\n",s3);
	printf("===== Question 7 =====\n");
	if (s1==s2) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	if (s1==s3) {
		printf("s1==s3\n");
	}
	else {
		printf("s1!=s3\n");
	}
	if (s2==s3) {
		printf("s2==s3\n");
	}
	else {
		printf("s2!=s3\n");
	}
	printf("===== Question 8 =====\n");

	if (strcmp(s1,s2)) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	if (strcmp(s1,s3)) {
		printf("s1==s3\n");
	}
	else {
		printf("s1!=s3\n");
	}
	if (strcmp(s2,s3)) {
		printf("s2==s3\n");
	}
	else {
		printf("s2!=s3\n");
	}
	
	s1[1] = 'a';
	printf("===== Question 9 =====\n");
	printf("%s\n",s1);
	printf("%s\n",s2);
	printf("%s\n",s3);

	printf("===== Question 10 =====\n");
	if (s1==s2) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	if (s1==s3) {
		printf("s1==s3\n");
	}
	else {
		printf("s1!=s3\n");
	}
	if (s2==s3) {
		printf("s2==s3\n");
	}
	else {
		printf("s2!=s3\n");
	}
	printf("===== Question 11 =====\n");
	if (!strcmp(s1,s2)) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	if (!strcmp(s1,s3)) {
		printf("s1==s3\n");
	}
	else {
		printf("s1!=s3\n");
	}
	if (!strcmp(s2,s3)) {
		printf("s2==s3\n");
	}
	else {
		printf("s2!=s3\n");
	}
}

void part3() {
	char s1[] = { 'a', 'b', 'c', 'd', 'e', 'f' };
	char s2[] = { 97, 98, 99, 100, 101, 102 };
	char s3[] = { 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

	printf("===== Question 12 =====\n");
	if (!memcmp(s1,s2,6)) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	if (!memcmp(s1,s3,6)) {
		printf("s1==s3\n");
	}
	else {
		printf("s1!=s3\n");
	}
	if (!memcmp(s2,s3,6)) {
		printf("s2==s3\n");
	}
	else {
		printf("s2!=s3\n");
	}
}

void part4() {
	char s1[] = { 'a', 'b', 'c', '\0', 'd', 'e', 'f', '\0' };
	char s2[] = { 'a', 'b', 'c', '\0', 'x', 'y', 'z', '\0' };
	char s3[8];
	char s4[8];
	int myval = 42;

	printf("===== Question 13 =====\n");
	if (!memcmp(&s1,&s2,6)) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	
	printf("===== Question 14 =====\n");
	if (!strcmp(s1,s2)) {
		printf("s1==s2\n");
	}
	else {
		printf("s1!=s2\n");
	}
	printf("===== Question 15 =====\n");
	memset(s3, 'z', 8);
	memprint(s3, "%02x", 8);
	memset(s4, 'z', 8);
	memprint(s4, "%02x", 8);
	printf("===== Question 16 =====\n");
	strcpy(s3, s1);
	memprint(s3, "%02x", sizeof(s3));
	printf("===== Question 17 =====\n");
	//sprintf(s4, "%s %d\n", s1, myval);
	memprint(s4, "%02x", sizeof(s4));
}

void part5(char *filename) {
	char buf[BUFSIZE];
	size_t nread, totread;
	int fd1, fd2;
	int i;
	int ret;
	FILE *f;

	printf("===== Question 18 =====\n");
	printf("0,1,2");
	printf("===== Question 19 =====\n");
	memset(buf,'z',sizeof(buf));
	memset(buf+24,0,1);
	memprint(buf,"%02x",sizeof(buf));
	printf("===== Question 20 =====\n");
	printf("%s\n", buf);
	write(1,buf,BUFSIZE);
	printf("\n");
	printf("===== Question 21 =====\n");
	fprintf(stderr, buf,"%02x");
	printf("\n");
	write(2,buf,BUFSIZE);
	printf("\n");
	printf("===== Question 22 =====\n");
	fd1 = open(filename,O_RDONLY);
	fd2 = fd1;
	printf("%d\n",fd1);
	printf("%d\n",fd2);

	nread = 0;
	totread = 0;
	printf("===== Question 23 =====\n");
	nread = read(fd1,buf,4);
	totread += nread;
	printf("nread: %d\n",(int)nread);
	printf("totread: %d\n",(int)totread);
	memprint(buf,"%02x",BUFSIZE);
	printf("===== Question 24 =====\n");
	nread = read(fd2,buf + totread,4);
	totread += nread;
	printf("nread: %d\n",(int)nread);
	printf("totread: %d\n",(int)totread);
	memprint(buf,"%02x",BUFSIZE);

	printf("===== Question 25 =====\n");
	nread = read(fd2,buf + totread,50);
	totread += nread;
	printf("nread: %d\n",(int)nread);
	printf("totread: %d\n",(int)totread);
	memprint(buf,"%02x",BUFSIZE);

	printf("===== Question 26 =====\n");
	nread = read(fd2,buf + totread,50);
	totread += nread;
	printf("nread: %d\n",(int)nread);
	printf("totread: %d\n",(int)totread);
	memprint(buf,"%02x",BUFSIZE);

	printf("===== Question 27 =====\n");
	FILE *newFile = fopen(filename,"r");
	printf("%d\n",fileno(newFile));
	fclose(newFile);
	printf("===== Question 28 =====\n");
	printf("Close fd1: %d\n",close(fd1));
	printf("Close fd2: %d\n",close(fd2));
}

void part6() {
	char *s1;

	printf("===== Question 29 =====\n");
	s1 = getenv("CS324_VAR");
	if (s1 != NULL) {
		printf("The value is: %s\n", s1);
	}
	else {
		printf("CS324_VAR not found\n");
	}
}
