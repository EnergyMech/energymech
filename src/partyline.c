/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2004 proton

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
#define TELNET_C
#include "config.h"

#ifdef TELNET

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"

int check_telnet(int s, char *rest)
{
	Client	*client;
	User	*user;

	user = NULL;
	for(current=botlist;current;current=current->next)
	{
		for(user=current->userlist;user;user=user->next)
		{
			if (!Strcasecmp(user->name,rest))
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

	Strcpy(CurrentNick,client->user->name);
	dcc_banner(client);
	client->flags = DCC_TELNET|DCC_ACTIVE;
}

#endif /* TELNET */
