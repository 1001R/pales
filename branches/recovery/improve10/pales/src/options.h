#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>

struct options {
	const wchar_t *workdir;
	const wchar_t *datadir;
	const wchar_t *outfile;
	const wchar_t *errfile;
	const wchar_t *procid;
};

typedef struct options options_t;

#define OPTION(flag, member) { (flag), offsetof(options_t, member) }

int parse_args(options_t *opts, int argc, wchar_t **argv, int *optind);

#endif
