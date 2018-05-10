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
#define UPTIME_C
#include "config.h"

#ifdef UPTIME
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *  Uptime stuffies
 */

typedef struct
{
	int	regnr;
	int	pid;
	int	type;
	uint32_t cookie;
	uint32_t uptime;
	uint32_t ontime;
	uint32_t now;
	uint32_t sysup;

} PackStub;

typedef struct
{
	int	regnr;
	int	pid;
	int	type;
	uint32_t cookie;
	uint32_t uptime;
	uint32_t ontime;
	uint32_t now;
	uint32_t sysup;
	char	string[512];

} PackUp;

void init_uptime(void)
{
	struct	sockaddr_in sai;

	uptimecookie = rand();

	if (!uptimehost)
	{
		set_mallocdoer(init_uptime);
		uptimehost = stringdup(defaultuptimehost);
	}

	if ((uptimesock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
	{
		uptimesock = -1;
		return;
	}

	memset(&sai,0,sizeof(sai));

	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = INADDR_ANY;

	if (bind(uptimesock,(struct sockaddr*)&sai,sizeof(sai)) < 0)
	{
		close(uptimesock);
		uptimesock = -1;
		return;
	}
	SockFlags(uptimesock);
}

void send_uptime(int type)
{
	PackUp	upPack;
	struct	sockaddr_in sai;
	struct	stat st;
	Server	*sp;
	const char *server,*nick;
	int	sz;

	if (uptimehost == NULL || uptimeport == 0)
		return;

#ifdef RAWDNS
	if (uptimeip == -1 && uptimehost)
	{
		char	*host;

		uptimelast = now + 10;
		if ((host = poll_rawdns(uptimehost)))
		{
			if ((uptimeip = inet_addr(host)) != -1)
			{
				Free((char**)&uptimehost);
				set_mallocdoer(send_uptime);
				uptimehost = stringdup(host);
			}
		}
		else
		{
			if (type < UPTIME_GENERICDEATH)
				rawdns(uptimehost);
			return;
		}
	}
#endif /* RAWDNS */

	/*
	 *  update the time when we last sent packet
	 */
	sz = (uptimelast + 1) & 7;
	uptimelast = (now & ~7) + 21600 + sz;		/* 21600 seconds = 6 hours */

	uptimecookie  = (uptimecookie + 1) * 18457;
	upPack.cookie = htonl(uptimecookie);

	upPack.now    = htonl(now);
	upPack.regnr  = uptimeregnr;
	upPack.type   = htonl(type);
	upPack.uptime = htonl(uptime);
	upPack.ontime = 0;			/* set a few lines down */

	/*
	 *  callouts to other functions should be done last (think compiler optimizations)
	 */
	upPack.pid = htonl(getpid());

	/*
	 *  this trick for most systems gets the system uptime
	 */
	if (stat("/proc",&st) < 0)
	{
		upPack.sysup = 0;
	}
	else
	{
		upPack.sysup = htonl(st.st_ctime);
	}

	server = UNKNOWN;
	nick   = BOTLOGIN;

	/*
	 *  set bot related stuff from the first bot in the list
	 */
	if (botlist)
	{
		nick = botlist->nick;
		upPack.ontime = htonl(botlist->ontime);
		if ((sp = find_server(botlist->server)))
		{
			server = (*sp->realname) ? sp->realname : sp->name;
		}
	}

	if (uptimenick)
	{
		nick = uptimenick;
	}

#ifndef RAWDNS
	if ((uptimeip == -1) || ((uptimelast & 7) == 0))
	{
		uptimeip = get_ip(uptimehost);
		if (uptimeip == -1)
			return;
	}
#endif /* ! RAWDNS */

	sz = sizeof(PackStub) + 3 + StrlenX(nick,server,VERSION,NULL);
	if (sz > sizeof(PackUp))
		return;

	sprintf(upPack.string,"%s %s %s",nick,server,VERSION);

	/*
	 *  udp sending...
	 */
	memset((char*)&sai,0,sizeof(sai));
	sai.sin_family = AF_INET;
	sai.sin_addr.s_addr = uptimeip;
	sai.sin_port = htons(uptimeport);

	sendto(uptimesock,(void*)&upPack,sz,0,(struct sockaddr*)&sai,sizeof(sai));
}

void uptime_death(int type)
{
#ifdef DEBUG
	debug("(uptime_death) sending death message\n");
#endif /* DEBUG */
	time(&now);
	uptimelast = 0;		/* avoid resolving the hostname */
	send_uptime(type);
	uptimeport = 0;		/* avoid sending more packets */
}

void process_uptime(void)
{
	struct	sockaddr_in sai;
	int	res,sz;
	struct
	{
		int	regnr;
		uint32_t cookie;

	} regPack;

	if (uptimesock == -1)
		return;

	if (FD_ISSET(uptimesock,&read_fds))
	{
		sz = sizeof(sai);
		res = recvfrom(uptimesock,(void*)&regPack,sizeof(regPack),0,(struct sockaddr*)&sai,&sz);
		if (res == sizeof(regPack))
		{
			if (uptimecookie == ntohl(regPack.cookie))
			{
				if (uptimeregnr == 0)
					uptimeregnr = ntohl(regPack.regnr);
			}
		}
	}

	if (uptimelast < now)
	{
		send_uptime(UPTIME_BOTTYPE);
	}
}

/*
 *
 *  commands related to uptimes
 *
 */

void do_upsend(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	send_uptime(UPTIME_BOTTYPE);
}

#endif /* UPTIME */
