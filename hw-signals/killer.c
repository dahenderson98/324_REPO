#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>

void sigint_handler(int signum) {
	// send SIGKILL to all processes in group, so this process and children
	// will terminate.  Use SIGKILL because SIGTERM and SIGINT (among
	// others) are overridden in the child.
	kill(-getpgid(0), SIGKILL);
}

int main(int argc, char *argv[]) {
	char *scenario = argv[1];
	int pid = atoi(argv[2]);

	struct sigaction sigact;

	// Explicitly set flags
	sigact.sa_flags = SA_RESTART;
	sigact.sa_handler = sigint_handler;
	// Override SIGINT, so that interrupting this process sends SIGKILL to
	// this one and, more importantly, to the child.
	sigaction(SIGINT, &sigact, NULL);

	/*
	handler1: kill(pid, SIGHUP);
	handler1: kill(pid, SIGINT);
	handler2: kill(pid, SIGQUIT);
	handler3: kill(pid, SIGTERM);
	handler4: kill(pid, 30);
	handler5: kill(pid, 10);
	handler6: kill(pid, 16);
	handler7: kill(pid, 31);
	handler8: kill(pid, 12);
	handler9: kill(pid, SIGCHLD);
	*/

	switch (scenario[0]) {
	case '0':	// done
		kill(pid, SIGHUP);
		sleep(1);
		break;
	case '1':	// done
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '2':	// done
		kill(pid, SIGHUP); // 1
		sleep(1);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '3':	// done
		kill(pid, SIGHUP); // 1
		sleep(1);
		kill(pid, 31); // 7 blocks SIGINT
		sleep(1);
		kill(pid, SIGHUP); // 1
		sleep(4);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '4':	// done
		kill(pid, SIGHUP); // 1
		sleep(1);
		kill(pid, SIGINT); // 1 replaced by default
		sleep(1);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '5': 	// done
		kill(pid, SIGHUP); // 1
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '6': 	// done
		kill(pid, SIGHUP); // 1 prints 1,2
		sleep(1);
		kill(pid, 10); // 5 prints 7
		sleep(4);
		kill(pid, 16); // 6 waits, fails, then prints errno 10
		sleep(1);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '7': 	// done
		kill(pid, SIGHUP); // 1 prints 1,2
		sleep(1);
		kill(pid, 10); // 5 prints 7
		sleep(4);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '8': 	// done
		kill(pid, SIGHUP); // 1 prints 1,2
		sleep(1);
		kill(pid, 31); // 7 blocks signals
		sleep(1);
		kill(pid, 10); // 5 tries to print 7
		sleep(1);
		kill(pid, 30); // 4 sets foo to 6  
		sleep(1);
		kill(pid, SIGTERM); // 3 prints foo 
		sleep(1);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;
	case '9': 	// done
		kill(pid, 31); // 7 blocks SIGINT
		sleep(4);
		kill(pid, SIGQUIT); // 2 prints 8, then calls kill-sigint (prints 1 and 2), then prints 9
		sleep(1);
		kill(pid, 31); // 7 unblocks SIGINT
		sleep(4);
		kill(pid, 12); // 8 returns handlers to default
		sleep(1);
		kill(pid, SIGTERM); // 3 replaced by default SIGTERM
		sleep(1);
		break;

	}
	waitpid(pid, NULL, 0);
}
