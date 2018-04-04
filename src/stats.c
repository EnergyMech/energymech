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
#define STATS_C
#include "config.h"

#ifdef STATS
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#include <math.h>

void stats_loghour(Chan *chan, char *filename, int hour)
{
	ChanStats *stats;
	time_t	when;
	int	fd;

	if (!(stats = chan->stats))
		return;

	when = (now - (now % 3600));

	if ((fd = open(filename,O_WRONLY|O_APPEND|O_CREAT,NEWFILEMODE)) >= 0)
	{
		stats->userseconds += stats->users * (when - stats->lastuser);
		to_file(fd,"H %s %i %i %i %i\n",chan->name,hour,
			(stats->flags & CSTAT_PARTIAL) ? -stats->userseconds : stats->userseconds,
			stats->userpeak,stats->userlow);
		close(fd);
	}
	stats->LHuserseconds = stats->userseconds;
	stats->userseconds = 0;
	stats->lastuser = when;
	stats->flags = 0;
}

void stats_plusminususer(Chan *chan, int plusminus)
{
	ChanStats *stats;
	ChanUser *cu;

	if (!(stats = chan->stats))
	{
		set_mallocdoer(stats_plusminususer);
		chan->stats = stats = (ChanStats*)Calloc(sizeof(ChanStats)); /* Calloc sets memory to 0 */
		for(cu=chan->users;cu;cu=cu->next)
			stats->users++;
		stats->userpeak = stats->users;
		stats->userlow = stats->users;
		stats->lastuser = now;
		stats->flags = CSTAT_PARTIAL;
	}

	/*
	 *  add (number of users until now * seconds since last user entered/left)
	 */
	stats->userseconds += stats->users * (now - stats->lastuser);

	stats->lastuser = now;
	stats->users += plusminus;	/* can be both negative (-1), zero (0) and positive (+1) */

	if (stats->userpeak < stats->users)
		stats->userpeak = stats->users;
	if (stats->userlow > stats->users)
		stats->userlow = stats->users;

#ifdef DEBUG
	debug("(stats_plusminususer) %s: %i users, %i userseconds, %i high, %i low; %s (%lu)\n",
		chan->name,stats->users,stats->userseconds,stats->userpeak,stats->userlow,
		atime(stats->lastuser),stats->lastuser);
#endif /* DEBUG */
}

void do_info(COMMAND_ARGS)
{
	ChanStats *stats;
	Chan	*chan;
	char	*p;
	char	text[MSGLEN];
	uint32_t avg;

	if (current->chanlist == NULL)
	{
		to_user(from,ERR_NOCHANNELS);
		return;
	}
	to_user(from,"\037channel\037                            "
		"\037average\037 \037peak\037 \037low\037");
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		*(p = text) = 0;
		p = stringcat(p,chan->name);
		if (chan == current->activechan)
			p = stringcat(p," (current)");
		if ((stats = chan->stats))
		{
			if (stats && stats->flags == CSTAT_PARTIAL)
				p = stringcat(p," (partial)");
			while(p < text+35)
				*(p++) = ' ';
			if (stats->LHuserseconds > 0)
			{
				avg = stats->LHuserseconds / (60*60);
			}
			else
			{
				avg = (stats->userpeak + stats->userlow) / 2;
			}
			sprintf(p,"%-7lu %-4i %i",avg,stats->userpeak,stats->userlow);
			to_user(from,FMT_PLAIN,text);
			sprintf(text,"Messages: %i   Notices: %i   Joins: %i   Parts: %i   Kicks: %i   Quits: %i",
				stats->privmsg,stats->notice,stats->joins,stats->parts,stats->kicks,stats->quits);
		}
		else
		{
			stringcpy(p," (no current data)");
		}
		to_user(from,FMT_PLAIN,text);
	}
}

#endif /* STATS */
