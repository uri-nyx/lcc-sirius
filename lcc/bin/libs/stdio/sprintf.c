/*
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include "local.h"

int
sprintf(char *s, const char *format, ...)
{
	int ret;
	va_list ap;
	FILE f;

	f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)s;
	f._bf._size = f._w = INT_MAX;
	va_start(ap, format);
	ret = vfprintf(&f, format, ap);
	va_end(ap);
	*f._p = 0;
	return ret;
}
