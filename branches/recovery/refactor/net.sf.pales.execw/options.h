#ifndef OPTIONS_H
#define OPTIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "error.h"

struct options {
	const wchar_t *workdir;
	const wchar_t *datadir;
	const wchar_t *outfile;
	const wchar_t *errfile;
	const wchar_t *procid;
	size_t argc;
	const wchar_t **argv;
};

typedef struct options options_t;

bool parse_args(options_t *opts, int argc, const wchar_t **argv, error_t *err);

extern const char *PALES_HELP;

#endif
