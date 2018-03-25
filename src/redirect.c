/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2018 proton

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
#define REDIRECT_C
#include "config.h"

#ifdef REDIRECT

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

int begin_redirect(char *from, char *args)
{
	char	*pt,*nick;

	if (!args)
		return(0);
	pt = STRCHR(args,'>');
	if (pt)
	{
		*pt = 0;
		nick = pt+1;
		pt--;
		while((pt > args) && (*pt == ' '))
		{
			*pt = 0;
			pt--;
		}
		while(*nick == ' ')
			nick++;
		if (*nick)
		{
#ifdef DEBUG
			debug("(begin_redirect) from %s --> %s\n",from,nick);
#endif /* DEBUG */
			if (ischannel(nick))
			{
				if (find_channel_ac(nick))
				{
					redirect.to = Strdup(nick);
					redirect.method = R_PRIVMSG;
					return(0);
				}
				else
				{
					to_user(from,ERR_CHAN,nick);
					return(-1);
				}
			}
			if (*nick == '>')
			{
				nick++;
				while(*nick == ' ')
					nick++;
				if (!*nick)
				{
					to_user(from,"Missing name for redirect.");
					return(-1);
				}
				if (is_safepath(nick,FILE_MAY_EXIST) != FILE_IS_SAFE) // redirect output is appended
				{
					to_user(from,"Bad filename.");
					return(-1);
				}
				redirect.to = Strdup(nick);
				redirect.method = R_FILE;
				return(0);
			}
			if ((pt = find_nuh(nick)))
			{
				redirect.to = Strdup(nick);
				redirect.method = R_NOTICE;
				return(0);
			}
			else
			{
				to_user(from,TEXT_UNKNOWNUSER,nick);
				return(-1);
			}
		}
		else
		{
			to_user(from,"Bad redirect");
			return(-1);
		}
	}
	return(0);
}

void send_redirect(char *message)
{
	Strp	*new,**pp;
	char	*fmt;
	int	fd;

	if (!redirect.to)
		return;

	switch(redirect.method)
	{
	case R_FILE:
		if ((fd = open(redirect.to,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) < 0)
			return;
		fmt = Strcat(message,"\n");
		write(fd,message,(fmt-message));
		close(fd);
		return;
#ifdef BOTNET
	case R_BOTNET:
		{
			char	tempdata[MAXLEN];
			Mech	*backup;

			/* PM<targetguid> <targetuserhost> <source> <message> */
			sprintf(tempdata,"%i %s %s %s",redirect.guid,redirect.to,current->nick,message);
			backup = current;
			partyMessage(NULL,tempdata);
			current = backup;
		}
		return;
#endif /* BOTNET */
	case R_NOTICE:
		fmt = "NOTICE %s :%s";
		break;
	/* case R_PRIVMSG: */
	default:
		fmt = "PRIVMSG %s :%s";
		break;
	}

	pp = &current->sendq;
	while(*pp)
		pp = &(*pp)->next;

	*pp = new = (Strp*)Calloc(sizeof(Strp) + StrlenX(message,fmt,redirect.to,NULL));
	/* Calloc sets to zero new->next = NULL; */
	sprintf(new->p,fmt,redirect.to,message);
}

void end_redirect(void)
{
	if (redirect.to)
		Free((char**)&redirect.to);
}

#endif /* REDIRECT */
