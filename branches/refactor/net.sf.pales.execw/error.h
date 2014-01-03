/*
 * error.h
 *
 *  Created on: 28.12.2013
 *      Author: phil
 */

#ifndef ERROR_H_
#define ERROR_H_

#include <Windows.h>
#include <stdlib.h>
#include <stdbool.h>

typedef enum {
	ERR_NOMEM,
	ERR_APICALL
} errtype_t;

typedef struct {
	const wchar_t *apicall;
	const char *function;
	unsigned int line;
	DWORD errnum;
	errtype_t type;
	const wchar_t *message;
	bool free_message;
} error_t;


void error_init(error_t *error);

void error_apicall_fail(error_t *error, const wchar_t *apicall);

void error_set_message_fmt(error_t *error, const wchar_t *fmt, ...);

void error_set_message(error_t *error, const wchar_t *message);

#endif /* ERROR_H_ */
