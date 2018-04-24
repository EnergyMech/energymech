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
#define PROTECTION_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

/*
 *
 *  kicking and screaming
 *
 */

void send_kick(Chan *chan, const char *nick, const char *format, ...)
{
	qKick	*new,**pp;
	va_list	vargs;

	/*
	 *  globaldata safe to use since we're a `tail' function
	 */
	va_start(vargs,format);
	vsprintf(globaldata,format,vargs);
	va_end(vargs);

	pp = &chan->kicklist;
	while(*pp)
		pp = &(*pp)->next;

	set_mallocdoer(send_kick);
	*pp = new = (qKick*)Calloc(sizeof(qKick) + Strlen2(nick,globaldata)); /* globaldata is never NULL */
	/* Calloc sets to zero new->next = NULL; */

	new->reason = stringcpy(new->nick,nick) + 1;
	stringcpy(new->reason,globaldata);
}

void push_kicks(Chan *chan)
{
	qKick	*kick;
	int	n;

	n = (current->sendq_time - now);
	while(n < 6)
	{
		if ((kick = chan->kicklist) == NULL)
			return;

		chan->kicklist = kick->next;
		to_server("KICK %s %s :%s\n",chan->name,kick->nick,kick->reason);
		Free((char**)&kick);
		n += 2;
	}
}

void unmode_chanuser(Chan *chan, ChanUser *cu)
{
	qMode	*mode,**pp;

	pp = &chan->modelist;
	while(*pp)
	{
		mode = *pp;
		if ((mode->type == QM_CHANUSER) && (mode->data == (void*)cu))
		{
			*pp = mode->next;
			Free((char**)&mode);
			/*
			 *  there might be more modes associated with this chanuser
			 */
			continue;
		}
		pp = &mode->next;
	}
}

void send_mode(Chan *chan, int pri, int type, char plusminus, char modeflag, void *data)
{
	qMode	*mode,**pp;

#ifdef NO_DEBUG
	debug("(send_mode) chan = %s; pri = %i; type = %i; plusminus = %c; modeflag = %c; data = "mx_pfmt"\n",
		chan->name,pri,type,plusminus,modeflag,(mx_ptr)data);
#endif /* DEBUG */

	/*
	 *  make minusmodes always one priority lower than plusmodes
	 */
	if (plusminus == '-')
		pri |= 1;

	pp = &chan->modelist;
	while(*pp)
	{
		mode = *pp;
		if (mode->pri == pri)
		{
			/*
			 *  check for duplicate
			 */
			if ((mode->type == type) && (mode->plusminus == plusminus) &&
				(mode->modeflag == modeflag) && (mode->data == data))
				return;
		}
		if (mode->pri > pri)
			break;
		pp = &mode->next;
	}

	set_mallocdoer(send_mode);
	mode = (qMode*)Calloc(sizeof(qMode));
	mode->next = *pp;
	*pp = mode;
	mode->pri = pri;
	mode->type = type;
	mode->plusminus = plusminus;
	mode->modeflag = modeflag;

	switch(type)
	{
	case QM_CHANUSER:
		mode->data = data;
		break;
	default:
		if (data)
		{
			set_mallocdoer(send_mode);
			mode->data = (void*)stringdup((char*)data);
		}
		else
		{
			mode->data = NULL;
		}
	}
}

int mode_effect(Chan *chan, qMode *mode)
{
	ChanUser *cu;
	int	f;

	if (mode->type == QM_CHANUSER)
	{
		cu = (ChanUser*)mode->data;
		f = 0;
		switch(mode->modeflag)
		{
		case 'o':
			f = (cu->flags & CU_CHANOP) ? '+' : '-';
			break;
		case 'v':
			f = (cu->flags & CU_VOICE) ? '+' : '-';
			break;
		}
		if (f && f == mode->plusminus)
			return(FALSE);
	}
	return(TRUE);
}

void push_modes(Chan *chan, int lowpri)
{
	qMode	*mode;
	const char *srcparm;
	char	flaglist[32],parmlist[MSGLEN];
	char	*dstflag,*dstparm,lastmode;
	int	n,maxmodes;

	n = (current->sendq_time - now);

loop:
	maxmodes = current->setting[INT_MODES].int_var;
	lastmode = 0;
	dstflag = flaglist;
	dstparm = parmlist;
	while((mode = chan->modelist))
	{
		/*
		 *  if the line has already been partially filled,
		 *  then its ok to fill up "empty slots" with non-priority modes.
		 */
		if (lowpri && !lastmode && (mode->pri >= QM_PRI_LOW))
			return;
		chan->modelist = mode->next;
		if (mode_effect(chan,mode))
		{
			switch(mode->type)
			{
			case QM_CHANUSER:
				srcparm = ((ChanUser*)mode->data)->nick;
				break;
			default:
				srcparm = (char*)mode->data;
			}
			if (mode->plusminus != lastmode)
				*(dstflag++) = lastmode = mode->plusminus;
			*(dstflag++) = mode->modeflag;
			if (srcparm)
			{
				*(dstparm++) = ' ';
				dstparm = stringcpy(dstparm,srcparm);
			}
			maxmodes--;
		}
#ifdef NO_DEBUG
		else
		{
			debug("(push_modes) ineffectual mode: %c%c %s\n",mode->plusminus,mode->modeflag,
				(mode->type == QM_CHANUSER) ? ((ChanUser*)mode->data)->nick : (char*)mode->data);
		}
#endif /* DEBUG */
		if (mode->type != QM_CHANUSER)
			Free((char**)&mode->data);
		Free((char**)&mode);
		if (!maxmodes)
			break;
	}
	if (!lastmode)
		return;
	*dstflag = 0;
	*dstparm = 0;
	/*
	 *  the missing space is not a bug.
	 */
	to_server("MODE %s %s%s\n",chan->name,flaglist,parmlist);
	n += 2;

	if (lowpri && n < lowpri)
		goto loop;
}

void update_modes(Chan *chan)
{
	ChanUser *cu;

	for(cu=chan->users;cu;cu=cu->next)
	{
		if (
			(chan->setting[TOG_AOP].int_var && cu->user && cu->user->x.x.aop) ||
			((cu->flags & (CU_CHANOP|CU_NEEDOP)) == CU_NEEDOP)
			)
		{
#ifdef NO_DEBUG
			debug("(update_modes) pushing: MODE %s +o %s!%s\n",
				chan->name,cu->nick,cu->userhost);
#endif /* DEBUG */
			send_mode(chan,50,QM_CHANUSER,'+','o',(void*)cu);
		}
	}
}

/*
 *  check_mass() takes no action, it only touches the abuse counters
 *  and timers, TRUE is returned if the limit is reached
 */
int check_mass(Chan *chan, ChanUser *doer, int type)
{
	time_t	when;
	int	num,limit;

	/*
	 *  must handle servers ... (netsplits, chanserv, nickserv, ...)
	 */
	if (!doer)
		return(FALSE);

	if (doer->user && doer->user->x.x.access >= ASSTLEVEL)
		return(FALSE);

	if ((type == INT_CKL || type == INT_FL) && (doer->flags & CU_CHANOP))
		return(FALSE);

	limit = chan->setting[type].int_var;
	switch(type)
	{
	/*
	 *  two things we dont want channel users to do
	 */
	/*case CHK_CAPS: */
	case INT_CKL:
		num = INDEX_CAPS;
		break;
	/*case CHK_PUB: */
	case INT_FL:
		num = INDEX_FLOOD;
		break;
	/*
	 *  three things we dont want channel ops to do
	 */
	/*case CHK_DEOP: */
	case INT_MDL:
		num = INDEX_DEOP;
		break;
	/*case CHK_BAN: */
	case INT_MBL:
		num = INDEX_BAN;
		break;
	default:
/*	case CHK_KICK: */
/*	case INT_MKL: */
		num = INDEX_KICK;
		break;
	}

	if ((now - doer->action_time[num]) > 10)
	{
		doer->action_time[num] = now;
		doer->action_num[num] = 0;
	}
	++(doer->action_num[num]);
	if (doer->action_num[num] >= limit && limit)
		return(TRUE);
	return(FALSE);
}

void mass_action(Chan *chan, ChanUser *doer)
{
	int	mpl;

	if ((mpl = chan->setting[INT_MPL].int_var) == 0)
		return;

	if (mpl >= 2)
	{
		if (0 == (doer->flags & CU_DEOPPED) || 0 == (doer->flags & CU_BANNED))
		{
			deop_ban(chan,doer,NULL);
			doer->flags |= CU_DEOPPED|CU_BANNED;
		}
	}

	if (0 == (doer->flags & CU_KICKED))
	{
		send_kick(chan,CurrentNick,KICK_MASSMODES);
		doer->flags |= CU_KICKED;
	}
}

void prot_action(Chan *chan, char *from, ChanUser *doer, char *target, ChanUser *victim)
{
	int	maxprot,uprot;

	/*
	 *  cant do anything to a user that isnt on the channel
	 *  doer is normally supplied, but not always
	 */
	if (!doer)
	{
		if ((doer = find_chanuser(chan,from)) == NULL)
			return;
	}

	/*
	 *  No protective measures for doers with high access
	 */
	if (doer->user && doer->user->x.x.access >= ASSTLEVEL)
		return;

	maxprot = chan->setting[INT_PROT].int_var;

	if (victim)
		uprot = (victim->user) ? victim->user->x.x.prot : 0;
	else
	{
		uprot = get_protaction(chan,target);
	}

	if ((uprot >= 4) && (!(doer->flags & CU_BANNED)))
	{
		doer->flags |= CU_BANNED|CU_DEOPPED;
		deop_ban(chan,doer,NULL);
	}

	if ((uprot >= 3) && (!(doer->flags & CU_KICKED)))
	{
		doer->flags |= CU_KICKED;
		send_kick(chan,doer->nick,"\002%s is Protected\002",(target) ? target : get_nuh(victim));
	}
	/*
	 *  with (uprot == 2) our ONLY action is to deop the guilty party
	 */
	if ((uprot == 2) && (!(doer->flags & CU_DEOPPED)))
	{
		doer->flags |= CU_DEOPPED;
		send_mode(chan,50,QM_CHANUSER,'-','o',(void*)doer);
	}
}

#ifdef DYNAMODE

void check_dynamode(Chan *chan)
{
	ChanUser *cu;
	char	tempconf[strlen(chan->setting[STR_DYNLIMIT].str_var)+2];
	char	ascnum[11];
	char	*src,*num,*end;
	int	n = 0,wind,v[3];

	/*
	 *  parse `delay:window:minwin'
	 */
	end = stringcpy(tempconf,chan->setting[STR_DYNLIMIT].str_var);
	num = src = tempconf;
	for(;(src<=end) && (n<3);src++)
	{
		if (*src == 0 || *src == ':')
		{
			*src = 0;
			v[n] = asc2int(num);
			if (errno)
			{
				v[0] = 90;	/* delay */
				v[1] = 10;	/* window */
				v[2] = 4;	/* minwin */
				break;
			}
			num = src+1;
			n++;
		}
	}
	v[0] = (v[0] < 20) ? 20 : (v[0] > 600) ? 600 : v[0];
	if ((now - chan->lastlimit) < v[0])
		return;
	v[1] = (v[1] < 5) ? 5 : (v[1] > 50) ? 50 : v[1];
	v[2] = (v[2] < 1) ? 1 : (v[2] > 50) ? 50 : v[2];

	chan->lastlimit = now;

	n = 0;
	for(cu=chan->users;cu;cu=cu->next)
		n++;

	wind = n / v[1];
	if (wind < v[2])
		wind = v[2];

	wind += n;

	n = wind - chan->limit;

	if (!chan->limitmode || (n < -2) || (n > 1))
	{
		sprintf(ascnum,"%i",wind);
		send_mode(chan,160,QM_RAWMODE,'+','l',ascnum);
	}
}

#endif /* DYNAMODE */

#ifdef CHANBAN

/*

if the chanban setting is on, make a whois for all that join #mychannel
if #bannedchannel is in their list of channels, kickban them with appropriate message
check people randomly, if their whois is old and sendq is empty, do a new whois on them
never kickban people on the same channel as the bot
TODO: never kickban authenticated people

*/

/*
 *  called from main doit() loop
 *  current is not set
 */
void process_chanbans(void)
{
	Strp	*sp,**pp;
	Chan	*anychan;
	ChanUser *cu,*selcu;

	for (current=botlist;current;current=current->next)
	{
		if (current->lastchanban > (now - 10))
		{
#ifdef DEBUG
			debug("(process_chanbans) skipping %s (%i), (lastchanban (%lu) > now - 10 (%lu)\n",
				current->nick,current->guid,current->lastchanban,(now - 10));
#endif /* DEBUG */
			continue;
		}
		if (current->sendq) /* only do chanbans on empty queue */
		{
#ifdef DEBUG
			debug("(process_chanbans) skipping %s (%i), sendq not empty\n",current->nick,current->guid);
#endif /* DEBUG */
			continue;
		}

		selcu = NULL;
		for(anychan=current->chanlist;anychan;anychan=anychan->next)
		{
			if (anychan->modelist || anychan->kicklist) /* only do chanbans on empty queue */
				goto has_queue;
			if (anychan->setting[TOG_CHANBAN].int_var && anychan->bot_is_op)
			{
				for(cu=anychan->users;cu;cu=cu->next)
				{
					if (cu->user)
					{
						if (!selcu || cu->lastwhois < selcu->lastwhois)
						{
							selcu = cu;
						}
					}
				}
			}
		}

		if (selcu && selcu->lastwhois < (now-30))
		{
			selcu->flags &= ~CU_CHANBAN;
			selcu->lastwhois = now;
			current->lastchanban = now;

			pp = &current->sendq;
			while(*pp)
				pp = &(*pp)->next;
			set_mallocdoer(process_chanbans);
			*pp = sp = (Strp*)Calloc(sizeof(Strp) + 6 + strlen(selcu->nick));
			sprintf(sp->p,"WHOIS %s",selcu->nick);
	                /* Calloc sets to zero sp->next = NULL; */
		}
has_queue:
		;
	}
}

void chanban_action(char *nick, char *channel, Shit *shit)
{
	ChanUser *cu;
	char	*nuh;

	/* the channel is shitted and the user is on it...
	   1, make sure the bot isnt on the channel
	   2, kb the user on all channels where the shit is active and i am op */

	/* check all current channels */
	for(CurrentChan=current->chanlist;CurrentChan;CurrentChan=CurrentChan->next)
	{
		if (!stringcasecmp(channel,CurrentChan->name)) /* if the bot is on the channel, skip it */
		{
#ifdef DEBUG
			debug("(chanban_action) skipping %s: bot is on channel\n",channel);
#endif /* DEBUG */
			return;
		}
		/* is the shit for this channel? */
		if (!stringcasecmp(shit->chan,CurrentChan->name))
		{
			/* if chanban is turned on && if bot is op (pretty pointless otherwise) */
			if (CurrentChan->setting[TOG_CHANBAN].int_var && CurrentChan->bot_is_op)
			{
#ifdef DEBUG
				debug("(chanban_action) %s: Tog is on, I am op\n",CurrentChan->name);
#endif /* DEBUG */
				cu = find_chanuser(CurrentChan,nick);
				if (!(cu->flags & CU_CHANBAN))
				/* dont kickban the same user multiple times from the same channel */
				{
					nuh = get_nuh(cu); /* clobbers nuh_buf */
#ifdef DEBUG
					debug("(chanban_action) slapping %s on %s for being on %s (mask %s): %s\n",
						nick,CurrentChan->name,channel,shit->mask,shit->reason);
#endif /* DEBUG */
					cu->flags |= CU_CHANBAN;
					format_uh(nuh,1); /* returns mask in 'nuh' buffer (nuh_buf) */
					send_mode(CurrentChan,90,QM_RAWMODE,'+','b',(void*)nuh);
					send_kick(CurrentChan,nick,"%s (%s)",shit->reason,channel); /* clobbers globaldata */
				}
			}
		}
	}
}

#endif /* CHANBAN */

KickSay *find_kicksay(char *text, char *channel)
{
	KickSay *kick,*save;
	int	num,best;

	save = NULL;
	best = 0;
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (!channel || *kick->chan == '*' || !stringcasecmp(channel,kick->chan))
		{
			num = num_matches(kick->mask,text);
			if (num > best)
			{
				best = num;
				save = kick;
			}
		}
	}
	return(save);
}

void check_kicksay(Chan *chan, ChanUser *doer, char *text)
{
	KickSay *kick,*save;
	char *mask;
	int action;

	save = NULL;
	action = -1;
	for(kick=current->kicklist;kick;kick=kick->next)
	{
		if (*kick->chan == '*' || !stringcasecmp(chan->name,kick->chan))
		{
			if (!matches(kick->mask,text))
			{
				if (kick->action > action)
				{
					action = kick->action;
					save = kick;
				}
			}
		}
	}
	if (save)
	{
		if (!action)
		{
			if (doer->flags & CU_KSWARN)
				action = 1;
			if (!(doer->flags & CU_KSWARN))
			{
				doer->flags |= CU_KSWARN;
				to_server("NOTICE %s :%s\n",doer->nick,save->reason);
			}
		}
		if (action > 1)
		{
			mask = format_uh(get_nuh(doer),FUH_USERHOST);
			if (action > 2)
			{
				add_shit("Auto KS",chan->name,mask,save->reason,2,now+3600);
			}
			if (!(doer->flags & CU_BANNED))
			{
				doer->flags |= CU_BANNED;
				send_mode(chan,90,QM_RAWMODE,'+','b',mask);
			}
		}
		if (action && !(doer->flags & CU_KICKED))
		{
			doer->flags |= CU_KICKED;
			send_kick(chan,CurrentNick,FMT_PLAIN,save->reason);
		}
	}
}

void remove_kicksay(KickSay *kick)
{
	KickSay **pp;

	pp = &current->kicklist;
	while(*pp)
	{
		if (*pp == kick)
		{
			*pp = kick->next;
			Free((char**)&kick);
			return;
		}
		pp = &(*pp)->next;
	}
}

void purge_kicklist(void)
{
	while(current->kicklist)
		remove_kicksay(current->kicklist);
}

/*
-+=====================================================================================================================+-
							prot.c commands
-+=====================================================================================================================+-
*/

void do_opdeopme(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Chan	*chan;
	ChanUser *cu;

	if ((chan = CurrentChan) && chan->bot_is_op)
	{
		if ((cu = find_chanuser(chan,from)))
		{
			send_mode(chan,80,QM_CHANUSER,
				(CurrentCmd->name == C_DOWN) ? '-' : '+','o',cu);
		}
	}
}

void do_opvoice(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + CARGS
	 */
	const Chan *chan;
	const char *m;

	if ((chan = CurrentChan) && chan->bot_is_op)
	{
		/* rest can only be NULL on OP/VOICE commands,
		   DEOP/UNVOICE commands require arguments (CARGS) */
		if (!*rest)
			rest = CurrentNick;
		m = CurrentCmd->cmdarg;
#ifdef DEBUG
		debug("(do_opvoice) Chan '%s', sign %c, mode %c, rest '%s'\n",chan->name,m[1],m[0],rest);
#endif /* DEBUG */
		channel_massmode(chan,rest,MODE_FORCE,m[1],m[0]);
	}
}

void do_kickban(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + CARGS + ACCHAN
	 */
	Chan	*chan;
	ChanUser *victim;
	const char *m;
	char	*nick,*nuh;
	uchar	c;
	int	uaccess;

	nick = chop(&rest);
	chan = CurrentChan;

	/*
	 *  is the bot opped on that channel?
	 */
	if (!chan->bot_is_op)
		return;

	if ((victim = find_chanuser(chan,nick)) == NULL)
		return;

	if (*rest == 0)
	{
		if ((rest = randstring(RANDKICKSFILE)) == NULL)
			rest = KICK_DEFAULT;
	}

	m = CurrentCmd->cmdarg;
	c = *(m++);

	uaccess = get_authaccess(from,to);
	if (victim->user && victim->user->x.x.access > uaccess)
	{
		nuh = get_nuh(victim);
		send_kick(chan,CurrentNick,"%s attempt of %s",m,nuh);
		return;
	}

	if (c & 0x4)
		send_kick(chan,nick,FMT_PLAIN,rest);

	switch(c & 0x3)
	{
	case 0:
		deop_ban(chan,victim,NULL);
		break;
	case 1:
		deop_siteban(chan,victim);
		break;
	case 2:
		deop_screwban(chan,victim);
		break;
	}

	m = STREND(m)+1;
	to_user(from,"%s %sed on %s",nick,m,to);
}

void do_unban(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CAXS
	 */
	Ban	*ban;
	Chan	*chan;
	char	*nick,*nuh;

	nick = (rest) ? chop(&rest) : NULL;

	if (((chan = find_channel_ac(to)) == NULL) || !chan->bot_is_op)
		return;

	if (nick && STRCHR(nick,'*'))
	{
		channel_massunban(chan,nick,0);
		return;
	}

	if (!nick)
	{
		to_user(from,"You have been unbanned on %s",to);
		nuh = from;
	}
	else
	{
		if ((nuh = nick2uh(from,nick)) == NULL)
			return;
		to_user(from,"%s unbanned on %s",nuh,to);
	}

	for(ban=chan->banlist;ban;ban=ban->next)
	{
		if (!matches(ban->banstring,nuh))
		{
			send_mode(chan,90,QM_RAWMODE,'-','b',(void*)ban->banstring);
		}
	}
}

void do_banlist(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Ban	*ban;
	Chan	*chan = CurrentChan;
#ifdef IRCD_EXTENSIONS
	char	*s;

	if (chan->banlist)
	{
		table_buffer(str_underline("channel") "\t" str_underline("type") "\t" str_underline("mask") "\t" str_underline("set by"));
		for(ban=chan->banlist;ban;ban=ban->next)
		{
#ifdef NEWBIE
			if (ban->imode)
				s = "invitelist";
			else
			if (ban->emode)
				s = "ban exception";
			else
				s = "ban";
#else /* NEWBIE */
			if (ban->imode)
				s = "I";
			else
			if (ban->emode)
				s = "e";
			else
				s = "b";
#endif /* NEWBIE */

			table_buffer(FMT_4XSTRTAB,to,s,ban->banstring,ban->bannedby);
		}
		table_send(from,2);
	}
	else
		to_user(from,"There are no active bans/exceptions on %s",to);
#else /* IRCD_EXTENSIONS */
	if (chan->banlist)
	{
		table_buffer(str_underline("channel") "\t" str_underline("ban mask") "\t" str_underline("set by"));
		for(ban=chan->banlist;ban;ban=ban->next)
			table_buffer(FMT_3XSTRTAB,to,ban->banstring,ban->bannedby);
		table_send(from,2);
	}
	else
		to_user(from,"There are no active bans on %s",to);
#endif /* IRCD_EXTENSIONS */
}

#ifdef NEWBIE
char *ks_actions[MAX_KS_LEVEL+1] = { "warn (0)","kick (1)","kickban (2)","autoshit (3)" };
#else
char *ks_actions[MAX_KS_LEVEL+1] = { "warn","kick","kickban","autoshit" };
#endif /* NEWBIE */

void do_kicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel,*mask;
	int	inum;

	channel = chop(&rest);
	if (!channel)
	{
		if (!current->kicklist)
		{
			to_user(from,"Kicksay list is empty");
			return;
		}

		if (partyline_only_command(from))
			return;

		table_buffer("\037channel\037\t\037action\037\t\037string\037\t\037kick reason\037");
		for(kick=current->kicklist;kick;kick=kick->next)
		{
			table_buffer(FMT_4XSTRTAB,kick->chan,ks_actions[kick->action],kick->mask,kick->reason);
		}
		table_send(from,2);
		return;
	}

	if (ischannel(channel) || *channel == '*')
	{

		if (get_useraccess(from,channel) < cmdaccess)
			return;

		inum = DEFAULT_KS_LEVEL;
		if (*rest != '"')
		{
			inum = asc2int(chop(&rest));
			if (errno || inum < 0 || inum > MAX_KS_LEVEL)
				return;
		}

		mask = get_token(&rest,"\"");

		if (!mask || !*mask)
			goto usage;

		/*
		 *  check for previously existing kicks
		 */
		if ((kick = find_kicksay(mask,channel)) != NULL)
		{
			to_user(from,"I'm already kicking on \"%s\"",kick->mask);
			return;
		}

		/*
		 *  dig out the reason (the rest)
		 */
		while(rest && *rest == ' ')
			rest++;
		if (!*rest)
			goto usage;

		/*
		 *  add it to the list
		 */
		set_mallocdoer(do_kicksay);
		kick = (KickSay*)Calloc(sizeof(KickSay) + StrlenX(channel,mask,rest,NULL));

		kick->next = current->kicklist;
		current->kicklist = kick;
		kick->action = inum;

		if (!matches("\\*?*\\*",mask))
			kick->chan = stringcpy(kick->mask,mask) + 1;
		else
		{
			kick->mask[0] = '*';
			stringcpy(kick->mask+1,mask);
			kick->chan = stringcat(kick->mask,MATCH_ALL) + 1;
		}
		kick->reason = stringcpy(kick->chan,channel) + 1;
		stringcpy(kick->reason,rest);

		to_user(from,"Now kicking on \"%s\" on %s",mask,channel);
		current->ul_save++;
		return;
	}
usage:
	usage(from);	/* usage for CurrentCmd->name */
}

void do_rkicksay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	KickSay *kick;
	char	*channel;

	channel = chop(&rest);	/* cant be NULL (CARGS) */
	if (!ischannel(channel) && *channel != '*')
		goto usage;

	if (!*rest)
		goto usage;

	if (!(kick = find_kicksay(rest,channel)))
	{
		to_user(from,"I'm not kicking on that");
		return;
	}
	to_user(from,"No longer kicking on %s",kick->mask);
	remove_kicksay(kick);
	current->ul_save++;
	return;
usage:
	usage(from);	/* usage for CurrentCmd->name */
}
