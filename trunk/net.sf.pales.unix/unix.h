/*
 * unix.h
 *
 *  Created on: Jan 2, 2013
 *      Author: phil
 */

#ifndef UNIX_H_
#define UNIX_H_

#include <sys/types.h>

pid_t process_run(const char *procid, const char *dbdir, const char *rundir, const char *outfile, const char *errfile, const char *executable, char **argv);


#endif /* UNIX_H_ */
