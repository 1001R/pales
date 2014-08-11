#include <Windows.h>
#include <stdio.h>
#include "log.h"

static const wchar_t *g_process_id = NULL;

void set_process_id(const wchar_t *process_id)
{
	g_process_id = _wcsdup(process_id);
}

void log_message(wchar_t *message, ...)
{
	wchar_t *wbuf;
	va_list args;
	va_start(args, message);
	int mblen;

	int wlen = _vscprintf(message, args);
	wbuf = malloc(sizeof(wchar_t) * (wlen + 1));
	if (wbuf == NULL) {
		return;
	}
	vsprintf(wbuf, message, args);


	mblen = WideCharToMultiByte(CP_UTF8, 0, wbuf, wlen, NULL, 0, NULL, FALSE);
	va_end(args);
}