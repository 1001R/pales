/*
 * error.c
 *
 *  Created on: 28.12.2013
 *      Author: phil
 */

#include "error.h"
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <stdio.h>


void error_init(error_t *err) {
	if (err == NULL) {
		return;
	}
	memset(err, 0, sizeof(error_t));
}

void error_free(error_t *error) {
	if (error != NULL && error->free_message) {
		free((void *)error->message);
		error->message = NULL;
		error->free_message = false;
	}
}

void error_apicall_fail(error_t *error, const wchar_t *apicall)
{
	if (error == NULL) {
		return;
	}
	error->apicall = apicall;
	error->type = ERR_APICALL;
	error->function = __FUNCTION__;
	error->errnum = GetLastError();
}

void error_set_message_fmt(error_t* error, const wchar_t *fmt, ...)
{
	va_list args1, args2;
	wchar_t *message;

	if (error == NULL) {
		return;
	}
	va_start(args1, fmt);
	va_copy(args2, args1);
	int msglen = _vscwprintf(fmt, args1) + 1;
	va_end(args1);
	message = (wchar_t *) malloc(msglen * sizeof(wchar_t));
	if (message != NULL) {
		if (error->free_message) {
			free((void *)error->message);
		}
		vswprintf(message, msglen, fmt, args2);
		error->message = message;
		error->free_message = true;
	}
	va_end(args2);
}

void error_set_message(error_t *error, const wchar_t *message)
{
	if (error == NULL) {
		return;
	}
	if (error->free_message) {
		free((void *)error->message);
	}
	error->message = message;
	error->free_message = false;
}
