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
#define SPY_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef STATS
#include <math.h>
#endif

#ifdef DEBUG

LS const char SPY_DEFS[][12] =
{
	"SPY_FILE",
	"SPY_CHANNEL",
	"SPY_DCC",
	"SPY_STATUS",
	"SPY_MESSAGE",
	"SPY_RAWIRC",
	"SPY_BOTNET",
	"SPY_URL",
	"SPY_SYSMON",
	"SPY_RANDSRC",
};

#endif /* DEBUG */

static int basepos(char c)
{
/*
 -lcrypt converts to [a-zA-Z0-9./]
 included sha1 converts to hex
 included md5 converts to [./0-9A-Za-z]
*/
	if (c >= 'a' && c <= 'z')
		return(c - 'a');
	if (c >= 'A' && c <= 'Z')
		return(c - 'A' + 26);
	if (c >= '0' && c <= '9')
		return(c - '0' + 52);
	if (c == '.')
		return(62);
	if (c == '/')
		return(63);
	return(0);
}

void send_spy(const char *src, const char *format, ...)
{
	Chan	*chan;
	Mech	*backup;
	Spy	*spy;
	va_list	msg;
	const char *tempsrc;
	const char *printmsg;
	char	tempdata[MAXLEN],*rnd,*dst,*end;
	int	fd,a,b,c,d;
	int	printed = FALSE;

	if (src == SPYSTR_RAWIRC)
	{
		tempsrc = printmsg = format;
		printed = TRUE;
		format = FMT_PLAIN;
	}
	else
	if (src == SPYSTR_STATUS)
	{
		tempsrc = time2medium(now);
	}
	else
	{
		tempsrc = src;
	}

#ifdef DEBUG
	debug("(send_spy) src %s format = '%s'\n",src,format);
#endif /* DEBUG */

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if (spy->t_src == SPY_RANDSRC)
		{
			if (src != SPYSTR_RAWIRC)
				continue;
			/* dont use four char server messages such as "PING :..." */
			if (tempsrc[5] == ':')
#ifdef DEBUG
			{
				debug("(send_spy) RANDSRC: skipping four char server message\n");
#endif /* DEBUG */
				continue;
#ifdef DEBUG
			}
#endif /* DEBUG */

			if (spy->data.delay > now)
				continue;
			spy->data.delay = now + 10 + RANDOM(0,9); /* make it unpredictable which messages will be sourced */

/*
 $6$ for sha512, $1$ for MD5
     MD5     | 22 characters
     SHA-512 | 86 characters
*/
#ifdef SHACRYPT
			sprintf(tempdata,"$6$%04x",(now & 0xFFFF));
			rnd = CRYPT_FUNC(tempsrc,tempdata);
#endif /* SHACRYPT */

#if !defined(SHACRYPT) && defined(MD5CRYPT)
			sprintf(tempdata,"$1$%04x",(now & 0xFFFF));
			rnd = CRYPT_FUNC(tempsrc,tempdata);
#endif /* !SHACRYPT && MD5CRYPT */

			dst = tempdata;
			end = STREND(rnd);
#if defined(SHACRYPT) || defined(MD5CRYPT)
			rnd += 8; /* skip salt */
#endif
			while(rnd < (end - 3))
			{
				/* base64 to bin, 4 chars to 3 */
				a = basepos(rnd[0]);
				b = basepos(rnd[1]);
				dst[0] = (a << 2) | (b >> 4);		/* aaaaaabb */
				c = basepos(rnd[2]);
				dst[1] = (b << 4) | (c >> 2);		/* bbbbcccc */
				d = basepos(rnd[3]);
				dst[2] = (c << 6) | (d);		/* ccdddddd */
				dst += 3;
				rnd += 4;
			}
			if ((dst - tempdata) > 0)
			{
#ifdef DEBUG
				debug("(send_spy) randsrc got %i bytes\n",dst - tempdata);
#endif /* DEBUG */
				if ((fd = open(spy->dest,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) >= 0)
				{
					write(fd,tempdata,dst - tempdata);
					close(fd);
				}
			}
			continue;
		}

		if ((*src == '#' || *src == '*') && spy->t_src == SPY_CHANNEL)
		{
			if ((*src != '*') && stringcasecmp(spy->src,src))
				continue;
			if ((chan = find_channel_ac(spy->src)) == NULL)
				continue;
			if (find_chanuser(chan,CurrentNick) == NULL)
				continue;
			tempsrc = spy->src;
		}
		else
		/*
		 *  by using string constants we can compare addresses
		 */
		if (spy->src != src)
			continue;

		if (!printed)
		{
			printed = TRUE;
			va_start(msg,format);
			vsprintf(tempdata,format,msg);
			va_end(msg);
			printmsg = tempdata;
		}

		switch(spy->t_dest)
		{
		case SPY_DCC:
			to_file(spy->data.dcc->sock,"[%s] %s\n",tempsrc,printmsg);
			break;
		case SPY_CHANNEL:
			if (spy->data.destbot >= 0)
			{
				backup = current;
				for(current=botlist;current;current=current->next)
				{
					if (current->guid == spy->data.destbot)
					{
						to_server("PRIVMSG %s :[%s] %s\n",spy->dest,tempsrc,printmsg);
						break;
					}
				}
				current = backup;
			}
			else
			{
				to_user(spy->dest,"[%s] %s",tempsrc,printmsg);
			}
			break;
		case SPY_FILE:
			if ((fd = open(spy->dest,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) >= 0)
			{
				to_file(fd,"[%s] %s\n",logtime(now),printmsg);
				close(fd);
			}
		}
	}
}

void send_global(const char *src, const char *format, ...)
{
	va_list msg;
	Mech	*backup;
	char	tempdata[MAXLEN];
	int	printed = FALSE;

	backup = current;
	for(current=botlist;current;current=current->next)
	{
		if (current->spy & SPYF_ANY)
		{
			if (!printed)
			{
				printed = TRUE;
				va_start(msg,format);
				vsprintf(tempdata,format,msg);
				va_end(msg);
			}

			send_spy(src,FMT_PLAIN,tempdata);
		}
	}
	current = backup;
}

void spy_typecount(Mech *bot)
{
	Spy	*spy;

	bot->spy = 0;
	for(spy=bot->spylist;spy;spy=spy->next)
	{
		bot->spy |= SPYF_ANY;
		bot->spy |= (1 << spy->t_src);
	}
}

struct
{
	const char *idstring;
	int typenum;

} spy_source_list[] =
{
{ SPYSTR_STATUS,	SPY_STATUS	},
{ SPYSTR_MESSAGE,	SPY_MESSAGE	},
{ SPYSTR_RAWIRC,	SPY_RAWIRC	},
{ SPYSTR_BOTNET,	SPY_BOTNET	},
#ifdef URLCAPTURE
{ SPYSTR_URL,		SPY_URL		},
#endif /* URLCAPTURE */
#ifdef HOSTINFO
{ SPYSTR_SYSMON,	SPY_SYSMON	},
#endif
{ SPYSTR_RANDSRC,	SPY_RANDSRC	},
{ NULL, 0 },
};

int spy_source(char *from, int *t_src, const char **src)
{
	int	i;

#ifdef DEBUG
	debug("(spy_source) t_src %i, src %s\n",*t_src,*src);
#endif /* ifdef DEBUG */

	for(i=0;spy_source_list[i].idstring;i++)
	{
		if (!stringcasecmp(*src,spy_source_list[i].idstring))
		{
			*src = spy_source_list[i].idstring;
			*t_src = spy_source_list[i].typenum;
			return(200);
		}
	}
	*t_src = SPY_CHANNEL;
	if (!ischannel(*src))
		return(-1);
	return(get_useraccess(from,*src));
}

#ifdef REDIRECT

int begin_redirect(char *from, char *args)
{
	char	*pt,*nick;

	if (!args)
		return(0);
	pt = STRCHR(args,'>');
	if (pt)
	{
		*pt = 0;
		nick = pt+1;
		pt--;
		while((pt > args) && (*pt == ' '))
		{
			*pt = 0;
			pt--;
		}
		while(*nick == ' ')
			nick++;
		if (*nick)
		{
#ifdef DEBUG
			debug("(begin_redirect) from %s --> %s\n",from,nick);
#endif /* DEBUG */
			if (ischannel(nick))
			{
				if (find_channel_ac(nick))
				{
					set_mallocdoer(begin_redirect);
					redirect.to = stringdup(nick);
					redirect.method = R_PRIVMSG;
					return(0);
				}
				else
				{
					to_user(from,ERR_CHAN,nick);
					return(-1);
				}
			}
			if (*nick == '>')
			{
				nick++;
				while(*nick == ' ')
					nick++;
				if (!*nick)
				{
					to_user(from,"Missing name for redirect.");
					return(-1);
				}
				if (is_safepath(nick,FILE_MAY_EXIST) != FILE_IS_SAFE) /* redirect output is appended */
				{
					to_user(from,"Bad filename.");
					return(-1);
				}
				set_mallocdoer(begin_redirect);
				redirect.to = stringdup(nick);
				redirect.method = R_FILE;
				return(0);
			}
			if ((pt = find_nuh(nick)))
			{
				set_mallocdoer(begin_redirect);
				redirect.to = stringdup(nick);
				redirect.method = R_NOTICE;
				return(0);
			}
			else
			{
				to_user(from,TEXT_UNKNOWNUSER,nick);
				return(-1);
			}
		}
		else
		{
			to_user(from,"Bad redirect");
			return(-1);
		}
	}
	return(0);
}

void send_redirect(char *message)
{
	Strp	*new,**pp;
	char	*fmt;
	int	fd;

	if (!redirect.to)
		return;

	switch(redirect.method)
	{
	case R_FILE:
		if ((fd = open(redirect.to,O_WRONLY|O_CREAT|O_APPEND,NEWFILEMODE)) < 0)
			return;
		fmt = stringcat(message,"\n");
		if (write(fd,message,(fmt-message)) == -1)
			return;

		close(fd);
		return;
#ifdef BOTNET
	case R_BOTNET:
		{
			char	tempdata[MAXLEN];
			Mech	*backup;

			/* PM<targetguid> <targetuserhost> <source> <message> */
			sprintf(tempdata,"%i %s %s %s",redirect.guid,redirect.to,current->nick,message);
			backup = current;
			partyMessage(NULL,tempdata);
			current = backup;
		}
		return;
#endif /* BOTNET */
	case R_NOTICE:
		fmt = "NOTICE %s :%s";
		break;
	/* case R_PRIVMSG: */
	default:
		fmt = "PRIVMSG %s :%s";
		break;
	}

	pp = &current->sendq;
	while(*pp)
		pp = &(*pp)->next;

	*pp = new = (Strp*)Calloc(sizeof(Strp) + StrlenX(message,fmt,redirect.to,NULL));
	/* Calloc sets to zero new->next = NULL; */
	sprintf(new->p,fmt,redirect.to,message);
}

void end_redirect(void)
{
	if (redirect.to)
		Free((char**)&redirect.to);
}

#endif /* REDIRECT */

#ifdef URLCAPTURE

char *urlhost(const char *url)
{
	char	copy[strlen(url)];
	const char *end,*beg,*dst;
	int	n = 0;

	beg = end = url;
	while(*end)
	{
		if (*end == '@')
			beg = end+1;
		else
		if (*end == '/')
		{
			if (n == 1)
				beg = end+1;
			else
			if (n == 2)
				break;
			n++;
		}
		end++;
	}
	stringcpy_n(copy,beg,(end-beg));
#ifdef DEBUG
	debug("(urlhost) host = %s\n",copy);
#endif
}

void urlcapture(const char *rest)
{
	Strp	*sp,*nx;
	char	*dest,url[MSGLEN];
	int	n;

	dest = url;
	while(*rest && *rest != ' ' && dest < url+MSGLEN-1)
		*(dest++) = *(rest++);
	*dest = 0;

#ifdef DEBUG
	debug("(urlcapture) URL = \"%s\"\n",url);
#endif /* ifdef DEBUG */
	urlhost(url);

	send_spy(SPYSTR_URL,"%s",url);

	if ((n = urlhistmax) < 0)
		return;

	prepend_strp(&urlhistory,url);

	for(sp=urlhistory;sp;sp=sp->next)
	{
		if (n <= 0)
		{
			purge_linklist((void**)&sp->next);
			return;
		}
		n--;
	}
}

#endif /* ifdef URLCAPTURE */

#ifdef STATS

void stats_loghour(Chan *chan, char *filename, int hour)
{
	ChanStats *stats;
	time_t	when;
	int	fd;

	if (!(stats = chan->stats))
		return;

	when = (now - (now % 3600));

	if ((fd = open(filename,O_WRONLY|O_APPEND|O_CREAT,NEWFILEMODE)) >= 0)
	{
		stats->userseconds += stats->users * (when - stats->lastuser);
		to_file(fd,"H %s %i %i %i %i\n",chan->name,hour,
			(stats->flags & CSTAT_PARTIAL) ? -stats->userseconds : stats->userseconds,
			stats->userpeak,stats->userlow);
		close(fd);
	}
	stats->LHuserseconds = stats->userseconds;
	stats->userseconds = 0;
	stats->lastuser = when;
	stats->flags = 0;
}

void stats_plusminususer(Chan *chan, int plusminus)
{
	ChanStats *stats;
	ChanUser *cu;

	if (!(stats = chan->stats))
	{
		set_mallocdoer(stats_plusminususer);
		chan->stats = stats = (ChanStats*)Calloc(sizeof(ChanStats)); /* Calloc sets memory to 0 */
		for(cu=chan->users;cu;cu=cu->next)
			stats->users++;
		stats->userpeak = stats->users;
		stats->userlow = stats->users;
		stats->lastuser = now;
		stats->flags = CSTAT_PARTIAL;
	}

	/*
	 *  add (number of users until now * seconds since last user entered/left)
	 */
	stats->userseconds += stats->users * (now - stats->lastuser);

	stats->lastuser = now;
	stats->users += plusminus;	/* can be both negative (-1), zero (0) and positive (+1) */

	if (stats->userpeak < stats->users)
		stats->userpeak = stats->users;
	if (stats->userlow > stats->users)
		stats->userlow = stats->users;

#ifdef DEBUG
	debug("(stats_plusminususer) %s: %i users, %i userseconds, %i high, %i low; %s (%lu)\n",
		chan->name,stats->users,stats->userseconds,stats->userpeak,stats->userlow,
		atime(stats->lastuser),stats->lastuser);
#endif /* DEBUG */
}

#endif /* ifdef STATS */

/*
 *
 *  commands related to spy-pipes
 *
 */

/*
help:SPY:[STATUS|MESSAGE|RAWIRC|URL|RANDSRC|[guid:|botnick:] [channel|> filename]

Spy on a certain source of messages. When you join DCC chat,
the STATUS source is added by default as a spy source for you.
If no arguments are given, the current list of active spy
channels is shown. Output is not line buffered and can cause
excess flood if not careful.

  (sources)
   STATUS     Status messages.
   MESSAGE    Pivate messages that the bot receives.
   RAWIRC     Lines received from irc server before processing.
   URL        URLs seen by the bot.
   RANDSRC    Produce random data from <RAWIRC>, can only output to file.
   guid:      Messages from a bot specified by guid.
   botnick:   Messages from a bot specified by nick.
   channel    Activities on the specified channel.

  (destinations)
   (none)     Send output to you (default).
   channel    Send output to the specified channel.
   >file      Send output to file. Lines are appended to the end of the file.

See also: rspy
*/
void do_spy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy;
	Mech	*backup,*destbot;
	const char *src;
	char	*dest;
	int	t_src,t_dest;
	int	sz,r,guid;

	if (!*rest)
	{
		if (!current->spylist)
		{
			to_user(from,"No active spy channels");
			return;
		}

		if (partyline_only_command(from))
			return;
		table_buffer(str_underline("source") "\t" str_underline("target"));
		for(spy=current->spylist;spy;spy=spy->next)
		{
			switch(spy->t_src)
			{
			case SPY_MESSAGE:
				src = "messages";
				break;
			default:
				src = spy->src;
			}
			dest = (spy->t_dest == SPY_FILE) ? " (file)" : "";
			table_buffer("%s\t%s%s",src,spy->dest,dest);
		}
		table_send(from,2);
		return;
	}

	src  = chop(&rest);
	dest = chop(&rest);

	if (!src)
	{
spy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	t_dest = SPY_DCC;
	guid = -1;
	destbot = NULL;

	if (*src >= '0' && *src <= '9')
	{
		guid = 0;
		while(*src && *src != ':')
		{
			guid = *src - '0';
			src++;
		}
		if (*src != ':' && !ischannel(src+1))
			goto spy_usage;
		src++;
		t_src = SPY_CHANNEL;
		/*
		 *  TODO: check access
		 */
#ifdef DEBUG
	debug("(do_spy) spy source guid = %i, channel = %s\n",guid,src);
#endif /* DEBUG */

		/*
		 *  is it a bot?
		 *  TODO: check botnet bots also
		 */
		for(backup=botlist;backup;backup=backup->next)
		{
			if (backup->guid == guid)
			{
				destbot = backup;
				goto guid_ok;
			}
		}
		to_user(from,"Unknown bot guid: %i",guid);
		return;
guid_ok:
		;
	}
	else
	{
		sz = spy_source(from,&t_src,&src);
		if (sz < 0) /* user has insufficient access to source */
			goto spy_usage;
		if (sz < cmdaccess) /* user has less access relative to source than the command level of SPY */
			return;
	}

	if (dest)
	{
		/*
		 *  log to a file
		 */
		if (*dest == '>')
		{
			/* accept both ">file" and "> file" */
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto spy_usage;
			}
			/*
			 *  Dont just open anything.
			 */
			r = is_safepath(dest,FILE_MAY_EXIST);
			if (r != FILE_IS_SAFE)
#ifdef DEBUG
			{
				debug("(do_spy) Filename \"%s\" was deemed unsafe (%i)\n",dest,r);
				goto spy_usage;
			}
#else
				goto spy_usage;
#endif /* DEBUG */
			t_dest = SPY_FILE;
			goto spy_dest_ok;
		}

		if (!ischannel(dest))
			goto spy_usage;
		if (get_useraccess(from,dest) < cmdaccess)
		{
			to_user(from,"You don't have enough access on %s",dest);
			return;
		}
		t_dest = SPY_CHANNEL;
	}

spy_dest_ok:
#ifdef DEBUG
	debug("(do_spy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],nullstr(dest),t_dest,SPY_DEFS[t_dest-1]);
	if (guid >= 0)
		debug("(do_spy) spying from remote bot guid %i (%s) channel %s\n",guid,(destbot) ? destbot->nick : "unknown",src);
#endif /* DEBUG */

	if (t_dest == SPY_DCC)
	{
		if (!CurrentDCC)
		{
			to_user(from,"Spying is only allowed in DCC chat");
			return;
		}
		dest = CurrentDCC->user->name;
	}

	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) &&
			!stringcasecmp(spy->src,src) && !stringcasecmp(spy->dest,dest))
		{
			to_user(from,"Requested spy channel is already active");
			return;
		}
	}

	if (t_src == SPY_RANDSRC && t_dest != SPY_FILE)
	{
		to_user(from,"Randsrc data can only be written to a file.");
		return;
	}

	set_mallocdoer(do_spy);

	sz = sizeof(Spy);

	if (t_dest != SPY_DCC)
		sz += strlen(dest);

	if (t_src == SPY_CHANNEL)
		sz += strlen(src);

	spy = Calloc(sz);

	if (t_dest != SPY_DCC)
	{
		spy->dest = spy->p;
		spy->src = stringcat(spy->p,dest) + 1;
	}
	else
	{
		spy->dest = CurrentDCC->user->name;
		spy->data.dcc = CurrentDCC;
		spy->src = spy->p;
	}

	if (t_src == SPY_CHANNEL)
	{
		stringcpy((char*)spy->src,src);
	}
	else
	{
		spy->src = src;
	}

	spy->t_src = t_src;
	spy->t_dest = t_dest;

	/*
	 *  finally link the spy record into the chain
	 *  TODO: botnet bots
	 */
	if (guid >= 0)
	{
		if (destbot)
		{
			spy->data.destbot = current->guid;
			spy->next = destbot->spylist;
			destbot->spylist = spy;
			spy_typecount(destbot);
		}
	}
	else
	{
		spy->data.destbot = -1;
		spy->next = current->spylist;
		current->spylist = spy;
		spy_typecount(current);
	}
	if (t_src == SPY_RANDSRC)
		spy->data.delay = 0;

	switch(t_src)
	{
	case SPY_STATUS:
		send_spy(SPYSTR_STATUS,"(%s) Added to mech core",nickcpy(NULL,from));
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been activated",src);
	}
}

void do_rspy(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Spy	*spy,**pspy;
	const char *src;
	char	*dest,*tmp;
	int	t_src,t_dest;
	int	n;

	src  = chop(&rest);
	dest = chop(&rest);

	t_dest = SPY_DCC;

	if (!src)
	{
rspy_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	n = spy_source(from,&t_src,&src);
	if (n < 0)
		goto rspy_usage;
	if (n < cmdaccess)
		return;

	if (dest)
	{
		if (*dest == '>')
		{
			dest++;
			if (!*dest)
			{
				dest = chop(&rest);
				if (!dest || !*dest)
					goto rspy_usage;
			}
			/*
			 *   this is about removing an existing spy channel
			 *   filename does not need to be checked because
			 *   - if its a bogus filename, it wont match any open spy channels
			 *   - if its a matching filename, its going to be removed right now
			 */
			t_dest = SPY_FILE;
			goto rspy_dest_ok;
		}

		if (ischannel(dest))
			t_dest = SPY_CHANNEL;
	}
	else
		dest = from;

rspy_dest_ok:
#ifdef DEBUG
	debug("(do_rspy) src = `%s'; t_src = %i (%s); dest = `%s'; t_dest = %i (%s)\n",
		src,t_src,SPY_DEFS[t_src-1],dest,t_dest,SPY_DEFS[t_dest-1]);
#endif /* DEBUG */

	/*
	 *  check if the spy channel exists
	 */
	for(spy=current->spylist;spy;spy=spy->next)
	{
		if ((spy->t_src == t_src) && (spy->t_dest == t_dest) && (!stringcasecmp(spy->src,src)))
		{
			if ((t_dest == SPY_DCC) && (!nickcmp(spy->dest,dest)))
				break;
			else
			if (!stringcasecmp(spy->dest,dest))
				break;
		}
	}
	if (!spy)
	{
		to_user(from,"No matching spy channel could be found");
		return;
	}

	switch(t_src)
	{
	case SPY_STATUS:
		tmp = (t_dest == SPY_DCC) ? nickcpy(NULL,spy->dest) : spy->dest;
		send_spy(SPYSTR_STATUS,"(%s) Removed from mech core",tmp);
		break;
	case SPY_MESSAGE:
		src = "messages";
	default:
		to_user(from,"Spy channel for %s has been removed",src);
		break;
	}

	pspy = &current->spylist;
	while(*pspy)
	{
		if (*pspy == spy)
		{
			*pspy = spy->next;
			Free((char**)&spy);
			return;
		}
		pspy = &(*pspy)->next;
	}
	spy_typecount(current);
}

#ifdef URLCAPTURE

/*
help:URLHIST:[max]

Display a list of URLs seen by the bot in order most recent to oldest.

   [max]   Maximum number of URLs to display.
*/
void do_urlhist(COMMAND_ARGS)
{
	Strp	*sp;
	char	*thenum;
	int	n,maxnum;

	if (urlhistory == NULL)
	{
		to_user(from,"No URLs recorded.");
		return;
	}

	if (!rest || !*rest)
		maxnum = urlhistmax;
	else
	{
		thenum = chop(&rest);
		maxnum = asc2int(thenum);
	}
	if ((maxnum < 1) || (maxnum > urlhistmax))
		usage(from);    /* usage for CurrentCmd->name */

	n = 1;
	for(sp=urlhistory;sp;sp=sp->next)
	{
		if (n > maxnum)
			break;
		to_user(from,"[%i] %s",n,sp->p);
		n++;
	}
}

#endif /* URLCAPTURE */

#ifdef STATS

void do_info(COMMAND_ARGS)
{
	ChanStats *stats;
	Chan	*chan;
	char	*p;
	char	text[MSGLEN];
	uint32_t avg;

	if (current->chanlist == NULL)
	{
		to_user(from,ERR_NOCHANNELS);
		return;
	}
	to_user(from,"\037channel\037                            "
		"\037average\037 \037peak\037 \037low\037");
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		*(p = text) = 0;
		p = stringcat(p,chan->name);
		if (chan == current->activechan)
			p = stringcat(p," (current)");
		if ((stats = chan->stats))
		{
			if (stats && stats->flags == CSTAT_PARTIAL)
				p = stringcat(p," (partial)");
			while(p < text+35)
				*(p++) = ' ';
			if (stats->LHuserseconds > 0)
			{
				avg = stats->LHuserseconds / (60*60);
			}
			else
			{
				avg = (stats->userpeak + stats->userlow) / 2;
			}
			sprintf(p,"%-7u %-4i %i",avg,stats->userpeak,stats->userlow);
			to_user(from,FMT_PLAIN,text);
			sprintf(text,"Messages: %i   Notices: %i   Joins: %i   Parts: %i   Kicks: %i   Quits: %i",
				stats->privmsg,stats->notice,stats->joins,stats->parts,stats->kicks,stats->quits);
		}
		else
		{
			stringcpy(p," (no current data)");
		}
		to_user(from,FMT_PLAIN,text);
	}
}

#endif /* STATS */

