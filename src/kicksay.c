/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2004 proton

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
#define KICKSAY_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

KickSay *find_kicksay(char *text, char *channel)
{
	KickSay *kick,*save;
	int	num,best;

	save = NULL;
	best = 0;
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (!channel || *kick->chan == '*' || !stringcasecmp(channel,kick->chan))
		{
			num = num_matches(kick->mask,text);
			if (num > best)
			{
				best = num;
				save = kick;
			}
		}
	}
	return(save);
}

void check_kicksay(Chan *chan, ChanUser *doer, char *text)
{
	KickSay *kick,*save;
	char *mask;
	int action;

	save = NULL;
	action = -1;
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (*kick->chan == '*' || !stringcasecmp(chan->name,kick->chan))
		{
			if (!matches(kick->mask,text))
			{
				if (kick->action > action)
				{
					action = kick->action;
					save = kick;
				}
			}
		}
	}
	if (save)
	{
		if (!action)
		{
			if (doer->flags & CU_KSWARN)
				action = 1;
			if (!(doer->flags & CU_KSWARN))
			{
				doer->flags |= CU_KSWARN;
				to_server("NOTICE %s :%s\n",doer->nick,save->reason);
			}
		}
		if (action > 1)
		{
			mask = format_uh(get_nuh(doer),FUH_USERHOST);
			if (action > 2)
			{
				add_shit("Auto KS",chan->name,mask,save->reason,2,now+3600);
			}
			if (!(doer->flags & CU_BANNED))
			{
				doer->flags |= CU_BANNED;
				send_mode(chan,90,QM_RAWMODE,'+','b',mask);
			}
		}
		if (action && !(doer->flags & CU_KICKED))
		{
			doer->flags |= CU_KICKED;
			send_kick(chan,CurrentNick,FMT_PLAIN,save->reason);
		}
	}
}

void remove_kicksay(KickSay *kick)
{
	KickSay **pp;

	pp = &current->kicklist;
	while(*pp)
	{
		if (*pp == kick)
		{
			*pp = kick->next;
			Free((char**)&kick);
			return;
		}
		pp = &(*pp)->next;
	}
}

void purge_kicklist(void)
{
	while(current->kicklist)
		remove_kicksay(current->kicklist);
}

/*
 *
 *  kicksay commands
 *
 */

#ifdef NEWBIE
char *ks_actions[MAX_KS_LEVEL+1] = { "warn (0)","kick (1)","kickban (2)","autoshit (3)" };
#else
char *ks_actions[MAX_KS_LEVEL+1] = { "warn","kick","kickban","autoshit" };
#endif /* NEWBIE */

void do_kicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel,*mask;
	int	inum;

	channel = chop(&rest);
	if (!channel)
	{
		if (!current->kicklist)
		{
			to_user(from,"Kicksay list is empty");
			return;
		}

		if (partyline_only_command(from))
			return;

		table_buffer("\037channel\037\t\037action\037\t\037string\037\t\037kick reason\037");
		for(kick=current->kicklist;kick;kick=kick->next)
		{
			table_buffer(FMT_4XSTRTAB,kick->chan,ks_actions[kick->action],kick->mask,kick->reason);
		}
		table_send(from,2);
		return;
	}

	if (ischannel(channel) || *channel == '*')
	{

		if (get_useraccess(from,channel) < cmdaccess)
			return;

		inum = DEFAULT_KS_LEVEL;
		if (*rest != '"')
		{
			inum = asc2int(chop(&rest));
			if (errno || inum < 0 || inum > MAX_KS_LEVEL)
				return;
		}

		mask = get_token(&rest,"\"");

		if (!mask || !*mask)
			goto usage;

		/*
		 *  check for previously existing kicks
		 */
		if ((kick = find_kicksay(mask,channel)) != NULL)
		{
			to_user(from,"I'm already kicking on \"%s\"",kick->mask);
			return;
		}

		/*
		 *  dig out the reason (the rest)
		 */
		while(rest && *rest == ' ')
			rest++;
		if (!*rest)
			goto usage;

		/*
		 *  add it to the list
		 */
		set_mallocdoer(do_kicksay);
		kick = (KickSay*)Calloc(sizeof(KickSay) + StrlenX(channel,mask,rest,NULL));

		kick->next = current->kicklist;
		current->kicklist = kick;
		kick->action = inum;

		if (!matches("\\*?*\\*",mask))
			kick->chan = stringcpy(kick->mask,mask) + 1;
		else
		{
			kick->mask[0] = '*';
			stringcpy(kick->mask+1,mask);
			kick->chan = stringcat(kick->mask,MATCH_ALL) + 1;
		}
		kick->reason = stringcpy(kick->chan,channel) + 1;
		stringcpy(kick->reason,rest);

		to_user(from,"Now kicking on \"%s\" on %s",mask,channel);
		current->ul_save++;
		return;
	}
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_rkicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel;

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if (!ischannel(channel) && *channel != '*')
		goto usage;

	if (!*rest)
		goto usage;

	if (!(kick = find_kicksay(rest,channel)))
	{
		to_user(from,"I'm not kicking on that");
		return;
	}
	to_user(from,"No longer kicking on %s",kick->mask);
	remove_kicksay(kick);
	current->ul_save++;
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}
