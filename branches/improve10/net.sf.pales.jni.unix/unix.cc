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
#include <string.h>
#include <stdbool.h>
#include "database.h"

#include "semaphore.h"
#include <sstream>
#include <memory>
#include <iostream>

using namespace std;

static unique_ptr<Semaphore> SEM_CANCEL;
static volatile bool PROC_EXITED = false;

static void sigaction_void(int signal, siginfo_t *info, void *context) {
    PROC_EXITED = true;
    if (SEM_CANCEL) {
	SEM_CANCEL->post();
    }
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
    sigact.sa_flags = SA_SIGINFO | SA_RESTART;
    if (sigaction(SIGCHLD, &sigact, NULL) != 0) {
        syslog(LOG_ERR, "Can't install handler for SIGCHLD: %m");
        _exit(EXIT_FAILURE);
    }
    sigfillset(&sigset);
    sigdelset(&sigset, SIGCHLD);
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
    pid = fork();
    if (pid == 0) {
        process_exec(rundir, outfile, errfile, executable, argv);
    }
    else if (pid > 0) {
	try {
	    Database db(dbdir, procid);
	    try {
		ostringstream terminationSemaphoreName;
		terminationSemaphoreName << "/PALES:" << procid;
		SEM_CANCEL.reset(new Semaphore(terminationSemaphoreName.str(), O_EXCL, 0600, 0, true));
		db.update(Database::RUNNING, pid);
		SEM_CANCEL->wait();
		if (!PROC_EXITED) {
		    syslog(LOG_NOTICE, "Process cancellation requested");
		    sigdelset(&sigset, SIGTERM);
		    killpg(pid, SIGKILL);
		}
		
		int status;
		int w = waitpid(pid, &status, 0);
		if (w == -1) {
		    throw SystemError("waitpid", __FILE__, __LINE__);
		}

		if (WIFSIGNALED(status) &&  WTERMSIG(status) == SIGKILL) {
		    db.update(Database::CANCELLED);
		}
		else {
		    db.update(Database::FINISHED, WEXITSTATUS(status));
		}
	    }
	    catch (const runtime_error& e) {
		db.update(Database::ERROR, e.what());
		db.close();
		killpg(pid, SIGKILL);
		exit(EXIT_FAILURE);
	    }
	    db.close();
	}
	catch (const runtime_error& rte) {
	    cerr << "A fatal unexpected error occured: " << rte.what() << endl;
	    killpg(pid, SIGKILL);
	    exit(EXIT_FAILURE);
	}
        exit(EXIT_SUCCESS);
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
    
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, nullptr);

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

