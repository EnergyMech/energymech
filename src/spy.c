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
#define SPY_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef DEBUG

LS const char SPY_DEFS[7][12] =
{
	"SPY_FILE",
	"SPY_CHANNEL",
	"SPY_DCC",
	"SPY_STATUS",
	"SPY_MESSAGE",
	"SPY_RAWIRC",
	"SPY_BOTNET"
};

#endif /* DEBUG */

void send_spy(const char *src, const char *format, ...)
{
	Chan	*chan;
	Mech	*backup;
	Spy	*spy;
	va_list	msg;
	const char *tempsrc;
	char	tempdata[MAXLEN];
	int	fd;
	int	printed = FALSE;

	tempsrc = (src == SPYSTR_STATUS) ? time2medium(now) : src;

#ifdef DEBUG
	debug("(send_spy) src %s format = '%s'\n",src,format);
#endif /* DEBUG */

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((*src == '#' || *src == '*') && spy->t_src == SPY_CHANNEL)
		{
			if ((*src != '*') && Strcasecmp(spy->src,src))
				continue;
			if ((chan = find_channel_ac(spy->src)) == NULL)
				continue;
			if (find_chanuser(chan,CurrentNick) == NULL)
				continue;
			tempsrc = spy->src;
		}
		else
		/*
		 *  by using string constants we can compare addresses
		 */
		if (spy->src != src)
			continue;

		if (!printed)
		{
			printed = TRUE;
			va_start(msg,format);
			vsprintf(tempdata,format,msg);
			va_end(msg);
		}

		switch(spy->t_dest)
		{
		case SPY_DCC:
			to_file(spy->dcc->sock,"[%s] %s\n",tempsrc,tempdata);
			break;
		case SPY_CHANNEL:
			if (spy->destbot >= 0)
			{
				backup = current;
				for(current=botlist;current;current=current->next)
				{
					if (current->guid == spy->destbot)
					{
						to_server("PRIVMSG %s :[%s] %s\n",spy->dest,tempsrc,tempdata);
						break;
					}
				}
				current = backup;
			}
			else
			{
				to_user(spy->dest,"[%s] %s",tempsrc,tempdata);
			}
			break;
		case SPY_FILE:
			if ((fd = open(spy->dest,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) >= 0)
			{
				to_file(fd,"[%s] %s\n",logtime(now),tempdata);
				close(fd);
			}
		}
	}
}

void send_global(const char *src, const char *format, ...)
{
	va_list msg;
	Mech	*backup;
	char	tempdata[MAXLEN];
	int	printed = FALSE;

	backup = current;
	for(current=botlist;current;current=current->next)
	{
		if (current->spy & SPYF_ANY)
		{
			if (!printed)
			{
				printed = TRUE;
				va_start(msg,format);
				vsprintf(tempdata,format,msg);
				va_end(msg);
			}

			send_spy(src,FMT_PLAIN,tempdata);
		}
	}
	current = backup;
}

void spy_typecount(Mech *bot)
{
	Spy	*spy;

	bot->spy = 0;
	for(spy=bot->spylist;spy;spy=spy->next)
	{
		bot->spy |= SPYF_ANY;
		bot->spy |= (1 << spy->t_src);
	}
}

struct
{
	char	*idstring;
	int	typenum;
	
} spy_source_list[] = 
{
{ SPYSTR_STATUS,	SPY_STATUS	},
{ SPYSTR_MESSAGE,	SPY_MESSAGE	},
{ SPYSTR_RAWIRC,	SPY_RAWIRC	},
{ SPYSTR_BOTNET,	SPY_BOTNET	},
{ NULL, 0 },
};

int spy_source(char *from, int *t_src, char **src)
{
	int	i;

	for(i=0;spy_source_list[i].idstring;i++)
	{
		if (!Strcasecmp(*src,spy_source_list[i].idstring))
		{
			*src = spy_source_list[i].idstring;
			*t_src = spy_source_list[i].typenum;
			return(200);
		}
	}
	*t_src = SPY_CHANNEL;
	if (!ischannel(*src))
		return(-1);
	return(get_useraccess(from,*src));
}

/*
 *
 *  commands related to spy-pipes
 *
 */

void do_spy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy;
	Mech	*backup,*destbot;
	char	*src,*dest;
	int	t_src,t_dest;
	int	sz;
	int	guid;

	if (!*rest)
	{
		if (!current->spylist)
		{
			to_user(from,"No active spy channels");
			return;
		}

		if (dcc_only_command(from))
			return;
		table_buffer("\037source\037\t\037target\037");
		for(spy=current->spylist;spy;spy=spy->next)
		{
			switch(spy->t_src)
			{
			case SPY_MESSAGE:
				src = "messages";
				break;
			default:
				src = spy->src;
			}
			dest = (spy->t_dest == SPY_FILE) ? " (file)" : "";
			table_buffer("%s\t%s%s",src,spy->dest,dest);
		}
		table_send(from,2);
		return;
	}

	src  = chop(&rest);
	dest = chop(&rest);

	if (!src)
	{
spy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	t_dest = SPY_DCC;
	guid = -1;
	destbot = NULL;

	if (*src >= '0' && *src <= '9')
	{
		guid = 0;
		while(*src && *src != ':')
		{
			guid = *src - '0';
			src++;
		}
		if (*src != ':' && !ischannel(src+1))
			goto spy_usage;
		src++;
		t_src = SPY_CHANNEL;
		/*
		 *  TODO: check access
		 */
#ifdef DEBUG
	debug("(do_spy) spy source guid = %i, channel = %s\n",guid,src);
#endif /* DEBUG */

		/*
		 *  is it a bot?
		 *  TODO: check botnet bots also
		 */
		for(backup=botlist;backup;backup=backup->next)
		{
			if (backup->guid == guid)
			{
				destbot = backup;
				goto guid_ok;
			}
		}
		to_user(from,"Unknown bot guid: %i",guid);
		return;
guid_ok:
		;
	}
	else
	{
		sz = spy_source(from,&t_src,&src);
		if (sz < 0)
			goto spy_usage;
		if (sz < cmdaccess)
			return;
	}

	if (dest)
	{
		/*
		 *  log to a file
		 */
		if (*dest == '>')
		{
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto spy_usage;
			}
			/*
			 *  Dont just open anything.
			 */
			if (!is_safepath(dest))
				goto spy_usage;
			t_dest = SPY_FILE;
			goto spy_dest_ok;
		}

		if (!ischannel(dest))
			goto spy_usage;
		if (get_useraccess(from,dest) < cmdaccess)
		{
			to_user(from,"You don't have enough access on %s",dest);
			return;
		}
		t_dest = SPY_CHANNEL;
	}

spy_dest_ok:
#ifdef DEBUG
	debug("(do_spy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],nullstr(dest),t_dest,SPY_DEFS[t_dest-1]);
	if (guid >= 0)
		debug("(do_spy) spying from remote bot guid %i (%s) channel %s\n",guid,(destbot) ? destbot->nick : "unknown",src);
#endif /* DEBUG */

	if (t_dest == SPY_DCC)
	{
		if (!CurrentDCC)
		{
			to_user(from,"Spying is only allowed in DCC chat");
			return;
		}
		dest = CurrentDCC->user->name;
	}

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) &&
			!Strcasecmp(spy->src,src) && !Strcasecmp(spy->dest,dest))
		{
			to_user(from,"Requested spy channel is already active");
			return;
		}
	}

	set_mallocdoer(do_spy);

	sz = sizeof(Spy);

	if (t_dest != SPY_DCC)
		sz += strlen(dest);

	if (t_src == SPY_CHANNEL)
		sz += strlen(src);

	spy = Calloc(sz);

	if (t_dest != SPY_DCC)
	{
		spy->dest = spy->p;
		spy->src = Strcat(spy->p,dest) + 1;
	}
	else
	{
		spy->dest = CurrentDCC->user->name;
		spy->dcc = CurrentDCC;
		spy->src = spy->p;
	}

	if (t_src == SPY_CHANNEL)
	{
		Strcpy(spy->src,src);
	}
	else
	{
		spy->src = src;
	}

	spy->t_src = t_src;
	spy->t_dest = t_dest;

	/*
	 *  finally link the spy record into the chain
	 *  TODO: botnet bots
	 */
	if (guid >= 0)
	{
		if (destbot)
		{
			spy->destbot = current->guid;
			spy->next = destbot->spylist;
			destbot->spylist = spy;
			spy_typecount(destbot);
		}
	}
	else
	{
		spy->destbot = -1;
		spy->next = current->spylist;
		current->spylist = spy;
		spy_typecount(current);
	}

	switch(t_src)
	{
	case SPY_STATUS:
		send_spy(SPYSTR_STATUS,"(%s) Added to mech core",nickcpy(NULL,from));
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been activated",src);
	}
}

void do_rspy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy,**pspy;
	char	*src,*dest,*tmp;
	int	t_src,t_dest;
	int	n;

	src  = chop(&rest);
	dest = chop(&rest);

	t_dest = SPY_DCC;

	if (!src)
	{
rspy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	n = spy_source(from,&t_src,&src);
	if (n < 0)
		goto rspy_usage;
	if (n < cmdaccess)
		return;

	if (dest)
	{
		if (*dest == '>')
		{
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto rspy_usage;
			}
			/*
			 *  Dont just open anything.
			 */
			if (!is_safepath(dest))
				goto rspy_usage;
			t_dest = SPY_FILE;
			goto rspy_dest_ok;
		}

		if (ischannel(dest))
			t_dest = SPY_CHANNEL;
	}
	else
		dest = from;

rspy_dest_ok:
#ifdef DEBUG
	debug("(do_rspy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],dest,t_dest,SPY_DEFS[t_dest-1]);
#endif /* DEBUG */

	/*
	 *  check if the spy channel exists
	 */
	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) && (!Strcasecmp(spy->src,src)))
		{
			if ((t_dest == SPY_DCC) && (!nickcmp(spy->dest,dest)))
				break;
			else
			if (!Strcasecmp(spy->dest,dest))
				break;
		}
	}
	if (!spy)
	{
		to_user(from,"No matching spy channel could be found");
		return;
	}

	switch(t_src)
	{
	case SPY_STATUS:
		tmp = (t_dest == SPY_DCC) ? nickcpy(NULL,spy->dest) : spy->dest;
		send_spy(SPYSTR_STATUS,"(%s) Removed from mech core",tmp);
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been removed",src);
		break;
	}

	pspy = &current->spylist;
	while(*pspy)
	{
		if (*pspy == spy)
		{
			*pspy = spy->next;
			Free((char**)&spy);
			return;
		}
		pspy = &(*pspy)->next;
	}
	spy_typecount(current);
}
