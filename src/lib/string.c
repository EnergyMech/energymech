/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2018 proton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#define STRING_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

/*
 *  returns NULL or non-zero length string
 *  callers responsibility that src is not NULL
 */
char *chop(char **src)
{
	char	*tok,*cut = *src;

	while(*cut && *cut == ' ')
		cut++;

	if (*cut)
	{
		tok = cut;
		while(*cut && *cut != ' ')
			cut++;
		*src = cut;
		while(*cut && *cut == ' ')
			cut++;
		**src = 0;
		*src = cut;
	}
	else
	{
		tok = NULL;
	}
	return(tok);
}

/*
 *  remove all '\0' in an array bounded by two pointers
 */
void unchop(char *orig, const char *rest)
{
	for(;orig<rest;orig++)
	{
		if (*orig == 0)
			*orig = ' ';
	}
}

int stringcasecmp(const char *p1, const char *p2)
{
	int	ret;

	if (p1 != p2)
	{
		while(!(ret = tolowertab[(uchar)*(p1++)] - tolowertab[(uchar)*p2]) && *(p2++))
			;
		return(ret);
	}
	return(0);
}

int stringcmp(const char *p1, const char *p2)
{
	int	ret;

	if (p1 != p2)
	{
		while(!(ret = *(p1++) - *p2) && *(p2++))
			;
		return(ret);
	}
	return(0);
}

int nickcmp(const char *p1, const char *p2)
{
	int	ret;
	int	c;

	if (p1 != p2)
	{
		while(!(ret = nickcmptab[(uchar)*(p1++)] - (c = nickcmptab[(uchar)*(p2++)])) && c)
			;
		return(ret);
	}
	return(0);
}

char *nickcpy(char *dest, const char *nuh)
{
	char	*ret;

	if (!dest)
		dest = nick_buf;
	ret = dest;

	while(*nuh && (*nuh != '!'))
		*(dest++) = *(nuh++);
	*dest = 0;

	return(ret);
}

void stringcpy_n(char *dst, const char *src, int sz)
{
	int	n = 0;

	while(src[n] && n<sz)
	{
		dst[n] = src[n];
		n++;
	}
	dst[n] = 0;
/*
	char	*stop = dst + sz - 1;

	while(*src)
	{
		*(dst++) = *(src++);
		if (dst == stop)
			break;
	}
	*dst = 0;
*/
}

char *stringcpy(char *dst, const char *src)
{
	int	n = 0;

	while(src[n])
	{
		dst[n] = src[n];
		n++;
	}
	dst[n] = 0;
	return(dst+n);
}

char *stringchr(const char *t, int c)
{
	char	ch = c;

	while(*t != ch && *t)
		t++;
	return((*t == ch) ? (char*)t : NULL);
}

char *stringdup(const char *src)
{
	char	*dest;

#ifdef TEST
	dest = (char*)calloc(1,strlen(src)+1);
#else
	dest = (char*)Calloc(strlen(src)+1);
#endif
	stringcpy(dest,src);
	return(dest);
}

const int StrlenX(const char *first, ...)
{
	const char *s,*o;
	int n;
	va_list vlist;

	va_start(vlist,first);

	n = 0;
	o = s = first;
	do
	{
		while(*s)
			s++;
		n += (s - o);
		s = o = va_arg(vlist,const char *);
	}
	while(s);

	va_end(vlist);
	return(n);
}

/*
 *  combined length of two strings
 *  both strings must be non-NULL, caller is responsible
 */
const int Strlen2(const char *one, const char *two)
{
	const char *s1,*s2;

	for(s1=one;*s1;s1++);
	for(s2=two;*s2;s2++);

	return((s1 - one) + (s2 - two));
}

/*
 *  This code might look odd but its optimized for size,
 *  so dont change it!
 */
char *stringcat(char *dst, const char *src)
{
	while(*(dst++))
		;
	--dst;
	while((*(dst++) = *(src++)) != 0)
		;
	return(dst-1);
}

char *tolowercat(char *dest, const char *src)
{
	dest = STREND(dest);
	while(*src)
		*(dest++) = (char)tolowertab[(uchar)*(src++)];
	return(dest);
}

