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

#define mkaxx(x)	(0x40404040 + (0x0f0f & x) + ((0xf0f0 & x) << 12))
#define getaxx(x)	(((x & 0x0f0f0000) >> 12) | (x & 0x00000f0f))

char *recover_client(char *env)
{
	union {
	uint32_t num[2];
	char	asc[8];
	} axx;
	struct	sockaddr_in sai;
	Client	*client;
	User	*user;
	char	*p,*handle;
	int	guid = 0,fd = 0,sz;

	if (env[8] != ':')
		return(env);

	memcpy(axx.asc,env,8); /* compiler is not stupid and will optimize the shit out of this */
	guid = getaxx(axx.num[0]);
	fd = getaxx(axx.num[1]);

	handle = p = (env = env + 9);
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
				if (!stringcasecmp(user->name,handle))
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
		stringcpy(client->sockdata,"status");
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
	union {
	uint32_t num;
	char	asc[4];
	} axx;
	struct	stat s;

	debug_fd = 0;

	if (env[4] != ' ' && env[4] != 0)
		return(env);
	/*
	 *  get the fd number
	 */
	memcpy(axx.asc,env,4); /* compiler is not stupid and will optimize the shit out of this */
	debug_fd = getaxx(axx.num);

	if (fstat(debug_fd,&s) < 0)
	{
		dodebug = FALSE;
		close(debug_fd);
		debug_fd = -1;
	}
	else
	{
		dodebug = TRUE;
		debug("(recover_debug) {%i} debug fd recovered\n",debug_fd);
		CoreClient.sock = debug_fd;
	}
	return(env+4);
}

#endif /* DEBUG */

/*
(do_reset) mkaxx(3) = C@@@
(do_reset) ircx mkaxx(12) = L@@@
(do_reset) sock mkaxx(2) = B@@@
(do_reset) guid mkaxx(1881) = IGE@
[StS] {2} PING :OT1523818405
(do_reset) MECHRESET=dC@@@ fXIGE@B@@@L@@@ tIGE@F@@@:joo [44]
execve( ./energymech, argv = { ./energymech <NULL> <NULL> <NULL> <NULL> }, envp = { MECHRESET=dC@@@ fXIGE@B@@@L@@@ tIGE@F@@@:joo } )
*/

char *recover_server(char *env)
{
	union {
	uint32_t num[4];
	char	asc[16];
	} axx;
	struct	sockaddr_in sai;
	char	*p;
	int	guid = 0,fd = 0,sz;
#ifdef IRCD_EXTENSIONS
	int	ircx = 0;
#endif /* IRCD_EXTENSIONS */

	switch(*env++)
	{
	case 'x':
		sz = 8;
		break;
	case 'X':
		sz = 12;
		break;
	default:
		return(env);
	}
	if (env[sz] != ' ' && env[sz] != 0)
		return(env);

	memcpy(axx.asc,env,sz); /* compiler is not stupid and will optimize the shit out of this */
	env += sz;
	guid = getaxx(axx.num[0]);
	fd = getaxx(axx.num[1]);
#ifdef IRCD_EXTENSIONS
	ircx = getaxx(axx.num[2]);
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
		return(env);
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
	/* if we recover a guid:socket without a matching bot in config, it got removed or changed guid */
	/* if the guid changed, we cant guess which old<-->new is the matching one so */
	if (fd != -1)
	{
		to_file(fd,"QUIT :I'm no longer wanted *cry*\n");
		killsock(fd);
	}
	return(env);
}

/*
(do_reset) MECHRESET=dC@@@ fXIGE@A@@@L@@@ tIGE@F@@@:joo [44]
execve( ./energymech, argv = { ./energymech <NULL> <NULL> <NULL> <NULL> }, envp = { MECHRESET=dC@@@ fXIGE@A@@@L@@@ tIGE@F@@@:joo } )
 */

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
		while(*env == ' ')
			env++;
	}
}

/*
-+=====================================================================================================================+-
                                                        reset.c commands
-+=====================================================================================================================+-
*/

void do_reset(COMMAND_ARGS)
{
	union {
	uint32_t num[8];
	char	asc[32];
	} axx;
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
	p = stringcat(env,STR_MECHRESET);
#ifdef DEBUG
	/*
	 *  debug stuff
	 */
	if (dodebug && (debug_fd >= 0))
	{
		axx.num[0] = mkaxx(debug_fd);
		axx.num[1] = 0;
		sprintf(p,"d%s",axx.asc);
		p = STREND(p);
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
			axx.num[0] = mkaxx(current->guid);
			axx.num[1] = mkaxx(current->sock);
#ifdef IRCD_EXTENSIONS
			axx.num[2] = mkaxx(current->ircx_flags);
			axx.num[3] = 0;
			sprintf(p," fX%s",axx.asc);
#else /* IRCD_EXTENSIONS */
			axx.num[2] = 0;
			sprintf(p," fx%s",axx.asc);
#endif /* IRCD_EXTENSIONS */
			p = STREND(p);
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
				axx.num[0] = mkaxx(current->guid);
				axx.num[1] = mkaxx(client->sock);
				axx.num[2] = 0;
#ifdef TELNET
				sprintf(p,(client->flags & DCC_TELNET) ? " t%s:%s" : " c%s:%s",
					axx.asc,client->user->name);
#else
				sprintf(p," c%s:%s",axx.asc,client->user->name);
#endif /* TELNET */
				p = STREND(p);
			}
		}
	}
	current = backup;

#ifdef DEBUG
	debug("(do_reset) %s [%i]\n",env,(int)(p - env));
#endif /* DEBUG */

	mechresetenv = (*env) ? env : NULL;

	do_exec = TRUE;
	mech_exec();
	/* NOT REACHED */
}
