/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2018 proton

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
#define CTCP_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

Client *find_client(const char *nick)
{
	Client	*client;

	if (CurrentDCC && CurrentDCC->user->name == nick)
		return(CurrentDCC);

	for(client=current->clientlist;client;client=client->next)
	{
		if (((client->flags & DCC_SEND) == 0) && !stringcasecmp(nick,client->user->name))
			return(client);
	}
	return(NULL);
}

void delete_client(Client *client)
{
	Client	**pp;
	Spy	*spy,**pspy;

#ifdef DEBUG
	if (client->sock >= 0)
		debug("(delete_client) closing {%i}\n",client->sock);
#endif /* DEBUG */

	pp = &current->clientlist;
	while(*pp)
	{
		if (*pp == client)
		{
			*pp = client->next;
			break;
		}
		pp = &(*pp)->next;
	}

	if (client->user)
	{
		pspy = &current->spylist;
		while(*pspy)
		{
			spy = *pspy;
			/*
			 *  for DCC spy dest we use the exact same pointer as in the userlist
			 */
			if (spy->dest == client->user->name)
			{
				*pspy = spy->next;
				Free((char**)&spy);
				continue;
			}
			pspy = &(*pspy)->next;
		}
		send_global(SPYSTR_STATUS,"[%s] %s[%i] has disconnected",
			current->nick,client->user->name,client->user->x.x.access);
	}
#ifdef DCC_FILE
	if (client->fileno >= 0)
		close(client->fileno);
#endif /* DCC_FILE */
	if (client->sock >= 0)
		close(client->sock);
	Free((char **)&client);
}

#ifdef DCC_FILE

int dcc_sendfile(char *target, char *filename)
{
	struct	sockaddr_in sai;
	Client	*client;
	int	s,f,sz;
	char	tempfile[strlen(filename)+strlen(DCC_PUBLICFILES)+2]; // strlen(DCC_PUBLICFILES) evaluates at compile time to a constant.

	stringcpy(tempfile,DCC_PUBLICFILES);
	stringcat(tempfile,filename);

#ifdef DEBUG
	debug("(dcc_sendfile) opening %s for transfer\n",tempfile);
#endif /* DEBUG */
	if ((f = open(tempfile,O_RDONLY)) < 0)
		return -1;
	if ((s = SockListener(0)) < 0)
		return -1;

	sz = sizeof(sai);
	if (getsockname(s,(struct sockaddr *)&sai,&sz) < 0)
	{
		close(s);
		return -1;
	}

	set_mallocdoer(dcc_sendfile);
	client = (Client*)Calloc(sizeof(Client) + Strlen2(filename,target)); // target is never NULL

	client->fileno = f;
	client->sock = s;
	client->user = NULL;
	client->flags = DCC_WAIT|DCC_ASYNC|DCC_SEND;
	client->lasttime = now;
	client->whom = stringcpy(client->filename,filename) + 1;
	stringcpy(client->whom,target);

	client->next = current->clientlist;
	current->clientlist = client;

	sz = lseek(f,0,SEEK_END);
	lseek(f,0,SEEK_SET);
	client->fileend = sz;

	to_server("PRIVMSG %s :\001DCC SEND %s %u %u %i\001\n",target,
		filename,htonl(current->ip.s_addr),ntohs(sai.sin_port),sz);
	return(sz);
}

void dcc_pushfile(Client *client, off_t where)
{
	char	tempdata[16384];
	int	sz;

	sz = (where + 16384) - lseek(client->fileno,0,SEEK_CUR);
	sz = read(client->fileno,tempdata,sz);
	if (sz < 1)
		return;
	sz = write(client->sock,tempdata,sz);
}

int dcc_freeslots(int uaccess)
{
	Client	*client;
	int	n;

	n = (uaccess > 0) ? current->setting[INT_DCCUSER].int_var : 0;
	n += current->setting[INT_DCCANON].int_var;
	for(client=current->clientlist;client;client=client->next)
	{
		if (client->flags & (DCC_SEND|DCC_RECV))
			n--;
	}
	return(n);
}

#endif /* DCC_FILE */

void parse_dcc(Client *client)
{
	char	text[MSGLEN];
	char	*ptr,*bp;
	int	s,oc;

	if (client->flags & DCC_WAIT)
	{
#ifdef DCC_FILE
		if (client->flags & DCC_RECV)
		{
			client->flags = DCC_SEND|DCC_RECV;
			return;
		}
#endif /* DCC_FILE */
#ifdef DEBUG
#ifdef DCC_FILE
		if (client->flags & DCC_SEND)
			debug("(parse_dcc) new dcc filetransfer connecting\n");
		else
#endif /* DCC_FILE */
			debug("(parse_dcc) new user connecting\n");
#endif /* DEBUG */
		s = SockAccept(client->sock);
		close(client->sock);
		client->sock = s;
		if (s == -1)
		{
			client->flags = DCC_DELETE;
			return;
		}

		if ((client->flags & DCC_SEND) == 0)
		{
			/*
			 *  tell them how much we love them
			 */
			partyline_banner(client);
		}
#ifdef DCC_FILE
		else
		{
			client->flags = DCC_SEND;
			client->start = now;
			dcc_pushfile(client,0);
		}
#endif /* DCC_FILE */
		return;
	}

	/*
	 *  read data from socket
	 */
#ifdef DCC_FILE
	if (client->flags & DCC_RECV)
	{
		char	bigtemp[4096];
		uint32_t where;

		do
		{
			s = read(client->sock,bigtemp,4096);
			if (s > 0)
				if (write(client->fileno,bigtemp,s) == -1)
					return;
		}
		while(s > 0);

		if ((s < 0) && (errno != EINTR) && (errno != EAGAIN))
			client->flags = DCC_DELETE;

		where = oc = lseek(client->fileno,0,SEEK_CUR);
		where = htonl(where);

		if (write(client->sock,&where,4) == -1)
			return;

		client->lasttime = now;

		if (oc == client->fileend)
		{
#ifdef DEBUG
			debug("(parse_dcc) DCC file recv `%s' completed in %lu seconds (%i bytes)\n",
				client->filename,(client->lasttime - client->start),client->fileend);
#endif /* DEBUG */
			client->flags = DCC_DELETE;
		}
		return;
	}
	if (client->flags & DCC_SEND)
	{
		uint32_t where;

		client->lasttime = now;
		s = client->inputcount;
		oc = read(client->sock,(client->sockdata+s),(4-s));
		if ((oc < 1) && (errno != EINTR) && (errno != EAGAIN))
		{
			client->flags = DCC_DELETE;
			return;
		}
		oc += s;
		if (oc == 4)
		{
			where = ((uchar)client->sockdata[0] << 24) |
				((uchar)client->sockdata[1] << 16) |
				((uchar)client->sockdata[2] << 8) |
				(uchar)client->sockdata[3];
			client->inputcount = 0;
			if (client->fileend == where)
			{
#ifdef TCL
				tcl_dcc_complete(client,0);
#endif /* TCL */
#ifdef DEBUG
				debug("(parse_dcc) dcc_sendfile: end of file\n");
#endif /* DEBUG */
				client->flags = DCC_DELETE;
			}
			else
				dcc_pushfile(client,where);
		}
		else
		{
#ifdef DEBUG
			debug("(parse_dcc) DCC_SEND partial ack\n");
#endif /* DEBUG */
			client->inputcount = oc;
		}
		return;
	}
#endif /* DCC_FILE */
	ptr = sockread(client->sock,client->sockdata,text);
	oc = errno;
#ifdef TELNET
	if (ptr && (client->flags & DCC_TELNETPASS))
	{
		check_telnet_pass(client,ptr);
	}
	else
#endif /* TELNET */
	if (ptr)
	{
		/*
		 *  DCC input flood protection
		 */
		s = now - client->lasttime;
		if (s > 10)
		{
			client->inputcount = strlen(ptr);
		}
		else
		{
			client->inputcount += strlen(ptr);
			if ((client->inputcount - (s * DCC_INPUT_DECAY)) > DCC_INPUT_LIMIT)
			{
				client->flags = DCC_DELETE;
				return;
			}
		}
		/*
		 *  set up source-destination information for on_msg/on_action
		 */
		CurrentShit = NULL;
		CurrentChan = NULL;
		client->lasttime = now;
		CurrentDCC  = client;
		CurrentUser = client->user;
		stringcpy(CurrentNick,CurrentUser->name);

		if (*ptr == 1)
		{
			bp = ptr;
			chop(&bp);
			ptr = get_token(&bp,"\001");
			on_action(CurrentUser->name,current->nick,ptr);
		}
		else
		{
			on_msg(CurrentUser->name,current->nick,ptr);
		}
		CurrentDCC = NULL;
		/*
		 *  get out of here ASAP, consider dangerous commands such as DIE and DEL (user)
		 */
		return;
	}
	switch(oc)
	{
	case EAGAIN:
	case EINTR:
		return;
	default:
		client->flags = DCC_DELETE;
		return;
	}
}

void process_dcc(void)
{
	Client	*client;

	for(client=current->clientlist;client;client=client->next)
	{
		if (FD_ISSET(client->sock,&read_fds))
		{
			parse_dcc(client);
		}
		else
		if ((client->flags & DCC_ASYNC) && (FD_ISSET(client->sock,&write_fds)))
		{
#ifdef DEBUG
			debug("(process_dcc) chat connected [ASYNC]\n");
#endif /* DEBUG */
			partyline_banner(client);
		}
		else
		if ((client->flags & DCC_WAIT) && ((now - client->lasttime) >= WAITTIMEOUT))
		{
#ifdef DEBUG
			debug("(process_dcc) connection timed out (%s)\n",
				(client->flags & (DCC_SEND|DCC_RECV)) ? "file transfer" : client->user->name);
#endif /* DEBUG */
			client->flags = DCC_DELETE;
		}
#ifdef DCC_FILE
		else
		if ((client->flags & DCC_SEND) && ((now - client->lasttime) >= DCC_FILETIMEOUT))
		{
#ifdef DEBUG
			debug("(process_dcc) {%i} DCC %s stalled (%s), closing connection\n",
				client->sock,(client->flags & DCC_RECV) ? "RECV" : "SEND",
				nullstr(client->filename));
#endif /* DEBUG */
			client->flags = DCC_DELETE;
		}
#endif /* DCC_FILE */
#ifdef TELNET
		else
		if ((client->flags & DCC_TELNETPASS) && ((now - client->lasttime) >= TELNET_TIMEOUT))
		{
			client->flags = DCC_DELETE;
		}
#endif /* TELNET */
	}
}

void ctcp_dcc(char *from, char *to, char *rest)
{
	struct	in_addr ia;
	Client	*client;
	User	*user;
	char	*addr,*port,ip_addr[20];
	uint32_t longip;
	int	x;
#ifdef DCC_FILE
	char	*filename;
	int	s,f,portnum,filesz;
#endif /* DCC_FILE */

	if (!rest || !*rest)
		return;

	addr = port = chop(&rest);

	while(*addr)
	{
		if (*addr >= 'a' && *addr <= 'z')
			*addr = *addr - 0x20;
		addr++;
	}

	x = get_maxaccess(from);
	send_spy(SPYSTR_STATUS,"[DCC] :%s[%i]: Requested DCC %s [%s]",CurrentNick,x,port,nullstr(rest));

#ifdef DCC_FILE
	if (!stringcasecmp(port,"SEND"))
	{
#ifdef DEBUG
		debug("(ctcp_dcc) rest: `%s'\n",nullstr(rest));
#endif /* DEBUG */
		filename = chop(&rest);
		if (is_safepath(filename,FILE_MUST_EXIST) != FILE_IS_SAFE)
		{
#ifdef DEBUG
			debug("(ctcp_dcc) filename `%s' is not safe\n",filename);
#endif /* DEBUG */
			return;
		}
		/*
		 *  check filename against masks in STR_DCCFILES
		 */
		s = 1;
		if ((addr = current->setting[STR_DCCFILES].str_var))
		{
			char	tempmask[strlen(addr)+1];

			stringcpy(tempmask,addr);
			do
			{
				port = chop(&addr);
				if (matches(port,filename) == 0)
				{
					s = 0;
					break;
				}
			}
			while(port && *port);
		}
		/*
		 *
		 */
		if (s)
			return;
		/*
		 *  check for free DCC slots (x is maxuserlevel from above)
		 */
		if (dcc_freeslots(x) < 1)
			return;

		addr = chop(&rest);
		port = chop(&rest);
		portnum = asc2int(port);
		x = errno;
		filesz = asc2int(rest);
		if (errno || x || portnum < 1024 || portnum > 63353 || filesz <= 0)
			return;

		longip = 0;
		while(*addr)
		{
			longip = (longip * 10) + (*addr - '0');
			addr++;
		}
		ia.s_addr = ntohl(longip);
		stringcpy(ip_addr,inet_ntoa(ia));

		if (1)
		{
			char tempname[strlen(filename)+strlen(DCC_PUBLICINCOMING)+1]; // strlen(DCC_PUBLICINCOMING) evaluates to a constant during compile.

			stringcpy(stringcpy(tempname,DCC_PUBLICINCOMING),filename);

			if ((f = open(tempname,O_RDWR|O_CREAT|O_EXCL,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) < 0)
				return;
			if ((s = SockConnect(ip_addr,portnum,FALSE)) < 0)
			{
				close(f);
				return;
			}
			set_mallocdoer(ctcp_dcc);
			client = (Client*)Calloc(sizeof(Client) + Strlen2(filename,from)); // from is never NULL
			client->fileno = f;
			client->fileend = filesz;
			client->sock = s;
			client->flags = DCC_WAIT|DCC_SEND|DCC_RECV;
			client->lasttime = client->start = now;
			client->whom = stringcpy(client->filename,filename) + 1;
			stringcpy(client->whom,from);

			client->next = current->clientlist;
			current->clientlist = client;
		}
	}
	else
#endif /* DCC_FILE */
	if (x && (x < BOTLEVEL) && !stringcasecmp(port,"CHAT"))
	{
		if ((user = get_authuser(from,NULL)) == NULL)
		{
			to_user(from,"Use \"VERIFY\" to get verified first");
			return;
		}

		if (find_client(user->name))
			return;

		chop(&rest);	/* discard "chat" */
		addr = chop(&rest);
		port = chop(&rest);
		if (!port || !*port)
			return;

		x = asc2int(port);
		if (errno || (x < 1024) || (x > 65535))
			return;

		/*
		 *  quick hack; if someone sends a fake/non-numeric long_ip, this all just fails
		 *  and we couldnt care less. blaha.
		 */
		longip = 0;
		while(*addr)
		{
			longip = (longip * 10) + (*addr - '0');
			addr++;
		}
		ia.s_addr = ntohl(longip);
		stringcpy(ip_addr,inet_ntoa(ia));

#ifdef DEBUG
		debug("(ctcp_dcc) %s [%s,%s]\n",from,ip_addr,port);
#endif /* DEBUG */

		if ((x = SockConnect(ip_addr,x,FALSE)) < 0)
			return;

		set_mallocdoer(ctcp_dcc);
		client = (Client*)Calloc(sizeof(Client));
#ifdef DCC_FILE
		client->fileno = -1;
#endif /* DCC_FILE */
		client->sock = x;
		client->user = user;
		client->flags = DCC_WAIT|DCC_ASYNC;
		client->lasttime = now;
		client->next = current->clientlist;
		current->clientlist = client;
	}
}

#ifdef CTCP

void ctcp_finger(char *from, char *to, char *rest)
{
	char	*reply;
	int	maxul;

	/*
	 *  checking for a spylist first saves a few clocks
	 */
	if (current->spy & SPYF_STATUS)
	{
		maxul = get_maxaccess(from);
		send_spy(SPYSTR_STATUS,"[CTCP] :%s[%i]: Requested Finger Info",CurrentNick,maxul);
	}

	if (!current->setting[TOG_CTCP].int_var)
		return;

	reply = (current->setting[TOG_RF].int_var) ? randstring(VERSIONFILE) : NULL;
	to_server("NOTICE %s :\001FINGER %s\001\n",CurrentNick,(reply) ? reply : EXFINGER);
}

void ctcp_ping(char *from, char *to, char *rest)
{
	int	maxul;

	if (!rest || !*rest || (strlen(rest) > 100))
		return;

	/*
	 *  this kludge saves a few clock cycles
	 */
	maxul = ASSTLEVEL;
	if (current->spylist || !current->setting[TOG_CTCP].int_var)
	{
		maxul = get_maxaccess(from);
		if (current->spy & SPYF_STATUS)
			send_spy(SPYSTR_STATUS,"[CTCP] :%s[%i]: Requested Ping Info",CurrentNick,maxul);
	}

	if (maxul >= ASSTLEVEL)
	{
		to_server("NOTICE %s :\001PING %s\001\n",CurrentNick,(rest) ? rest : "");
	}
}

void ctcp_version(char *from, char *to, char *rest)
{
	char	*reply;
	int	maxul;

	/*
	 *  checking for a spylist first saves a few clocks
	 */
	if (current->spy & SPYF_STATUS)
	{
		maxul = get_maxaccess(from);
		send_spy(SPYSTR_STATUS,"[CTCP] :%s[%i]: Requested Version Info",CurrentNick,maxul);
	}

	if (!current->setting[TOG_CTCP].int_var)
		return;

	reply = (current->setting[TOG_RV].int_var) ? randstring(VERSIONFILE) : NULL;
	to_server("NOTICE %s :\001VERSION %s\001\n",CurrentNick,(reply) ? reply : EXVERSION);
}

#define NEED_SLOT_NO	,0
#define NEED_SLOT_YES	,1

#else /* CTCP */

#define NEED_SLOT_NO	/* nothing */
#define NEED_SLOT_YES	/* nothing */

#endif /* CTCP */

LS const struct
{
	char    *name;
	void    (*func)(char *, char *, char *);
#ifdef CTCP
	int	need_slot;
#endif /* CTCP */

} ctcp_commands[] =
{
/*
 *  most common types first
 */
{ "ACTION",		on_action	NEED_SLOT_NO	},
#ifdef CTCP
{ "PING",		ctcp_ping	NEED_SLOT_YES	},
{ "VERSION",		ctcp_version	NEED_SLOT_YES	},
{ "FINGER",		ctcp_finger	NEED_SLOT_YES	},
#endif /* CTCP */
{ "DCC",		ctcp_dcc	NEED_SLOT_NO	},
{ NULL, }};

void on_ctcp(char *from, char *to, char *rest)
{
	char    *command,*p;
	int     i,mul;

	/*
	 *  some of these stupid CTCP's have leading spaces, bad!
	 */
	while(*rest == ' ')
		rest++;

	i = FALSE;
	p = command = rest;
	for(;*p;p++)
	{
		if (*p == ' ' && !i)
		{
			i = TRUE;
			*p = 0;
			rest = p + 1;
			while(*rest == ' ')
				rest++;
		}
		if (*p == 1)
		{
			*p = 0;
			break;
		}
	}
	for(i=0;ctcp_commands[i].name;i++)
	{
		if (!stringcasecmp(ctcp_commands[i].name,command))
		{
#ifdef CTCP
			if (ctcp_commands[i].need_slot)
			{
				for(mul=0;mul<CTCP_SLOTS;mul++)
				{
					if (ctcp_slot[mul] < now)
					{
						ctcp_slot[mul] = now + CTCP_TIMEOUT;
						break;
					}
				}
				if (mul>=CTCP_SLOTS)
				{
#ifdef DEBUG
					debug("(on_ctcp) ctcp_slot for %s not found, being flooded?\n",
						nullstr(command));
#endif /* DEBUG */
					return;
				}
			}
#endif /* CTCP */
			ctcp_commands[i].func(from,to,rest);
			return;
		}
	}
	if (current->spy & SPYF_STATUS)
	{
		mul = get_maxaccess(from);
		send_spy(SPYSTR_STATUS,"[CTCP] :%s[%i]: Unknown [%s]",CurrentNick,mul,command);
	}
}

/*
 *
 *  commands associated with CTCP and DCC
 *
 */

#ifdef CTCP

void do_ping_ctcp(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*target;

	if ((target = chop(&rest)))
	{
		if (CurrentCmd->name == C_PING || !stringcasecmp(rest,"PING"))
		{
			to_server("PRIVMSG %s :\001PING %lu\001\n",target,now);
			return;
		}
		if (*rest)
		{
			to_server("PRIVMSG %s :\001%s\001\n",target,rest);
			return;
		}
	}
	usage(from);
}

#endif /* CTCP */

#ifdef DCC_FILE

void do_send(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*filename,*target;
	int	sz;


	target = chop(&rest);
	filename = chop(&rest);
	if (!filename)
	{
		if (CurrentDCC)
		{
			to_user_q(from,"Unable to send files over DCC connections");
			return;
		}
		filename = target;
		target = CurrentNick;
	}
	if (dcc_freeslots(get_maxaccess(from)) < 1)
	{
		to_user_q(from,"Unable to send: No free DCC slots");
		return;
	}
	sz = dcc_sendfile(target,filename);
	if (sz < 0)
	{
		to_user_q(from,"Unable to locate file: %s",filename);
		return;
	}
	if (target == CurrentNick)
	{
		to_user_q(from,"Sending %s (%i bytes)",filename,sz);
	}
	else
	{
		to_user_q(from,"Sending %s (%i bytes) to %s",filename,sz,target);
	}
}

#endif /* DCC_FILE */
