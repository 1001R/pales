#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

struct options {
	char workdir[256];
	char datadir[256];
	char outfile[256];
	char errfile[256];
	char procid[64];
};

typedef struct options options_t;

#define OPTION(flag, member) { (flag), offsetof(options_t, member) }

int parse_args(options_t *opts, int argc, char **argv, int *optind);

#endif
