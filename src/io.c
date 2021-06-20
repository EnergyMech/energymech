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
#ifndef GENCMD_C
#define SOCKET_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

#ifndef RAWDNS

/*
 *  only include this hack if rawdns isnt compiled in
 */
uint32_t get_ip(const char *host)
{
	struct	hostent *he;
	uint32_t ip;

	if ((ip = inet_addr(host)) == INADDR_NONE)
	{
		if ((he = gethostbyname(host)) == NULL)
		{
#ifdef DEBUG
			debug("(get_ip) unable to resolve %s\n",host);
#endif /* DEBUG */
			return(-1);
		}
		ip = (uint32_t)((struct in_addr*)he->h_addr)->s_addr;
	}
#ifdef DEBUG
	debug("(get_ip) %s -> %s\n",host,inet_ntoa(*((struct in_addr*)&ip)));
#endif /* DEBUG */
	return(ip);
}

#endif /* ! RAWDNS */

/*
 *  some default code for socket flags
 */
void SockFlags(int fd)
{
#ifdef ASSUME_SOCKOPTS
	fcntl(fd,F_SETFL,O_NONBLOCK|O_RDWR);
	fcntl(fd,F_SETFD,FD_CLOEXEC);
#else /* not ASSUME_SOCKOPTS */
	fcntl(fd,F_SETFL,O_NONBLOCK | fcntl(fd,F_GETFL));
	fcntl(fd,F_SETFD,FD_CLOEXEC | fcntl(fd,F_GETFD));
#endif /* ASSUME_SOCKOPTS */
}

int SockOpts(void)
{
	struct { int onoff; int linger; } parm;
	int	s;

	if ((s = socket(AF_INET,SOCK_STREAM,0)) < 0)
		return(-1);

	parm.onoff = parm.linger = 0;
	setsockopt(s,SOL_SOCKET,SO_LINGER,(char*)&parm,sizeof(parm));
	parm.onoff++;
	setsockopt(s,SOL_SOCKET,SO_KEEPALIVE,(char*)&parm.onoff,sizeof(int));

	SockFlags(s);

	return(s);
}

int SockListener(int port)
{
	struct	sockaddr_in sai;
	int	s;

	if ((s = SockOpts()) < 0)
		return(-1);
	memset((char*)&sai,0,sizeof(sai));
	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = INADDR_ANY;
	sai.sin_port = htons(port);
	if ((bind(s,(struct sockaddr*)&sai,sizeof(sai)) < 0) || (listen(s,1) < 0))
	{
		close(s);
		return(-1);
	}
	return(s);
}

#ifdef RAWDNS
#define get_ip rawdns_get_ip
#endif /* RAWDNS */

int SockConnect(char *host, int port, int use_vhost)
{
	struct  sockaddr_in sai;
	int	s;
#ifdef IDWRAP
	char	*id,identfile[64];
	int	t = FALSE;
#endif /* IDWRAP */

#ifdef DEBUG
	debug("(SockConnect) %s %i%s\n",nullstr(host),port,(use_vhost) ? " [VHOST]" : "");
#endif /* DEBUG */

	if ((s = SockOpts()) < 0)
		return(-1);

	memset((char*)&sai,0,sizeof(sai));
	sai.sin_family = AF_INET;

	/*
	 *  special case, BOUNCE feature may call SockConnect()
	 *  to create the IDWRAP symlink, using special use_vhost value == 2
	 */
#if defined(BOUNCE) && defined(IDWRAP)
	if ((use_vhost == TRUE)
#else /* not ... */
	if (use_vhost
#endif /* ... */
		&& ((current->vhost_type & VH_IPALIAS_FAIL) == 0)
		&& current->setting[STR_VIRTUAL].str_var)
	{
		current->vhost_type |= VH_IPALIAS_BOTH;
		if ((sai.sin_addr.s_addr = get_ip(current->setting[STR_VIRTUAL].str_var)) != -1)
		{
			if (bind(s,(struct sockaddr *)&sai,sizeof(sai)) >= 0)
			{
				current->vhost_type &= ~(VH_IPALIAS_FAIL|VH_WINGATE);
#ifdef WINGATE
				use_vhost++;
#endif /* WINGATE */
#ifdef IDWRAP
				t = TRUE;
#endif /* IDWRAP */
#ifdef DEBUG
				debug("(SockConnect) IP Alias virtual host bound OK\n");
#endif /* DEBUG */
			}
		}
	}
#ifdef IDWRAP
	/*
	 *  do a blank bind to get a port number
	 */
	if (!t)
	{
		sai.sin_addr.s_addr = INADDR_ANY;
		bind(s,(struct sockaddr *)&sai,sizeof(sai));
	}
#endif /* IDWRAP */

	memset((char*)&sai,0,sizeof(sai));
	sai.sin_family = AF_INET;

#ifdef WINGATE
	/*
	 *  bouncer connects (WinGate, ...)
	 */
	if ((use_vhost == TRUE)	&& ((current->vhost_type & VH_WINGATE_FAIL) == 0)
		&& current->setting[STR_WINGATE].str_var)
	{
#ifdef DEBUG
		debug("(SockConnect) Trying wingate @ %s:%i\n",
			current->setting[STR_WINGATE].str_var,
			current->setting[INT_WINGPORT].int_var);
#endif /* DEBUG */
		current->vhost_type |= VH_WINGATE_BOTH;
		sai.sin_port = htons(current->setting[INT_WINGPORT].int_var);
		if ((sai.sin_addr.s_addr = get_ip(current->setting[STR_WINGATE].str_var)) == -1)
		{
			close(s);
			return(-1);
		}
		current->vhost_type &= ~(VH_WINGATE_FAIL|VH_IPALIAS);
#ifdef DEBUG
		debug("(SockConnect) WINGATE host resolved OK\n");
#endif /* DEBUG */
	}
	else
#endif /* WINGATE */
	{
		/*
		 *  Normal connect, no bounces...
		 */
#ifdef IDWRAP
		if (use_vhost)
		{
			t = sizeof(sai);
			if (getsockname(s,(struct sockaddr*)&sai,&t) == 0)
			{
				if (current->identfile)
					Free((char**)&current->identfile);
				sprintf(identfile,IDWRAP_PATH "%i.%i",ntohs(sai.sin_port),port);
				id = current->setting[STR_IDENT].str_var;
				if (symlink((id) ? id : BOTLOGIN,identfile) == 0)
				{
					set_mallocdoer(SockConnect);
					current->identfile = Strdup(identfile);
#ifdef DEBUG
					debug("(SockConnect) symlink: %s -> %s\n",identfile,(id) ? id : BOTLOGIN);
#endif /* DEBUG */
				}
			}
			memset((char*)&sai,0,sizeof(sai));
			sai.sin_family = AF_INET;
		}
#endif /* IDWRAP */
		sai.sin_port = htons(port);
		if ((sai.sin_addr.s_addr = get_ip(host)) == -1)
		{
			close(s);
			return(-1);
		}
		sai.sin_family = AF_INET;
	}

	if ((connect(s,(struct sockaddr*)&sai,sizeof(sai)) < 0) && (errno != EINPROGRESS))
	{
#ifdef DEBUG
		debug("[CbN] unable to connect. errno = %i\n",errno);
#endif /* DEBUG */
		close(s);
		return(-1);
	}
#ifdef DEBUG
	debug("(SockConnect) {%i} %s %i%s\n",s,nullstr(host),port,(use_vhost) ? " [VHOST]" : "");
#endif /* DEBUG */
	return(s);
}

int SockAccept(int sock)
{
	struct	sockaddr_in sai;
	int	s,sz;

	sz = sizeof(sai);
	s = accept(sock,(struct sockaddr*)&sai,&sz);
	if (s >= 0)
	{
		SockFlags(s);
	}
	return(s);
}

#endif /* GENCMD_C */

/*
 *  Format text and send to a socket or file descriptor
 */
int to_file(int sock, const char *format, ...)
{
	va_list msg;
#if defined(DEBUG) && !defined(GENCMD_C)
	char	*line,*rest;
	int	i;
#endif /* DEBUG */

	if (sock == -1)
		return(-1);

	va_start(msg,format);
	vsprintf(globaldata,format,msg);
	va_end(msg);

#if defined(DEBUG) && !defined(GENCMD_C)
	i = write(sock,globaldata,strlen(globaldata));
	rest = globaldata;
	while((line = get_token(&rest,"\n")))	/* rest cannot be NULL */
		debug("(out) {%i} %s\n",sock,nullstr(line));
	if (i < 0)
		debug("(out) {%i} errno = %i\n",sock,errno);
	return(i);
#else /* DEBUG */
	return(write(sock,globaldata,strlen(globaldata)));
#endif /* DEBUG */
}

#ifndef GENCMD_C

/*
 *  Format a message and send it to the current bots server
 *  to_server needs a newline (\n) it wont manufacture it itself.
 */
void to_server(char *format, ...)
{
	va_list msg;
#ifdef DEBUG
	char	*line,*rest;
#endif /* DEBUG */

	if (current->sock == -1)
		return;

	va_start(msg,format);
	vsprintf(globaldata,format,msg);
	va_end(msg);

	/*
	 *  each line we send to the server pushes the sendq time
	 *  forward *two* seconds
	 */
	current->sendq_time += 2;

	if (write(current->sock,globaldata,strlen(globaldata)) < 0)
	{
#ifdef DEBUG
		debug("[StS] {%i} errno = %i\n",current->sock,errno);
#endif /* DEBUG */
		close(current->sock);
		current->sock = -1;
		current->connect = CN_NOSOCK;
		return;
	}
#ifdef DEBUG
	rest = globaldata;
	while((line = get_token(&rest,"\n")))	/* rest cannot be NULL */
		debug("[StS] {%i} %s\n",current->sock,line);
#endif /* DEBUG */
}

void to_user_q(const char *target, const char *format, ...)
{
	va_list args;
	Client	*client;
	Strp	*new,**pp;
	char	message[MAXLEN];
	char	nick[MAXHOSTLEN];
	char	*fmt;

	va_start(args,format);
	vsprintf(message,format,args);
	va_end(args);

	stringcat(message,"\n");

	if (STARTUP_ECHOTOCONSOLE)
	{
		write(1,message,strlen(message));
		return;
	}

	fmt = "NOTICE %s :%s";
	if (CurrentChan)
	{
		target = CurrentChan->name;
		fmt = "PRIVMSG %s :%s";
	}

	if (!ischannel(target))
		target = nickcpy(nick,target);

	if ((client = find_client(target)))
	{
		if (write(client->sock,message,strlen(message)) < 0)
			client->flags = DCC_DELETE;
#ifdef DEBUG
		debug("(to_user) {%i} [%s] %s",client->sock,target,message);
#endif /* DEBUG */
		return;
	}

#ifdef REDIRECT
	if (redirect.to && target == redirect.to)
	{
		send_redirect(message);
		return;
	}
#endif /* REDIRECT */

	pp = &current->sendq;
	while(*pp)
		pp = &(*pp)->next;

	set_mallocdoer(to_user_q);
	*pp = new = (Strp*)Calloc(sizeof(Strp) + StrlenX(fmt,target,message,NULL));
	/* Calloc sets to zero new->next = NULL; */
	sprintf(new->p,fmt,target,message);
}

/*
 *  Format a message and send it either through DCC if the user is
 *  connected to the partyline, or send it as a NOTICE if he's not
 */
void to_user(const char *target, const char *format, ...)
{
	va_list args;
	Client	*client;
	char	message[MAXLEN];
	char	*s;

#ifdef DEBUG
	if (!*target)
		return;
#endif /* DEBUG */

	s = message;

#ifdef REDIRECT

	if (redirect.to)
		client = NULL;
	else
		client = find_client(target);

	if (!redirect.to && !client)

#else /* REDIRECT */

	if ((client = find_client(target)) == NULL)

#endif /* REDIRECT */

	{
		if (current->sock == -1)
		{
#ifdef DEBUG
			debug("(to_user) [%s] current->sock == -1\n",target);
#endif /* DEBUG */
			return;
		}
		s = stringcpy(message,"NOTICE ");
		if (ischannel(target))
			stringcat(message,target);
		else
			nickcpy(s,target);
		s = stringcat(message," :");
	}

	va_start(args,format);
	vsprintf(s,format,args);
	va_end(args);

#ifdef REDIRECT
	if (redirect.to)
	{
		send_redirect(message);
		return;
	}
#endif /* REDIRECT */

	/*
	 *  tag on a newline for DCC or server
	 */
	s = stringcat(message,"\n");

#ifdef DEBUG
	*s = 0;
#endif /* DEBUG */

	if (client)
	{
		if (write(client->sock,message,(s - message)) < 0)
			client->flags = DCC_DELETE;
#ifdef DEBUG
		debug("(to_user) {%i} [%s] %s",client->sock,target,message);
#endif /* DEBUG */
		return;
	}

	current->sendq_time += 2;

	if (write(current->sock,message,(s - message)) < 0)
	{
#ifdef DEBUG
		debug("(to_user) {%i} [%s] errno = %i\n",current->sock,target,errno);
#endif /* DEBUG */
		close(current->sock);
		current->sock = -1;
		current->connect = CN_NOSOCK;
		return;
	}
#ifdef DEBUG
	debug("(to_user) {%i} [%s] %s",current->sock,target,message);
#endif /* DEBUG */
}

#endif /* ifndef GENCMD_C */

/*
 *  Read any data waiting on a socket or file descriptor
 *  and return any complete lines to the caller
 *
 *  1: Try to get a whole line from <rest> data
 *  2: If <rest> data is insufficient, try to read in more
 *  3: Try again to make a whole line
 */
char *sockread(int s, char *rest, char *line)
{
	char	*src,*dst,*rdst;
	int	n;

	errno = EAGAIN;

	src = rest;
	dst = line;

	while(*src)
	{
		if (*src == '\n' || *src == '\r')
		{
		gotline:
			while(*src == '\n' || *src == '\r')
				src++;
			*dst = 0;
			dst = rest;
			while(*src)
				*(dst++) = *(src++);
			*dst = 0;
#if defined(DEBUG) && !defined(GENCMD_C)
			debug("(in)  {%i} %s\n",s,line);
#endif /* DEBUG */
			return((*line) ? line : NULL);
		}
		*(dst++) = *(src++);
	}
	rdst = src;

	n = read(s,globaldata,MSGLEN-2);
	switch(n)
	{
	case 0:
		errno = EPIPE;
	case -1:
		return(NULL);
	}

	globaldata[n] = 0;
	src = globaldata;

	while(*src)
	{
		if (*src == '\r' || *src == '\n')
			goto gotline;
		if ((dst - line) >= (MSGLEN-2))
		{
			/*
			 *  line is longer than buffer, let the wheel spin
			 */
			src++;
			continue;
		}
		*(rdst++) = *(dst++) = *(src++);
	}
	*rdst = 0;
	return(NULL);
}

void readline(int fd, int (*callback)(char *))
{
	char	linebuf[MSGLEN],readbuf[MSGLEN];
	char	*ptr;
	int	oc;

	*readbuf = 0;

	do
	{
		ptr = sockread(fd,readbuf,linebuf);
		oc = errno;
		if (ptr && *ptr)
		{
			if (callback(ptr) == TRUE)
				oc = EPIPE;
		}
	}
	while(oc == EAGAIN);

	close(fd);

#if defined(DEBUG) && !defined(GENCMD_C)
	debug("(readline) done reading lines\n");
#endif /* DEBUG */
}

#ifndef GENCMD_C

void remove_ks(KillSock *ks)
{
	KillSock *ksp;

	close(ks->sock);
	if (ks == killsocks)
	{
		killsocks = ks->next;
	}
	else
	{
		ksp = killsocks;
		while(ksp && (ksp->next != ks))
			ksp = ksp->next;
		if (ksp)
		{
			ksp->next = ks->next;
		}
	}
#ifdef DEBUG
	debug("(killsock) {%i} removing killsocket\n",ks->sock);
#endif /* DEBUG */
	Free((char**)&ks);
}

int killsock(int sock)
{
	KillSock *ks,*ksnext;
	struct	timeval t;
	fd_set	rd,wd;
	char	bitbucket[MSGLEN];
	int	hi,n;

	if (sock >= 0)
	{
		set_mallocdoer(killsock);
		ks = (KillSock*)Calloc(sizeof(KillSock));
		ks->time = now;
		ks->sock = sock;
		ks->next = killsocks;
		killsocks = ks;
#ifdef DEBUG
		debug("(killsock) {%i} added killsocket\n",ks->sock);
#endif /* DEBUG */
		return(TRUE);
	}

	if (killsocks == NULL)
		return(FALSE);

	if (sock == -1)
		t.tv_sec = 0;
	else
		t.tv_sec = 1;
	t.tv_usec = 0;

	FD_ZERO(&rd);
	FD_ZERO(&wd);

	hi = -1;
	for(ks=killsocks;ks;ks=ks->next)
	{
		if (ks->sock > hi)
			hi = ks->sock;
		FD_SET(ks->sock,&rd);
		FD_SET(ks->sock,&wd);
	}

	if (select(hi+1,&rd,&wd,NULL,&t) == -1)
	{
		switch(errno)
		{
		case EINTR:
			/* have to redo the select */
		case ENOMEM:
			/* should never happen, but still */
			return(TRUE);
		}
	}

	for(ks=killsocks;ks;)
	{
		ksnext = ks->next;
		if (FD_ISSET(ks->sock,&rd))
		{
			n = read(ks->sock,&bitbucket,MSGLEN);
			if ((n == 0) || ((n == -1) && (errno != EAGAIN)))
				remove_ks(ks);
		}
		if ((now - ks->time) > KILLSOCKTIMEOUT)
			remove_ks(ks);
		ks = ksnext;
	}
	return(TRUE);
}

void do_clearqueue(COMMAND_ARGS)
{
#ifdef DEBUG
	debug("(do_clearqueue) purging sendq...\n");
#endif
	purge_linklist((void**)&current->sendq);
}

#endif /* GENCMD_C */
