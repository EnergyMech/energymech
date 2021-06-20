/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2021 proton

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
#define PARSE_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"

void parse_error(char *from, char *rest)
{
	Server	*sp;

	if (rest && (sp = find_server(current->server)))
	{
		if (!matches("*no authorization*",rest))
			sp->err = SP_NOAUTH;
		else
		if (!matches("*k*lined*",rest))
			sp->err = SP_KLINED;
		else
		if (!matches("*throttled*",rest))
			sp->err = SP_THROTTLED;
		else
		if (!matches("*connection*class*",rest))
			sp->err = SP_FULLCLASS;
		else
		if (!matches("*different port*",rest))
			sp->err = SP_DIFFPORT;
		else
			sp->err = SP_NULL;
	}
	close(current->sock);
	current->sock = -1;
}

void parse_invite(char *from, char *rest)
{
	char	*chan;
	int	i;

	if ((chan = chop(&rest)) == NULL)
		return;
	if (*chan == ':')
		chan++;
	i = get_authaccess(from,chan);
	if ((i >= JOINLEVEL) && (i < BOTLEVEL))
	{
		join_channel(chan,NULL);
		current->lastrejoin = now;
	}
}

void parse_join(char *from, char *rest)
{
	Chan	*chan;
	char	*src,*dst;

	/*
	 *  with the new reset feature its theoretically possible that the bot
	 *  can see a join before it knows it is on the channel
	 *  otherwise this would only happen if there was a bug somewhere
	 */
	if ((CurrentChan = chan = find_channel_ny(rest)) == NULL)
		return;

	if (!nickcmp(current->nick,from))
	{
#ifdef DEBUG
		debug("(parse_join) Im joining %s\n",chan->name);
#endif /* DEBUG */
		/*
		 *  reset various flags
		 */
		chan->active	= TRUE;
		chan->private	= FALSE;
		chan->secret	= FALSE;
		chan->moderated	= FALSE;
		chan->topprot	= FALSE;
		chan->limitmode	= FALSE;
		chan->invite	= FALSE;
		chan->nomsg	= FALSE;
		chan->keymode	= FALSE;
		chan->bot_is_op	= FALSE;
		chan->sync	= FALSE;
		chan->wholist	= FALSE;
		chan->rejoin	= FALSE;

		current->activechan = chan;
#ifdef CHANBAN
		current->lastchanban = 0;
#endif /* CHANBAN */
		to_server("WHO %s\nMODE %s\nMODE %s b\n",rest,rest,rest);
		purge_linklist((void**)&chan->banlist);
		purge_chanusers(chan);

#ifdef STATS
		if (chan->setting[STR_STATS].str_var && chan->stats)
		{
			ChanStats *stats;

			stats = chan->stats;
			stats->userseconds = 0;
			stats->users = 0;
			stats->lastuser = now;
			stats->flags |= CSTAT_PARTIAL;
		}
#endif /* STATS */
	}
	if (chan->active)
	{
		chan->this10++;
		chan->this60++;
		/*
		 *  instead of nickcpy(), do it here, so that src can be used for the make_chanuser() call.
		 *  avoids spinning through the "from" buffer twice to find the '!'.
		 */
		src = from;
		dst = CurrentNick;
		while(*src)
		{
			if (*src == '!')
				break;
			*(dst++) = *(src++);
		}
		*dst = 0;
		src++;

		/*
		 *  find the User and Shit record for this user
		 */
		if (is_bot(from))
		{
			CurrentUser = (User*)&LocalBot;
			CurrentShit = NULL;
		}
		else
		{
			CurrentUser = get_user(from,rest);
			CurrentShit = find_shit(from,rest);
		}

		/*
		 *  make a ChanUser for the channel->users list
		 */
		if (chan->wholist)
		{
#ifdef STATS
			if (chan->setting[STR_STATS].str_var)
				stats_plusminususer(chan,1);
			if (chan->stats)
				chan->stats->joins++;
#endif /* STATS */
			make_chanuser(CurrentNick,src);
			chan->users->user = CurrentUser;
			chan->users->shit = CurrentShit;
#ifdef BOTNET
			if (CurrentUser && CurrentUser->x.x.access == BOTLEVEL)
				check_botjoin(chan,chan->users);
#endif /* BOTNET */
		}
		on_join(chan,from);
	}
}

/*
 *  :<botnick> MODE <botnick> :<modes>
 *  :<server> 221 <botnick> <modes>
 */
void parse_mode(char *from, char *rest)
{
	char	*to;

	if ((to = chop(&rest)) == NULL)
		return;

	if (ischannel(to))
	{
		CurrentUser = get_user(from,to);
		CurrentShit = find_shit(from,to);
		nickcpy(CurrentNick,from);
		on_mode(from,to,rest);
	}
	else
	if (!stringcasecmp(current->nick,to))
	{
		char	*dst;
		char	sign;

		sign = 0;
		while(*rest)
		{
			if (*rest == '+' || *rest == '-')
				sign = *rest;
			else
			{
				for(dst=current->modes;*dst;dst++)
				{
					if (*dst == *rest)
						break;
				}
				if (*dst && sign == '-')
				{
					while(dst[1])
					{
						*dst = dst[1];
						dst++;
					}
					*dst = 0;
				}
				else
				if (*dst == 0 && sign == '+')
				{
					*(dst++) = *rest;
					*dst = 0;
				}
			}
			rest++;
		}
	}
}

void parse_notice(char *from, char *rest)
{
	char	*ctcp,*to;
	uint32_t pingtime;

	to = chop(&rest);
	if (*rest == ':')
		rest++;

	nickcpy(CurrentNick,from);

	if (*rest == 1 && *(rest+1))
	{
		rest++;
		ctcp = get_token(&rest," \001");	/* rest cannot be NULL */

		if (!stringcasecmp(ctcp,"PING") && ((pingtime = get_number(rest)) != -1))
		{
			send_spy(SPYSTR_STATUS,"[CTCP PING Reply From %s] %i second(s)",
				CurrentNick,(int)(now - pingtime));
		}
		else
		{
			to = get_token(&rest,"\001");	/* rest cannot be NULL */
			send_spy(SPYSTR_STATUS,"[CTCP %s Reply From %s] %s",ctcp,CurrentNick,to);
		}
		return;
	}

	if (ischannel(to))
	{
		send_spy(to,"-%s:%s- %s",CurrentNick,to,rest);
#ifdef STATS
		{
			Chan *chan;
			if ((chan = find_channel_ny(to)))
			{
				if (chan->stats)
					chan->stats->notice++;
			}
		}
#endif /* STATS */
		return;
	}
	send_spy(SPYSTR_MESSAGE,"-%s- %s",CurrentNick,rest);
}

void parse_part(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;
	char	*nick;

	channel = chop(&rest);
	if ((chan = find_channel_ny(channel)) == NULL)
		return;

	nick = from;
	if ((from = stringchr(from,'!')))
	{
		*from = 0;
		from++;
	}
	else
	{
		from = "";
	}

	if (current->spy & SPYF_CHANNEL)
		send_spy(channel,"*** Parts: %s (%s)",nick,from);

	if (!nickcmp(current->nick,nick))
	{
#ifdef DEBUG
		debug("(parse_part) Im parting %s\n",chan->name);
#endif /* DEBUG */
		chan->active = FALSE;
		chan->bot_is_op = FALSE;
#ifdef STATS
		if (chan->stats)
			chan->stats->flags |= CSTAT_PARTIAL;
#endif /* STATS */
	}

#ifdef STATS
	if (chan->stats)
	{
		stats_plusminususer(chan,-1);
		chan->stats->parts++;
	}
#endif /* STATS */

#ifdef SEEN
	make_seen(nick,from,channel,NULL,now,SEEN_PARTED);
#endif /* SEEN */

	remove_chanuser(chan,nick);

	/*
	 *  cycle the channel to gain ops once its empty
	 */
	if (chan->active && !chan->bot_is_op && chan->users && !chan->users->next)
		cycle_channel(chan);
}

void parse_ping(char *from, char *rest)
{
	to_server("PONG :%s\n",rest);
}

void parse_pong(char *from, char *rest)
{
	time_t	ot;
	char	*src;

#ifdef DEBUG
	debug("(parse_pong) rest == %s\n",rest);
#endif

	if (rest[0] == ':' && rest[1] == 'O' && rest[2] == 'T')
	{
		ot = 0;
		src = &rest[3];
		while(attrtab[(uchar)*src] & NUM)
			ot = (ot * 10) + (*src++ - '0');
		current->ontime = ot;
#ifdef DEBUG
		debug("(parse_pong) recovering ontime = %lu (%s)\n",ot,idle2str(now - ot,TRUE));
#endif
	}
}

void parse_privmsg(char *from, char *rest)
{
	ChanUser *cu;
	char	*to,*channel;
#ifdef URLCAPTURE
	const char *src;
#endif /* URLCAPTURE */

	to = chop(&rest);
	if (*rest == ':')
		rest++;

	channel = NULL;
	CurrentChan = NULL;

	if (ischannel(to) && (CurrentChan = find_channel_ac(to)))
	{
		if ((cu = find_chanuser(CurrentChan,from)))
		{
			cu->idletime = now;
			if (cu->shit)
				return;
			CurrentUser = cu->user;
		}
		else
		{
			channel = CurrentChan->name;
			goto hard_lookup;
		}
	}
	else
	{
	hard_lookup:
		if (find_shit(from,channel))
			return;
		CurrentUser = get_user(from,channel);
	}

	CurrentShit = NULL;
	nickcpy(CurrentNick,from);

	/*
	 *  if its a CTCP, it goes to on_ctcp (dcc.c)
	 */
	if (*rest == 1)
	{
		on_ctcp(from,to,rest+1);
		return;
	}
#ifdef TRIVIA
	if (CurrentChan)
		trivia_check(CurrentChan,rest);
#endif /* TRIVIA */
#ifdef STATS
	if (CurrentChan && CurrentChan->stats)
		CurrentChan->stats->privmsg++;
#endif /* STATS */
#ifdef URLCAPTURE
	src = rest;
	while(*src)
	{
		if (tolowertab[*src] == 'h')
		{
			if (tolowertab[src[1]] == 't' && tolowertab[src[2]] == 't' && tolowertab[src[3]] == 'p')
			{
				if ((src[4] == ':') || /* "http:" */
				    (tolowertab[src[4]] == 's' && src[5] == ':')) /* "https:" */
				{
					urlcapture(src);
				}
			}
		}
		src++;
	}
#endif /* URLCAPTURE */
	on_msg(from,to,rest);
}

void parse_quit(char *from, char *rest)
{
	Chan	*chan;

	nickcpy(CurrentNick,from);

#ifdef SEEN
	make_seen(CurrentNick,from,rest,NULL,now,SEEN_QUIT);
#endif /* SEEN */

#ifdef FASTNICK
	if (!nickcmp(CurrentNick,current->wantnick))
		to_server("NICK %s\n",current->wantnick);
#endif /* FASTNICK */

	if (current->spy & SPYF_CHANNEL)
		send_spy(MATCH_ALL,"*** Quits: %s (%s)",CurrentNick,rest);

	for(chan=current->chanlist;chan;chan=chan->next)
	{
#ifdef STATS
		if (chan->setting[STR_STATS].str_var)
		{
			if (find_chanuser(chan,CurrentNick))
			{
				stats_plusminususer(chan,-1);
				if (chan->stats)
					chan->stats->quits++;
			}
		}
#endif /* STATS */
		remove_chanuser(chan,CurrentNick);
		if (chan->active && !chan->bot_is_op && chan->users && !chan->users->next)
			cycle_channel(chan);
	}

	delete_auth(from);
}

void parse_topic(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	if (*rest == ':')
		rest++;

	send_spy(chan->name,"*** %s changes topic to \"%s\"",nickcpy(NULL,from),rest);
	reverse_topic(chan,from,rest);
}

void parse_wallops(char *from, char *rest)
{
	nickcpy(CurrentNick,from);
	send_spy(SPYSTR_MESSAGE,"!%s! %s",CurrentNick,rest);
}

/*
 *
 *  Numerics
 *
 */

/*
 *  213 RPL_STATSCLINE "C <host> * <name> <port> <class>"
 */
void parse_213(char *from, char *rest)
{
	send_pa(PA_STATS,NULL,"[\002%s\002] %s",from,rest);
}

/*
 *  219 RPL_ENDOFSTATS "<stats letter> :End of /STATS report"
 */
void parse_219(char *from, char *rest)
{
	send_pa(PA_STATS|PA_END,NULL,NULL);
}

/*
 *  251 RPL_LUSERCLIENT ":There are <integer> users and <integer> invisible on <integer> servers"
 */
void parse_251(char *from, char *rest)
{
	Server	*sp;
	char	*nick;
	int	num[3] = {0,0,0};
	int	i = 0,n = -1;

	if (current->reset)
	{
		nick = chop(&rest);
		setbotnick(current,nick);
		for(sp=serverlist;sp;sp=sp->next)
		{
			if (!stringcasecmp(sp->name,from) || !stringcasecmp(sp->realname,from))
			{
				sp->lastconnect = now;
				current->ontime = now;
				current->server = sp->ident;
			}
		}
#ifdef DEBUG
		debug("(parse_251) discovered nick = %s\n",nick);
		debug("(parse_251) discovered server = %s\n",from);
#endif /* DEBUG */
		to_server("WHOIS %s\nMODE %s\n",nick,nick);
		return;
	}

	chop(&rest);	/* discard bot nick */

	while(*rest)
	{
		if (*rest >= '0' && *rest <= '9')
		{
			if (n == -1)
				n = 0;
			n = (n * 10) + (*rest - '0');
		}
		else
		if (n != -1)
		{
			num[i++] = n;
			n = -1;
			if (i == 3)
				break;
		}
		rest++;
	}

	send_pa(PA_LUSERS,NULL,"\037Lusers status\037");
	send_pa(PA_LUSERS,NULL,"Users: \002%i\002 (Invisible: \002%i\002)",num[0],num[1]);
	send_pa(PA_LUSERS,NULL,"Servers: \002%i\002",num[2]);
}

/*
 *  252 RPL_LUSEROP "<integer> :operator(s) online"
 */
void parse_252(char *from, char *rest)
{
	int	n;

	n = get_number(rest);
	send_pa(PA_LUSERS,NULL,"IRC Operators: \002%i\002",n);
}

/*
 *  253 RPL_LUSERUNKNOWN "<integer> :unknown connection(s)"
 */
void parse_253(char *from, char *rest)
{
	int	n;

	n = get_number(rest);
	send_pa(PA_LUSERS,NULL,"Unknown Connections: \002%i\002",n);
}

/*
 *  254 RPL_LUSERCHANNELS "<integer> :channels formed"
 */
void parse_254(char *from, char *rest)
{
	int	n;

	n = get_number(rest);
	send_pa(PA_LUSERS,NULL,"Channels: \002%i\002",n);
}

/*
 *  255 RPL_LUSERME ":I have <integer> clients and <integer> servers"
 */
void parse_255(char *from, char *rest)
{
	int	num[2] = {0,0};
	int	i = 0,n = -1;

	while(*rest)
	{
		if (*rest >= '0' && *rest <= '9')
		{
			if (n == -1)
				n = 0;
			n = (n * 10) + (*rest - '0');
		}
		else
		if (n != -1)
		{
			num[i++] = n;
			n = -1;
			if (i == 2)
				break;
		}
		rest++;
	}

	send_pa(PA_LUSERS|PA_END,NULL,"Local Clients: \002%i\002 Local Servers: \002%i\002",num[0],num[1]);
}

/*
 *  301 RPL_AWAY "<nick> :<away message>"
 */
void parse_301(char *from, char *rest)
{
	char	*nick;

	nick = chop(&rest);
	if (*rest == ':')
		rest++;

	send_pa(PA_WHOIS,nick,"Away            - %s",rest);
}

#ifdef NOTIFY

/*
 *  303 RPL_ISON ":[<nick> {<space><nick>}]"
 */
void parse_303(char *from, char *rest)
{
	if (*rest == ':')
		rest++;
	catch_ison(rest);
}

#endif /* NOTIFY */

/*
 *  311 RPL_WHOISUSER "<nick> <user> <host> * :<real name>"
 */
void parse_311(char *from, char *rest)
{
	char	*nick,*user,*host;

	nick = chop(&rest);
	user = chop(&rest);
	host = chop(&rest);

	chop(&rest);		/* discard "*" */
	if (*rest == ':')
		rest++;

	/*
	 *  create a "user@host" string
	 */
	if (host)
		host[-1] = '@';

	if (!nickcmp(nick,current->nick))
	{
		Free((char**)&current->userhost);
		set_mallocdoer(parse_311);
		current->userhost = stringdup(user);
#ifdef BOTNET
		botnet_refreshbotinfo();
#endif /* BOTNET */
	}

	send_pa(PA_USERHOST|PA_END,nick,"Userhost: %s=%s",nick,user);
	send_pa(PA_WHOIS,nick,"Whois: \037%s\037 (%s) : %s",nick,user,rest);
#ifdef NOTIFY
	catch_whois(nick,user,rest);
#endif /* NOTIFY */
}

/*
 *  312 RPL_WHOISSERVER "<nick> <server> :<server info>"
 */
void parse_312(char *from, char *rest)
{
	char	*nick,*server;

	nick = chop(&rest);
	server = chop(&rest);

	if (*rest == ':')
		rest++;

	send_pa(PA_WHOIS,nick,"Server: %s (%s)",server,rest);
}

/*
 *  313 RPL_WHOISOPERATOR "<nick> :is an IRC operator"
 */
void parse_313(char *from, char *rest)
{
	char	*nick;

	nick = chop(&rest);
	if (*rest == ':')
		rest++;

	send_pa(PA_WHOIS,nick,"IRCop: %s %s",nick,rest);
}

/*
 *  315 RPL_ENDOFWHO "<name> :End of /WHO list"
 */
void parse_315(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) != NULL)
	{
		chan->wholist = TRUE;
		chan->sync = TRUE;
#ifdef STATS
		if (chan->setting[STR_STATS].str_var)
			stats_plusminususer(chan,0);
#endif /* STATS */
	}
}

/*
 *  317 RPL_WHOISIDLE "<nick> <integer> :seconds idle"
 *  317 RPL_WHOISIDLE "<nick> <integer> <integer> :seconds idle, signon time"
 */
void parse_317(char *from, char *rest)
{
	char	*nick;
	time_t	when = -1;
	int	min,sec = -1;

	nick = chop(&rest);

	while(*rest)
	{
		if (*rest >= '0' && *rest <= '9')
		{
			if (sec == -1)
				sec = 0;
			sec = (sec * 10) + (*rest - '0');
		}
		else
		if (sec != -1)
			break;
		rest++;
	}
	min = sec / 60;
	sec = sec % 60;

	while(*rest)
	{
		if (*rest >= '0' && *rest <= '9')
		{
			if (when == -1)
				when = 0;
			when = (when * 10) + (*rest - '0');
		}
		else
		if (when != -1)
			break;
		rest++;
	}

	if (when != -1)
		send_pa(PA_WHOIS,nick,"Signed On: %s",time2away(when));

	send_pa(PA_WHOIS,nick,
		(sec) ? "Idle: %i minute%s, %i second%s" : "Idle: %i minute%s",
		min,(min == 1) ? "" : "s",sec,(sec == 1) ? "" : "s");
}

/*
 *  318 RPL_ENDOFWHOIS "<nick> :End of /WHOIS list"
 */
void parse_318(char *from, char *rest)
{
	char	*nick;

	while((nick = get_token(&rest," ,")))	/* rest cannot be NULL */
	{
		if (*nick == ':')
			break;

		send_pa(PA_WHOIS|PA_END,nick,NULL);
	}
}

/*
 *  319 RPL_WHOISCHANNELS "<nick> :{[@|+]<channel><space>}"
 */
void parse_319(char *from, char *rest)
{
	char	nuh[NUHLEN];
	char	*nick,*channel;
#ifdef CHANBAN
	Shit	*shit;
#endif /* CHANBAN */

	nick = chop(&rest);
	if (*rest == ':')
		rest++;
	if (*rest == 0)
		return;

	send_pa(PA_WHOIS,nick,"Channels: %s",rest);

	/* if nick is myself (the bot), check for reset recovery */
	if (!nickcmp(nick,current->nick))
	{
		if (current->reset)
		{
loop:
			channel = chop(&rest);
			if (!channel)
			{
				current->reset = FALSE;
				return;
			}
			/*
			 *  skip past '+', '-' and '@', etc.
			 */
			while(*channel && *channel != '#') /* this is a recipe for disaster with other valid channels than '#' */
				channel++;
			sprintf(nuh,"%s!%s",current->nick,current->userhost);
#ifdef DEBUG
			debug("(parse_319) :%s JOIN :%s\n",nuh,channel);
#endif /* DEBUG */
			parse_join(nuh,channel);
			goto loop;
		}
	}
#ifdef CHANBAN
/*
[StS] {2} WHOIS f0l3r
(in)  {2} :Vancouver.BC.CA.Undernet.org 311 eenucks f0l3r ~folyourat folyourat.users.undernet.org * :Huh?
(in)  {2} :Vancouver.BC.CA.Undernet.org 319 eenucks f0l3r :#emech @#folers @#margarita @#caracas
(in)  {2} :Vancouver.BC.CA.Undernet.org 312 eenucks f0l3r *.undernet.org :The Undernet Underworld
(in)  {2} :Vancouver.BC.CA.Undernet.org 330 eenucks f0l3r folyourat :is logged in as
(in)  {2} :Vancouver.BC.CA.Undernet.org 318 eenucks f0l3r :End of /WHOIS list.
*/
	else
	{
		/* check if all sendq is empty (text, kicks and modes) */
		if (current->sendq)
		{
#ifdef DEBUG
			debug("(parse_319) skipping chanban, sendq is not empty\n");
#endif /* DEBUG */
			return;
		}
		/* CurrentChan is unset and can be used */
		for(CurrentChan=current->chanlist;CurrentChan;CurrentChan=CurrentChan->next)
		{
			if (CurrentChan->kicklist || CurrentChan->modelist)
				return;
		}

		while((channel = chop(&rest)))
		{
#ifdef DEBUG
			debug("(parse_319) Cb: checking %s\n",channel);
#endif /* DEBUG */
			if (*channel == '@' || *channel == '+') /* opped/voiced user, spin 1 */
				channel++;
			/* other user modes might also be present (crazy ircd's), this is insufficient testing */

			/* check if the channel is shitted */
			for(shit=current->shitlist;shit;shit=shit->next)
			{
				if (!matches(shit->mask,channel))
				{
#ifdef DEBUG
					debug("(parse_319) Cb: executing chanban_action()\n",channel);
#endif /* DEBUG */
					chanban_action(nick,channel,shit);
				}
			}
		}
	}
#endif /* CHANBAN */
}

/*
 *  324 RPL_CHANNELMODEIS "<channel> <mode> <mode params>"
 */
void parse_324(char *from, char *rest)
{
	char	*channel;

	if ((channel = chop(&rest)))
	{
		CurrentUser = NULL;
		CurrentShit = NULL;
		*CurrentNick = 0;
		on_mode(CurrentNick,channel,rest);
	}
}

/*
 *  352 RPL_WHOREPLY "<channel> <user> <host> <server> <nick> <H|G>[*][@|+] :<hopcount> <real name>"
 */
void parse_352(char *from, char *rest)
{
	Chan	*chan;
	char	nuh[NUHLEN];
	char	*channel,*nick,*userhost;

	channel = chop(&rest);
	if ((CurrentChan = chan = find_channel_ac(channel)) == NULL)
		return;

	if (chan->wholist == TRUE)
		return;

	userhost = chop(&rest);
	rest[-1] = '@';		/* glue: "user\0host" --> "user@host" */
	chop(&rest);
	chop(&rest);
	nick = chop(&rest);

	sprintf(nuh,"%s!%s",nick,userhost);

	make_chanuser(nick,userhost);

	if (is_bot(nick))
	{
#ifdef DEBUG
		debug("(parse_352) setting as local bot: %s (%s)\n",nuh,channel);
#endif /* DEBUG */
		chan->users->user = (User*)&LocalBot;
		chan->users->shit = NULL;
	}
	else
	{
		chan->users->user = get_user(nuh,channel);
		chan->users->shit = find_shit(nuh,channel);
	}

#ifdef BOTNET
	if (chan->users->user && chan->users->user->x.x.access == BOTLEVEL)
		check_botjoin(chan,chan->users);
#endif /* BOTNET */

	while(*rest != ' ')
	{
		if (*rest == '@')
		{
			chan->users->flags = CU_CHANOP;
			if (!nickcmp(current->nick,nick))
			{
#ifdef DEBUG
				debug("(parse_352) According to wholist I have ops\n");
#endif /* DEBUG */
				chan->bot_is_op = TRUE;
			}
			/*
			 *  chanop is the most relevant flag at this stage
			 */
			return;
		}
		else
		if (*rest == '+')
		{
			chan->users->flags = CU_VOICE;
		}
		rest++;
	}
}

/*
 *  367 RPL_BANLIST "<channel> <banid>"
 *  367 RPL_BANLIST "<channel> <banid> <whodunnit> <when>"
 */
void parse_367(char *from, char *rest)
{
	Chan	*chan;
	char	*channel,*banmask,*banfrom;
	time_t	bantime;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	banmask = chop(&rest);

	/*
	 *  according to RFC1459, the next two args are optional
	 */
	if ((banfrom = chop(&rest)) == NULL)
		banfrom = "?";

	if ((bantime = get_number(rest)) == -1)
		bantime = now;

	make_ban(&chan->banlist,banfrom,banmask,bantime);
}

/*
 *  376 RPL_ENDOFMOTD ":End of /MOTD command"
 */
void parse_376(char *from, char *rest)
{
	Server	*sp;
	char	*mode;

	/*
	 *  Save the name that the server uses for itself
	 */
	if ((sp = find_server(current->server)))
	{
		if (*sp->realname == 0)
			stringcpy_n(sp->realname,from,NAMELEN);
		sp->lastconnect = now;
	}
	if (current->connect != CN_ONLINE)
	{
		current->connect = CN_ONLINE;
		current->ontime = now;
		to_server("WHOIS %s\n",current->nick);
		if ((mode = current->setting[STR_UMODES].str_var))
			to_server("MODE %s %s\n",current->nick,mode);
	}
#ifdef IDWRAP
	unlink_identfile();
#endif /* IDWRAP */
}

/*
 *  401 ERR_NOSUCHNICK <botnick> <nickname> :No such nick/channel
 */
void parse_401(char *from, char *rest)
{
	char	*nick;

	nick = chop(&rest);
	send_pa(PA_WHOIS,nick,"%s: No such nick",nick);
}

/*
 *  How to determine the new nick: 
 *  - If it's possible to add a '_' to the nick, do it (done)
 *  - else loop through the nick from begin to end and replace
 *    the first non-_ with a _. i.e. __derServ -> ___erServ
 *  - If the nick is 9 _'s, try the original nick with something random
 */
void parse_433(char *from, char *rest)
{
	char	*s;

	/*
	 *  try to get a better nick even if we are already online
	 */
	if ((s = current->setting[STR_ALTNICK].str_var))
	{
		char scopy[strlen(s)+1];
		char *s2,*nick,*trynick;

		chop(&rest);
		trynick = chop(&rest);
		stringcpy(scopy,s);
		s = scopy;
		nick = NULL;
		do
		{
			s2 = chop(&s);
			if (current->connect == CN_ONLINE && !stringcasecmp(current->nick,s2))
			{
				/* nicks listed first are more worth, dont try nicks after */
				break;
			}
			if (!nick)
				nick = s2;
			if (!stringcasecmp(trynick,s2))
			{
				nick = NULL;
			}
#ifdef DEBUG
			debug("(parse_433) s = %s, s2 = %s, nick = %s\n",
				nullbuf(s),nullstr(s2),nullstr(nick));
#endif
		}
		while(*s);

		if (nick)
		{
#ifdef DEBUG
			debug("(parse_433) Nick: %s, Altnick: %s\n",current->nick,nick);
#endif /* DEBUG */
			to_server("NICK %s\n",nick);
			if (current->connect != CN_ONLINE)
			{
				Free((char**)&current->nick);
				set_mallocdoer(parse_433);
				current->nick = stringdup(nick);
			}
			return;
		}
	}
}

void parse_451(char *from, char *rest)
{
	register_with_server();
}

/*
 *  405 ERR_TOOMANYCHANNELS "<channel name> :You have joined too many channels"
 *  471 ERR_CHANNELISFULL "<channel> :Cannot join channel (+l)"
 *  474 ERR_BANNEDFROMCHAN "<channel> :Cannot join channel (+b)"
 *  475 ERR_BADCHANNELKEY "<channel> :Cannot join channel (+k)"
 *  476 ERR_BADCHANMASK ???
 */
void parse_471(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);

	if ((chan = find_channel_ny(channel)) != NULL)
	{
		chan->active = FALSE;
		chan->sync = TRUE;
	}
}

/*
 *  473 ERR_INVITEONLYCHAN "<channel> :Cannot join channel (+i)"
 */
void parse_473(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);

	if ((chan = find_channel_ny(channel)) != NULL)
	{
		chan->active = FALSE;
		chan->sync = TRUE;
	}
}

#ifdef IRCD_EXTENSIONS

/*
(in)  {2} :ircnet.choopa.net 348 eenucks #emech *kong!*@*
(in)  {2} :ircnet.choopa.net 349 eenucks #emech :End of Channel Exception List

(in)  {2} :ircnet.choopa.net 346 eenucks #emech *king*!*@*
(in)  {2} :ircnet.choopa.net 347 eenucks #emech :End of Channel Invite List
*/

/*
 *  346 xxx "<channel> <banid>"
 *  346 xxx "<channel> <banid> <whodunnit> <when>"
 */
void parse_346(char *from, char *rest)
{
	Chan	*chan;
	char	*channel,*banmask,*banfrom;
	time_t	bantime;
	Ban	*new;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	banmask = chop(&rest);

	/*
	 *  according to RFC1459, the next two args are optional
	 */
	if ((banfrom = chop(&rest)) == NULL)
		banfrom = "?";

	if ((bantime = get_number(rest)) == -1)
		bantime = now;

	new = make_ban(&chan->banlist,banfrom,banmask,bantime);
	new->imode = TRUE;
}

/*
 *  348 xxx "<channel> <banid>"
 *  348 xxx "<channel> <banid> <whodunnit> <when>"
 */
void parse_348(char *from, char *rest)
{
	Chan	*chan;
	char	*channel,*banmask,*banfrom;
	time_t	bantime;
	Ban	*new;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	banmask = chop(&rest);

	/*
	 *  according to RFC1459, the next two args are optional
	 */
	if ((banfrom = chop(&rest)) == NULL)
		banfrom = "?";

	if ((bantime = get_number(rest)) == -1)
		bantime = now;

	new = make_ban(&chan->banlist,banfrom,banmask,bantime);
	new->emode = TRUE;
}

/*
 *  catch end of banlist reply and use it to send requests for I/e modes if they are supported.
 */
void parse_368(char *from, char *rest)
{
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);
	if ((chan = find_channel_ac(channel)) == NULL)
		return;

	if (current->ircx_flags & IRCX_IMODE)
	{
		to_server("MODE %s +I\n",chan->name);
	}
	if (current->ircx_flags & IRCX_EMODE)
	{
		to_server("MODE %s +e\n",chan->name);
	}
}

/*
 *  catch the capabilities line and extract some details from it.
 */
/*
:ircnet.choopa.net 005 botname
RFC2812
PREFIX=(ov)@+		Legal channel user prefixes (ops and voice)
CHANTYPES=#&!+		Legal channel name prefixes
MODES=3			Permitted number of modes per MODE-line
CHANLIMIT=#&!+:21
NICKLEN=15
TOPICLEN=255
KICKLEN=255
MAXLIST=beIR:64
CHANNELLEN=50
IDCHAN=!:5
CHANMODES=beIR,k,l,imnpstaqr
:are supported by this server
*/
void parse_005(char *from, char *rest)
{
	char	*opt,*s;
	int	n;

	while((opt = chop(&rest)))
	{
		if (*opt == ':')
			break;
		if (!matches("modes=?",opt))
		{
			n = asc2int(opt+6);
			if (!errno && n >= 1 && n <= 20)
			{
				current->setting[INT_MODES].int_var = n;
#ifdef DEBUG
				debug("(parse_005) autoconf: mode number: %i\n",n);
#endif /* DEBUG */
			}
		}
		else
		if (!stringcasecmp("WALLCHOPS",opt))
		{
			current->ircx_flags |= IRCX_WALLCHOPS;
#ifdef DEBUG
			debug("(parse_005) autoconf: wallchops enabled\n");
#endif /* DEBUG */
		}
		else
		if (!stringcasecmp("WALLVOICES",opt))
		{
			current->ircx_flags |= IRCX_WALLVOICES;
#ifdef DEBUG
			debug("(parse_005) autoconf: wallvoices enabled\n");
#endif /* DEBUG */
		}
		else
		if (!matches("chanmodes=*",opt))
		{
			s = opt+10;
#ifdef DEBUG
			debug("(parse_005) autoconf: s = \"%s\"\n",s);
#endif /* DEBUG */
			while(*s)
			{
				/*CHANMODES=beIR,k,l,imnpstaqr */
				if (*s == 'e')
				{
					current->ircx_flags |= IRCX_EMODE;
#ifdef DEBUG
					debug("(parse_005) autoconf: +e modes (ban exceptions) enabled\n");
#endif /* DEBUG */
				}
				if (*s == 'I')
				{
					current->ircx_flags |= IRCX_IMODE;
#ifdef DEBUG
					debug("(parse_005) autoconf: +I modes (invite exceptions) enabled\n");
#endif /* DEBUG */
				}
				s++;
			}
		}
	}
}

#endif /* IRCD_EXTENSIONS */

#define NEEDFROM	1
#define DROPONE		2

LS const struct
{
	uint32_t hash;
	short	flags;
	void	(*func)(char *, char *);

} pFuncs[] =
{
	{ 0x50524956,	NEEDFROM,		parse_privmsg	},	/* PRIVMSG */
	{ 0x4A4F494E,	NEEDFROM,		parse_join	},	/* JOIN */
	{ 0x50415254,	NEEDFROM,		parse_part	},	/* PART */
	{ 0x4D4F4445,	NEEDFROM,		parse_mode	},	/* MODE */
	{ 0x4E49434B,	NEEDFROM,		on_nick		},	/* NICK */
	{ 0x4B49434B,	NEEDFROM,		on_kick		},	/* KICK */
	{ 0x50494E47,	0,			parse_ping	},	/* PING */
	{ 0x504F4E47,	DROPONE,		parse_pong	},	/* PONG */
	{ 0x544F5049,	NEEDFROM,		parse_topic	},	/* TOPIC */
	{ 0x4E4F5449,	NEEDFROM,		parse_notice	},	/* NOTICE */
	{ 0x51554954,	NEEDFROM,		parse_quit	},	/* QUIT */
	{ 0x494E5649,	NEEDFROM|DROPONE,	parse_invite	},	/* INVITE */
	{ 0x57414C4C,	NEEDFROM,		parse_wallops	},	/* WALLOPS */
	{ 0x4552524F,	0,			parse_error	},	/* ERROR */
	{ 0x00333532,	NEEDFROM|DROPONE,	parse_352	},	/* 352 RPL_WHOREPLY		*/
	{ 0x00333135,	NEEDFROM|DROPONE,	parse_315	},	/* 315 RPL_ENDOFWHO		*/
	{ 0x00323231,	NEEDFROM,		parse_mode	},	/* 221 RPL_UMODEIS		*/
	{ 0x00333131,	NEEDFROM|DROPONE,	parse_311	},	/* 311 RPL_WHOISUSER		*/
	{ 0x00333132,	NEEDFROM|DROPONE,	parse_312	},	/* 312 RPL_WHOISSERVER		*/
	{ 0x00333138,	NEEDFROM|DROPONE,	parse_318	},	/* 318 RPL_ENDOFWHOIS		*/
	{ 0x00343031,	NEEDFROM|DROPONE,	parse_401	},	/* 401 ERR_NOSUCHNICK		*/
	{ 0x00343333,	NEEDFROM,		parse_433	},	/* 433 ERR_NICKNAMEINUSE	*/
	{ 0x00323139,	NEEDFROM,		parse_219	},	/* 219 RPL_ENDOFSTATS		*/
	{ 0x00323131,	NEEDFROM|DROPONE,	parse_213	},	/* 211 RPL_STATSLINKINFO	*/
	{ 0x00323133,	NEEDFROM|DROPONE,	parse_213	},	/* 213 RPL_STATSCLINE		*/
	{ 0x00323134,	NEEDFROM|DROPONE,	parse_213	},	/* 214 RPL_STATSNLINE		*/
	{ 0x00323135,	NEEDFROM|DROPONE,	parse_213	},	/* 215 RPL_STATSILINE		*/
	{ 0x00323136,	NEEDFROM|DROPONE,	parse_213	},	/* 216 RPL_STATSKLINE		*/
	{ 0x00323137,	NEEDFROM|DROPONE,	parse_213	},	/* 217 RPL_STATSQLINE		*/
	{ 0x00323138,	NEEDFROM|DROPONE,	parse_213	},	/* 218 RPL_STATSYLINE		*/
	{ 0x00323431,	NEEDFROM|DROPONE,	parse_213	},	/* 241 RPL_STATSLLINE		*/
	{ 0x00323432,	NEEDFROM|DROPONE,	parse_213	},	/* 242 RPL_STATSUPTIME		*/
	{ 0x00323433,	NEEDFROM|DROPONE,	parse_213	},	/* 243 RPL_STATSOLINE		*/
	{ 0x00323434,	NEEDFROM|DROPONE,	parse_213	},	/* 244 RPL_STATSHLINE		*/
	{ 0x00323435,	NEEDFROM|DROPONE,	parse_213	},	/* 245 RPL_STATSSLINE		*/
	{ 0x00323531,	NEEDFROM,		parse_251	},	/* 251 RPL_LUSERCLIENT		*/
	{ 0x00323532,	NEEDFROM|DROPONE,	parse_252	},	/* 252 RPL_LUSEROP		*/
	{ 0x00323533,	NEEDFROM|DROPONE,	parse_253	},	/* 253 RPL_LUSERUNKNOWN		*/
	{ 0x00323534,	NEEDFROM|DROPONE,	parse_254	},	/* 254 RPL_LUSERCHANNELS	*/
	{ 0x00323535,	NEEDFROM|DROPONE,	parse_255	},	/* 255 RPL_LUSERME		*/
	{ 0x00333031,	NEEDFROM|DROPONE,	parse_301	},	/* 301 RPL_AWAY			*/
#ifdef NOTIFY
	{ 0x00333033,	DROPONE,		parse_303	},	/* 303 RPL_ISON			*/
#endif /* NOTIFY */
	{ 0x00333133,	NEEDFROM|DROPONE,	parse_313	},	/* 313 RPL_WHOISOPERATOR	*/
	{ 0x00333137,	NEEDFROM|DROPONE,	parse_317	},	/* 317 RPL_WHOISIDLE		*/
	{ 0x00333139,	NEEDFROM|DROPONE,	parse_319	},	/* 319 RPL_WHOISCHANNELS	*/
	{ 0x00333234,	NEEDFROM|DROPONE,	parse_324	},	/* 324 RPL_CHANNELMODEIS	*/
	{ 0x00333637,	NEEDFROM|DROPONE,	parse_367	},	/* 367 RPL_BANLIST		*/
	{ 0x00343531,	0,			parse_451	},	/* 451 ERR_NOTREGISTERED	*/
	{ 0x00343035,	NEEDFROM|DROPONE,	parse_471	},	/* 405 ERR_TOOMANYCHANNELS	*/
	{ 0x00343731,	NEEDFROM|DROPONE,	parse_471	},	/* 471 ERR_CHANNELISFULL	*/
	{ 0x00343733,	NEEDFROM|DROPONE,	parse_473	},	/* 473 ERR_INVITEONLYCHAN	*/
	{ 0x00343734,	NEEDFROM|DROPONE,	parse_471	},	/* 474 ERR_BANNEDFROMCHAN	*/
	{ 0x00343735,	NEEDFROM|DROPONE,	parse_471	},	/* 475 ERR_BADCHANNELKEY	*/
	{ 0x00343736,	NEEDFROM|DROPONE,	parse_471	},	/* 476 ERR_BADCHANMASK		*/
	{ 0x00333736,	NEEDFROM,		parse_376	},	/* 376 RPL_ENDOFMOTD		*/
	{ 0x00343232,	NEEDFROM,		parse_376	},	/* 422 ERR_NOMOTD		*/
#ifdef IRCD_EXTENSIONS
	{ 0x00303035,	NEEDFROM|DROPONE,	parse_005	},	/* 005 ircd extensions		*/
	{ 0x00333638,	NEEDFROM|DROPONE,	parse_368	},	/* 368 RPL_ENDOFBANLIST		*/
	{ 0x00333436,	NEEDFROM|DROPONE,	parse_346	},	/* 346 +I invitelist		*/
	{ 0x00333438,	NEEDFROM|DROPONE,	parse_348	},	/* 348 +e ban exception		*/
#endif /* IRCD_EXTENSIONS */
	{ 0,		0,			NULL		}
};

uint32_t stringhash(char *s)
{
	uint32_t hash;
	int	i;

	hash = 0;
	for(i=0;(i<4 && *s);i++)
		hash = (hash << 8) | (uchar)*(s++);
	return(hash);
}

void parse_server_input(char *rest)
{
#ifdef SCRIPTING
	Hook	*hook;
#endif /* SCRIPTING */
	char	*from,*command;
	uint32_t cmdhash;
	int	i;

	if (current->spy & (SPYF_RAWIRC|SPYF_RANDSRC))
		send_spy(SPYSTR_RAWIRC,rest);

/*new undernet amusements */
/*(in)  {5} NOTICE AUTH :*** You have identd disabled (or broken), to continue to connect you must type /QUOTE PASS 17071 */
	if (current->connect == CN_CONNECTED && *rest == 'N' && !matches("NOTICE AUTH * /QUOTE PASS *",rest))
	{
		from = STREND(rest);
		while(from > rest && *from != ' ')
			from--;
		if (*from == ' ' && *(from+1))
		{
			to_server("PASS%s\n",from);
		}
	}

	if (*rest == ':')
	{
		rest++;
		from = rest;
		while(*rest != ' ')
			rest++;
		*(rest++) = 0;
	}
	else
	{
		from = NULL;
	}

	command = chop(&rest);
	if (*rest == ':')
		rest++;

#ifdef SCRIPTING
	cmdhash = 1;
	for(hook=hooklist;hook;hook=hook->next)
	{
		/*
		 *  check if the hook applies to this particular bot
		 */
		if (hook->guid && hook->guid != current->guid)
			continue;
		/*
		 *  does the hook match?
		 */
		if (hook->flags == MEV_PARSE && !stringcasecmp(command,hook->type.command))
		{
			if (hook->func(from,rest,hook))
				/* if the hook returns non-zero, the input should not be parsed internally */
				cmdhash = 0;
		}
	}
	if (cmdhash == 0)
		return;
#endif /* SCRIPTING */

	cmdhash = stringhash(command);

	/*debug("cmdhash = %08X\n",cmdhash); */
	for(i=0;pFuncs[i].hash;i++)
	{
		if (cmdhash == pFuncs[i].hash)
		{
			if ((pFuncs[i].flags & NEEDFROM) && !from)
				return;
			if (pFuncs[i].flags & DROPONE)
				chop(&rest);	/* discard one argument (usually bot nick) */
			pFuncs[i].func(from,rest);
			return;
		}
	}
	/*debug("unmatched cmdhash %08X\n",cmdhash); */
}
