/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2004 proton

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
#define NET_C
#include "config.h"

#ifdef BOTNET

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

ChanUser *find_chanbot(Chan *chan, char *nick)
{
	ChanUser *cu;

	if (chan->cacheuser && !nickcmp(nick,chan->cacheuser->nick)
		&& chan->cacheuser->user && chan->cacheuser->user->x.x.access == BOTLEVEL)
		return(chan->cacheuser);

	for(cu=chan->users;cu;cu=cu->next)
	{
		if (cu->user && cu->user->x.x.access == BOTLEVEL)
		{
			if (!nickcmp(nick,cu->nick))
				return(chan->cacheuser = cu);
		}
	}
	return(NULL);
}

void check_botjoin(Chan *chan, ChanUser *cu)
{
	BotNet	*bn;
	BotInfo *binfo;

#ifdef DEBUG
	debug("(check_botjoin) chan = %s; cu = %s!%s\n",chan->name,cu->nick,cu->userhost);
#endif /* DEBUG */

	for(bn=botnetlist;bn;bn=bn->next)
	{
		if (bn->status != BN_LINKED)
			continue;

		for(binfo=bn->botinfo;binfo;binfo=binfo->next)
		{
			if (!nickcmp(cu->nick,binfo->nuh) &&
				!Strcasecmp(cu->userhost,getuh(binfo->nuh)))
			{
				if ((cu = find_chanbot(chan,binfo->nuh)) == NULL)
					return;
				cu->flags |= CU_NEEDOP;
				send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
#ifdef DEBUG
				debug("(check_botjoin) CU_NEEDOP set, mode pushed\n");
#endif /* DEBUG */
				return;
			}
		}
	}
}

void check_botinfo(BotInfo *binfo, const char *channel)
{
	Chan	*chan;
	ChanUser *cu;
	Mech	*backup;
	char	*userhost;

	userhost = getuh(binfo->nuh);

	backup = current;
	for(current=botlist;current;current=current->next)
	{
		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if (channel && Strcasecmp(channel,chan->name))
				continue;
			if ((cu = find_chanbot(chan,binfo->nuh)) == NULL)
				continue;
			if (!Strcasecmp(cu->userhost,userhost))
			{
				cu->flags |= CU_NEEDOP;
				send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
			}
		}
	}
	current = backup;
}

/*
 *
 *  protocol routines
 *
 */

void netchanNeedop(BotNet *source, char *rest)
{
	BotNet	*bn;
	BotInfo	*binfo;
	char	*channel;
	int	guid;

	guid = a2i(chop(&rest));
	channel = chop(&rest);
	if (errno || guid < 1 || !channel)
		return;

	botnet_relay(source,"CO%i %s\n",guid,channel);

	for(bn=botnetlist;bn;bn=bn->next)
	{
		if (bn->status != BN_LINKED)
			continue;
		for(binfo=bn->botinfo;binfo;binfo=binfo->next)
		{
			if (binfo->guid == guid)
				check_botinfo(binfo,channel);
		}
        }
}

#endif /* BOTNET */
