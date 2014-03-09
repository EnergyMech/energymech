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
#define ALIAS_C
#include "config.h"

#ifdef ALIAS
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

void afmt(char *copy_to, const char *src, const char *input)
{
#define BUFTAIL	(tmp+MSGLEN-1)		/* avoid buffer overflows */
	char	tmp[MSGLEN];
	const char *np;
	char	*dest;
	int	n,t,tu,spc;

	dest = tmp;
	while(*src)
	{
check_fmt:
		if (*src == '$')
		{
			src++;
			if (*src == '$')
				goto copychar;
			tu = t = n = 0;
			while(attrtab[(uchar)*src] & NUM)
			{
				n = (n * 10) + (*(src++) - '0');
			}
			if (n)
			{
				if (*src == '-')
				{
					tu = n;
					src++;
					while(attrtab[(uchar)*src] & NUM)
					{
						t = (t * 10) + (*(src++) - '0');
					}
				}
				n--;
				spc = 0;
				for(np=input;*np;)
				{
					if (*np == ' ')
					{
						if (!spc)
							n--;
						spc = 1;
					}
					else
					{
						spc = 0;
						if (!n)
							break;
					}
					np++;
				}
				spc = 0;
				while(*np)
				{
					if (*np == ' ')
					{
						if (!tu || (t && tu >= t))
							goto check_fmt;
						if (!spc)
							tu++;
						spc = 1;
					}
					else
					{
						spc = 0;
					}
					if (dest == BUFTAIL)
						goto afmt_escape;
					*(dest++) = *(np++);
				}
			}
			goto check_fmt;
		}
copychar:
		if (dest == BUFTAIL)
			goto afmt_escape;
		*(dest++) = *(src++);
	}
afmt_escape:
	*dest = 0;
	Strcpy(copy_to,tmp);
}

/*
 *
 *  associated commands
 *
 */

void do_alias(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	Alias	*alias;
	char	*cmd;

	if (!*rest)
	{
		/* list all aliases */
		if (!aliaslist)
			to_user(from,TEXT_NOALIASES);
		else
		{
			if (dcc_only_command(from))
				return;
			to_user(from,"\037Alias\037              \037Format\037");
			for(alias=aliaslist;alias;alias=alias->next)
			{
				to_user(from,"%-18s %s",alias->alias,alias->format);
			}
		}
		return;
	}

	cmd = chop(&rest);
	if (!*rest)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	for(alias=aliaslist;alias;alias=alias->next)
	{
		if (!Strcasecmp(alias->alias,cmd))
		{
			Free(&alias->format);
			set_mallocdoer(do_alias);
			alias->format = Strdup(rest);
			to_user(from,"Replaced alias: %s --> %s",cmd,rest);
			return;
		}
	}
	set_mallocdoer(do_alias);
	alias = (Alias*)Calloc(sizeof(Alias)+strlen(cmd));
	Strcpy(alias->alias,cmd);
	set_mallocdoer(do_alias);
	alias->format = Strdup(rest);
	alias->next = aliaslist;
	aliaslist = alias;
	to_user(from,"Added alias: %s --> %s",cmd,rest);
}

void do_unalias(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	Alias	*alias,**ap;

	for(ap=&aliaslist;*ap;ap=&(*ap)->next)
	{
		if (!Strcasecmp(rest,(*ap)->alias))
		{
			alias = *ap;
			*ap = alias->next;
			to_user(from,"Removed alias: %s (--> %s)",alias->alias,alias->format);
			Free(&alias->format);
			Free((void*)&alias);
			return;
		}
	}
	to_user(from,"Couldnt find matching alias");
}

#endif /* ALIAS */
