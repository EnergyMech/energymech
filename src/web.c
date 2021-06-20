/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2021 proton

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
#define WEB_C
#include "config.h"

#ifdef WEB

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"

#define WEB_DEAD	0
#define WEB_WAITURL	1
#define WEB_WAITCR	2

#define WEBROOT		"web/"

LS WebSock *weblist;

LS WebDoc docraw = { NULL, NULL, &web_raw };

#if 0
{
	":last20lines>"
};
#endif

LS WebDoc doclist[] =
{
{ 	NULL,	"/internalstatus.html",	&web_botstatus		},
#ifdef DEBUG
{	NULL,	"/internaldebug.html",	&web_debug		},
#endif /* DEBUG */
/**/
{	NULL,	NULL,			NULL			},
};

char *webread(int s, char *rest, char *line)
{
	char	tmp[MSGLEN];
	char	*pt,*tp,*np;
	int	l;

	np = NULL;
	pt = rest;
	while(*pt)
	{
		if ((*pt == '\r') && (!np))
			np = pt;
		if (*pt == '\n')
		{
			if (np)
				*np = 0;
			*pt = 0;
			stringcpy(line,rest);
			pt++;
			stringcpy(rest,pt);
#ifdef DEBUG
			debug("[WoR] {%i} `%s'\n",s,line);
#endif /* DEBUG */
			errno = EAGAIN;
			return(line);
		}
		pt++;
	}
	l = pt - rest;
	tp = pt;
	memset(tmp,0,sizeof(tmp));
	switch(read(s,tmp,MSGLEN-2))
	{
	case 0:
		errno = EPIPE;
	case -1:
		return(NULL);
	}
	np = NULL;
	pt = tmp;
	while(*pt)
	{
		if ((*pt == '\r') && (!np))
			np = tp;
		if (*pt == '\n')
		{
			if (np)
				*np = 0;
			*tp = *pt = 0;
			stringcpy(line,rest);
			pt++;
			stringcpy(rest,pt);
#ifdef DEBUG
			debug("[WoR] {%i} `%s'\n",s,line);
#endif /* DEBUG */
			errno = EAGAIN;
			return(line);
		}
		if (l >= MSGLEN-2)
		{
			pt++;
			continue;
		}
		*(tp++) = *(pt++);
		l++;
	}
	*tp = 0;
	return(NULL);
}

#define NOBO	if (dest == &mem[MSGLEN-2]) { write(s,mem,dest-mem); dest = mem; }

void eml_fmt(WebSock *client, char *format)
{
	char	mem[MSGLEN];
	char	*src,*dest,*org;
	int	out;
	int	s = client->sock;

	org = NULL;
	out = TRUE;
	dest = mem;
restart:
	while(*format)
	{
		if (*format != '%')
		{
			if (out)
			{
				NOBO;
				*(dest++) = *format;
			}
			format++;
			goto restart;
		}
		format++;
		switch(*format)
		{
		case '%':
			NOBO;
			*(dest++) = *(format++);
			goto restart;
		case 'B':
			client->ebot = botlist;
			break;
		case 'C':
			client->echan = (client->ebot) ? client->ebot->chanlist : NULL;
			break;
		case 'b':
			src = (client->ebot) ? client->ebot->nick : (char*)NULLSTR;
			while(*src)
			{
				NOBO;
				*(dest++) = *(src++);
			}
			break;
		case 'O':
#ifdef DEBUG
			debug("> setting org\n");
#endif /* DEBUG */
			org = format+1;
			break;
		case 'R':
			if (out && org)
			{
#ifdef DEBUG
				debug("> repeat jump\n");
#endif /* DEBUG */
				format = org;
				goto restart;
			}
			break;
		case '?':
			format++;
			switch(*format)
			{
			case 'b':
				out = (client->ebot) ? 1 : 0;
#ifdef DEBUG
				debug("> ?b: out = %i\n",out);
#endif /* DEBUG */
				break;
			case 'c':
				out = (client->echan) ? 1 : 0;
#ifdef DEBUG
				debug("> ?c: out = %i\n",out);
#endif /* DEBUG */
				break;
			case '1':
				out = 1;
				break;
			}
			break;
		case '+':
			format++;
			switch(*format)
			{
			case 'b':
				client->ebot = (client->ebot) ? client->ebot->next : NULL;
#ifdef DEBUG
				debug("> +b: ebot = %p\n",client->ebot);
#endif /* DEBUG */
				break;
			case 'c':
				client->echan = (client->echan) ? client->echan->next : NULL;
#ifdef DEBUG
				debug("> +c: echan = %p\n",client->echan);
#endif /* DEBUG */
				break;
			}
			break;
		}
		format++;
	}
	if (dest-mem > 0)
		write(s,mem,dest-mem);
	write(s,"\r\n",2);
}

void web_raw(WebSock *client, char *url)
{
	struct	stat s;
	char	path[MSGLEN];
	char	*src,*dest;
	ino_t	ino;
	int	fd;
	int	eml;
	size_t	sz;

	eml = (matches("*.html",url)) ? TRUE : FALSE;

	if (stat(WEBROOT "..",&s) < 0)
		goto error;
	ino = s.st_ino;

	dest = stringcpy(path,WEBROOT);
	src = url;
	if (*src == '/')
		src++;
	while(*src)
	{
		if (*src == '/')
		{
			*dest = 0;
			if (stat(path,&s) < 0)
			{
#ifdef DEBUG
				debug("(web_raw) unable to stat `%s'\n",path);
#endif /* DEBUG */
				goto error;
			}
			if (s.st_ino == ino)
			{
#ifdef DEBUG
				debug("(web_raw) attempt to access docs outside WEBROOT (%s)\n",url);
#endif /* DEBUG */
				goto error;
			}
		}
		if (dest == &path[MSGLEN-2])
			goto error;
		*(dest++) = *(src++);
	}
	*dest = 0;
	if ((fd = open(path,O_RDONLY)) < 0)
	{
#ifdef DEBUG
		debug("(web_raw) open failed = `%s'\n",path);
#endif /* DEBUG */
		goto error;
	}
	sz = lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
#ifdef DEBUG
	debug("(web_raw) file open for reading `%s'\n",path);
#endif /* DEBUG */
	set_mallocdoer(web_raw);
	src = Calloc(sz+1);
	read(fd,src,sz);
	if (eml)
	{
		to_file(client->sock,"%s\r\n",src);
	}
	else
	{
		eml_fmt(client,src);
	}
	Free((char**)&src);
	close(fd);
	return;
error:
	web_404(client,url);
}

void web_botstatus(WebSock *client, char *url)
{
	Server	*sp;
	Mech	*bot;
	Chan	*chan;

	to_file(client->sock,"HTTP/1.0 200 OK\r\n");
	to_file(client->sock,"Server: %s %s\r\n",BOTCLASS,VERSION);
	to_file(client->sock,"Connection: close\r\n");
	to_file(client->sock,"Content-Type: text/html\r\n\r\n");

	bot = botlist;

	to_file(client->sock,
		"<html><head><title>EnergyMech Status</title></head>\r\n"
		"<body>\r\n"
		"<h1>EnergyMech Status</h1>\r\n"
		"Uptime: %s<br><br>\r\n"
		"<u>Current bots</u>:<br>\r\n",
		idle2str(now - uptime,FALSE));

	for(;bot;bot=bot->next)
	{
		sp = find_server(bot->server);
		to_file(client->sock,
			"Nick: %s (want %s)<br>\r\n"
			"On server: %s<br>\r\n",
			bot->nick,bot->wantnick,
			(sp) ? ((sp->realname[0]) ? sp->realname : sp->name) : TEXT_NOTINSERVLIST);
		to_file(client->sock,"Channels: ");
		for(chan=bot->chanlist;chan;chan=chan->next)
			to_file(client->sock,"%s ",chan->name);
		to_file(client->sock,"<br><br>\r\n");
	}
	to_file(client->sock,"</body></html>\r\n");
}

#ifdef DEBUG

void web_debug(WebSock *client, char *url)
{
	int	backup_fd,backup_dodebug;

	backup_dodebug = dodebug;
	backup_fd = debug_fd;

	to_file(client->sock,"HTTP/1.0 200 OK\r\n");
	to_file(client->sock,"Server: %s %s\r\n",BOTCLASS,VERSION);
	to_file(client->sock,"Connection: close\r\n");
	to_file(client->sock,"Content-Type: text/html\r\n\r\n");

	to_file(client->sock,"<pre>\r\n");

	debug_fd = client->sock;
	dodebug = TRUE;

	run_debug();

	debug_fd = backup_fd;
	dodebug = backup_dodebug;
}

#endif /* DEBUG */

void web_404(WebSock *client, char *url)
{
	to_file(client->sock,"HTTP/1.0 404 Not Found\r\n");
	to_file(client->sock,"Server: %s %s\r\n",BOTCLASS,VERSION);
	to_file(client->sock,"Connection: close\r\n");
	to_file(client->sock,"Content-Type: text/html\r\n\r\n");

	to_file(client->sock,
		"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
		"<HTML><HEAD>"
		"<TITLE>404 Not Found</TITLE>"
		"</HEAD><BODY>"
		"<H1>Not Found</H1>"
		"The requested URL %s was not found on this server.<P>"
		"</BODY></HTML>",
		url);
}

void parse_web(WebSock *client, char *rest)
{
	char	*method,*url,*proto;
	int	i;

	switch(client->status)
	{
	case WEB_WAITURL:
		method = chop(&rest);
		url = chop(&rest);
		proto = chop(&rest);
		if (!method || !proto || stringcasecmp(method,"GET"))
		{
			client->status = WEB_DEAD;
			return;
		}
		client->docptr = &docraw;
		for(i=0;doclist[i].proc;i++)
		{
			if (!stringcasecmp(doclist[i].url,url))
			{
				client->docptr = &doclist[i];
				break;
			}
		}
		set_mallocdoer(parse_web);
		client->url = stringdup(url);
		client->status = WEB_WAITCR;
		break;
	case WEB_WAITCR:
		if (*rest == 0)
		{
#ifdef DEBUG
			debug("(parse_web) web document output\n");
#endif /* DEBUG */
			if (client->docptr)
				client->docptr->proc(client,client->url);
			client->status = WEB_DEAD;
		}
		break;
	}
}

void select_web(void)
{
	WebSock	*client;

	if ((webport > 0) && (websock == -1))
	{
		websock = SockListener(webport);
#ifdef DEBUG
		if (websock != -1)
			debug("(select_web) websock is active (%i)\n",webport);
#endif /* DEBUG */
	}

	if (websock != -1)
	{
		chkhigh(websock);
		FD_SET(websock,&read_fds);
	}

	for(client=weblist;client;client=client->next)
	{
		chkhigh(client->sock);
		FD_SET(client->sock,&read_fds);
	}
}

void process_web(void)
{
	WebSock	*client,*new,**cptr;
	char	linebuf[MSGLEN];
	char	*rest;
	int	s;

	if ((websock != -1) && (FD_ISSET(websock,&read_fds)))
	{
		if ((s = SockAccept(websock)) >= 0)
		{
			new         = (WebSock*)Calloc(sizeof(WebSock));
			new->sock   = s;
			new->status = WEB_WAITURL;
			new->when   = now;
			new->next   = weblist;
			weblist     = new;
		}
	}

	for(client=weblist;client;client=client->next)
	{
		if (FD_ISSET(client->sock,&read_fds))
		{
read_again:
			errno = EAGAIN;
			rest = webread(client->sock,client->sockdata,linebuf);
			if (rest)
			{
				parse_web(client,rest);
				goto read_again;
			}
			switch(errno)
			{
			case EINTR:
			case EAGAIN:
				break;
			default:
#ifdef DEBUG
				debug("(web_process) client error = %i\n",errno);
#endif /* DEBUG */
				client->status = WEB_DEAD;
			}
		}
	}
	cptr = &weblist;
	while(*cptr)
	{
		client = *cptr;
		if (client->status == WEB_DEAD)
		{
#ifdef DEBUG
			debug("(web_process) {%i} dropping client\n",client->sock);
#endif /* DEBUG */
			close(client->sock);
			*cptr = client->next;
			if (client->url)
				Free((char**)&client->url);
			Free((char**)&client);
		}
		else
			cptr = &client->next;
	}
}

#endif /* WEB */
