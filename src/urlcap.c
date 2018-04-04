/*

    EnergyMech, IRC bot software
    Copyright (c) 2018 proton

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
#define URLCAP_C
#include "config.h"

#ifdef URLCAPTURE

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"

char *urlhost(const char *url)
{
	char	copy[strlen(url)];
	const char *end,*beg,*dst;
	int	n = 0;

	beg = end = url;
	while(*end)
	{
		if (*end == '@')
			beg = end+1;
		else
		if (*end == '/')
		{
			if (n == 1)
				beg = end+1;
			else
			if (n == 2)
				break;
			n++;
		}
		end++;
	}
	stringcpy_n(copy,beg,(end-beg));
#ifdef DEBUG
	debug("(urlhost) host = %s\n",copy);
#endif
}

void urlcapture(const char *rest)
{
	Strp	*sp,*nx;
	char	*dest,url[MSGLEN];
	int	n;

	dest = url;
	while(*rest && *rest != ' ' && dest < url+MSGLEN-1)
		*(dest++) = *(rest++);
	*dest = 0;

#ifdef DEBUG
	debug("(urlcapture) URL = \"%s\"\n",url);
#endif /* ifdef DEBUG */
	urlhost(url);

	send_spy(SPYSTR_URL,"%s",url);

	if ((n = urlhistmax) < 0)
		return;

	prepend_strp(&urlhistory,url);

	for(sp=urlhistory;sp;sp=sp->next)
	{
		if (n <= 0)
		{
			purge_strplist(sp->next);
			sp->next = NULL;
			return;
		}
		n--;
	}
}

/*
 *
 *   Commands associated with URLCAPTURE
 *
 */

/*---Help:URLHIST:[max]

Display a list of URLs seen by the bot in order most recent to oldest.

   [max]   Maximum number of URLs to display.
*/
void do_urlhist(COMMAND_ARGS)
{
	Strp	*sp;
	char	*thenum;
	int	n,maxnum;

	if (urlhistory == NULL)
	{
		to_user(from,"No URLs recorded.");
		return;
	}

	if (!rest || !*rest)
		maxnum = urlhistmax;
	else
	{
		thenum = chop(&rest);
		maxnum = asc2int(thenum);
	}
	if ((maxnum < 1) || (maxnum > urlhistmax))
		usage(from);    /* usage for CurrentCmd->name */

	n = 1;
	for(sp=urlhistory;sp;sp=sp->next)
	{
		if (n > maxnum)
			break;
		to_user(from,"[%i] %s",n,sp->p);
		n++;
	}
}

#endif /* ifdef URLCAPTURE */
