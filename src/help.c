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
#define HELP_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "usage.h"

void print_help(char *from, char *line, int len)
{
	char	*lp;
	int	tl;

	tl = TRUE;
	if ((strlen(line) + len) > 70)
	{
		lp = line;
		while(*lp)
		{
			if (*lp == '\037')
				tl = !tl;
			if (tl)
				*lp = tolowertab[(uchar)*lp];
			lp++;
		}
		to_user(from,FMT_PLAIN,line);
		*line = 0;
	}
}

int do_help_callback(char *line)
{
#ifdef DEBUG
	debug("(do_help_callback) `%s'\n",line);
#endif /* DEBUG */
	table_buffer(FMT_PLAIN,line);
	return(FALSE);
}

void do_help(COMMAND_ARGS)
{
	char	line[MSGLEN];
	char	*pt;
	int	i,level,axs;
	int	cur,nxt,count,ci,tl;
	int	in;

	if (partyline_only_command(from))
		return;

	axs = get_maxaccess(from);

	if (!*rest)
	{
		cur = -1;
		*line = 0;
help_loop:
		count = 0;
		nxt = OWNERLEVEL;

		for(i=0;mcmd[i].name;i++)
		{
			tl = acmd[i];
			if ((tl < nxt) && (tl > cur))
				nxt = tl;
			if (tl != cur)
				continue;
			if (count == 0)
				sprintf(line,"\037Level %3i\037: ",cur);
			count++;
		}
		if (!count)
		{
			cur = nxt;
			goto help_loop;
		}
		ci = count;
		for(i=0;mcmd[i].name;i++)
		{
			tl = acmd[i];
			if (tl != cur)
				continue;
			if (ci != count)
				stringcat(line,", ");
			print_help(from,line,strlen(mcmd[i].name));
			if (*line == 0)
				stringcpy(line,"           ");
			stringcat(line,(char*)mcmd[i].name);
			count--;
		}
		print_help(from,line,500);
		if ((cur != OWNERLEVEL) && (nxt <= axs))
			cur = nxt;
		else
			return;
		goto help_loop;
	}

	level = asc2int(rest);
	if (!errno)
	{
		if ((level > axs) || (level < 0))
			level = axs;
		to_user(from,"\037Commands available at Level %i:\037",level);
		*line = 0;
		ci = 0;
		for(i=0;mcmd[i].name;i++)
		{
			if (acmd[i] <= level)
			{
				if (ci != 0)
					stringcat(line,", ");
				ci++;
				print_help(from,line,strlen(mcmd[i].name));
				stringcat(line,(char*)mcmd[i].name);
			}
		}
		if (ci)
			print_help(from,line,58);
		else
			to_user(from,TEXT_NONE);
		return;
	}

	if (STRCHR(rest,'*'))
	{
		line[0] = 0;
		ci = 0;
		to_user(from,"\037Commands that match query %s\037:",rest);
		for(i=0;mcmd[i].name;i++)
		{
			if ((!matches(rest,(char*)mcmd[i].name)) && (acmd[i] <= axs))
			{
				if (ci != 0)
					stringcat(line,", ");
				ci++;
				print_help(from,line,strlen(mcmd[i].name));
				stringcat(line,(char*)mcmd[i].name);
			}
		}
		if (ci)
			print_help(from,line,500);
		else
			to_user(from,TEXT_NONE);
		return;
	}

	pt = stringcpy(line,HELPDIR);
	for(i=0;(rest[i]);i++)
	{
		if (rest[i] >= 'a' && rest[i] <= 'z')
			*pt = rest[i] - 0x20;
		else
			*pt = rest[i];
		pt++;
	}
	*pt = 0;

	/*
	 *  We dont want to show help for "../../../../../../etc/passwd"
	 */
	if (is_safepath(line,FILE_MUST_EXIST) != FILE_IS_SAFE)
	{
#ifdef DEBUG
		debug("(do_help) unsafe help filename (%s), exiting\n",line);
#endif /* DEBUG */
		to_user(from,"Help file for \"%s\" is unavailable.",rest);
		return;
	}

#ifdef DEBUG
	debug("(do_help) help file check: %s\n",line);
#endif /* DEBUG */
	if ((in = open(line,O_RDONLY)) < 0)
	{
		to_user(from,"No help found for \"%s\"",rest);
		return;
	}

#ifdef DEBUG
	debug("(do_help) helpfile for = '%s'\n",rest);
#endif /* DEBUG */

	table_buffer("\037Help on %s\037",rest);
	level = access_needed(rest);
	if (level > 200)
		table_buffer("Level needed: Command disabled");
	else
	if (level > 0)
		table_buffer("Level needed: %i",level);
	for(i=0;ulist[i].command;i++)
	{
		if (!stringcasecmp(rest,ulist[i].command))
		{
			pt = (ulist[i].usage) ? ulist[i].usage : "";
			table_buffer("Usage: %s %s",ulist[i].command,pt);
			break;
		}
	}
	readline(in,&do_help_callback);				/* readline closes in */
	table_send(from,0);
}

void usage_command(char *to, const char *arg)
{
	char	*pt;
	int	i;

	for(i=0;ulist[i].command;i++)
	{
		if (!stringcasecmp(arg,ulist[i].command))
		{
			pt = ulist[i].usage;
			to_user_q(to,(pt) ? "Usage: %s %s" : "Usage: %s",ulist[i].command,pt);
			return;
		}
	}
	to_user(to,"Usage: (missing)");
}

void usage(char *to)
{
	CurrentChan = NULL;
	usage_command(to,CurrentCmd->name);
}

void do_usage(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*cmd;
	int	i;

	cmd = chop(&rest);
	for(i=0;mcmd[i].name;i++)
	{
		if (!stringcasecmp(cmd,mcmd[i].name))
		{
			usage_command(from,mcmd[i].name);
			return;
		}
	}
	to_user_q(from,"Unknown command: %s",cmd);
}
