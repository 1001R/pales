#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


static void sigaction_void(int signal, siginfo_t *info, void *context) {
    fprintf(stderr, "SIGCHLD\n");
}

static void foo(int s) {
}

int main(int argc, char **argv) {
    struct sigaction sigact;
    pid_t pid;
    sem_t *sem_cancel;

   
    sigact.sa_sigaction = sigaction_void;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    if (sigaction(SIGCHLD, &sigact, NULL) != 0) {
	err(1, "sigaction");
    }

    /*
    sigfillset(&sigset);
    sigdelset(&sigset, SIGCHLD);
    if (sigprocmask(SIG_BLOCK, &sigset, NULL) != 0) {
        syslog(LOG_ERR, "Can't set signal mask: %m");
        _exit(EXIT_FAILURE);
    }
    */

    sem_cancel = sem_open("/sigchld", O_CREAT, 0600, 0);
    if (sem_cancel == SEM_FAILED) {
	err(1, "sem_cancel");
    }

    pid = fork();
    if (pid == 0) {
	sleep(2);
	fprintf(stderr, "child exit\n");
	_exit(0);
    }
    else if (pid > 0) {
	int status;
	pid_t child;

	if (sem_wait(sem_cancel) == -1) {
	    if (errno != EINTR) {
		err(1, "sem_wait");
	    }
	    else {
		fprintf(stderr, "sem_wait interrupted\n");
	    }
	}

	child = wait(&status);
	fprintf(stderr, "wait succeded, pid = %d\n", child);
    }
    return 0;
}
