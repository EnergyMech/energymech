/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2009 proton

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
#define GREET_C
#include "config.h"

#ifdef GREET

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *  woo.. no args? we use CurrentChan, CurrentNick and CurrentUser.
 */
void greet(void)
{
	Strp	*sp,**pp;
	char	linebuf[MSGLEN],readbuf[MSGLEN];
	char	*str;
	int	fd,sz;

	pp = &current->sendq;
	while(*pp)
		pp = &(*pp)->next;

	if (CurrentUser->x.x.greetfile)
	{
		if ((fd = open(CurrentUser->greet,O_RDONLY)) < 0)
			return;

		sz = sizeof(Strp) + 9 + strlen(CurrentNick);

		memset(readbuf,0,sizeof(readbuf));
		while(TRUE)
		{
			str = sockread(fd,readbuf,linebuf);
			if (str)
			{
				*pp = sp = (Strp*)Calloc(sz + strlen(str));
				/* Calloc sets to zero sp->next = NULL; */
				pp = &sp->next;
				sprintf(sp->p,"NOTICE %s :%s",CurrentNick,str);
			}
			else
			if (errno != EAGAIN)
				break;
		}

		close(fd);
	}
	else
	if (CurrentUser->x.x.randline)
	{
		if ((str = randstring(CurrentUser->greet)))
			goto single_line;
		return;
	}
	else
	{
		str = CurrentUser->greet;
single_line:
		*pp = sp = (Strp*)Calloc(sizeof(Strp) + 13 + StrlenX(CurrentChan->name,CurrentNick,str,NULL));
		sprintf(sp->p,"PRIVMSG %s :[%s] %s",CurrentChan->name,CurrentNick,str);
		/* Calloc sets to zero sp->next = NULL; */
	}
}

/*
 *
 *  commands tied to the greeting feature
 *
 */

void do_greet(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*handle;
	int	isfile;

	if ((handle = chop(&rest)) == NULL)
		goto usage;

	if ((user = find_handle(handle)) == NULL)
	{
		to_user(from,TEXT_UNKNOWNUSER,handle);
		return;
	}

	if (!usercanmodify(from,user))
	{
		to_user(from,TEXT_USEROWNSYOU,user->name);
		return;
	}

	isfile = FALSE;

	if (*rest == '@' || *rest == '%')
	{
		isfile = *rest;
		rest++;
		while(*rest == ' ')
			rest++;
	}

	if (*rest)
	{
		if (isfile)
		{
			if (is_safepath(rest,FILE_MUST_EXIST) != FILE_IS_SAFE)
				goto usage;
		}

		user->x.x.greetfile = (isfile == '@') ? TRUE : FALSE;
		user->x.x.randline  = (isfile == '%') ? TRUE : FALSE;

		set_mallocdoer(do_greet);
		user->greet = stringdup(rest);

		to_user(from,"greeting for user %s has been set to: %s%s",user->name,user->greet,
			(isfile == '@') ? " (file)" : ((isfile == '%') ? " (random line from file)" : ""));
		current->ul_save++;
	}
	else
	if (isfile)
	{
		goto usage;
	}
	else
	if (user->greet)
	{
		Free((char**)&user->greet);
		to_user(from,"greeting for user %s has been removed",user->name);
		current->ul_save++;
	}
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

#endif /* GREET */
