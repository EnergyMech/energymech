/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2004 proton

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
#define TRIVIA_C
#include "config.h"

#ifdef TRIVIA
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#define TRIV_WAIT_QUESTION	0
#define TRIV_HINT_TWO		1
#define TRIV_HINT_THREE		2
#define TRIV_NO_ANSWER		3

#define TRIV_HINT_DELAY		15

#define TRIV_HINT_DELAY_STR1	"15"
#define TRIV_HINT_DELAY_STR2	"30"

#define TRIV_METACHARS		" .,-'%&/?!:;\""

#define DAY_IN_SECONDS		(24*60*60)
#define WEEK_IN_SECONDS		(7*24*60*60)

LS TrivScore *lastwinner;
LS Chan *triv_chan = NULL;
LS Strp *triv_answers = NULL;
LS time_t triv_ask_time;
LS time_t triv_next_time = 0;
LS time_t triv_weektop10;
LS int triv_mode;
LS int triv_score;
LS int triv_streak;
LS int triv_halt_flag;

/*
 *
 *  scorings
 *
 */

void trivia_week_toppers(void)
{
	char	triv_str[MSGLEN];
	TrivScore *chosen[10];
	TrivScore *su;
	char	*p;
	int	week;
	int	i,x;

	chosen[0] = NULL;
	week = (now + (3 * DAY_IN_SECONDS)) / WEEK_IN_SECONDS;

	for(su=scorelist;su;su=su->next)
	{
		if (su->week_nr != week)
			continue;
		for(i=0;i<10;i++)
		{
			if (!chosen[i] || (chosen[i]->score_wk < su->score_wk))
			{
				for(x=8;x>=i;x--)
					chosen[x+1] = chosen[x];
				chosen[i] = su;
				break;
			}
		}
	}

	if (chosen[0])
	{
		p = triv_str;
		for(i=0;i<10;i++)
		{
			if (!chosen[i])
				break;
			if (i)
			{
				*(p++) = ' ';
				*(p++) = ' ';
			}
			sprintf(p,"#%i: %s (%ipts)",i+1,chosen[i]->nick,chosen[i]->score_wk);
			p = STREND(p);
		}
		to_server("PRIVMSG %s :This Weeks Top 10: %s\n",triv_chan->name,triv_str);
	}
}

/*
 *
 *
 *
 */

void hint_one(void)
{
	char	triv_str[MSGLEN];
	char	*src,*dst;

	sprintf(triv_str,"PRIVMSG %s :1st hint: ",triv_chan->name);
	dst = STREND(triv_str);
	src = triv_answers->p;
	while(*src)
	{
		if (STRCHR(TRIV_METACHARS,*src))
			*(dst++) = *src;
		else
			*(dst++) = triv_qchar;
		src++;
	}
	*dst = 0;
	to_server("%s   Question score: %i points\n",triv_str,triv_score);
}

void hint_two(void)
{
	char	triv_str[MSGLEN];
	char	*src,*dst;
	int	n;

	sprintf(triv_str,"PRIVMSG %s :2nd hint: ",triv_chan->name);
	dst = STREND(triv_str);
	src = triv_answers->p;

	n = asc2int(src);
	if (!errno)
	{
		if (n >   99 && *src) *(dst++) = *(src++);
		if (n >  999 && *src) *(dst++) = *(src++);
		if (n > 9999 && *src) *(dst++) = *(src++);
	}
	else
	{
		n = strlen(src);
		if (n > 2 && *src) *(dst++) = *(src++);
		if (n > 4 && *src) *(dst++) = *(src++);
		if (n > 6 && *src) *(dst++) = *(src++);
	}

	while(*src)
	{
		if (STRCHR(TRIV_METACHARS,*src))
			*(dst++) = *src;
		else
			*(dst++) = triv_qchar;
		src++;
	}
	*dst = 0;
	to_server("%s   " TRIV_HINT_DELAY_STR2 " seconds remaining.\n",triv_str);
}

void hint_three(void)
{
	char	triv_str[MSGLEN];
	char	*src,*dst;
	int	n;

	sprintf(triv_str,"PRIVMSG %s :3rd hint: ",triv_chan->name);
	dst = STREND(triv_str);
	src = triv_answers->p;

	n = asc2int(src);
	if (!errno)
	{
		if (n >   9 && *src) *(dst++) = *(src++);
		if (n >  99 && *src) *(dst++) = *(src++);
		if (n > 999 && *src) *(dst++) = *(src++);
	}
	else
	{
		n = strlen(src);
		if (n > 1 && *src) *(dst++) = *(src++);
		if (n > 3 && *src) *(dst++) = *(src++);
		if (n > 4 && *src) *(dst++) = *(src++);
	}

	while(*src)
	{
		if (STRCHR(TRIV_METACHARS "aeiouyAEIOUY",*src))
			*(dst++) = *src;
		else
			*(dst++) = triv_qchar;
		src++;
	}
	*dst = 0;
	to_server("%s   " TRIV_HINT_DELAY_STR1 " seconds remaining.\n",triv_str);
}

/*
 *
 *
 *
 */

void trivia_cleanup(void)
{
	Strp	*ans;

	triv_mode = TRIV_WAIT_QUESTION;
	triv_next_time = now + triv_qdelay;
	while((ans = triv_answers))
	{
		triv_answers = ans->next;
		Free((char**)&ans);
	}
}

void trivia_check(Chan *chan, char *rest)
{
	TrivScore *su;
	Strp	*ans;
	int	week;

	if (chan != triv_chan)
		return;

	for(ans=triv_answers;ans;ans=ans->next)
	{
		if (!stringcasecmp(ans->p,rest))
			goto have_answer;
	}
	return;

have_answer:
	week = (now + (3 * DAY_IN_SECONDS)) / WEEK_IN_SECONDS;

	for(su=scorelist;su;su=su->next)
	{
		if (!nickcmp(su->nick,CurrentNick))
		{
			su->score_mo += triv_score;

			if (su->week_nr == week)
				su->score_wk += triv_score;
			else
			{
				if (su->week_nr == (week - 1))
					su->score_last_wk = su->score_wk;
				else
					su->score_last_wk = 0;
				su->week_nr = week;
				su->score_wk = triv_score;
			}
			break;
		}
	}
	if (!su)
	{
		set_mallocdoer(trivia_check);
		su = (TrivScore*)Calloc(sizeof(TrivScore) + strlen(CurrentNick));
		su->next = scorelist;
		scorelist = su;
		su->score_wk = su->score_mo = triv_score;
		su->week_nr = week;
		/* su->month_nr = 0; * fix this */
		stringcpy(su->nick,CurrentNick);
	}

	to_server("PRIVMSG %s :Yes, %s! got the answer -> %s <- in %i seconds, and gets %i points!\n",
		triv_chan->name,CurrentNick,triv_answers->p,
		(int)(now - triv_ask_time),triv_score);

	if (su == lastwinner)
	{
		triv_streak++;
		if (su->score_wk == su->score_mo)
		{
			to_server("PRIVMSG %s :%s has won %i in a row! Total score this Week: %i points\n",
				triv_chan->name,CurrentNick,triv_streak,su->score_wk);
		}
		else
		{
			to_server("PRIVMSG %s :%s has won %i in a row! Total score this Week: %i points, and this Month: %i points\n",
				triv_chan->name,CurrentNick,triv_streak,su->score_wk,su->score_mo);
		}
	}
	else
	{
		lastwinner = su;
		triv_streak = 1;
		if (su->score_wk == su->score_mo)
		{
			to_server("PRIVMSG %s :%s has won the question! Total score this Week: %i points\n",
				triv_chan->name,CurrentNick,su->score_wk);
		}
		else
		{
			to_server("PRIVMSG %s :%s has won the question! Total score this Week: %i points, and this Month: %i points\n",
				triv_chan->name,CurrentNick,su->score_wk,su->score_mo);
		}
	}

	trivia_cleanup();
}

void trivia_no_answer(void)
{
	to_server("PRIVMSG %s :Time's up! The answer was -> %s <-\n",
		triv_chan->name,triv_answers->p);

	trivia_cleanup();
}

char *random_question(char *triv_rand)
{
	char	*p,tmpname[120];
	off_t	sz;
	int	fd,ifd;
	int	n;
	struct
	{
		int	off;
		int	sz;

	} entry;

	if (STRCHR(triv_qfile,'/') || strlen(triv_qfile) > 100) // really bad filenames...
		return(NULL);

	stringcat(stringcpy(tmpname,"trivia/"),triv_qfile);

	if ((fd = open(tmpname,O_RDONLY)) < 0)
#ifdef DEBUG
	{
		debug("(random_question) %s: %s\n",tmpname,strerror(errno));
		return(NULL);
	}
#else
		return(NULL);
#endif /* DEBUG */

	stringcpy(triv_rand,tmpname);
	if ((p = STRCHR(triv_rand,'.')) == NULL)
		p = STREND(triv_rand);
	stringcpy(p,".index");

	if ((ifd = open(triv_rand,O_RDONLY)) < 0)
		return(NULL);

	sz = lseek(ifd,0,SEEK_END);
	sz = sz / sizeof(entry);
	n = RANDOM(1,sz);
	n--;

	lseek(ifd,(n * sizeof(entry)),SEEK_SET);
	read(ifd,&entry,sizeof(entry));

	lseek(fd,entry.off,SEEK_SET);
	read(fd,triv_rand,entry.sz);
	triv_rand[entry.sz] = 0;

	close(fd);
	close(ifd);

	return(triv_rand);
}

void trivia_question(void)
{
	char	buffer[MSGLEN];
	char	*question,*answer,*rest;

	if (triv_halt_flag)
	{
		to_server("PRIVMSG %s :Trivia has been stopped!\n",triv_chan->name);
		goto stop_trivia;
	}

	if ((rest = random_question(buffer)) == NULL)
	{
bad_question:
		to_server("PRIVMSG %s :Bad Question File\n",triv_chan->name);
stop_trivia:
		trivia_cleanup();
		triv_chan = NULL;
		triv_next_time = 0;
		triv_halt_flag = FALSE;
		short_tv &= ~TV_TRIVIA;
		return;
	}

	question = get_token(&rest,MATCH_ALL);

	while((answer = get_token(&rest,MATCH_ALL)))
		append_strp(&triv_answers,answer);

	if (triv_answers == NULL)
		goto bad_question;

	triv_score = (RANDOM(2,9) + RANDOM(2,10) + RANDOM(2,10)) / 3;
	triv_ask_time = now;

	if (now > (triv_weektop10 + 1200))
	{
		trivia_week_toppers();
		triv_weektop10 = now;
	}

	to_server("PRIVMSG %s :%s\n",triv_chan->name,question);
	hint_one();
}

void trivia_tick(void)
{
	Chan	*chan;
	Mech	*bot;

	if (triv_next_time && (now >= triv_next_time))
	{
		for(bot=botlist;bot;bot=bot->next)
		{
			for(chan=bot->chanlist;chan;chan=chan->next)
			{
				if (triv_chan == chan)
				{
					current = bot;
					triv_next_time = now + TRIV_HINT_DELAY;
					switch(triv_mode)
					{
					case TRIV_WAIT_QUESTION:
						trivia_question();
						break;
					case TRIV_HINT_TWO:
						hint_two();
						break;
					case TRIV_HINT_THREE:
						hint_three();
						break;
					case TRIV_NO_ANSWER:
						trivia_no_answer();
						return; /* dont increment with triv_mode */
					}
					triv_mode++;
#ifdef DEBUG
					current = NULL;
#endif /* DEBUG */
					return;
				}
			}
		}
	}
}

/*
 *
 *  File operations
 *
 */

void write_triviascore(void)
{
	TrivScore *su;
	int	fd;

	if (scorelist)
	{
		if ((fd = open(TRIVIASCOREFILE,O_WRONLY|O_TRUNC|O_CREAT,NEWFILEMODE)) < 0)
			return;
		to_file(fd,COMMENT_STRCHR " nick score_wk score_mo score_last_wk score_last_mo\n");
		for(su=scorelist;su;su=su->next)
		{
			to_file(fd,"%s %i %i %i %i %i %i\n",su->nick,su->score_wk,su->score_mo,
				su->score_last_wk,su->score_last_mo,su->week_nr,su->month_nr);
		}
		close(fd);
	}
}

int trivia_score_callback(char *rest)
{
	TrivScore *su;
	char	*nick,*wk,*mo,*lwk,*lmo,*wnr,*mnr;
	int	score_wk,score_mo,score_last_wk,score_last_mo;
	int	week_nr,month_nr;
	int	err;

	if (*rest != COMMENT_CHAR)
	{
		nick = chop(&rest);
		wk   = chop(&rest);
		mo   = chop(&rest);
		lwk  = chop(&rest);
		lmo  = chop(&rest);
		wnr  = chop(&rest);
		mnr  = chop(&rest);

		if (mnr)
		{
			score_wk = asc2int(wk);
			err  = errno;
			score_mo = asc2int(mo);
			err += errno;
			score_last_wk = asc2int(lwk);
			err += errno;
			score_last_mo = asc2int(lmo);
			err += errno;
			week_nr = asc2int(wnr);
			err += errno;
			month_nr = asc2int(mnr);
			err += errno;

			if (!err)
			{
				set_mallocdoer(trivia_score_callback);
				su = (TrivScore*)Calloc(sizeof(TrivScore) + strlen(nick));
				su->next = scorelist;
				scorelist = su;
				su->score_wk = score_wk;
				su->score_mo = score_mo;
				su->score_last_wk = score_last_wk;
				su->score_last_mo = score_last_mo;
				su->week_nr = week_nr;
				su->month_nr = month_nr;
				stringcpy(su->nick,nick);
			}
		}
	}
	return(FALSE);
}

void read_triviascore(void)
{
	TrivScore *su;
	int	fd;

	if ((fd = open(TRIVIASCOREFILE,O_RDONLY)) < 0)
		return;

	while(scorelist)
	{
		su = scorelist;
		scorelist = su->next;
		Free((char**)&su);
	}

	readline(fd,&trivia_score_callback);		/* readline closes fd */
}

/*
 *
 *
 *
 */

void do_trivia(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + CARGS
	 */
	Chan	*chan;
	int	uaccess;

	uaccess = get_maxaccess(from);

	if ((chan = find_channel_ac(to)) == NULL)
	{
		if (uaccess) to_user(from,ERR_CHAN,to);
		return;
	}

	if (!stringcasecmp(rest,"start"))
	{
		if (triv_chan)
		{
			if (triv_chan == chan)
			{
				triv_halt_flag = FALSE;
				return;
			}
			if (uaccess) to_user(from,"trivia is already activated on %s!",
				(get_useraccess(from,triv_chan->name)) ? triv_chan->name : "another channel");
			return;
		}
		to_server("PRIVMSG %s :Trivia starting! Get ready...\n",chan->name);
		triv_chan = chan;
		triv_mode = TRIV_WAIT_QUESTION;
		triv_next_time = now + triv_qdelay;
		triv_weektop10 = now;
		lastwinner = NULL;
		short_tv |= TV_TRIVIA;
		if (!scorelist)
		{
			read_triviascore();
		}
	}
	else
	if (!stringcasecmp(rest,"stop"))
	{
		if (chan == triv_chan)
		{
			if (chan == triv_chan)
				to_server("PRIVMSG %s :Trivia shutting down...\n",chan->name);
			triv_halt_flag = TRUE;
		}
	}
	else
	if (!stringcasecmp(rest,"top10"))
	{
		int	n;

		if (triv_chan)
		{
			uaccess = get_authaccess(from,triv_chan->name);
			if (now > (triv_weektop10 + 300))
				n =  1;
			else
				n = uaccess;

			if (n)
			{
				trivia_week_toppers();
				if (!uaccess) triv_weektop10 = now;
			}
		}
	}
}

#endif /* TRIVIA */
