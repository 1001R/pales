#include "unix.h"
#include <stdlib.h>

#define BASEDIR "/home/improve/temp/pales"

int main(int argc, char **argv)
{
    char *proc_argv[] = { "date", NULL };
    process_run("DEADBEEF",
	BASEDIR "/db",
	BASEDIR "/wd",
	BASEDIR "/stdout.txt",
	BASEDIR "/stderr.txt",
	"/bin/date",
	proc_argv);
    return 0;
}
