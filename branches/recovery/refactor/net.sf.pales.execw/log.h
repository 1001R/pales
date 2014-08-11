/*
 * log.h
 *
 *  Created on: 02.01.2014
 *      Author: phil
 */

#ifndef LOG_H_
#define LOG_H_

typedef enum {
	LOG_ERROR,
	LOG_WARNING,
	LOG_DEBUG
} loglevel_t;


void pales_log(loglevel_t level, const wchar_t *message, ...);


#endif /* LOG_H_ */
