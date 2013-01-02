/*
 * pales.c
 *
 *  Created on: Jan 2, 2013
 *      Author: phil
 */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>

typedef enum procstat {
	running,
	finished,
	cancelled
}
procstat_t;

static char *process_encode(const char *dbdir, const char *procid, procstat_t status, pid_t pid) {
	char *result = NULL, c;

	switch (status) {
	case running:
		c = 'R';
		break;
	case finished:
		c = 'F';
		break;
	case cancelled:
		c = 'C';
		break;
	default:
		return NULL;
	}

	if (status == running) {
		asprintf(&result, "%s/%s-R-%d", dbdir, procid, pid);
	}
	else {
		asprintf(&result, "%s/%s-%c", dbdir, procid, c);
	}
	return result;
}

static int db_update(const char *dbdir, const char *procid, procstat_t status, pid_t pid) {
	char *path = NULL;
	int fd;

	if ((path = process_encode(dbdir, procid, status, pid)) == NULL) {
		return -1;
	}
	fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0640);
	free(path);
	if (fd == -1) {
		return -1;
	}
	close(fd);
	return 0;
}

static void sigaction_void(int signal, siginfo_t *info, void *context) {

}

static int redirect_fd(const char *path, int oflag, mode_t mode, int fd) {
	int r, err, target;
	if ((oflag & O_CREAT) != 0) {
		target = open(path, oflag, mode);
	}
	else {
		target = open(path, oflag);
	}
	if (target == -1) {
		return -1;
	}
	r = dup2(target, fd);
	err = errno;
	close(target);
	errno = err;
	return r;
}

static void process_exec(const char *rundir, const char *outfile, const char *errfile, const char *executable, char **argv) {
	if (setpgid(0, 0) == -1) {
		syslog(LOG_ERR, "Can't create new process group: %m");
		_exit(EXIT_FAILURE);
	}
	if (chdir(rundir) == -1) {
		syslog(LOG_ERR, "chdir failed");
		_exit(EXIT_FAILURE);
	}
	if (redirect_fd("/dev/null", O_RDONLY, 0, 0) == -1) {
		syslog(LOG_ERR, "Can't redirect /dev/null to stdin: %m");
		_exit(EXIT_FAILURE);
	}
	if (redirect_fd(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0640, 1) == -1) {
		syslog(LOG_ERR, "Can't redirect stdout to %s: %m", outfile);
		_exit(EXIT_FAILURE);
	}
	if (redirect_fd(errfile, O_WRONLY | O_TRUNC | O_CREAT, 0640, 2) == -1) {
		syslog(LOG_ERR, "Can't redirect stderr to %s: %m", errfile);
		_exit(EXIT_FAILURE);
	}
	execv(executable, argv);
	syslog(LOG_ERR, "System call `execv' failed unexpectedly: %m");
	_exit(EXIT_FAILURE);
}

static void process_monitor(const char *procid, const char *dbdir, const char *rundir, const char *outfile, const char *errfile, const char *executable, char **argv) {
	pid_t pid, sid;
	sigset_t sigset;
    struct sigaction sigact;

	openlog("pales", LOG_PID, LOG_USER);
    sigact.sa_sigaction = sigaction_void;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO;
	if (sigaction(SIGCHLD, &sigact, NULL) != 0) {
		syslog(LOG_ERR, "Can't install handler for SIGCHLD: %m");
		_exit(EXIT_FAILURE);
	}
    sigfillset(&sigset);
    if (sigprocmask(SIG_BLOCK, &sigset, NULL) != 0) {
    	syslog(LOG_ERR, "Can't set signal mask: %m");
    	_exit(EXIT_FAILURE);
    }
	if (chdir("/") == -1) {
		syslog(LOG_ERR, "Can't change current working directory to /: %m");
		_exit(EXIT_FAILURE);
	}
	if ((sid = setsid()) == -1) {
		syslog(LOG_ERR, "Can't create new session: %m");
		_exit(EXIT_FAILURE);
	}
	umask(0);
	pid = vfork();
	if (pid == 0) {
		process_exec(rundir, outfile, errfile, executable, argv);
	}
	else if (pid > 0) {
		int s;
		bool done;
		procstat_t pstat = running;
		int status, ecode = EXIT_SUCCESS;

		if (db_update(dbdir, procid, running, pid) == -1) {
			syslog(LOG_ERR, "Can't update process database: %m");
			killpg(pid, SIGKILL);
			_exit(EXIT_FAILURE);
		}

		done = false;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGCHLD);
        sigaddset(&sigset, SIGTERM);
        while (!done) {
            if (sigwait(&sigset, &s) == 0) {
            	switch (s) {
            	case SIGTERM:
            		syslog(LOG_NOTICE, "Process cancellation requested");
                    sigdelset(&sigset, SIGTERM);
                    killpg(pid, SIGKILL);
                    pstat = cancelled;
                    break;
            	case SIGCHLD:
                    done = true;
                    pstat = finished;
                    break;
            	case SIGALRM:
            		break;
                }
            }
            else {
            	syslog(LOG_ERR, "System call `sigwait' failed unexpectedly: %m");
            	_exit(EXIT_FAILURE);
            }
        }
        s = waitpid(pid, &status, 0);
        if (db_update(dbdir, procid, pstat, pid) == -1) {
        	syslog(LOG_ERR, "Can't update process database: %m");
        	ecode = EXIT_FAILURE;
        }
        if (s == -1) {
            syslog(LOG_ERR, "System call `waitpid' failed unexpectedly: %m");
            ecode = EXIT_FAILURE;
        }
        _exit(ecode);
	}
	syslog(LOG_ERR, "Can't fork child process: %m");
	_exit(EXIT_FAILURE);
}

pid_t process_run(const char *procid, const char *dbdir, const char *workdir, const char *outfile, const char *errfile, const char *executable, char **argv)
{
	pid_t pid;

	if (executable == NULL) {
		return -1;
	}
	pid = fork();
	if (pid == -1) {
		return -1;
	}
	if (pid == 0) {
		process_monitor(procid, dbdir, workdir, outfile, errfile, executable, argv);
		return -1;
	}
	return pid;
}
