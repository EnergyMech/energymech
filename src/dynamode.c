/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2004 proton

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
#define DYNAMODE_C
#include "config.h"

#ifdef DYNAMODE

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

void check_dynamode(Chan *chan)
{
	ChanUser *cu;
	char	tempconf[strlen(chan->setting[STR_DYNLIMIT].str_var)+2];
	char	ascnum[11];
	char	*src,*num,*end;
	int	n = 0,wind,v[3];

	/*
	 *  parse `delay:window:minwin'
	 */
	end = Strcpy(tempconf,chan->setting[STR_DYNLIMIT].str_var);
	num = src = tempconf;
	for(;(src<=end) && (n<3);src++)
	{
		if (*src == 0 || *src == ':')
		{
			*src = 0;
			v[n] = a2i(num);
			if (errno)
			{
				v[0] = 90;	/* delay */
				v[1] = 10;	/* window */
				v[2] = 4;	/* minwin */
				break;
			}
			num = src+1;
			n++;
		}
	}
	v[0] = (v[0] < 20) ? 20 : (v[0] > 600) ? 600 : v[0];
	if ((now - chan->lastlimit) < v[0])
		return;
	v[1] = (v[1] < 5) ? 5 : (v[1] > 50) ? 50 : v[1];
	v[2] = (v[2] < 1) ? 1 : (v[2] > 50) ? 50 : v[2];

	chan->lastlimit = now;

	n = 0;
	for(cu=chan->users;cu;cu=cu->next)
		n++;

	wind = n / v[1];
	if (wind < v[2])
		wind = v[2];

	wind += n;

	n = wind - chan->limit;

	if (!chan->limitmode || (n < -2) || (n > 1))
	{
		sprintf(ascnum,"%i",wind);
		send_mode(chan,160,QM_RAWMODE,'+','l',ascnum);
	}
}

#endif /* DYNAMODE */
