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
#define CHANNEL_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *  runs ever RESETINTERVAL (90) seconds from update()
 */
void check_idlekick(void)
{
	ChanUser *cu;
	Chan	*chan;
	time_t	timeout;
	int	limit;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		limit = chan->setting[INT_IKT].int_var;
		timeout = (now - (60 * limit));
		for(cu=chan->users;cu;cu=cu->next)
		{
			cu->flags &= ~CU_KSWARN;	/* remove KS warnings */
			if (!chan->bot_is_op || limit == 0)
				continue;
			if (cu->flags & CU_CHANOP)
				continue;
			if (timeout < cu->idletime)
				continue;
			if (get_useraccess(get_nuh(cu),chan->name))
				continue;
			send_kick(chan,cu->nick,"Idle for %i minutes",limit);
		}
	}
}

Chan *find_channel(const char *name, int anychannel)
{
	Chan	*chan;
	uchar	ni;

	ni = tolowertab[(uchar)(name[1])];

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (chan->active < anychannel)
			continue;
		if (ni != tolowertab[(uchar)(chan->name[1])])
			continue;
		if (!stringcasecmp(name,chan->name))
			return(chan);
	}
	return(NULL);
}

Chan *find_channel_ac(const char *name)
{
	return(find_channel(name,CHAN_ACTIVE));
}

Chan *find_channel_ny(const char *name)
{
	return(find_channel(name,CHAN_ANY));
}

void remove_chan(Chan *chan)
{
	Chan	**pp;

	pp = &current->chanlist;
	while(*pp)
	{
		if (*pp == chan)
		{
			*pp = chan->next;
			purge_linklist((void**)&chan->banlist);
			purge_chanusers(chan);
			delete_vars(chan->setting,CHANSET_SIZE);
			Free(&chan->name);
			Free(&chan->key);
			Free(&chan->topic);
			Free(&chan->kickedby);
			Free((char **)&chan);
			return;
		}
		pp = &(*pp)->next;
	}
}

void join_channel(char *name, char *key)
{
	Chan	*chan;

	if (!ischannel(name))
		return;

	if ((chan = find_channel_ny(name)) == NULL)
	{
		set_mallocdoer(join_channel);
		chan = (Chan*)Calloc(sizeof(Chan));
		set_mallocdoer(join_channel);
		chan->name = stringdup(name);
		if (key)
		{
			set_mallocdoer(join_channel);
			chan->key = stringdup(key);
		}
		copy_vars(chan->setting,current->setting);
		chan->next = current->chanlist;
		chan->rejoin = TRUE;
		chan->active = FALSE;
		current->chanlist = chan;
		current->rejoin = TRUE;
		if (current->sock == -1)
		{
			current->activechan = chan;
			chan->sync = TRUE;
		}
		else
		{
			to_server("JOIN %s %s\n",name,(key && *key) ? key : "");
			chan->sync = FALSE;
		}
		return;
	}
	if (key && (key != chan->key))
	{
		Free(&chan->key);
		set_mallocdoer(join_channel);
		chan->key = stringdup(key);
	}
	if (chan->active)
	{
		current->activechan = chan;
		return;
	}
	/*
	 *  If its not CH_ACTIVE, its CH_OLD; there are only those 2 states.
	 */
	if (current->sock >= 0 && chan->sync)
	{
		to_server("JOIN %s %s\n",name,(key) ? key : "");
		chan->sync = FALSE;
	}
	chan->rejoin = TRUE;
	current->rejoin = TRUE;
}

void reverse_topic(Chan *chan, char *from, char *topic)
{
	if ((chan->setting[TOG_TOP].int_var) &&
		(get_useraccess(from,chan->name) < ASSTLEVEL))
	{
		if (chan->topic && stringcasecmp(chan->topic,topic))
			to_server("TOPIC %s :%s\n",chan->name,chan->topic);
		return;
	}

	Free((char**)&chan->topic);
	set_mallocdoer(reverse_topic);
	chan->topic = stringdup(topic);
}

void cycle_channel(Chan *chan)
{
	if (!chan->sync)
		return;
	chan->rejoin = TRUE;
	to_server("PART %s\nJOIN %s %s\n",chan->name,
		chan->name,(chan->key) ? chan->key : "");
}

int reverse_mode(char *from, Chan *chan, int m, int s)
{
	char	buffer[100];
	char	*ptr,*ptr2;
	char	mode,sign;

	if (!chan->bot_is_op || !chan->setting[TOG_ENFM].int_var ||
	    ((ptr = chan->setting[STR_ENFMODES].str_var) == NULL))
		return(FALSE);

	mode = (char)m;
	sign = (char)s;

	if (STRCHR(ptr,mode) && (sign == '+'))
		return(FALSE);
	if (!STRCHR(ptr,mode) && (sign == '-'))
		return(FALSE);
	if (get_useraccess(from,chan->name) >= ASSTLEVEL)
	{
		ptr2 = buffer;
		if (sign == '-')
		{
			while(*ptr)
			{
				if (*ptr != mode)
					*ptr2++ = *ptr;
				ptr++;
			}
			*ptr2 = 0;
		}
		else
		{
			buffer[0] = mode;
			buffer[1] = 0;
			stringcat(buffer,ptr);
		}
		set_str_varc(chan,STR_ENFMODES,buffer);
		return(FALSE);
	}
	return(TRUE);
}

void chan_modestr(Chan *chan, char *dest)
{
	*(dest++) = '+';
	if (chan->private)
		*(dest++) = 'p';
	if (chan->secret)
		*(dest++) = 's';
	if (chan->moderated)
		*(dest++) = 'm';
	if (chan->topprot)
		*(dest++) = 't';
	if (chan->invite)
		*(dest++) = 'i';
	if (chan->nomsg)
		*(dest++) = 'n';
	if (chan->limitmode && chan->limit)
		*(dest++) = 'l';
	if (chan->keymode)
		*(dest++) = 'k';
	*dest = 0;

	if ((chan->limitmode) && (chan->limit))
	{
		sprintf(dest," %i",chan->limit);
	}
	if (chan->keymode)
	{
		stringcat(dest," ");
		stringcat(dest,(chan->key) ? chan->key : "???");
	}
}

char *find_nuh(char *nick)
{
	Chan	*chan;
	ChanUser *cu;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,nick)))
			return(get_nuh(cu));
	}
	return(NULL);
}

Ban *make_ban(Ban **banlist, char *from, char *banmask, time_t when)
{
	Ban	*new;
	int	sz;

	for(new=*banlist;new;new=new->next)
	{
		if (!stringcasecmp(new->banstring,banmask))
			return(NULL);
	}

	sz = sizeof(Ban) + Strlen2(from,banmask); /* banmask is never NULL */

	set_mallocdoer(make_ban);
	new = (Ban*)Calloc(sz);

	new->bannedby = stringcpy(new->banstring,banmask) + 1;
	stringcpy(new->bannedby,from);

	new->time = when;
	new->next = *banlist;
	*banlist  = new;
	return(new);
}

void delete_ban(Chan *chan, char *banmask)
{
	Ban	*ban,**pp;

	pp = &chan->banlist;
	while(*pp)
	{
		ban = *pp;
		if (!stringcasecmp(ban->banstring,banmask))
		{
			*pp = ban->next;
			Free((char**)&ban);
			return;
		}
		pp = &(*pp)->next;
	}
}

#ifdef IRCD_EXTENSIONS

void delete_modemask(Chan *chan, char *mask, int mode)
{
	Ban	*ban,**pp;

	pp = &chan->banlist;
	while(*pp)
	{
		ban = *pp;
		if ((mode == 'I' && ban->imode == TRUE) ||
		    (mode == 'e' && ban->emode == TRUE))
		{
			if (!stringcasecmp(ban->banstring,mask))
			{
				*pp = ban->next;
				Free((char**)&ban);
				return;
			}
		}
		pp = &(*pp)->next;
	}
}

#endif /* IRCD_EXTENSIONS */

void channel_massmode(const Chan *chan, char *pattern, int filtmode, char mode, char typechar)
{
	ChanUser *cu;
	char	*pat,*uh,burst[MSGLEN],deopstring[MSGLEN];
	int	i,maxmode,mal,willdo,uaccess,ispat;

	if ((pat = chop(&pattern)) == NULL)
		return;

	ispat   = (STRCHR(pat,'*')) ? TRUE : FALSE;
	maxmode = current->setting[INT_MODES].int_var;
	mal     = chan->setting[INT_MAL].int_var;
	*burst  = 0;

#ifdef DEBUG
	debug("(...) sign %c, mode %c, maxmode %i\n",mode,typechar,maxmode);
#endif /* DEBUG */
	for(cu=chan->users;cu;)
	{
		uh = deopstring + sprintf(deopstring,"MODE %s %c",chan->name,mode);
#ifdef DEBUG
		{
			char *s;
			s = deopstring;
			while(*s) s++;
			debug("(...) deopstring "mx_pfmt" uh "mx_pfmt" ("mx_pfmt")\n",(mx_ptr)deopstring,(mx_ptr)uh,(mx_ptr)s);
			s = STRCHR(deopstring,0);
			debug("(...) deopstring "mx_pfmt" uh "mx_pfmt" ("mx_pfmt")\n",(mx_ptr)deopstring,(mx_ptr)uh,(mx_ptr)s);
		}
#endif /* DEBUG */
		i = maxmode;
		do {
			*uh = typechar;
			uh++;
		}
		while(--i);
		*uh = 0;
#ifdef DEBUG
		debug("(...) %s\n",deopstring);
#endif /* DEBUG */

		/* i == 0 from the while loop */
		while(cu && (i < maxmode))
		{
			willdo = FALSE;
			if ((mode == '+') && ((cu->flags & filtmode) == 0))
				willdo = TRUE;
			if ((mode == '-') && ((cu->flags & filtmode) != 0))
				willdo = TRUE;
#ifdef DEBUG
			uaccess = 0;
#endif /* DEBUG */
			if (willdo)
			{
				willdo = FALSE;
				uh = get_nuh(cu);
				uaccess = get_useraccess(uh,chan->name);
				if (ispat)
				{
					if (!matches(pat,uh))
					{
						if (typechar == 'v')
							willdo = TRUE;
						if ((mode == '+') && (uaccess >= mal))
							willdo = TRUE;
						if ((mode == '-') && (uaccess < mal))
							willdo = TRUE;
					}
				}
				else
				if (!nickcmp(pat,cu->nick))
				{
					if (mode == '-')
					{
						/*
						 *  never deop yourself, stupid bot
						 */
						if (nickcmp(pat,current->nick))
							willdo = TRUE;
					}
					else
						willdo = TRUE;
				}
			}
#ifdef DEBUG
			else
			{
				uh  = get_nuh(cu);
			}
			debug("(massmode(2)) willdo = %s (%s[%i]) (pat=%s)\n",
				(willdo) ? "TRUE" : "FALSE",uh,uaccess,pat);
#endif /* DEBUG */
			if (willdo && ((cu->flags & CU_MASSTMP) == 0))
			{
				stringcat(deopstring," ");
				stringcat(deopstring,cu->nick);
				cu->flags |= CU_MASSTMP;
				i++;
			}

			cu = cu->next;
			if (!cu && (pat = chop(&pattern)))
			{
				ispat = (STRCHR(pat,'*')) ? TRUE : FALSE;
				cu = chan->users;
			}
		}

		if (i)
		{
			if ((Strlen2(deopstring,burst)) >= MSGLEN-2) /* burst is never NULL */
			{
				if (write(current->sock,burst,strlen(burst)) == -1)
					return;
#ifdef DEBUG
				debug("(channel_massmode)\n%s\n",burst);
#endif /* DEBUG */
				*burst = 0;
			}
			stringcat(burst,deopstring);
			stringcat(burst,"\n");
		}
	}

	if (strlen(burst))
	{
		if (write(current->sock,burst,strlen(burst)) == -1)
			return;
#ifdef DEBUG
		debug("(...)\n%s\n",burst);
#endif /* DEBUG */
	}

	for(cu=chan->users;cu;cu=cu->next)
		cu->flags &= ~CU_MASSTMP;
}

void channel_massunban(Chan *chan, char *pattern, time_t seconds)
{
	Shit	*shit;
	Ban	*ban;
	int	pri;

	pri = (seconds) ? 180 : 90;

	for(ban=chan->banlist;ban;ban=ban->next)
	{
		if (!matches(pattern,ban->banstring) || !matches(ban->banstring,pattern))
		{
			if (!seconds || ((now - ban->time) > seconds))
			{
				if (chan->setting[TOG_SHIT].int_var)
				{
					shit = find_shit(ban->banstring,chan->name);
					if (shit && shit->action > 2)
						continue;
				}
				send_mode(chan,pri,QM_RAWMODE,'-','b',(void*)ban->banstring);
			}
		}
	}
}

/*
 *  Channel userlist stuff
 */

/*
 *  this is one of the big cpu hogs in the energymech.
 *  its been debugged a whole lot and probably cant be
 *  made much better without big changes.
 *
 *  cache hit ratio (%): 2 - 4    (smaller channels)
 *                       4 - 10   (larger channels)
 *
 *  nickcmp is called for an average of 1/19th of all chanusers,
 *  the rest is avoided with the first-char comparison.
 *
 *  for each nickcmp call, 10-15% cpu is saved by skipping one char
 *  into both nicks (first-char comparison has already been made).
 */
ChanUser *find_chanuser(Chan *chan, const char *nick)
{
	ChanUser *cu;
	uchar	ni;

	/*
	 *  small quick'n'dirty cache
	 */
	if (chan->cacheuser && !nickcmp(nick,chan->cacheuser->nick))
		return(chan->cacheuser);

	/*
	 *  avoid calling nickcmp if first char doesnt match
	 */
	ni = nickcmptab[(uchar)(*nick)];
	nick++;

	/*
	 *  hog some cpu...
	 */
	for(cu=chan->users;cu;cu=cu->next)
	{
		if (ni == nickcmptab[(uchar)(*cu->nick)])
		{
			if (!nickcmp(nick,cu->nick+1))
				return(chan->cacheuser = cu);
		}
	}
	return(NULL);
}

#ifdef BOTNET

ChanUser *find_chanbot(Chan *chan, const char *nick)
{
	ChanUser *cu;

	if (chan->cacheuser && !nickcmp(nick,chan->cacheuser->nick)
		&& chan->cacheuser->user && chan->cacheuser->user->x.x.access == BOTLEVEL)
		return(chan->cacheuser);

	for(cu=chan->users;cu;cu=cu->next)
	{
		if (cu->user && cu->user->x.x.access == BOTLEVEL)
		{
			if (!nickcmp(nick,cu->nick))
				return(chan->cacheuser = cu);
		}
	}
	return(NULL);
}

#endif /* BOTNET */

void remove_chanuser(Chan *chan, const char *nick)
{
	ChanUser *cu,**pp;
	uchar	ni;

	/*
	 *  avoid calling stringcasecmp if first char doesnt match
	 */
	ni = nickcmptab[(uchar)(*nick)];
	nick++;

	/*
	 *  Dont call find_chanuser() because it caches the found user
	 *  and we dont want to cache a user who quits/parts/is kicked...
	 */
	pp = &chan->users;
	while(*pp)
	{
		cu = *pp;
		if (ni == nickcmptab[(uchar)(*cu->nick)])
		{
			if (!nickcmp(nick,cu->nick+1))
			{
				if (cu == chan->cacheuser)
					chan->cacheuser = NULL;
				*pp = cu->next;
				/*
				 *  the mode-queue might contain a reference to this
				 *  chanuser, remove it.
				 */
				unmode_chanuser(chan,cu);
				/*
				 *  byebye chanuser
				 */
				Free((char**)&cu->nick);
				Free((char**)&cu);
				return;
			}
		}
		pp = &cu->next;
	}
}

/*
 *  Requires CurrentChan to be set properly
 */
void make_chanuser(char *nick, char *userhost)
{
	ChanUser *new;

	/*
	 *  malloc ChanUser record with buffer space for user and host in
	 *  a single chunk and calculate the offsets for the strings
	 */
	set_mallocdoer(make_chanuser);
	new = (ChanUser*)Calloc(sizeof(ChanUser) + strlen(userhost));
	/* Calloc sets it all to zero */

	new->idletime = now;
	new->next = CurrentChan->users;
	CurrentChan->users = new;
	stringcpy(new->userhost,userhost);

	/*
	 *  nick can change without anything else changing with it
	 */
	set_mallocdoer(make_chanuser);
	new->nick = stringdup(nick);
}

void purge_chanusers(Chan *chan)
{
	while(chan->users)
		remove_chanuser(chan,chan->users->nick);
}

char *get_nuh(const ChanUser *user)
{
	sprintf(nuh_buf,"%s!%s",user->nick,user->userhost);
	return(nuh_buf);
}

/*
 *
 *  commands associated with channels
 *
 */

/*
help:JOIN
usage:JOIN <#channel> [key]
file:../help/JOIN
begin:

Makes the bot join a channel

See also: cycle, part
:end
*/
void do_join(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*channel,*key;

	channel = chop(&rest);
	if (!ischannel(channel))
	{
		to_user(from,TEXT_CHANINVALID);
		return;
	}
	if (get_authaccess(from,channel) < cmdaccess)
		return;
	to_user(from,"Attempting the join of %s",channel);
	key = chop(&rest);
	join_channel(channel,key);
}

/*
help:PART
usage:PART <#channel>
file:../help/PART
begin:
:end
*/
void do_part(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS ACCHAN
	 */
	Chan	*chan = CurrentChan;

	chan->rejoin = FALSE;
	chan->active = FALSE;
	to_user(from,"Parting %s",to);
	to_server("PART %s\n",to);
	if (chan == current->activechan)
	{
		for(chan=current->chanlist;chan;chan=chan->next)
			if (chan->active)
				break;
		current->activechan = chan; /* might be NULL */
	}
}

/*
help:CYCLE
usage:CYCLE [#channel]
file:../help/CYCLE
begin:
:end
*/
void do_cycle(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	to_user(from,"Cycling channel %s",to);
	cycle_channel(CurrentChan);
}

/*
help:FORGET
usage:FORGET <#channel>
file:../help/FORGET
begin:
:end
*/
void do_forget(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Chan	*chan;
	char	*channel;

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if ((chan = find_channel_ny(channel)) == NULL)
	{
		to_user(from,"Channel %s is not in memory",channel);
		return;
	}
	if (chan->active)
	{
		to_user(from,"I'm currently active on %s",channel);
		return;
	}
	to_user(from,"Channel %s is now forgotten",channel);
	remove_chan(chan);
}

/*
help:CHANNELS
usage:CHANNELS (no arguments)
file:../help/CHANNELS
begin:
:end
*/
void do_channels(COMMAND_ARGS)
{
	ChanUser *cu;
	Chan	*chan;
	char	text[MSGLEN];
	char	*p;
	int	u,o,v;

	if (current->chanlist == NULL)
	{
		to_user(from,ERR_NOCHANNELS);
		return;
	}
	table_buffer(str_underline("channel") "\t" str_underline("@") "\t" str_underline("users") "\t" str_underline("ops") "\t" str_underline("voiced") "\t" str_underline("modes"));
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		p = stringcpy(text,chan->name);
		if (chan == current->activechan)
			p = stringcat(p," (current)");
		if (chan->active)
		{
			u = o = v = 0;
			for(cu=chan->users;cu;cu=cu->next)
			{
				u++;
				if (cu->flags & CU_CHANOP)
					o++;
				else
				if (cu->flags & CU_VOICE)
					v++;
			}
			sprintf(p,"\t %c\t%i\t%i\t%i\t",(chan->bot_is_op) ? '@' : ' ',u,o,v);
			p = STREND(p);
			chan_modestr(chan,p);
		}
		else
		{
			sprintf(p,"\t\t--\t--\t--\t%s",(chan->rejoin) ? "(Trying to rejoin...)" : "(Inactive)");
		}
		table_buffer(FMT_PLAIN,text);
	}
	table_send(from,1);
}

/*
help:WALL
usage:WALL [#channel] <message>
file:../help/WALL
begin:
:end
*/
void do_wall(COMMAND_ARGS)
{
	ChanUser *cu;

#ifdef IRCD_EXTENSIONS
	if(current->ircx_flags & IRCX_WALLCHOPS)
	{
		to_server("WALLCHOPS %s :[Wallop/%s] %s\n",to,to,rest);
	}
	else
#endif /* IRCD_EXTENSIONS */
	if (current->setting[TOG_ONOTICE].int_var)
	{
		to_server("NOTICE @%s :[Wallop/%s] %s\n",to,to,rest);
	}
	else
	{
		cu = CurrentChan->users;
		CurrentChan = NULL;
		for(;cu;cu=cu->next)
		{
			if (cu->flags & CU_CHANOP)
			{
				if (cu->user == NULL || cu->user->x.x.access < BOTLEVEL)
					to_user_q(cu->nick,"[Wallop/%s] %s\n",to,rest);
			}
		}
	}
	to_user(from,TEXT_SENTWALLOP,to);
}

/*
help:MODE
usage:MODE [#channel|botnick] <mode ...>
file:../help/MODE
begin:
:end
*/
void do_mode(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*target;
	int	uaccess;

#ifdef DEBUG
	debug("(do_mode) to = %s, rest = %s\n",nullstr(to),nullstr(rest));
#endif /* DEBUG */

	if ((uaccess = get_authaccess(from,to)) < cmdaccess)
		return;
	if (ischannel(to))
	{
		to_server("MODE %s %s\n",to,rest);
	}
	else
	{
		target = chop(&rest);

		if (!nickcmp(current->nick,target))
		{
			to_server("MODE %s %s\n",target,rest);
		}
	}
}

/*
help:NAMES
usage:NAMES [#channel]
file:../help/NAMES
begin:
:end
*/
void do_names(COMMAND_ARGS)
{
	ChanUser *cu;
	Chan	*chan;
	char	*p,names[MSGLEN];
	const char *tochan;

	tochan = get_channel(to,&rest);
	if ((chan = find_channel_ny(tochan)) == NULL)
	{
		to_user(from,ERR_CHAN,tochan);
		return;
	}

	to_user(from,"Names on %s%s:",tochan,(chan->active) ? "" : " (from memory)");
	for(cu=chan->users;cu;)
	{
		p = names;

		while(cu && ((p - names) < 60))
		{
			if (p > names)
				*(p++) = ' ';

			if ((cu->flags) & CU_CHANOP)
				*(p++) = '@';
			else
			if ((cu->flags) & CU_VOICE)
				*(p++) = '+';

			p = stringcpy(p,cu->nick);
			cu = cu->next;
		}

		if (p > names)
		{
			*p = 0;
			to_user(from,FMT_PLAIN,names);
		}
	}
}

/*
help:CCHAN
usage:CCHAN [#channel]
file:../help/CCHAN
begin:
:end
*/
void do_cchan(COMMAND_ARGS)
{
	Chan	*chan;
	char	*channel;

	if (*rest)
	{
		channel = chop(&rest);
		if ((chan = find_channel_ac(channel)) != NULL)
		{
			current->activechan = chan;
			to_user(from,"Current channel set to %s",chan->name);
		}
		else
			to_user(from,ERR_CHAN,channel);
		return;
	}
	to_user(from,"Current channel: %s",
		(current->activechan) ? current->activechan->name : TEXT_NONE);
}

/*
help:INVITE
usage:INVITE [#channel] [nick]
file:../help/INVITE
begin:
:end
*/
void do_invite(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + ACCHAN
	 */
	char	*nick;

	nick = (*rest) ? rest : CurrentNick;

	while(nick && *nick)
		to_server("INVITE %s %s\n",chop(&nick),to);

	to_user(from,"User(s) invited to %s",to);
}

/*
help:ME
usage:ME [#channel] <message>
file:../help/ME
begin:
:end
*/
/*
help:SAY
usage:SAY [#channel] <message>
file:../help/SAY
begin:
:end
*/
void do_sayme(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS
	 */

	if (!ischannel(to))
		to = from;

	if (ischannel(rest))
	{
		to = chop(&rest);
		/*
		 *  need to check channel access
		 */
		if (get_authaccess(from,to) < cmdaccess)
			return;
	}
	else
	if (CurrentDCC && current->activechan)
	{
		to = current->activechan->name;
	}

	CoreChan.name = to;
	CurrentChan = (Chan*)&CoreChan;
	to_user_q(from,(CurrentCmd->name == C_SAY) ? FMT_PLAIN : "\001ACTION %s\001",rest);
}

/*
help:WHO
usage:WHO
file:../help/WHO
begin:
:end
*/
void do_who(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS
	 */
	Chan	*chan;
	ChanUser *cu;
	char	*pattern,*nuh;
	char	modechar,thechar;
	int	umode,uaccess,flags;

	flags = 0;
	pattern = nuh = NULL;
	if (*rest)
	{
		pattern = chop(&rest);
		if (*pattern == '-')
		{
			nuh = pattern;
			pattern = chop(&rest);
		}
		else
		if (*rest == '-')
			nuh = chop(&rest);

		if (nuh)
		{
			if (!stringcasecmp(nuh,"-ops"))
				flags = 1;
			else
			if (!stringcasecmp(nuh,"-nonops"))
				flags = 2;
			else
			{
				usage(from);	/* usage for CurrentCmd->name */
				return;
			}
		}
	}

	if ((chan = find_channel_ny(to)) == NULL)
	{
		to_user(from,"I have no information on %s",to);
		return;
	}
	table_buffer(str_underline("Users on %s%s"),to,(chan->active) ? "" : " (from memory)");

	if (chan->users)
	{
		thechar = 0;
		for(cu=chan->users;cu;cu=cu->next)
		{
			umode = cu->flags;

			if ((flags == 1) && !(umode & CU_CHANOP))
				continue;
			if ((flags == 2) && (umode & CU_CHANOP))
				continue;

			nuh = get_nuh(cu);
			if (pattern && matches(pattern,nuh))
				continue;

			modechar = (umode & CU_CHANOP) ? '@' : ((umode & CU_VOICE) ? '+' : ' ');

			thechar = 'u';
			uaccess = get_useraccess(nuh,to);
			if (!uaccess)
			{
				if ((uaccess = get_shitaction(nuh,to)) != 0)
					thechar = 's';
			}
			else
			if (uaccess == 200)
				thechar = 'b';

			table_buffer("\r%i%c %c \r%s\t %s",uaccess,thechar,modechar,cu->nick,cu->userhost);
		}
		if (!thechar)
			table_buffer("No matching users found");
	}
	table_send(from,0);
}

/*
help:
usage:
file:../help/
begin:
:end
*/
void do_topic(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	Chan	*chan = CurrentChan;

	if (chan->bot_is_op || !chan->topprot)
	{
		to_server("TOPIC %s :%s\n",to,rest);
		to_user(from,TEXT_TOPICCHANGED,to);
		return;
	}
	to_user(from,ERR_NOTOPPED,to);
}

/*
help:
usage:
file:../help/
begin:
:end
*/
void do_showidle(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	ChanUser *cu;
	Chan	*chan = CurrentChan;
	int	n;

	n = 10;
	if (*rest)
	{
		n = asc2int(chop(&rest));
		if (errno)
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}

	table_buffer(str_underline("Users on %s that are idle more than %i seconds"),chan->name,n);
	for(cu=chan->users;cu;cu=cu->next)
	{
		if (n >= (now - cu->idletime))
			continue;
		table_buffer("%s\r %s\t%s",idle2str((now - cu->idletime),TRUE),cu->nick,cu->userhost);
        }
	table_send(from,1);
}

/*
help:
usage:
file:../help/
begin:
:end
*/
void do_idle(COMMAND_ARGS)
{
	ChanUser *cu,*cu2;
	Chan	*chan;

	cu2 = NULL;
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,rest)))
		{
			if (!cu2 || (cu->idletime < cu2->idletime))
				cu2 = cu;
		}
	}
	if (!cu2)
	{
		to_user(from,TEXT_UNKNOWNUSER,rest);
		return;
	}
	to_user(from,"%s has been idle for %s",rest,idle2str(now - cu2->idletime,TRUE));
}
