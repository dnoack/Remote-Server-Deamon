/*
 * Utils.h
 *
 *  Created on: 31.03.2015
 *      Author: dnoack
 */

#include <cstdio>
#include <stdarg.h>

#ifndef INCLUDE_UTILS_H_
#define INCLUDE_UTILS_H_



static int dyn_print(const char* data, ...)
{
	int printed = 0;
	va_list arguments;

	#ifdef DEBUG
		va_start(arguments, data);
		printed = vprintf(data, arguments);
		va_end(arguments);
	#endif
		//there can be other print-like functions

	return printed;
}



#endif /* INCLUDE_UTILS_H_ */
