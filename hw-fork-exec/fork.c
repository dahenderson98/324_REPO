#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
	int pid;

	printf("Starting program; process has pid %d\n", getpid());

	FILE* f = fopen("fork-output.txt","w");

	fprintf(f,"BEFORE FORK\n");

	int pipefd[2];
	pipe(pipefd);

	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */
	fprintf(f,"SECTION A (%d)\n", fileno(f));
	printf("Section A;  pid %d\n", getpid());
	//sleep(30);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */
		/*close(pipefd[0]);
		write(pipefd[1],"hello from Section B\n",20);
		sleep(5);
		fprintf(f,"SECTION B (%d)\n", fileno(f));
		printf("Section B\n");
		*/
		char *newenviron[] = { NULL };

		printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
		sleep(30);

		if (argc <= 1) {
			printf("No program to exec.  Exiting...\n");
			exit(0);
		}

		printf("Running exec of \"%s\"\n", argv[1]);
		int dup = dup2(fileno(f),fileno(f));
		execve(argv[1], &argv[1], newenviron);
		printf("End of program \"%s\".\n", argv[0]);
		//sleep(30);
		//sleep(30);
		//printf("Section B done sleeping\n");

		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */
		close(pipefd[1]);
		char buf[1024];
		int read_cnt = read(pipefd[1],&buf,1024);
		buf[read_cnt] = 00;
		fprintf(f,"SECTION C (%d)\n", fileno(f));
		fclose(f);
		//sleep(5);
		printf("Section C\n");
		wait(NULL);
		//sleep(30);
		//printf("Section C done sleeping\n");

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
	//sleep(30);

	/* END SECTION D */
}

