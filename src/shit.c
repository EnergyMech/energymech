/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2009 proton

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
#define SHITLIST_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *
 *  shitlist enforcing
 *
 */

void shit_action(Chan *chan, ChanUser *cu)
{
	Shit	*shit;
	const char *nick;
	char	*fromnick;
	char	*userhost;

	if (!chan->setting[TOG_SHIT].int_var || !chan->bot_is_op || cu->user)
		return;

	userhost = get_nuh(cu);
	if ((cu->shit = find_shit(userhost,chan->name)) == NULL)
		return;

	shit = cu->shit;

	if (shit->action == SHIT_KB || shit->action == SHIT_PERMABAN)
	{
		nick = cu->nick;

		send_mode(chan,85,QM_CHANUSER,'-','o',cu);
		send_mode(chan,90,QM_RAWMODE,'+','b',shit->mask);

		fromnick = nickcpy(NULL,shit->from);
		send_kick(chan,nick,"%s %s: %s",time2small(shit->time),fromnick,
			(shit->reason) ? shit->reason : "GET THE HELL OUT!!!");
		return;
	}
	else
	/*
	 *  shitlevel 1: not allowed to be chanop
	 */
	if (shit->action == SHIT_CHANOP)
	{
		send_mode(chan,160,QM_CHANUSER,'-','o',(void*)cu);
	}
}

void check_shit(void)
{
	ChanUser *cu;
	Chan	*chan;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		for(cu=chan->users;cu;cu=cu->next)
		{
			shit_action(chan,cu);
		}
	}
}

/*
 *
 *  shitlist management. adding, deleting, clearing, searching, ...
 *
 */

void remove_shit(Shit *shit)
{
	Chan	*chan;
	ChanUser *cu;
	Shit	**pp;

#ifdef DEBUG
	debug("(remove_shit) removing shit %s on channel %s (Level %i)\n",shit->mask,shit->chan,shit->action);
#endif /* DEBUG */
	pp = &current->shitlist;
	while(*pp)
	{
		if (*pp == shit)
		{
			/*
			 *  remove links to this shit record from the chanuserlist
			 */
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				for(cu=chan->users;cu;cu=cu->next)
				{
					if (cu->shit == shit)
						cu->shit = NULL;
				}
			}
			*pp = shit->next;
			Free((char**)&shit);
			current->ul_save++;
			return;
		}
		pp = &(*pp)->next;
	}
}

void purge_shitlist(void)
{
	while(current->shitlist)
		remove_shit(current->shitlist);
}

Shit *add_shit(char *from, char *chan, char *mask, char *reason, int axs, int expire)
{
	Shit	*shit;

	set_mallocdoer(add_shit);
	shit = (Shit*)Calloc(sizeof(Shit) + StrlenX(from,chan,mask,reason,NULL));

	shit->action = axs;
	shit->time   = now;
	shit->expire = expire;

	shit->next = current->shitlist;
	current->shitlist = shit;

	shit->chan   = stringcpy(shit->mask,mask) + 1;
	shit->from   = stringcpy(shit->chan,chan) + 1;
	shit->reason = stringcpy(shit->from,from) + 1;
	stringcpy(shit->reason,reason);

	current->ul_save++;
	return(shit);
}

Shit *find_shit(const char *userhost, const char *channel)
{
	Shit	*shit,*save;
	int	num,best;

	if (!userhost)
		return(NULL);
	save = NULL;
	best = 0;
	for(shit=current->shitlist;shit;shit=shit->next)
	{
		if (!channel || !stringcasecmp(channel,shit->chan) ||
		    (*shit->chan == '*') || (*channel == '*'))
		{
			num = num_matches(shit->mask,userhost);
			if (num > best)
			{
				best = num;
				save = shit;
			}
		}
	}
	if (save && save->expire < now)
	{
		remove_shit(save);
		save = NULL;
	}
	return(save);
}

Shit *get_shituser(char *userhost, char *channel)
{
	ChanUser *cu;
	Chan	*chan;
	Shit	*shit;
	char	*p;

#ifdef DEBUG
	debug("(get_shituser) userhost = '%s', channel = '%s'\n",
		nullstr(userhost),nullstr(channel));
#endif /* DEBUG */
	/*
	 *  save us a few million function calls if the shitlist is empty
	 */
	if (!current->shitlist)
		return(NULL);
	if (!nickcmp(current->nick,userhost))
		return(NULL);
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		for(cu=chan->users;cu;cu=cu->next)
		{
			p = get_nuh(cu);
			if (matches(userhost,p))
				continue;
			if ((shit = find_shit(p,channel)) != NULL)
				return(shit);
		}
	}
	return(NULL);
}

int get_shitaction(const char *userhost, const char *chan)
{
	Shit	*shit;

	if ((shit = find_shit(userhost,chan)))
		return(shit->action);
	return(0);
}

/*
 *
 *  commands related to shitlist
 *
 */

/*
 *  SHIT <channel|*> <nick|mask> <action> [expire] <reason>
 */
void do_shit(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS
	 */
	char	*channel,*nick,*nuh;
	int	shitlevel,days,uaccess,shitaccess;

	if (CurrentCmd->name == C_QSHIT)
	{
		channel = MATCH_ALL;
	}
	else
	{
		channel = chop(&rest);
		if (!ischannel(channel) && *channel != '*')
		{
		usage:
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}

	if ((uaccess = get_useraccess(from,channel)) < cmdaccess)
		return;

	if ((nick = chop(&rest)) == NULL)
		goto usage;

	if (CurrentCmd->name == C_QSHIT)
	{
		shitlevel = DEFAULTSHITLEVEL;
		days = 86400 * DEFAULTSHITLENGTH;
		if (*rest == 0)
			rest = TEXT_DEFAULTSHIT;
	}
	else
	{
		shitlevel = asc2int(chop(&rest));
		if (errno)
			goto usage;

		if (shitlevel < 1 || shitlevel > MAXSHITLEVEL)
		{
			to_user(from,"Valid levels are from 1 thru " MAXSHITLEVELSTRING);
			return;
		}

		/*
		 *  option: expire in XXX days
		 */
		days = 86400 * 30;
		if (*rest >= '1' && *rest <= '9')
		{
			days = 86400 * asc2int(chop(&rest));
			if (errno)
				goto usage;
		}

		if (*rest == 0)
			goto usage;
	}


#ifdef CHANBAN
	if (shitlevel == SHIT_CHANBAN)
	{
		nuh = nick;
	}
	else
#endif /* CHANBAN */
	{
		if ((nuh = nick2uh(from,nick)) == NULL)
			return;

		if (find_shit(nuh,channel))
		{
			to_user(from,TEXT_ALREADYSHITTED,nuh);
			return;
		}

		if (uaccess != OWNERLEVEL)
		{
			shitaccess = get_useraccess(nuh,channel);
			if (shitaccess > uaccess)
			{
				to_user(from,TEXT_SHITLOWACCESS,nuh);
				return;
			}
		}

		format_uh(nuh,FUH_USERHOST);
	}

#ifdef DEBUG
	debug("(do_shit) adding %s to %s (Level %i)\n",nuh,channel,shitlevel);
#endif /* DEBUG */
	add_shit(from,channel,nuh,rest,shitlevel,now + days);

	to_user(from,TEXT_HASSHITTED,nuh,channel);
	to_user(from,TEXT_SHITEXPIRES,time2str(now + days));

	check_shit();
}

void do_rshit(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Shit	*shit;
	char	*chan,*nick,*nuh;
	int	uaccess;

	chan = chop(&rest);
	if (!chan || !*chan || (!ischannel(chan) && *chan != '*'))
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if ((uaccess = get_useraccess(from,chan)) < cmdaccess)
		return;

	if ((nick = chop(&rest)) == NULL)
	{
		to_user(from,"No nick or userhost specified");
		return;
	}
	if ((nuh = nick2uh(from,nick)) == NULL)
		return;
	if ((shit = find_shit(nuh,chan)) == NULL)
	{
		to_user(from,"%s is not in my shit list on that channel",nuh);
		return;
	}
	if ((get_useraccess(shit->from,chan)) > uaccess)
	{
		to_user(from,"The person who did this shitlist has a higher level than you");
		return;
	}
	remove_shit(shit);
	to_user(from,"User %s is no longer being shitted on %s",nuh,chan);
}

char *shit_actions[MAXSHITLEVEL+1] = { "nothing (0)", "no op/voice (1)", "kickban (2)", "permaban (3)", "chanban (4)" };

void do_shitlist(COMMAND_ARGS)
{
	Shit	*shit;

	table_buffer("\037channel\037\t\037mask\037\t\037action\037\t\037set by\037\t\037reason\037\t\037expires\037");
	for(shit=current->shitlist;shit;shit=shit->next)
	{
		table_buffer(FMT_6XSTRTAB,shit->chan,shit->mask,shit_actions[shit->action],
			nickcpy(NULL,shit->from),shit->reason,time2away(shit->expire));
	}
	table_send(from,2);
}

void do_clearshit(COMMAND_ARGS)
{
	purge_shitlist();
	to_user(from,TEXT_CLEAREDSHITLIST);
}
