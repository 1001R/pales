#include "unix.h"
#include <stdlib.h>
#include <semaphore.h>
#include <err.h>
#include <unistd.h>

#define BASEDIR "/home/improve/temp/pales"

int main(int argc, char **argv)
{
    char *proc_argv[] = { "script.sh", "60", NULL };
    process_run("DEADBEEF",
	BASEDIR "/db",
	BASEDIR "/wd",
	BASEDIR "/stdout.txt",
	BASEDIR "/stderr.txt",
	BASEDIR "/script.sh",
	proc_argv);
    sleep(10);
    sem_t *s = sem_open("/PALES:DEADBEEF", 0);
    if (s == SEM_FAILED) {
	err(1, "sem_open");
    }
    if (sem_post(s) != 0) {
	err(1, "sem_post");
    }
    sem_close(s);
    return 0;
}
