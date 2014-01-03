#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "options.h"


bool is_valid(options_t *opts, error_t *err)
{
	if (opts->datadir == NULL || opts->datadir[0] == L'\0') {
		error_set_message(err, L"No database directory specified");
		return false;
	}
	if (opts->procid == NULL || opts->procid[0] == L'\0') {
		error_set_message(err, L"No process identifier specified");
		return false;
	}
	for (const wchar_t *s = opts->procid; *s != L'\0'; s++) {
		if (!iswalnum(*s)) {
			error_set_message(err, L"Process identifier contains non-alphanumeric characters");
			return false;
		}
	}
	if (opts->argc == 0) {
		error_set_message(err, L"No executable specified");
		return false;
	}
	return true;
}

bool parse_args(options_t *opts, int argc, const wchar_t **argv, error_t *err)
{
	int i;

	memset(opts, 0, sizeof(options_t));
	opts->outfile = L"NUL";
	opts->errfile = L"NUL";
	for (i = 1; i < argc; i++) {
		const wchar_t **ref = NULL;
		if (wcscmp(argv[i], L"-w") == 0) {
			ref = &(opts->workdir);
		}
		else if (wcscmp(argv[i], L"-o") == 0) {
			ref = &(opts->outfile);
		}
		else if (wcscmp(argv[i], L"-e") == 0) {
			ref = &(opts->errfile);
		}
		else if (wcscmp(argv[i], L"-i") == 0) {
			ref = &(opts->procid);
		}
		else if (wcscmp(argv[i], L"-d") == 0) {
			ref = &(opts->datadir);
		}
		if (ref != NULL) {
			if (i == argc - 1) {
				error_set_message(err, L"Missing argument to command line option");
				return false;
			}
			*ref = argv[++i];
		}
		else {
			break;
		}
	}
	opts->argc = argc - i;
	opts->argv = argv + i;
	return is_valid(opts, err);
}
