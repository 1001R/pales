#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

struct options {
	const char *workdir;
	const char *datadir;
	const char *outfile;
	const char *errfile;
	const char *procid;
};

typedef struct options options_t;

#define OPTION(flag, member) { (flag), offsetof(options_t, member) }

int parse_args(options_t *opts, int argc, char **argv, int *optind);

#endif
