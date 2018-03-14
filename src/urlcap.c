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

__page(CORE_SEG)
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

	send_spy(SPYSTR_URL,"%s",url);

	if ((n = urlhistmax))
	{
		set_mallocdoer(urlcapture);
		sp = (Strp*)Calloc(sizeof(Strp) + strlen(url));
		Strcpy(sp->p,url);
		sp->next = urlhistory;
		urlhistory = sp;

		for(sp=urlhistory;sp;sp=sp->next)
		{
			if (n <= 0)
			{
				do
				{
					nx = sp->next;
					Free((char**)&sp);
					sp = nx;
				}
				while(sp);
			}
			n--;
		}
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
__page(CMD1_SEG)
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
		maxnum = a2i(thenum);
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
