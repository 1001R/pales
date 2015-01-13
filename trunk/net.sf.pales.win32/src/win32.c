#include "win32.h"
#include "log.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <malloc.h>

__inline wchar_t *wcpcpy(wchar_t *to, const wchar_t *from)
{
	for (; (*to = *from); ++from, ++to);
	return(to);
}

bool ArgumentMustBeQuoted(const wchar_t *s, size_t *len) {
	size_t slen = 0, qlen = 2;
	bool retval = (*s == L'\0');
	while (*s != L'\0') {
		size_t bscnt = 0;
		while (*s == L'\\') {
			bscnt++;
			s++;
		}
		if (*s == L'\0') {
			qlen += bscnt * 2;
			slen += bscnt;
			break;
		}
		else if (*s == L'"') {
			qlen += bscnt * 2 + 2;
			slen += bscnt + 1;
			retval = true;
		}
		else {
			qlen += bscnt + 1;
			slen += bscnt + 1;
		}

		if (iswspace(*s)) {
			retval = true;
		}
		s++;
	}
	*len = retval ? qlen : slen;
	return retval;
}

wchar_t *QuoteArgument(wchar_t *s, const wchar_t *arg) {
	*s++ = L'"';
	while (*arg != L'\0') {
		size_t bscnt = 0;
		while (*arg == L'\\') {
			bscnt++;
			arg++;
		}
		if (*arg == L'\0') {
			for (size_t j = 0; j < 2 * bscnt; j++) {
				*s++ = L'\\';
			}
			break;
		}
		else if (*arg == L'"') {
			for (size_t j = 0; j < 2 * bscnt + 1; j++) {
				*s++ = L'\\';
			}
			*s = L'"';
		}
		else {
			for (size_t j = 0; j < bscnt; j++) {
				*s++ = L'\\';
			}
			*s = *arg;
		}
		s++;
		arg++;
	}
	*s++ = L'"';
	*s = L'\0';
	return s;
}

wchar_t *BuildCommandLine(size_t argc, const wchar_t **argv)
{
	size_t cmdlineLen = 0;
	bool *quoteArgument;
	wchar_t *cmdline, *s;

	quoteArgument = _alloca(sizeof(bool) * argc);
	if (quoteArgument == NULL) {
		return NULL;
		//log_message(L"Failed to allocate %d bytes from the stack", sizeof(bool) * argc);
	}
	for (size_t i = 0; i < argc; i++) {
		size_t arglen;
		quoteArgument[i] = ArgumentMustBeQuoted(argv[i], &arglen);
		cmdlineLen += arglen;
		if (i > 0) {
			cmdlineLen++;
		}
	}
	cmdline = (wchar_t *) malloc((cmdlineLen + 1) * sizeof(wchar_t));
	if (cmdline == NULL) {
		//log_message(L"Out of memory");
		return NULL;
	}
	s = cmdline;
	for (size_t i = 0; i < argc; i++) {
		if (i > 0) {
			*s++ = L' ';
		}
		s = quoteArgument[i] ? QuoteArgument(s, argv[i]) : wcpcpy(s, argv[i]);
	}
	return cmdline;
}

int CreateEmptyFile(const wchar_t *filepath)
{
	HANDLE h;
	h = CreateFile(filepath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		//log_message(L"Cannot create empty file: %s", filepath);
		return -1;
	}
	FlushFileBuffers(h);
	CloseHandle(h);
	return 0;
}
