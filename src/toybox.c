/*

    EnergyMech, IRC bot software
    Copyright (c) 2000-2018 proton

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
#define TOYBOX_C
#include "config.h"

#ifdef TOYBOX
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef TRIVIA

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

#endif /* TRIVIA */

#define BIGSAY_DEFAULTFONT	"default"
#define FONT_EXTENSION		".bigchars"

BigC *newchar;
BigC *orig_fontlist = NULL;
int orig_charlines;
int orig_charheight;
int orig_spacewidth;
int orig_kerning;

int read_bigcharset_callback(char *rest)
{
	char	*opt;
	int	*n,sz;

	if (charlines)
	{
		charlines--;
		if (stringcmp("-blank-",rest) == 0)
			rest = "";
		else
		{
			sz = strlen(rest);
			if (sz > newchar->width)
				newchar->width = sz;
		}
		append_strp(&newchar->data,rest);
		return(FALSE);
	}

	opt = chop(&rest);
	n = NULL;

	if (!stringcasecmp(opt,"chars") && charheight)
	{
		charlines = charheight;
		opt = chop(&rest);

#ifdef DEBUG
		debug("(read_bigcharset_callback) New character definition: %s\n",opt);
#endif /* DEBUG */
		set_mallocdoer(read_bigcharset_callback);
		newchar = (BigC*)Calloc(sizeof(BigC) + strlen(opt));

		newchar->next = fontlist;
		fontlist = newchar;
		stringcpy(newchar->chars,opt);
	}
	else
	if (!stringcasecmp(opt,"spacewidth"))
	{
		n = &spacewidth;
	}
	else
	if (!stringcasecmp(opt,"charheight"))
	{
		n = &charheight;
	}
	else
	if (!stringcasecmp(opt,"kerning"))
	{
		n = &kerning;
	}
	else
	if (!stringcasecmp(opt,"fontname"))
	{
		opt = chop(&rest);
		if (fontname && !stringcasecmp(fontname,opt))
		{
			fontlist   = orig_fontlist;
			charlines  = orig_charlines;
			charheight = orig_charheight;
			spacewidth = orig_spacewidth;
			kerning    = orig_kerning;
			orig_fontlist = NULL;
			return(TRUE);
		}
		Free((char**)&fontname);
		set_mallocdoer(read_bigcharset_callback);
		fontname = stringdup(opt);
	}

	if (n)
	{
		*n = asc2int(rest);
		if (errno) *n = 0;
	}
	return(FALSE);
}

int read_bigcharset(char *fname)
{
	BigC	*bigc;
	int	fd;

	if ((fd = open(fname,O_RDONLY)) < 0)
	{
		stringcat(fname,FONT_EXTENSION);
		if ((fd = open(fname,O_RDONLY)) < 0)
			return(-1);
	}

	orig_fontlist = fontlist;
	orig_charlines = charlines;
	orig_charheight = charheight;
	orig_spacewidth = spacewidth;
	orig_kerning = kerning;

	fontlist = NULL;
	charlines = 0;
	spacewidth = 0;
	charheight = 0;
	kerning = 0;

	readline(fd,&read_bigcharset_callback);		/* readline closes fd */

	/*
	 *  free the old font if there is one
	 */
#ifdef DEBUG
	debug("(read_bigcharset) purge old charset, if there is one. orig = %p\n",orig_fontlist);
#endif
	while(orig_fontlist)
	{
		bigc = orig_fontlist;
		orig_fontlist = bigc->next;
		if (bigc->data)
			purge_linklist((void**)&bigc->data);
		Free((char**)&bigc);
	}

	return(0);
}

int read_ascii(char *rest)
{
	to_user_q(global_from,FMT_PLAIN,rest);
}

#ifdef TRIVIA

/*
 *
 *  trivia scorings
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

	if (STRCHR(triv_qfile,'/') || strlen(triv_qfile) > 100) /* really bad filenames... */
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
	if (read(ifd,&entry,sizeof(entry)) == -1)
		return(NULL);

	lseek(fd,entry.off,SEEK_SET);
	if (read(fd,triv_rand,entry.sz) == -1)
		return(NULL);

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

#endif /* TRIVIA */

/*
-+=====================================================================================================================+-
                                                        toybox.c commands
-+=====================================================================================================================+-
*/

void do_bigsay(COMMAND_ARGS)
{
#define OEND	(output + MSGLEN - 1)
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	BigC	*bigc;
	Strp	*sp;
	char	output[MSGLEN];
	char	*pt,*tail,*temp,*temp2;
	int	i,x;

	if (fontname && *rest != '-')
		goto reuse_font;

	stringcat(stringcpy(output,COMMONDIR),BIGSAY_DEFAULTFONT);

	if (rest[0] == '-')
	{
		temp = chop(&rest);
		if (temp[1] == '-')
			; /* allow .bigsay -- -dash- */
		else
		if (STRCHR(temp,'/') == NULL) /* no filesystem perversions... */
		{
			stringcat(stringcat(stringcpy(output,COMMONDIR),temp+1),".bigchars"); /* temp+1 = skip initial '-' */
		}
	}
#ifdef DEBUG
	debug("(do_bigsay) fontfile %s, text = \"%s\"\n",output,rest);
#endif /* DEBUG */

	if (read_bigcharset(output) < 0)
	{
		to_user(from,ERR_FILEOPEN,output);
		return;
	}

reuse_font:
	for(i=0;i<charheight;i++)
	{
		*output = 0;
		tail = output;
		for(pt=rest;*pt;pt++)
		{
			/* find a matching character */
			if (*pt == ' ')
			{
				x = spacewidth;
				while(x-- && tail < OEND)
					*(tail++) = ' ';
				*tail = 0;
				continue;
			}
			for(bigc=fontlist;bigc;bigc=bigc->next)
			{
				if (STRCHR(bigc->chars,*pt))
				{
					sp = bigc->data;
					for(x=0;x<i;x++)
						if (sp) sp = sp->next;
					temp = (sp) ? sp->p : "";
					temp2 = tail;
					while(*temp && tail < OEND)
						*(tail++) = *(temp++);
					while(tail < (temp2 + bigc->width) && tail < OEND)
						*(tail++) = ' ';
					if (pt[1])
					{
						x = kerning;
						while(x-- && tail < OEND)
							*(tail++) = ' ';
					}
					*tail = 0;
					break;
				}
			}
#ifdef DEBUG
			if (bigc == NULL)
			{
				debug("(do_bigsay) no matching character found for '%c'\n",*pt);
			}
#endif /* DEBUG */
		}
		temp = NULL;
		for(tail=output;*tail;tail++)
		{
			if (!temp && *tail == ' ')
				temp = tail;
			if (*tail != ' ')
				temp = NULL;
		}
		if (temp)
		{
			if (temp == output)
				temp++;
			*temp = 0;
		}
		to_user_q(from,FMT_PLAIN,output);
	}
}

void do_random_msg(COMMAND_ARGS)
{
	const char *filename;
	const char *message;

	filename =  CurrentCmd->cmdarg;

	if (*rest)
	{
		to = chop(&rest);
		if (ischannel(to) && (get_authaccess(from,to) < cmdaccess))
			return;
	}
	else
	if (!ischannel(to))
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if ((message = randstring(filename)) == NULL)
	{
		to_user(from,ERR_FILEOPEN,filename);
		return;
	}

	/*
	 *  send message to target nick/channel
	 */
	to_server("PRIVMSG %s :%s\n",to,message);
	/*
	 *  if its not a channel we send a copy to the user who did the command also
	 */
	if (!ischannel(to))
	{
		to_user(from,"(%s) %s",to,message);
	}
}

void do_randtopic(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS
	 */
	Chan	*chan = CurrentChan;
	char	*topic;

	/*
	 *  the bot can set a random topic if chanmode is -t
	 */
	if (chan->bot_is_op || chan->topprot == FALSE)
	{
		if ((topic = randstring(RANDTOPICSFILE)) == NULL)
		{
			to_user(from,ERR_FILEOPEN,RANDTOPICSFILE);
			return;
		}
		to_server("TOPIC %s :%s\n",to,topic);
		to_user(from,TEXT_TOPICCHANGED,to);
		return;
	}
	to_user(from,ERR_NOTOPPED,to);
}

void do_8ball(COMMAND_ARGS)
{
	char	*message;

	if ((message = randstring(RAND8BALLFILE)) == NULL)
	{
		to_user_q(from,ERR_FILEOPEN,RAND8BALLFILE);
		return;
	}

	to_user_q(from,FMT_PLAIN,message);
}

void do_ascii(COMMAND_ARGS)
{
	char	fname[MSGLEN];
	int	fd;

#ifdef DEBUG
	if (STRCHR(rest,'/'))
	{
		debug("(do_ascii) '/' not permitted in filename\n");
ascii_badfile:
		to_user_q(from,"%s","Bad filename or file does not exist");
		return;
	}
	stringcat(stringcpy(fname,"ascii/"),rest);
	debug("(do_ascii) file = \"%s\"\n",fname);
	if (is_safepath(fname,FILE_MUST_EXIST) != FILE_IS_SAFE)
	{
		debug("(do_ascii) is_safepath() not safe\n");
		goto ascii_badfile;
	}
	if ((fd = open(fname,O_RDONLY)) < 0)
	{
		debug("(do_ascii) open(%s): %s\n",fname,strerror(errno));
		goto ascii_badfile;
	}
#else
	if (STRCHR(rest,'/'))
	{
ascii_badfile:
		to_user_q(from,"%s","Bad filename or file does not exist");
		return;
	}
	stringcat(stringcpy(fname,"ascii/"),rest);
	if (is_safepath(fname,FILE_MUST_EXIST) != FILE_IS_SAFE)
		goto ascii_badfile;
	if ((fd = open(fname,O_RDONLY)) < 0)
		goto ascii_badfile;
#endif

	global_from = from;
	readline(fd,&read_ascii);		/* readline closes fd */
}

/*
help:RAND:[[min[-| ]]max|"nick"|"luser"]

Tell the user a random number or channel nick.

Valid forms:

RAND		Number between 1 and 100 (incl. 1 and 100)
RAND 10		Number between 1 and 10  (incl. 1 and 10)
RAND 0 10	Number between 0 and 10  (incl. 0 and 10)
RAND 0-10	Same
RAND nick	Random channel non-bot user
RAND luser	Random channel user with no bot access

By default, the bot picks a number between 1 and 100 (incl. 1 and 100)

If the command ``RAND nick'' is issued in a channel, the bot picks a random channel
user (after eliminating all known bots) and says the nick in the channel.

If the command ``RAND luser'' is issued in a channel, the bot picks a random channel
user that has no access in the bots userlist and says the nick in the channel.

*/
void do_rand(COMMAND_ARGS)
{
	const char *opt;
	const ChanUser *cu;
	int	r,min = 1,max = 100,maxaccess = -1;

	if (!rest || *rest == 0)
		goto pick_randnum;

	if (attrtab[(uchar)*rest] & NUM)
	{
		max = 0;
		while(attrtab[(uchar)*rest] & NUM)
			max = 10 * max + (*(rest++) - '0');
		if (*rest == '-' || *rest == ' ')
			rest++;
		if ((attrtab[(uchar)*rest] & NUM) == 0)
			goto pick_randnum;
		min = max;
		max = 0;
		while(attrtab[(uchar)*rest] & NUM)
			max = 10 * max + (*(rest++) - '0');
		goto pick_randnum;
	}
	opt = chop(&rest);
	if (stringcasecmp(opt,"nick") == 0)
		maxaccess = OWNERLEVEL;
	if (stringcasecmp(opt,"luser") == 0)
		maxaccess = 0;
	if (maxaccess >= 0 && CurrentChan)
	{
		max = 0;
		for(cu=CurrentChan->users;cu;cu=cu->next)
		{
			if (get_useraccess(get_nuh(cu),to) <= maxaccess)
				max++;
		}
		if (max < 2)
		{
			to_user_q(from,"Unable to pick at random, too few choices");
			return;
		}
		r = RANDOM(0,max);
#ifdef DEBUG
		debug("(do_rand) pick a rand user: max = %i, r = %i\n",max,r);
#endif
		max = 0;
		for(cu=CurrentChan->users;cu;cu=cu->next)
		{
			if (get_useraccess(get_nuh(cu),to) <= maxaccess)
			{
				if (r == max)
				{
					to_user_q(from,"Random %suser: %s",(maxaccess == 0) ? "l" : "",cu->nick);
					return;
				}
				max++;
			}
		}
		to_user_q(from,"Unable to pick at random...");
		return;
	}
pick_randnum:
	if (max == 0 || max < min)
		max = 100;
	if (min < 0 || min > max)
		min = 1;
	r = RANDOM(min,max);
#ifdef DEBUG
	debug("(do_rand) pick a rand number: min = %i, max = %i, r = %i\n",min,max,r);
#endif
	to_user_q(from,"Random number: %i",r);
}

#endif /* TOYBOX */

#ifdef TRIVIA

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
