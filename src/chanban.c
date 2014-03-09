/*

    EnergyMech, IRC bot software
    Copyright (c) 2009 proton

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
#define CHANBAN_C
#include "config.h"

#ifdef CHANBAN

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"

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
		if (current->sendq) // only do chanbans on empty queue
		{
#ifdef DEBUG
			debug("(process_chanbans) skipping %s (%i), sendq not empty\n",current->nick,current->guid);
#endif /* DEBUG */
			continue;
		}

		selcu = NULL;
		for(anychan=current->chanlist;anychan;anychan=anychan->next)
		{
			if (anychan->modelist || anychan->kicklist) // only do chanbans on empty queue
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

	// the channel is shitted and the user is on it...
	// 1, make sure the bot isnt on the channel
	// 2, kb the user on all channels where the shit is active and i am op

	// check all current channels
	for(CurrentChan=current->chanlist;CurrentChan;CurrentChan=CurrentChan->next)
	{
		if (!Strcasecmp(channel,CurrentChan->name)) // if the bot is on the channel, skip it
		{
#ifdef DEBUG
			debug("(chanban_action) skipping %s: bot is on channel\n",channel);
#endif /* DEBUG */
			return;
		}
		// is the shit for this channel?
		if (!Strcasecmp(shit->chan,CurrentChan->name))
		{
			// if chanban is turned on && if bot is op (pretty pointless otherwise)
			if (CurrentChan->setting[TOG_CHANBAN].int_var && CurrentChan->bot_is_op)
			{
#ifdef DEBUG
				debug("(chanban_action) %s: Tog is on, I am op\n",CurrentChan->name);
#endif /* DEBUG */
				cu = find_chanuser(CurrentChan,nick);
				if (!(cu->flags & CU_CHANBAN))
				// dont kickban the same user multiple times from the same channel
				{
					nuh = get_nuh(cu); // clobbers nuh_buf
#ifdef DEBUG
					debug("(chanban_action) slapping %s on %s for being on %s (mask %s): %s\n",
						nick,CurrentChan->name,channel,shit->mask,shit->reason);
#endif /* DEBUG */
					cu->flags |= CU_CHANBAN;
					format_uh(nuh,1); // returns mask in 'nuh' buffer (nuh_buf)
					send_mode(CurrentChan,90,QM_RAWMODE,'+','b',(void*)nuh);
					send_kick(CurrentChan,nick,"%s (%s)",shit->reason,channel); // clobbers gsockdata
				}
			}
		}
	}
}

/*
 *
 *  commands
 *
 */

#endif /* CHANBAN */
