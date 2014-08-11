#include <string.h>
#include <stdbool.h>
#include "options.h"


#define STRCPY_BUF(t, s, x) \
	if (strlen(s) < sizeof(t)) { strcpy((t), (s)); } else { x; }

struct optspec {
	const char *flag;
	int offset;
};

typedef struct optspec optspec_t;


bool is_valid(options_t *opts)
{
	if (opts->datadir[0] == '\0') {
		return false;
	}

	return valid;
}

int parse_args(options_t *opts, int argc, char **argv, int *optind)
{
	int i, j;

	memset(opts, 0, sizeof(options_t));
	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			return -1;
		}
		switch (argv[i][1]) {
		case 'w':
			STRCPY_BUF(opts->workdir, argv[++i], return -1);
			break;
		case 'o':
			STRCPY_BUF(opts->outfile, argv[++i], return -1);
			break;
		case 'e':
			STRCPY_BUF(opts->errfile, argv[++i], return -1);
			break;
		case 'i':
			STRCPY_BUF(opts->procid, argv[++i], return -1);
			break;
		case 'd':
			STRCPY_BUF(opts->datadir, argv[++i], return -1);
			break;
		}
	}
	*optind = i;
	return is_valid(opts) ? 0 : -1;
}
