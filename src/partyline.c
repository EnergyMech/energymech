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
#define PARTYLINE_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"

#ifdef TELNET

int check_telnet(int s, char *rest)
{
	Client	*client;
	User	*user;

	user = NULL;
	for(current=botlist;current;current=current->next)
	{
		for(user=current->userlist;user;user=user->next)
		{
			if (!stringcasecmp(user->name,rest))
				goto check_telnet_malloc;
		}
	}
	if (!current)
		current = botlist;
check_telnet_malloc:
	to_file(s,TEXT_ENTERPASSWORD "\n");
	set_mallocdoer(check_telnet);
	client = (Client*)Calloc(sizeof(Client));
	client->user = user;
	client->sock = s;
#ifdef DCC_FILE
	client->fileno = -1;
#endif /* DCC_FILE */
	client->flags = DCC_TELNETPASS;
	client->lasttime = now;
	client->next = current->clientlist;
	current->clientlist = client;
#ifdef DEBUG
	current = NULL;
	debug("(check_telnet) catching telnet client\n");
#endif /* DEBUG */
	return(TRUE);
}

void check_telnet_pass(Client *client, char *rest)
{
	char	*pass;

	pass = chop(&rest);
	if (!pass || !client->user || !client->user->pass || !passmatch(pass,client->user->pass))
	{
		client->flags = DCC_DELETE;
		return;
	}

	stringcpy(CurrentNick,client->user->name);
	partyline_banner(client);
	client->flags = DCC_TELNET|DCC_ACTIVE;
}

#endif /* TELNET */

int partyline_only_command(const char *from)
{
#ifdef REDIRECT
	if (!redirect.to)
#endif /* REDIRECT */
	if (!CurrentDCC)
	{
		to_user(from,TEXT_DCC_ONLY,CurrentCmd->name);
		return(TRUE);
	}
	return(FALSE);
}

void partyline_broadcast(const Client *from, const char *format, const char *rest)
{
	Client	*client;
	Mech	*bot;

	for(bot=botlist;bot;bot=bot->next)
	{
		for(client=bot->clientlist;client;client=client->next)
		{
			if (0 == (client->flags & (DCC_ACTIVE|DCC_TELNET)))
				continue;
			if (client == from && client->user->x.x.echo == FALSE)
				continue;
			to_file(client->sock,format,CurrentNick,rest);
		}
	}
}

void partyline_banner(Client *client)
{
	char	tmp[MSGLEN];

	client->flags = DCC_ACTIVE;
	client->lasttime = now;

	sprintf(tmp,"[%s] %s[%i] has connected",
		current->nick,client->user->name,(int)client->user->x.x.access);

	if ((to_file(client->sock,"[%s] %s\n",time2medium(now),tmp)) < 0)
	{
		client->flags = DCC_DELETE;
		return;
	}
	send_global(SPYSTR_STATUS,tmp);
	if (client->user->x.x.access == OWNERLEVEL)
	{
		CurrentDCC = client;
		stringcpy(tmp,SPYSTR_STATUS);
		do_spy(client->user->name,current->nick,tmp,0);
		CurrentDCC = NULL;
	}
}

void dcc_chat(char *from)
{
	struct	sockaddr_in sai;
	Client	*client;
	User	*user;
	int	sock,sz;

	if ((user = get_authuser(from,NULL)) == NULL)
		return;
	if (find_client(user->name))
		return;

	if ((sock = SockListener(0)) < 0)
		return;

	sz = sizeof(sai);
	if (getsockname(sock,(struct sockaddr *)&sai,&sz) < 0)
	{
		close(sock);
		return;
	}

	set_mallocdoer(dcc_chat);
	client = (Client*)Calloc(sizeof(Client));
#ifdef DCC_FILE
	client->fileno = -1;
#endif /* DCC_FILE */
	client->user = user;
	client->sock = sock;
	client->flags = DCC_WAIT;
	client->lasttime = now;

	client->next = current->clientlist;
	current->clientlist = client;

	to_server("PRIVMSG %s :\001DCC CHAT CHAT %u %u\001\n",
		CurrentNick,htonl(current->ip.s_addr),ntohs(sai.sin_port));
}

#ifdef BOTNET

void whom_printbot(char *from, BotInfo *binfo, char *stt)
{
	char    *us;
	int     uaccess;

	us = "";
	if (binfo->nuh)
	{
		uaccess = get_maxaccess(binfo->nuh);
		if (uaccess == BOTLEVEL)
			us = "b200";
		else
		if (uaccess)
			sprintf((us = stt),"u%i",uaccess);
	}
	uaccess = get_authaccess(from,MATCH_ALL);
	table_buffer((uaccess >= ASSTLEVEL) ? TEXT_WHOMBOTGUID : TEXT_WHOMBOTLINE,(binfo->nuh) ? nickcpy(NULL,binfo->nuh) : "???",us,
		(binfo->server) ? binfo->server : "???",(binfo->version) ? binfo->version : "???",binfo->guid);
}

#endif /* BOTNET */

/*
 *
 *
 *
 */
void do_whom(COMMAND_ARGS)
{
#ifdef BOTNET
	BotNet	*bn;
	BotInfo	*binfo;
#endif /* BOTNET */
	Server	*sp;
	char	stt[NUHLEN];
	Client	*client;
	Mech	*bot;
	int	m,s;

	if (partyline_only_command(from))
		return;

	for(bot=botlist;bot;bot=bot->next)
	{
		if (bot->connect == CN_ONLINE)
		{
			sp = find_server(bot->server);
			if (sp)
			{
				sprintf(stt,"%s:%i",(*sp->realname) ? sp->realname : sp->name,sp->port);
			}
			else
			{
				stringcpy(stt,TEXT_NOTINSERVLIST);
			}
		}
		else
		{
			stringcpy(stt,TEXT_NOTCONNECTED);
		}
		table_buffer(TEXT_WHOMSELFLINE,bot->nick,(bot == current) ? "(me)" : "b200",stt);
		for(client=bot->clientlist;client;client=client->next)
		{
			m = (now - client->lasttime) / 60;
			s = (now - client->lasttime) % 60;
			table_buffer(TEXT_WHOMUSERLINE,
#ifdef TELNET
				client->user->name,client->user->x.x.access,(client->flags & DCC_TELNET) ? "telnet" : "DCC",m,s);
#else
				client->user->name,client->user->x.x.access,"DCC",m,s);
#endif /* TELNET */
		}
	}
#ifdef BOTNET
	for(bn=botnetlist;bn;bn=bn->next)
	{
		if (bn->status != BN_LINKED)
			continue;
		for(binfo=bn->botinfo;binfo;binfo=binfo->next)
			whom_printbot(from,binfo,stt);
	}
#endif /* BOTNET */
	table_send(from,3);
}

/*
help:CHAT:(no arguments)
begin
end
*/
void do_chat(COMMAND_ARGS)
{
	User    *user;

	user = get_authuser(from,NULL);
	if (!user)
		return;
	if (find_client(user->name))
	{
		to_user(from,"You are already DCC chatting me");
		return;
	}
	dcc_chat(from);
}

/*
help:BYE:(no arguments)
begin
end
*/
void do_bye(COMMAND_ARGS)
{
	if (CurrentDCC)
	{
		to_user(from,TEXT_DCC_GOODBYE);
		CurrentDCC->flags = DCC_DELETE;
	}
}

/*
help:BOOT:<nick>
begin
Forcefully disconnect someone from the partyline.
end
*/
void do_boot(COMMAND_ARGS)
{
}
