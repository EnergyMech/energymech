/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2005 proton

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
#define BOUNCE_C
#include "config.h"

#ifdef BOUNCE
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#define INTERNAL_NICK		"bounce"
#define INTERNAL_SOURCE		":bounce!bounce@bounce"
#define TEXT_ASK_HANDLE		INTERNAL_SOURCE " PRIVMSG %s :" TEXT_ENTERNICKNAME "\n"
#define TEXT_ASK_PASSWORD	INTERNAL_SOURCE " PRIVMSG %s :" TEXT_ENTERPASSWORD "\n"
#define TEXT_ASK_SERVER		INTERNAL_SOURCE " PRIVMSG %s :Which server would you like to connect to?\n"
#define TEXT_USING_VHOST	INTERNAL_SOURCE " PRIVMSG %s :Using virtual host %s\n"
#define TEXT_NOW_CONNECTING	INTERNAL_SOURCE " PRIVMSG %s :Now connecting to %s:%i ...\n"

#define BNC_LOGIN		0
#define BNC_ASK_HANDLE		1
#define BNC_ASK_PASSWORD	2
#define BNC_ASK_SERVER		3
#define BNC_CONNECTING		4
#define BNC_ACTIVE		5
#define BNC_DEAD		6

#ifdef IDWRAP

#define USE_VHOST		2

#else /* not IDWRAP */

#define USE_VHOST		TRUE

#endif /* IDWRAP */

void bounce_parse(ircLink *irc, char *message)
{
	Mech	fakebot;
	User	*user;
	char	*cmd,*server,*aport,*virtual;
	int	iport;

	if (irc->status == BNC_LOGIN)
	{
		cmd = chop(&message);
		if (!stringcasecmp(cmd,"USER"))
		{
			if (irc->userLine)
				return;
			set_mallocdoer(bounce_parse);
			irc->userLine = stringdup(message);
		}
		else
		if (!stringcasecmp(cmd,"NICK"))
		{
			if (irc->nickLine)
				return;
			set_mallocdoer(bounce_parse);
			irc->nickLine = stringdup(message);
			if ((cmd = chop(&message)) == NULL)
			{
				irc->status = BNC_DEAD;
				return;
			}
			set_mallocdoer(bounce_parse);
			irc->nick = stringdup(cmd);
		}
		if (irc->userLine && irc->nickLine)
		{
			to_file(irc->usersock,TEXT_ASK_HANDLE,irc->nick);
			irc->active = now;
			++irc->status;
		}
		return;
	}

	/* "PRIVMSG bounce :<handle>"			*/
	/* "PRIVMSG bounce :<password>"			*/
	/* "PRIVMSG bounce :<server> [port [virtual]]"	*/

	server = chop(&message);
	cmd    = chop(&message);

	if (!cmd || stringcasecmp(server,"PRIVMSG") || nickcmp(cmd,INTERNAL_NICK)
		|| (*message != ':'))
	{
		irc->status = BNC_DEAD;
		return;
	}

	message++;

	if (irc->status == BNC_ASK_HANDLE)
	{
		for(current=botlist;current;current=current->next)
		{
			if ((user = find_handle(message)))
			{
				set_mallocdoer(bounce_parse);
				irc->handle = stringdup(user->name);
				break;
			}
		}
		to_file(irc->usersock,TEXT_ASK_PASSWORD,irc->nick);
		++irc->status;
		/*
		 *  dont bomb out even if the username is invalid
		 *  guessing of valid usernames would be possible otherwise
		 */
	}
	else
	if (irc->status == BNC_ASK_PASSWORD)
	{
		irc->status = BNC_DEAD;
		for(current=botlist;current;current=current->next)
		{
			if (irc->handle && (user = find_handle(irc->handle)))
			{
				if (user->x.x.bounce && user->pass
					&& passmatch(message,user->pass))
				{
					to_file(irc->usersock,TEXT_ASK_SERVER,irc->nick);
					irc->status = BNC_ASK_SERVER;
					irc->active = now;
					return;
				}
			}
		}
	}
	else
	if (irc->status == BNC_ASK_SERVER)
	{
		server  = chop(&message);
		aport   = chop(&message);
		virtual = chop(&message);

		irc->status = BNC_DEAD;

		if (!server)
			return;

		iport = 6667;
		if (aport)
		{
			iport = asc2int(aport);
			if (errno || (iport < 1) || (iport > 65536))
				return;
		}

		current = &fakebot;
		current->vhost_type = 0;
		current->setting[STR_VIRTUAL].str_var = virtual; /* may be NULL, is OK */
#ifdef WINGATE
		current->setting[STR_WINGATE].str_var = NULL;
#endif /* WINGATE */
#ifdef IDWRAP
		current->identfile = NULL;
		current->setting[STR_IDENT].str_var = irc->handle;
#endif /* IDWRAP */

		if (virtual)
		{
			to_file(irc->usersock,TEXT_USING_VHOST,irc->nick,virtual);
		}
		to_file(irc->usersock,TEXT_NOW_CONNECTING,irc->nick,server,iport);
		if ((irc->servsock = SockConnect(server,iport,USE_VHOST)) >= 0)
		{
			irc->status = BNC_CONNECTING;
			irc->active = now + 60;	/* 120 second timeout */
#ifdef IDWRAP
			irc->idfile = current->identfile;
#endif /* IDWRAP */
		}
	}
}

#ifdef IDWRAP

void bounce_cleanup(void)
{
	ircLink *irc;

	for(irc=bnclist;irc;irc=irc->next)
	{
		if (irc->idfile)
			unlink(irc->idfile);
	}
}

#endif /* IDWRAP */

void new_port_bounce(const struct Setting *no_op)
{
	if (bounce_sock != -1)
		close(bounce_sock);
	bounce_sock = -1;
}

void select_bounce(void)
{
	ircLink	*irc;

	if ((bounce_sock == -1) && (bounce_port > 0))
	{
		bounce_sock = SockListener(bounce_port);
#ifdef DEBUG
		if (bounce_sock >= 0)
		{
			debug("(select_bounce) {%i} irc proxy socket is active (%i)\n",
				bounce_sock,bounce_port);
		}
#endif /* DEBUG */
	}

	if (bounce_sock != -1)
	{
		chkhigh(bounce_sock);
		FD_SET(bounce_sock,&read_fds);
	}

	for(irc=bnclist;irc;irc=irc->next)
	{
		if (irc->servsock != -1)
		{
			chkhigh(irc->servsock);
			if (irc->status == BNC_CONNECTING)
				FD_SET(irc->servsock,&write_fds);
			else
				FD_SET(irc->servsock,&read_fds);
		}
		/*
		 *  dont read data from client when connecting to server
		 */
		if (irc->status != BNC_CONNECTING)
		{
			chkhigh(irc->usersock);
			FD_SET(irc->usersock,&read_fds);
		}
	}
}

void process_bounce(void)
{
	char	message[MSGLEN];
	ircLink *irc,**pp;
	char	*p;
	int	s;

	if ((bounce_sock != -1) && FD_ISSET(bounce_sock,&read_fds))
	{
		if ((s = SockAccept(bounce_sock)) >= 0)
		{
			set_mallocdoer(process_bounce);
			irc = (ircLink*)Calloc(sizeof(ircLink));		/* sets all to zero */
			irc->next = bnclist;
			bnclist = irc;
			irc->active = now;
			irc->usersock = s;
			--irc->servsock; /* == -1 */
		}
	}

	for(irc=bnclist;irc;irc=irc->next)
	{
		/*
		 *  client socket has data to read
		 */
		if (FD_ISSET(irc->usersock,&read_fds))
		{
			while((p = sockread(irc->usersock,irc->usermem,message)))
			{
				if (irc->status == BNC_ACTIVE)
				{
					if (to_file(irc->servsock,FMT_PLAINLINE,message) < 0)
						/* fall into switch() with a valid errno */
						break;
				}
				else
				{
					bounce_parse(irc,message);
				}
			}
			switch(errno)
			{
			default:
				irc->status = BNC_DEAD;
			case EAGAIN:
			case EINTR:
				break;
			}
		}

		if (irc->servsock < 0)
			continue;

		/*
		 *  server socket just got connected
		 */
		if (FD_ISSET(irc->servsock,&write_fds))
		{
#ifdef DEBUG
			debug("(process_bounce) {%i} servsock connected\n",irc->servsock);
#endif /* DEBUG */
			irc->status = BNC_ACTIVE;
			irc->active = now;
			to_file(irc->servsock,"USER %s\n",irc->userLine);
			if (to_file(irc->servsock,"NICK %s\n",irc->nickLine) < 0)
			{
				/*
				 *  write error of some kind, dump both client and server
				 */
				irc->status = BNC_DEAD;
			}
			Free((char**)&irc->userLine);
			Free((char**)&irc->nickLine);
		}
		/*
		 *  server socket has data to read
		 */
		if (FD_ISSET(irc->servsock,&read_fds))
		{
			irc->active = now;
			while((p = sockread(irc->servsock,irc->servmem,message)))
			{
				if (to_file(irc->usersock,FMT_PLAINLINE,message) < 0)
					/* fall into switch() with a valid errno */
					break;
			}
			switch(errno)
			{
			default:
				irc->status = BNC_DEAD;
			case EAGAIN:
			case EINTR:
				break;
			}
		}
	}

	/*
	 *  clean out dead and time outed sockets
	 */
	pp = &bnclist;
	while((irc = *pp))
	{
		if (irc->status == BNC_DEAD || ((irc->status != BNC_ACTIVE) && ((now - irc->active) > 60)))
		{
#ifdef DEBUG
			debug("(process_bounce) {%i} {%i} BNC_DEAD or timeout, removing...\n",irc->usersock,irc->servsock);
#endif /* DEBUG */
			*pp = irc->next;
			if (irc->servsock >= 0)
				close(irc->servsock);
			if (irc->usersock >= 0)
				close(irc->usersock);
			Free((char**)&irc->userLine);
			Free((char**)&irc->nickLine);
			Free((char**)&irc->nick);
			Free((char**)&irc->handle);
#ifdef IDWRAP
			if (irc->idfile)
			{
				unlink(irc->idfile);
				Free((char**)&irc->idfile);
			}
#endif /* IDWRAP */
			Free((char**)&irc);
			continue;
		}
		pp = &irc->next;
	}
}

#endif /* BOUNCE */
