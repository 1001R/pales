#include <string.h>
#include <stdbool.h>
#include "options.h"

struct optspec {
	const char *flag;
	int offset;
};

typedef struct optspec optspec_t;

static optspec_t optspecs[] = {
	OPTION("-w", workdir),
	OPTION("-o", outfile),
	OPTION("-e", errfile),
	OPTION("-i", procid),
	OPTION("-d", datadir)
};

bool is_valid(options_t *opts)
{
	bool valid = true;

	valid &= (opts->datadir != NULL);
	return valid;
}

int parse_args(options_t *opts, int argc, char **argv, int *optind)
{
	int i, j;

	memset(opts, 0, sizeof(options_t));
	for (i = 1; i < argc; i++) {
		bool match = false;
		for (j = 0; j < sizeof(optspecs) / sizeof(optspec_t); j++) {
			if (strcmp(argv[i], optspecs[j].flag) == 0) {
				match = true;
				if (optspecs[j].offset >= 0) {
					if (i == argc - 1) {
						return -1;
					}
					*(const char**)(((char*) opts) + optspecs[j].offset) = argv[++i];
				}
			}
		}
		if (!match) {
			break;
		}
	}
	*optind = i;
	return is_valid(opts) ? 0 : -1;
}
