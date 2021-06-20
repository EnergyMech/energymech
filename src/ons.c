/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2009 proton

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
#define COM_ONS_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef SUPPRESS

uint32_t makecrc(const char *args)
{
	uint32_t crc = 0;
	int	n = 0;

	while(args[n])
		crc += ((((crc * 0xc960ebb3) ^ 0x14d0bd4d) + 0x9ff77d71) * args[n++]) - 0xb07daba7;

	return(crc);
}

void send_suppress(const char *command, const char *args)
{
	Mech	*backup;
	int	crc;

	crc = makecrc(args);
	for(backup=botlist;backup;backup=backup->next)
	{
		if (backup != current)
		{
			backup->supres_cmd = command;
			backup->supres_crc = crc;
		}
	}
	botnet_relay(NULL,"CS%s %i\n",command,crc);
}

#endif /* SUPPRESS */

/*
 *  :nick!user@host KICK #channel kicknick :message
 */
void on_kick(char *from, char *rest)
{
	Chan	*chan;
	ChanUser *doer,*victim;
	char	*channel,*nick;

	channel = chop(&rest);
	if ((CurrentChan = chan = find_channel_ac(channel)) == NULL)
		return;

	nick = chop(&rest);
	if (rest && *rest == ':')
		rest++;

	nickcpy(CurrentNick,from);

	if (current->spy & SPYF_CHANNEL)
		send_spy(chan->name,"*** %s was kicked by %s (%s)",nick,CurrentNick,rest);

	if (!nickcmp(current->nick,nick))
	{
#ifdef DEBUG
		debug("(on_kick) I was kicked from %s\n",chan->name);
#endif /* DEBUG */
		Free(&chan->kickedby);
		set_mallocdoer(on_kick);
		chan->kickedby = stringdup(from);
		chan->active = FALSE;
		chan->sync = TRUE;
		chan->bot_is_op = FALSE;
		join_channel(chan->name,chan->key);
		/*
		 *  if we're kicked from the active channel, we need to find a new
		 *  channel to set as the active one.
		 */
		if (chan == current->activechan)
		{
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if (chan->active)
					break;
			}
			current->activechan = chan;
			/*
			 *  Might be set to NULL now, but its supposed to be checked whenever used.
			 *  If not, we get a SEGV; and fix it.
			 */
		}
#ifdef STATS
		if (chan && chan->stats)
			chan->stats->flags |= CSTAT_PARTIAL;
#endif /* STATS */
		/*
		 *  this is the only return we can do: its the bot itself!
		 *  channel userlist will be reconstructed on rejoin
		 */
		return;
	}

#ifdef STATS
	if (chan->setting[STR_STATS].str_var)
		stats_plusminususer(chan,-1);
	if (chan->stats)
		chan->stats->kicks++;
#endif /* STATS */

	/*
	 *  making life easy for ourselves
	 */
	victim = find_chanuser(chan,nick);
	doer = NULL;

	if (chan->bot_is_op)
	{
		/*
		 *  are we supposed to check for channel mass kicks?
		 */
		if (chan->setting[INT_MPL].int_var)
		{
			doer = find_chanuser(chan,from);
			if (check_mass(chan,doer,INT_MKL))
				mass_action(chan,doer);
		}
		/*
		 *  are we supposed to protect users?
		 */
		if (chan->setting[INT_PROT].int_var)
		{
			if (victim->user && victim->user->x.x.prot)
			{
				/*
				 *  doer might be NULL, prot_action() handles it
				 */
				prot_action(chan,from,doer,NULL,victim);
				to_server("INVITE %s %s\n",nick,channel);
			}
		}
	}

	/*
	 *  cant delete users who arent there
	 */
	if (victim)
	{
#ifdef SEEN
		make_seen(nick,victim->userhost,from,rest,now,SEEN_KICKED);
#endif /* SEEN */

		/*
		 *  Dont delete the poor sod before all has been processed
		 */
		remove_chanuser(chan,nick);
	}
}

void on_join(Chan *chan, char *from)
{
	Ban	*ban;
	ChanUser *cu;
	int	vpri;

	/*
	 *  Satisfy spies before we continue...
	 */
	if (current->spy & SPYF_CHANNEL)
		send_spy(chan->name,"*** Joins: %s (%s)",CurrentNick,getuh(from));
	/*
	 *
	 */
#ifdef GREET
	if (!CurrentShit && CurrentUser && CurrentUser->greet)
		greet();
#endif /* GREET */
	/*
	 *  No further actions to be taken if the bot isnt opped
	 */
	if (!chan->bot_is_op)
		return;

	cu      = chan->users;

	/*
	 *  Some stuff only applies to non-users
	 */
	if (!CurrentUser)
	{
		/*
		 *  Kick banned (desynched) users if ABK is set
		 */
		if (chan->setting[TOG_ABK].int_var)
		{
			for(ban=chan->banlist;ban;ban=ban->next)
			{
				if (!matches(ban->banstring,from))
					break;
			}
			if (ban)
			{
				send_kick(chan,CurrentNick,KICK_BANNED);
				return;
			}
		}
		/*
		 *  Kickban users with control chars in their ident
		 *  (which doesnt violate RFC1413 but is bloody annoying)
		 */
		if (chan->setting[TOG_CTL].int_var)
		{
			if (STRCHR(from,'\031') || STRCHR(from,'\002') || STRCHR(from,'\022') || STRCHR(from,'\026'))
			{
				deop_siteban(chan,cu);
				send_kick(chan,CurrentNick,KICK_BAD_IDENT);
				return;
			}
		}
	}
	/*
	 *  If they're shitted, they're not allowed to be opped or voiced
	 */
	if (CurrentShit)
	{
		shit_action(chan,cu);
		return;
	}
	/*
	 *  Check for +ao users if AOP is toggled on
	 */
	if (chan->setting[TOG_AOP].int_var)
	{
		if (cu->user && cu->user->x.x.aop)
		{
			send_mode(chan,140,QM_CHANUSER,'+','o',(void*)cu);
			return;
		}
	}
	/*
	 *  If AVOICE eq 0 we have nothing more to do
	 */
	vpri = 200;
	switch(chan->setting[INT_AVOICE].int_var)
	{
	case 1:
		vpri = 150;
		if (cu->user && cu->user->x.x.avoice)
			break;
		/* fall through */
	case 0:
		return;
	}
	send_mode(chan,vpri,QM_CHANUSER,'+','v',(void*)cu);
}

void on_nick(char *from, char *newnick)
{
	ChanUser *cu;
	Chan	*chan;
	char	newnuh[NUHLEN];
	int	maxcount;
	int	isbot;

	nickcpy(CurrentNick,from);

#ifdef FASTNICK
	/*
	 *  grab the nick *RIGHT NOW*
	 *  this is a setting because this is risky, you might get collided as a result
	 */
	if (!nickcmp(CurrentNick,current->wantnick))
		to_server("NICK %s\n",current->wantnick);
#endif /* FASTNICK */

	/*
	 *  make the new From string
	 */
	sprintf(newnuh,"%s!%s",newnick,getuh(from));

#ifdef SEEN
	make_seen(CurrentNick,from,newnick,NULL,now,SEEN_NEWNICK);
#endif /* SEEN */

	/*
	 *  snooping buggers
	 */
	if (current->spy & SPYF_CHANNEL)
		send_spy(MATCH_ALL,"*** %s is now known as %s",CurrentNick,newnick);

	change_authnick(from,newnuh);

	if ((isbot = !nickcmp(current->nick,CurrentNick)))
	{
		setbotnick(current,newnick);
	}

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if ((cu = find_chanuser(chan,from)) == NULL)
			continue;

		/*
		 *  only need to realloc the buffer if its too small
		 */
		if (strlen(cu->nick) >= strlen(newnick))
		{
			stringcpy(cu->nick,newnick);
		}
		else
		{
			Free((char**)&cu->nick);
			set_mallocdoer(on_nick);
			cu->nick = stringdup(newnick);
		}

		/*
		 *  if the bot isnt opped, there's nothing more to do
		 */
		if (!chan->bot_is_op)
			continue;

		/*
		 *  if its the current bot, we dont do diddly squat
		 */
		if (isbot)
			continue;

		shit_action(chan,cu);

		/*
		 *  check for nick-change-flood
		 */
		if ((maxcount = chan->setting[INT_NCL].int_var) < 2)
			continue;

		if ((now - cu->action_time[INDEX_NICK]) > NICKFLOODTIME)
		{
			cu->action_time[INDEX_NICK] = now + (NICKFLOODTIME / (maxcount - 1));
			cu->action_num[INDEX_NICK] = 1;
		}
		else
		{
			cu->action_time[INDEX_NICK] += (NICKFLOODTIME / (maxcount - 1));
			if (++cu->action_num[INDEX_NICK] >= maxcount)
			{
				deop_ban(chan,cu,NULL);
				send_kick(chan,newnick,KICK_NICKFLOOD);
			}
		}
	}
}

void on_msg(char *from, char *to, char *rest)
{
#ifdef SCRIPTING
	Hook	*hook;
#endif /* SCRIPTING */
#ifdef ALIAS
	char	amem[MSGLEN];		/* big buffers at the top */
	Alias	*alias;
	int	arec;
#endif /* ALIAS */
#ifdef REDIRECT
	char	*orig_to;
#endif /* REDIRECT */
	char	*pt,*origstart,*command;
	uchar	*p1,*p2;
	int	has_cc,has_bang;
	int	uaccess;
	int	i,j;

	/*
	 *  No line sent to this routine should be longer than MSGLEN
	 *  Callers responsibility to check that from, to and msg is
	 *  non-NULL and non-zerolength
	 */

#ifdef NOTE
	if (notelist && catch_note(from,to,rest))
		return;
#endif /* NOTE */

	/*
	 * If the message is for a channel and we dont accept
	 * public commands, we can go directly to common_public()
	 */
	if (CurrentChan && !CurrentChan->setting[TOG_PUB].int_var)
	{
		common_public(CurrentChan,from,"<%s> %s",rest);
		return;
	}

	if (CurrentDCC)
	{
		uaccess = CurrentUser->x.x.access;
	}
	else
	if ((uaccess = get_authaccess(from,NULL)) > OWNERLEVEL)
	{
		/*
		 *  If its a bot we want nothing to do with it
		 */
		return;
	}

	/*
	 *  remember where the string started
	 */
	origstart = rest;

	if (from == CoreUser.name)
	{
		has_cc = TRUE;
	}
	else
	{
		has_cc = (current->setting[TOG_CC].int_var) ? FALSE : TRUE;
	}

	/*
	 *  check for command bots nick replacing command char
	 */
	if ((p2 = (uchar*)(command = chop(&rest))) == NULL)
		return;

	p1 = (uchar*)current->nick;
	while(!(i = tolowertab[*(p1++)] - tolowertab[*p2]) && *(p2++))
		;

	if (!i || ((p2 > (uchar*)command) && (*p2 == ':' || *p2 == ';' || *p2 == ',') && p2[1] == 0))
	{
		if ((command = chop(&rest)) == NULL)
			return;
		has_cc = TRUE;
	}

	has_bang = FALSE;
	if (*command == current->setting[CHR_CMDCHAR].char_var)
	{
		has_cc = TRUE;
		command++;
	}
	else
	if (!has_cc && *command == '!')
	{
		has_bang = TRUE;
		command++;
	}

#ifdef ALIAS
	arec = 0;
recheck_alias:
#endif /* ALIAS */

#ifdef ALIAS
	for(alias=aliaslist;alias;alias=alias->next)
	{
		if (!stringcasecmp(alias->alias,command))
		{
			unchop(command,rest);
			afmt(amem,alias->format,command);
#ifdef DEBUG
			debug("(on_msg) [ALIAS] %s --> %s\n",command,amem);
#endif /* DEBUG */
			rest = amem;
			pt = chop(&rest);
			i = stringcasecmp(pt,command);
			command = pt;
			arec++;
			if ((arec < MAXALIASRECURSE) && (i != 0))
				goto recheck_alias;
		}
	}
#endif /* ALIAS */

#ifdef REDIRECT
	orig_to = to;
#endif /* REDIRECT */

	i = 0;
#ifdef SCRIPTING
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
		if (hook->flags == MEV_COMMAND && !stringcasecmp(command,hook->type.command))
		{
			if (hook->func(from,rest,hook))
				/* if the hook returns non-zero, the input should not be parsed internally */
				i = 1;
		}
	}
	if (i) return;
#endif /* SCRIPTING */

	/*
	 *  match "command" against internal command list
	 */
	for(;mcmd[i].name;i++)
	{
		if (!has_cc && mcmd[i].cc && !(has_bang && mcmd[i].cbang))
			continue;
		if (uaccess < acmd[i])
			continue;
		j = stringcasecmp(mcmd[i].name,command);
		if (j < 0)
			continue;
		if (j > 0)
			break;

#if defined(BOTNET) && defined(REDIRECT)
		if (mcmd[i].nocmd && redirect.to)
			return;
#endif /* BOTNET && REDIRECT */

		if (mcmd[i].nopub && CurrentChan)
		{
#ifdef DEBUG
			debug("(on_msg) Public command (%s) ignored\n",command);
#endif /* DEBUG */
			return;
		}

		CurrentCmd = &mcmd[i];

#ifdef SUPPRESS
#ifdef BOTNET
		/* experimental command supression */
		if (CurrentCmd->name == current->supres_cmd)
		{
			int	crc;

			crc = makecrc(rest);
			if (current->supres_crc == crc)
			{
				/* another bot has already executed this command and is trying to supress its execution on other bots */
				current->supres_cmd = NULL;
				current->supres_crc = 0;
#ifdef DEBUG
				debug("(on_msg) command \"%s\" from %s was supressed\n",CurrentCmd->name,CurrentNick);
#endif
				return;
			}
		}
		/*if command should be supressed ... */
		if (mcmd[i].supres && CurrentChan)
		{
			send_suppress(CurrentCmd->name,rest);
		}
#endif
#endif /* SUPPRESS */
		/*
		 *  convert the command to uppercase
		 */
		stringcpy(command,mcmd[i].name);

		/*
		 *  send statmsg with info on the command executed
		 */
		if (current->setting[TOG_SPY].int_var)
		{
			send_spy(SPYSTR_STATUS,":%s[%i]: Executing %s[%i]",
				CurrentNick,uaccess,command,(int)acmd[i]);
		}

		/*
		 *  list of last LASTCMDSIZE commands
		 */
		if (from != CoreUser.name)
		{
			Free(&current->lastcmds[LASTCMDSIZE-1]);
			for(j=LASTCMDSIZE-2;j>=0;j--)
				current->lastcmds[j+1] = current->lastcmds[j];
			if ((pt = STRCHR(from,'@')) == NULL)
				pt = from;
			set_mallocdoer(on_msg);
			current->lastcmds[0] = (char*)Calloc(strlen(pt) + 45);
			if (CurrentUser)
			{
				sprintf(current->lastcmds[0],"[%s] %s\r%s[%-3i]\t(*%s)",
					time2medium(now),command,CurrentUser->name,
					(CurrentUser->x.x.access),pt);
			}
			else
			{
				sprintf(current->lastcmds[0],"[%s] %s\r%s[---]\t(*%s)",
					time2medium(now),command,CurrentNick,pt);
			}
		}

		/*
		 *  CAXS check: first argument might be a channel
		 *              check user access on target channel
		 */
		if (mcmd[i].caxs)
		{
			/* get channel name; 1: msg, 2: to, 3: active channel */
			to = (char*)get_channel(to,&rest);
			if (!ischannel(to))
				return;
			uaccess = get_authaccess(from,to);
			if (uaccess < acmd[i])
				return;
			CurrentChan = find_channel_ac(to);
			if (mcmd[i].acchan && !CurrentChan)
			{
				to_user(from,ERR_CHAN,to);
				return;
			}
		}
		else
		/*
		 *  GAXS check: user needs global access
		 */
		if (mcmd[i].gaxs)
		{
			uaccess = get_authaccess(from,MATCH_ALL);
			if (uaccess < acmd[i])
				return;
		}

		/*
		 *  CARGS check: at least one argument is required
		 */
		if (mcmd[i].args && !*rest)
		{
			if (uaccess) usage_command(from,command);
			return;
		}

#ifdef REDIRECT
		/*
		 *  can this command be redirected?
		 */
		if (!redirect.to && mcmd[i].redir)
		{
			if (mcmd[i].lbuf && ischannel(orig_to))
			{
				set_mallocdoer(on_msg);
				redirect.to = stringdup(to);
				redirect.method = R_PRIVMSG;
			}
			else
			if (begin_redirect(from,rest) < 0)
				return;
		}
#endif /* REDIRECT */

		if (mcmd[i].dcc && partyline_only_command(from))
			return;

		mcmd[i].func(from,to,rest,acmd[i]);

#ifdef DEBUG
		CurrentCmd = NULL;
#endif /* DEBUG */
#ifdef REDIRECT
		end_redirect();
#endif /* REDIRECT */

		/*
		 *  be quick to exit afterwards, there are "dangerous" commands like DIE and DEL (user)
		 */
		return;
	}

	/*
	 *  un-chop() the message string
	 */
	unchop(origstart,rest);

	if (CurrentChan)
	{
		common_public(CurrentChan,from,"<%s> %s",origstart);
	}
	else
	if (has_cc && *command && uaccess)
	{
		to_user(from,ERR_UNKNOWN_COMMAND);
	}
	else
	if (CurrentDCC)
	{
		partyline_broadcast(CurrentDCC,"<%s> %s\n",origstart);
#ifdef BOTNET
		botnet_relay(NULL,"PM* * %s@%s %s\n",CurrentNick,current->nick,origstart);
#endif /* BOTNET */
	}
	else
	{
		send_spy(SPYSTR_MESSAGE,"<%s> %s",CurrentNick,origstart);
  	}
}

void on_mode(char *from, char *channel, char *rest)
{
	Chan	*chan;
	ChanUser *doer;
	ChanUser *victim;
	Shit	*shit;
	char	templimit[20];
	char	*nick;
	char	*parm,*nickuh,*mode;
	int	i,sign,enfm,maxprot;

	if ((chan = find_channel_ac(channel)) == NULL)
		return;
	channel = chan->name;

	if (current->spy & SPYF_CHANNEL)
		send_spy(channel,"*** %s sets mode: %s",CurrentNick,rest);

	maxprot = chan->setting[INT_PROT].int_var;
	sign = '+';

	mode = chop(&rest);

	/*
	 *  might be NULL but we have to handle that due to server modes
	 */
	doer = find_chanuser(chan,from);

modeloop:
	if (*mode == 'o' || *mode == 'v')
	{
		nick = chop(&rest);
		if ((victim = find_chanuser(chan,nick)) == NULL)
		{
			mode++;
			goto modeloop;
		}
	}

	switch(*mode)
	{
	case '+':
	case '-':
		sign = *mode;
		break;
	/*
	 *
	 *  MODE <channel> +/-o <nick>
	 *
	 */
	case 'o':
		i = (victim->user) ? victim->user->x.x.access : 0;
/* +o */	if (sign == '+')
		{
			victim->flags |= CU_CHANOP;
			victim->flags &= ~CU_DEOPPED;
			if (!i)
			{
				if (victim->shit || (chan->setting[TOG_SD].int_var && !doer) ||
					chan->setting[TOG_SO].int_var)
				{
					send_mode(chan,60,QM_CHANUSER,'-','o',victim);
				}
			}
			else
			if (!nickcmp(current->nick,nick))
			{
				/*
				 *  wooohoooo! they gave me ops!!!
				 */
				chan->bot_is_op = TRUE;
				if (chan->kickedby)
				{
					if (chan->setting[TOG_RK].int_var)
						send_kick(chan,nickcpy(NULL,chan->kickedby),KICK_REVENGE);
					Free(&chan->kickedby);
				}
				check_shit();
				update_modes(chan);
			}
#ifdef DEBUG
			debug("(on_mode) %s!%s --> %i\n",victim->nick,victim->userhost,i);
#endif /* DEBUG */
		}
/* -o */	else
		{
			victim->flags &= ~(CU_CHANOP|CU_DEOPPED);
			if (i == BOTLEVEL)
			{
				if (!nickcmp(current->nick,nick))
				{
					/*
					 *  they dont love me!!! :~(
					 */
					chan->bot_is_op = FALSE;
				}
			}
			/*
			 *  idiots deopping themselves
			 */
			if (!nickcmp(from,nick))
				break;
			/*
			 *  1. Use enfm var to temporarily store users access
			 *  2. get_userlevel also checks is_localbot()...
			 */
			enfm = (doer && doer->user) ? doer->user->x.x.access : 0;
			if (enfm == BOTLEVEL)
				break;
			if (check_mass(chan,doer,INT_MDL))
				mass_action(chan,doer);
			if (maxprot && (victim->user && victim->user->x.x.prot) && !victim->shit)
			{
				/*
				 *  FIXME: does it matter if the user is logged in or not?
				 */
				nickuh = get_nuh(victim);
				if (get_authaccess(nickuh,channel))
				{
					send_mode(chan,60,QM_CHANUSER,'+','o',victim);
					prot_action(chan,from,doer,NULL,victim);
				}
			}
		}
		break;
	/*
	 *
	 *  MODE <channel> +/-v <nick>
	 *
	 */
	case 'v':
		if (sign == '+')
			victim->flags |= CU_VOICE;
		else
			victim->flags &= ~CU_VOICE;
		break;
#ifdef IRCD_EXTENSIONS
/*
:joonicks!*@* MODE #emech +I *king*!*@*
:joonicks!*@* MODE #emech +e *kong*!*@*
*/
#endif /* IRCD_EXTENSIONS */
	/*
	 *
	 *  MODE <channel> +/-b <parm>
	 *
	 */
#ifdef IRCD_EXTENSIONS
	/*
	 *  ircnet braindamage modes
	 */
	case 'I':
	case 'e':
#endif /* IRCD_EXTENSIONS */
	case 'b':
		parm = chop(&rest);
/* +b */	if (sign == '+')
		{
#ifdef IRCD_EXTENSIONS
			Ban	*newban;

			newban = make_ban(&chan->banlist,from,parm,now);
			if (*mode == 'I') newban->imode = TRUE;
			if (*mode == 'e') newban->emode = TRUE;
			/*
			 * I/e modes are low privilige and not checked for protection (yet?)
			 */
			break;
#else /* IRCD_EXTENSIONS */
			make_ban(&chan->banlist,from,parm,now);
#endif /* IRCD_EXTENSIONS */
			/*
			 *  skip protection checks if the doer is myself or another known bot
			 */
			if (doer && doer->user && doer->user->x.x.access == BOTLEVEL)
				break;
			if (check_mass(chan,doer,INT_MBL))
				mass_action(chan,doer);
			if (maxprot && get_protaction(chan,parm))
			{
				shit = get_shituser(parm,channel);
				if (!shit || !shit->action)
				{
					/*
					 *  FIXME: do we have a CU for the `from' user? -- yes: doer
					 *  bot_is_op checked: no
					 */
					send_mode(chan,160,QM_RAWMODE,'-','b',parm);
					prot_action(chan,from,doer,parm,NULL);
				}
			}
		}
/* -b */	else
		{
#ifdef IRCD_EXTENSIONS
			if (*mode == 'I' || *mode == 'e')
			{
				delete_modemask(chan,parm,*mode);
				break;
			}
#endif /* IRCD_EXTENSIONS */
			delete_ban(chan,parm);
			if (!chan->setting[TOG_SHIT].int_var)
				break;

			/* whats this??? */
			shit = get_shituser(parm,channel);  /* calls find_shit? clobbers get_nuh buffer */
			i = (shit) ? shit->action : 0;
			if (i < SHIT_PERMABAN)
			{
				shit = find_shit(parm,channel);
				i = (shit) ? shit->action : 0;
			}
			if (i == SHIT_PERMABAN)
			{
				send_mode(chan,160,QM_RAWMODE,'+','b',shit->mask);
			}
		}
		break;
	case 'p':
	case 's':
	case 'm':
	case 't':
	case 'i':
	case 'n':
		if (reverse_mode(from,chan,*mode,sign))
		{
			send_mode(chan,160,QM_RAWMODE,(sign == '+') ? '-' : '+',*mode,NULL);
		}
		i = (sign == '+');
		switch(*mode)
		{
		case 'p':
			chan->private = i;
			break;
		case 's':
			chan->secret = i;
			break;
		case 'm':
			chan->moderated = i;
			break;
		case 't':
			chan->topprot = i;
			break;
		case 'i':
			chan->invite = i;
			break;
		case 'n':
			chan->nomsg = i;
			break;
		}
		break;
/* k */
	case 'k':
		parm = chop(&rest);
		enfm = reverse_mode(from,chan,'k',sign);
		if (sign == '+')
		{
			chan->keymode = TRUE;
			/*
			 *  Undernet clueless-coder-kludge
			 */
			chan->hiddenkey = (parm) ? FALSE : TRUE;
			if (enfm && parm)
			{
				send_mode(chan,160,QM_RAWMODE,'-','k',parm);
			}
			Free(&chan->key);
			set_mallocdoer(on_mode);
			chan->key = stringdup((parm) ? parm : "???");
		}
		else
		{
			if (enfm && parm)
			{
				send_mode(chan,160,QM_RAWMODE,'+','k',parm);
			}
			chan->keymode = FALSE;
		}
		break;
/* l */
	case 'l':
		if (sign == '+')
		{
			parm = chop(&rest);
			chan->limit = asc2int(parm);
			if (errno)
				chan->limit = 0;
			chan->limitmode = TRUE;
		}
		else
		{
			chan->limitmode = FALSE;
		}
		if (reverse_mode(from,chan,'l',sign))
		{
			if (sign == '+')
			{
				send_mode(chan,160,QM_RAWMODE,'-','l',NULL);
			}
			else
			{

				sprintf(templimit,"%i",chan->limit);
				send_mode(chan,160,QM_RAWMODE,'+','l',templimit);
			}
		}
		break;
	case 0:
		return;
	}
	mode++;
	goto modeloop;
}

void common_public(Chan *chan, char *from, char *spyformat, char *rest)
{
	ChanUser *doer;
	int	n,upper;

	if (current->spy & SPYF_CHANNEL)
		send_spy(chan->name,spyformat,CurrentNick,rest);

	if (!chan->bot_is_op)
		return;

	doer = find_chanuser(chan,from);

	/* check if more than half of rest is caps */
	n = upper = 0;
	while(rest[n])
	{
		if ((rest[n] >= 'A' && rest[n] <= 'Z') || (rest[n] == '!'))
			upper += 2;
		n++;
	}

	/* trigger caps flood action */
	if (upper >= n)
	{
		if (check_mass(chan,doer,INT_CKL))
			send_kick(chan,CurrentNick,KICK_CAPS);
	}

	if (chan->setting[TOG_KS].int_var)
	{
		if (!CurrentUser || !CurrentUser->x.x.access)
			check_kicksay(chan,doer,rest);
	}

	if (check_mass(chan,doer,INT_FL))
	{
		if (chan->setting[INT_FPL].int_var > 1)
			deop_ban(chan,doer,NULL);
		send_kick(chan,CurrentNick,KICK_TEXTFLOOD);
		send_spy(SPYSTR_STATUS,"%s kicked from %s for flooding",from,chan->name);
	}
}

void on_action(char *from, char *to, char *rest)
{
	if (CurrentChan)
	{
		common_public(CurrentChan,from,"* %s %s",rest);
		return;
	}
	if (CurrentDCC)
	{
		partyline_broadcast(CurrentDCC,"* %s %s\n",rest);
#ifdef BOTNET
		botnet_relay(NULL,"PM* * %s@%s \001%s\n",CurrentNick,current->nick,rest);
#endif /* BOTNET */
		return;
	}
	if (current->spy & SPYF_MESSAGE)
		send_spy(SPYSTR_MESSAGE,"* %s %s",CurrentNick,rest);
}

#ifdef DYNCMD

/*
 *
 *
 *
 */

void do_chaccess(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	int	i,oldaccess,newaccess,uaccess,dis;
	char	*name,*axs;

	name = chop(&rest);
	axs = chop(&rest);

	if (!axs && !name)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	dis = FALSE;
	newaccess = asc2int(axs);
	if (axs)
	{
		if (!stringcasecmp(axs,"disable"))
		{
			dis = TRUE;
			newaccess = 100;
		}
		else
		{
			if (errno || (newaccess < 0) || (newaccess > OWNERLEVEL))
			{
				to_user(from,"Command access level must be between 0 and %i",OWNERLEVEL);
				return;
			}
		}
	}

	uaccess = get_useraccess(from,ANY_CHANNEL);
	if (newaccess > uaccess)
	{
		to_user(from,"Can't change access level to one higher than yours");
		return;
	}
	if (dis && uaccess < OWNERLEVEL)
	{
		to_user(from,"Insufficient access to disable commands");
		return;
	}

	for(i=0;mcmd[i].name;i++)
	{
		if (!stringcasecmp(mcmd[i].name,name))
		{
			oldaccess = acmd[i];
			if (dis || oldaccess > 200)
			{
				to_user(from,"The command \"%s\" has been permanently disabled",name);
				acmd[i] = 250; /* unsigned char, max 255 */
				return;
			}
			if (newaccess == -1)
			{
				to_user(from,"The access level needed for that command is %i",oldaccess);
				to_user(from,"To change it, specify new access level");
				return;
			}
			if (oldaccess > uaccess)
			{
				to_user(from,"Can't change an access level that is higher than yours");
				return;
			}
			if (oldaccess == newaccess)
				to_user(from,"The access level was not changed");
			else
				to_user(from,"Command access level changed from %i to %i",oldaccess,newaccess);
			acmd[i] = newaccess;
			return;
		}
	}
	to_user(from,"Unknown command: %s",name);
}

#endif /* DYNCMD */

int access_needed(char *name)
{
	int	i;

	for(i=0;mcmd[i].name;i++)
	{
		if (!stringcasecmp(mcmd[i].name,name))
		{
			return(acmd[i]);
		}
	}
	return(-1);
}

void do_last(COMMAND_ARGS)
{
	char	*pt,*thenum;
	int	i,num;

	if (!rest || !*rest)
		num = 5;
	else
	{
		thenum = chop(&rest);
		num = asc2int(thenum);
	}
	if ((num < 1) || (num > LASTCMDSIZE))
		usage(from);	/* usage for CurrentCmd->name */
	else
	{
		pt = (char*)1;
		table_buffer(TEXT_LASTHDR,num);
		for(i=0;i<num;i++)
		{
			if (!pt)
				break;
			pt = current->lastcmds[i];
			table_buffer(FMT_PLAIN,(pt) ? pt : TEXT_NONE);
		}
		table_send(from,1);
	}
}
