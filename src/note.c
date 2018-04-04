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
#define NOTE_C
#include "config.h"

#ifdef NOTE

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

int catch_note(char *from, char *to, char *rest)
{
	User	*u;
	Note	*n,**pp;
	Strp	*sp,**np;

#ifdef DEBUG
	debug("(catch_note) from = %s, to = %s, rest = %s\n",from,to,rest);
#endif /* DEBUG */

	pp = &notelist;
	while(*pp)
	{
		n = *pp;
#ifdef DEBUG
		debug("(catch_note) n->from = %s, n->to = %s\n",n->from,n->to);
#endif /* DEBUG */
		if (!stringcasecmp(from,n->from) && !stringcasecmp(to,n->to))
		{
#ifdef DEBUG
			debug("(catch_note) note to user = %s\n",n->user);
#endif /* DEBUG */
			if (rest[0] == '.' && rest[1] == 0)
			{
				to_user(from,"Note for %s has been saved",n->user);
				*pp = n->next;
				Free((char**)&n);
				return(TRUE);
			}
			if (!(u = find_handle(n->user)))
				return(TRUE);
			append_strp(&u->note,rest);
			return(TRUE);
		}
		if ((now - n->start) > 120)
		{
			*pp = n->next;
			Free((char**)&n);
			return(TRUE);
		}
		pp = &(*pp)->next;
	}
	return(FALSE);
}

/*
 *
 *
 *
 */

void do_note(COMMAND_ARGS)
{
	User	*u;
	Note	*n;
	Strp	*sp,**np;
	char	header[MSGLEN];

	/*
	 *  no-args is handled in on_msg()
	 */
	if (!(u = find_handle(rest)))
	{
		to_user(from,TEXT_UNKNOWNUSER,rest);
		return;
	}
	to_user(from,"Enter your note for %s, end with \".\" on a line by itself",
		u->name);

	set_mallocdoer(do_note);
	n = Calloc(sizeof(Note) + StrlenX(from,to,u->name,NULL));
	n->start = now;
	n->next = notelist;
	notelist = n;

	n->to = stringcat(n->from,from) + 1;
	n->user = stringcat(n->to,to) + 1;
	stringcpy(n->user,rest);

	/*
	 *  add a note header
	 */
	sprintf(header,"\001%s %s",from,time2str(now));
	append_strp(&u->note,header);
}

void do_read(COMMAND_ARGS)
{
	Strp	*sp,**pp;
	User	*user;
	char	*opt,*sender;
	int	which,n,sz;

	if (CurrentDCC)
		user = CurrentUser;
	else
	if ((user = get_authuser(from,ANY_CHANNEL)) == NULL)
		return;

	sz = n = 0;
	for(sp=user->note;sp;sp=sp->next)
	{
		if (*sp->p == 1)
			n++;
		else
			sz += strlen(sp->p);
	}

	if (rest && (opt = chop(&rest)))
	{
		which = asc2int(opt);
		if (errno || !which)
			goto read_usage;

		if (which < -n || which > n)
		{
			to_user(from,"invalid message number");
			return;
		}

		n = 0;
		pp = &user->note;
		while(*pp)
		{
			sp = *pp;
			if (*sp->p == 1)
				n++;

			if (which == n)
			{
				opt = sp->p + 1;
				sender = chop(&opt);
				to_user(from,"From: %s on %s",sender,opt);
				opt[-1] = ' ';
				to_user(from," ");
				sp = sp->next;
				while(sp && *sp->p != 1)
				{
					to_user(from,FMT_PLAIN,sp->p);
					sp = sp->next;
				}
				to_user(from," ");
				return;
			}
			else
			if (which == -n)
			{
				while(TRUE)
				{
					*pp = sp->next;
					Free((char**)&sp);
					sp = *pp;
					if (!sp || *sp->p == 1)
						break;
				}
				to_user(from,"message number %i has been deleted",-which);
				return;
			}
		}
	}

	to_user(from,(n) ? "you have %i message%s (%i bytes)" : "you have no messages",n,(n == 1) ? "" : "s",sz);
	return;
read_usage:
	usage(from);	/* usage for CurrentCmd->name */
}

#endif /* NOTE */
