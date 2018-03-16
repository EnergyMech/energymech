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
#define RESET_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

#ifdef TELNET
LS int client_type = DCC_ACTIVE;
#endif /* TELNET */

char *recover_client(char *env)
{
	struct	sockaddr_in sai;
	Client	*client;
	User	*user;
	char	*p,*handle;
	int	guid,fd,sz;

	guid = fd = 0;
	p = env;

	/*
	 *  get the guid number
	 */
	while(*p >= '0' && *p <= '9')
	{
		guid = (guid * 10) + (*p - '0');
		p++;
	}
	if (*p != ':')
		return(env);
	p++;

	/*
	 *  get the fd number
	 */
	while(*p >= '0' && *p <= '9')
	{
		fd = (fd * 10) + (*p - '0');
		p++;
	}
	if (*p != ':')
		return(env);
	p++;

	handle = p;
	while(*p)
	{
		if (*p == ' ' || *p == 0)
			break;
		p++;
	}
	if (p == handle)
		return(env);

	if (*p == ' ')
		*(p++) = 0;

#ifdef DEBUG
	debug("(recover_client) guid = %i; fd = %i; handle = %s\n",guid,fd,handle);
#endif /* DEBUG */

	/*
	 *  check that it's an inet stream socket
	 */
	sz = sizeof(sai);
	if (getsockname(fd,(struct sockaddr*)&sai,&sz) < 0)
	{
		close(fd);
		return(p);
	}

	for(current=botlist;current;current=current->next)
	{
		if (current->guid == guid)
		{
			for(user=current->userlist;user;user=user->next)
			{
				if (!Strcasecmp(user->name,handle))
					goto found_user;
			}
			break;
		}
	}
	close(fd);
	killsock(fd);
	return(p);

found_user:
	if (to_file(fd,"[%s] [%s] %s[%i] has connected (reset recover)\n",
		time2medium(now),current->wantnick,handle,user->x.x.access) < 0)
	{
		close(fd);
		return(p);
	}

	set_mallocdoer(recover_client);
	client = (Client*)Calloc(sizeof(Client));
	client->user = user;
	client->sock = fd;
#ifdef TELNET
	client->flags = client_type;
#else
	client->flags = DCC_ACTIVE;
#endif /* TELNET */
	client->lasttime = now;

	client->next = current->clientlist;
	current->clientlist = client;

	if (user->x.x.access == OWNERLEVEL)
	{
		CurrentDCC = client;
		Strcpy(client->sockdata,"status");
		do_spy(user->name,current->wantnick,client->sockdata,0);
		*client->sockdata = 0;
		CurrentDCC = NULL;
	}

#ifdef DEBUG
	debug("(recover_client) client socket recovered\n");
#endif /* DEBUG */

	return(p);
}

#ifdef DEBUG

char *recover_debug(char *env)
{
	struct	stat s;
	char	*p;

	debug_fd = 0;
	p = env;

	/*
	 *  get the fd number
	 */
	while(*p >= '0' && *p <= '9')
	{
		debug_fd = (debug_fd * 10) + (*p - '0');
		p++;
	}
	if (*p != ' ' && *p != 0)
		return(env);

	if (fstat(debug_fd,&s) < 0)
	{
		dodebug = FALSE;
		close(debug_fd);
		debug_fd = -1;
	}
	else
	{
		dodebug = TRUE;
		debug("(recover_debug) debug fd recovered\n");
		CoreClient.sock = debug_fd;
	}
	return(p);
}

#endif /* DEBUG */

//execve( ./energymech, argv = { ./energymech <NULL> <NULL> <NULL> <NULL> }, envp = { MECHRESET=d3 f1881:2:X12 f99:4:X12 } )

char *recover_server(char *env)
{
	struct	sockaddr_in sai;
	char	*p;
	int	guid,fd,sz;
#ifdef IRCD_EXTENSIONS
	int	ircx = 0;
#endif /* IRCD_EXTENSIONS */

	guid = fd = 0;
	p = env;

	/*
	 *  get the guid number
	 */
	while(*p >= '0' && *p <= '9')
	{
		guid = (guid * 10) + (*p - '0');
		p++;
	}
	if (*p != ':')
		return(env);
	p++;

	/*
	 *  get the fd number
	 */
	while(*p >= '0' && *p <= '9')
	{
		fd = (fd * 10) + (*p - '0');
		p++;
	}
#ifndef IRCD_EXTENSIONS
	if (*p != ' ' && *p != 0)
		return(env);
#endif /* ! IRCD_EXTENSIONS */

#ifdef IRCD_EXTENSIONS
	if (*p == ':' && *(p+1) == 'X')
	{
		p += 2;
		/*
		 *  get the ircx flags number
		 */
		while(*p >= '0' && *p <= '9')
		{
			ircx = (ircx * 10) + (*p - '0');
			p++;
		}
		if (*p != ' ' && *p != 0)
			return(env);
	}
	else
	if (*p != ' ' && *p != 0)
		return(env);
#ifdef DEBUG
	debug("(recover_server) guid = %i; fd = %i, ircx = %i\n",guid,fd,ircx);
#endif /* DEBUG */
#else /* IRCD_EXTENSIONS */
#ifdef DEBUG
	debug("(recover_server) guid = %i; fd = %i\n",guid,fd);
#endif /* DEBUG */
#endif /* IRCD_EXTENSIONS */

	sz = sizeof(sai);
	if (getsockname(fd,(struct sockaddr*)&sai,&sz) < 0)
	{
		close(fd);
		return(p);
	}
	for(current=botlist;current;current=current->next)
	{
		if (current->guid == guid)
		{
			current->reset = 1;
			current->sock = fd;
			current->connect = CN_ONLINE;
			current->ontime = now;
#ifdef IRCD_EXTENSIONS
			current->ircx_flags = ircx;
#endif /* IRCD_EXTENSIONS */
#ifdef DEBUG
			debug("(recover_server) {%i} server socket recovered\n",fd);
#endif /* DEBUG */
			to_file(fd,"LUSERS\n");
			fd = -1;
			break;
		}
	}
	// if we recover a guid:socket without a matching bot in config, it got removed or changed guid
	// if the guid changed, we cant guess which old<-->new is the matching one so
	if (fd != -1)
	{
		to_file(fd,"QUIT :I'm no longer wanted *cry*\n");
		killsock(fd);
	}
	return(p);
}

#ifdef IRCD_EXTENSIONS

/*
#define IRCX_WALLCHOPS          1
#define IRCX_WALLVOICES         2
#define IRCX_IMODE              4
#define IRCX_EMODE              8
*/

#endif /* IRCD_EXTENSIONS */

void recover_reset(void)
{
	char	*env = mechresetenv;

	mechresetenv = NULL;

	while(*env)
	{
		switch(*env)
		{
		case 'c':
#ifdef TELNET
			client_type = DCC_ACTIVE;
#endif /* TELNET */
			env = recover_client(env+1);
			break;
#ifdef DEBUG
		case 'd':
			env = recover_debug(env+1);
			break;
#endif /* DEBUG */
#ifdef TELNET
		case 't':
			client_type = DCC_TELNET;
			env = recover_client(env+1);
			break;
#endif /* TELNET */
		case 'f':
			env = recover_server(env+1);
			break;
		default:
			env++;
		}
	}
}

/*
 *
 *  commands
 *
 */

void do_reset(COMMAND_ARGS)
{
	Client	*client;
	Mech	*backup;
	char	env[MSGLEN];
	char	*p;
	int	n,sz;

	if (current->userlist && current->ul_save)
	{
		p = current->setting[STR_USERFILE].str_var;
		write_userlist(p);
	}

#ifdef SESSION
	write_session();
#endif /* SESSION */

#ifdef SEEN
	if (seenfile)
		write_seenlist();
#endif /* SEEN */

#ifdef TRIVIA
	write_triviascore();
#endif /* TRIVIA */

#ifdef NOTIFY
	if (current->notifylist)
		write_notifylog();
#endif /* NOTIFY */

	*env = 0;
	p = Strcat(env,STR_MECHRESET);
	n = 0;
#ifdef DEBUG
	/*
	 *  debug stuff
	 */
	if (dodebug && (debug_fd >= 0))
	{
		sprintf(p,"d%i",debug_fd);
		p = STREND(p);
		n++;
	}
#endif /* DEBUG */
	/*
	 *  Save server connections
	 */
	backup = current;
	for(current=botlist;current;current=current->next)
	{
		if ((current->connect == CN_ONLINE) && ((MSGLEN - (p - env)) > 25))
		{
			unset_closeonexec(current->sock);
			if (n)
				*(p++) = ' ';
#ifdef IRCD_EXTENSIONS
			sprintf(p,"f%i:%i:X%i",current->guid,current->sock,current->ircx_flags);
#else /* IRCD_EXTENSIONS */
			sprintf(p,"f%i:%i",current->guid,current->sock);
#endif /* IRCD_EXTENSIONS */
			p = STREND(p);
			n++;
			to_server("PING :OT%lu\n",current->ontime);
		}
		for(client=current->clientlist;client;client=client->next)
		{
#ifdef TELNET
			if ((client->flags & (DCC_ACTIVE|DCC_TELNET)) == 0)
				continue;
#else
			if (client->flags != DCC_ACTIVE)
				continue;
#endif /* TELNET */
			sz = strlen(client->user->name) + 26;
			if ((MSGLEN - (p - env)) > sz)
			{
				unset_closeonexec(client->sock);
				if (n)
					*(p++) = ' ';
#ifdef TELNET
				sprintf(p,(client->flags & DCC_TELNET) ? "t%i:%i:%s" : "c%i:%i:%s",
					current->guid,current->sock,client->user->name);
#else
				sprintf(p,"c%i:%i:%s",current->guid,current->sock,client->user->name);
#endif /* TELNET */
				p = STREND(p);
				n++;
			}
		}
	}
	current = backup;

#ifdef DEBUG
	debug("(do_reset) %s [%i]\n",env,(int)(p - env));
#endif /* DEBUG */

	mechresetenv = (n) ? env : NULL;

	do_exec = TRUE;
	mech_exec();
	/* NOT REACHED */
}
