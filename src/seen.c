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
#define SEEN_C
#include "config.h"

#ifdef SEEN

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

int write_seenlist(void)
{
	Seen	*seen;
	int	f;
#ifdef DEBUG
	int	dodeb;
#endif /* DEBUG */

	if (!seenfile)
		return(FALSE);
	if ((f = open(seenfile,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) < 0)
		return(FALSE);

#ifdef DEBUG
	dodeb = dodebug;
	dodebug = FALSE;
#endif /* DEBUG */

	for(seen=seenlist;seen;seen=seen->next)
	{
		if ((seen->when - now) > (86400 * SEEN_TIME))
			continue;
		else
		{
			if (seen->t != 0)
			{
				to_file(f,"%s %s %lu %i %s %s\n",
					seen->nick,seen->userhost,
					seen->when,seen->t,
					(seen->pa) ? seen->pa : "",
					(seen->pb) ? seen->pb : "");
			}
		}
	}
	close(f);

#ifdef DEBUG
	dodebug = dodeb;
#endif /* DEBUG */
	return(TRUE);
}

int read_seenlist_callback(char *rest)
{
	char	*nick,*uh,*pa,*pb;
	time_t	when;
	int	t;

	nick = chop(&rest);
	uh   = chop(&rest);

	when = a2i(chop(&rest)); /* seen time, a2i handles NULL */
	if (errno)
		return(FALSE);

	t    = a2i(chop(&rest)); /* seen type, a2i handles NULL */
	if (errno)
		return(FALSE);

	pa = chop(&rest);
	pb = rest;

	if ((now - when) < (SEEN_TIME * 86400))
	{
		/* if (pa && !*pa)
			pa = NULL; chop() doesnt return empty strings */
		if (!*pb)
			pb = NULL;
		make_seen(nick,uh,pa,pb,when,t);
	}
	return(FALSE);
}

int read_seenlist(void)
{
	Seen	*seen;
	int	in;
#ifdef DEBUG
	int	dodeb;
#endif /* DEBUG */

	if (!seenfile || ((in = open(seenfile,O_RDONLY)) < 0))
		return(FALSE);

#ifdef DEBUG
	dodeb = dodebug;
	dodebug = FALSE;
#endif /* DEBUG */

	while((seen = seenlist))
	{
		seenlist = seen->next;
		Free((char**)&seen);
	}

	readline(in,&read_seenlist_callback); /* readline closes in */

#ifdef DEBUG
	dodebug = dodeb;
#endif /* DEBUG */
	return(TRUE);
}

void make_seen(char *nick, char *userhost, char *pa, char *pb, time_t when, int t)
{
	Seen	*seen,**pp;
	char	*pt;
	uchar	c1;
	int	i;

	for(pt=userhost;*pt;pt++)
	{
		if (*pt == '!')
		{
			userhost = pt + 1;
			break;
		}
	}

	c1 = nickcmptab[(uchar)(*nick)];
	pt = nick + 1;
	pp = &seenlist;

step_one:
	if (*pp)
	{
		if (c1 > nickcmptab[(uchar)(*(*pp)->nick)])
		{
			pp = &(*pp)->next;
			goto step_one;
		}
	}

step_two:
	if (*pp)
	{
		if (c1 == nickcmptab[(uchar)(*(*pp)->nick)])
		{
			i = nickcmp(pt,(*pp)->nick+1);
			if (i > 0)
			{
				pp = &(*pp)->next;
				goto step_two;
			}
			if (!i)
			{
				seen = *pp;
				*pp = seen->next;
				Free((char**)&seen);
			}
		}
	}

	/*
	 *  dont fuck with this code unless you really know what you're doing
	 *  pa might be NULL, but then pb is NULL also; pb might be NULL
	 *  any NULL terminates the Strlen() check
	 */
	set_mallocdoer(make_seen);
	seen = (Seen*)Calloc(sizeof(Seen) + Strlen(nick,userhost,pa,pb,NULL));

	seen->next = *pp;
	*pp = seen;
	seen->when = when;
	seen->t = t;
	/* Calloc sets to zero seen->pa = seen->pb = NULL; */

	seen->userhost = Strcpy(seen->nick,nick) + 1;
	pt = Strcpy(seen->userhost,userhost) + 1;
	if (pa)
	{
		seen->pa = pt;
		pt = Strcpy(seen->pa,pa) + 1;
		if (pb)
		{
			seen->pb = pt;
			Strcpy(seen->pb,pb);
		}
	}

}

/*
 *
 *  commands for seen features
 *
 */

void do_seen(COMMAND_ARGS)
{
	Seen	*seen;
	char	ago[35];		/* enought for "36500 days, 23 hours and 59 minutes" (100 years) */
	char	*chan,*fmt,*n,*u,*c1,*c2,*c3;
	time_t	when;
	int	d,h,m,mul;

	chan = get_channel(to,&rest);
	mul = get_maxaccess(from);

	if (!*rest)
	{
		if (mul) to_user_q(from,"Who do you want me look for?");
		return;
	}

	n = chop(&rest);
	if (!is_nick(n))
	{
		if (mul) to_user_q(from,ERR_NICK,n);
		return;
	}

	if (!nickcmp(n,current->nick))
	{
		fmt = "%s is me you dweeb!";
	}
	else
	if (!nickcmp(n,from))
	{
		fmt = "Trying to find yourself %s?";
	}
	else
	{
		for(seen=seenlist;seen;seen=seen->next)
		{
			if (!Strcasecmp(n,seen->nick))
				break;
		}

		if (!seen)
		{
			fmt = "I have no memory of %s";
		}
		else
		{
			when = now - seen->when;
			d = when / 86400;
			h = (when -= d * 86400) / 3600;
			m = (when -= h * 3600) / 60;

			*ago = 0;
			c2 = ago;

			if (d)
			{
				sprintf(c2,"%i day%s, ",d,EXTRA_CHAR(d));
			}
			if (h || d)
			{
				sprintf(ago,"%s%i hour%s and ",ago,h,EXTRA_CHAR(h));
			}
			sprintf(ago,"%s%i minute%s",ago,m,EXTRA_CHAR(m));

			n = seen->nick;
			u = seen->userhost;
			c1 = seen->pa;
			c2 = ago;

			switch(seen->t)
			{
			case SEEN_PARTED:
				fmt = "%s (%s) parted from %s, %s ago";
				break;
			case SEEN_QUIT:
				fmt = "%s (%s) signed off with message \"%s\", %s ago";
				break;
			case SEEN_NEWNICK:
				fmt = "%s (%s) changed nicks to %s, %s ago";
				break;
			case SEEN_KICKED:
				c2 = seen->pb;
				c3 = ago;
				fmt = "%s (%s) was kicked by %s with message \"%s\", %s ago";
			}
		}
	}

	to_user_q(from,fmt,n,u,c1,c2,c3);
}

#endif /* SEEN */
