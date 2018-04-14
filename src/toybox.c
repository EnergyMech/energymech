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
	Strp	*sp,**pp;
	char	*opt;
	int	*n,sz;

	if (charlines)
	{
		charlines--;
		sz = strlen(rest);
		if (sz > newchar->width)
			newchar->width = sz;
		append_strp(&newchar->data,rest);
		return(FALSE);
	}

	opt = chop(&rest);
	n = NULL;

	if (!stringcasecmp(opt,"chars") && charheight)
	{
		charlines = charheight;

		opt = chop(&rest);
		set_mallocdoer(read_bigcharset_callback);
		newchar = (BigC*)Calloc(sizeof(BigC) + strlen(opt));
		/* Calloc sets to zero
		newchar->width = 0;
		newchar->data = NULL;
		*/
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
	Strp	*sp;
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

	charlines = 0;
	spacewidth = 0;
	charheight = 0;
	kerning = 0;

	readline(fd,&read_bigcharset_callback);		/* readline closes fd */

	/*
	 *  free the old font if there is one
	 */
	while(orig_fontlist)
	{
		bigc = orig_fontlist;
		orig_fontlist = bigc->next;
		if (bigc->data)
			purge_strplist(bigc->data);
		Free((char**)&bigc);
	}

	return(0);
}

int read_ascii(char *rest)
{
	to_user_q(global_from,FMT_PLAIN,rest);
}

/*
 *
 *
 *
 */

void do_bigsay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CARGS + CAXS
	 */
	BigC	*bigc;
	Strp	*sp;
	char	output[MSGLEN];
	char	*pt,*tail,*temp,*temp2;
	int	i,x,sz;

#ifdef DEBUG
	debug("(do_bigsay) rest = \"%s\"\n",rest);
#endif /* DEBUG */

	stringcpy(output,BIGSAY_DEFAULTFONT);

	if (read_bigcharset(output) < 0)
	{
		to_user(from,ERR_FILEOPEN,output);
		return;
	}

#define OEND	(output + MSGLEN - 1)
	for(i=0;i<charheight;i++)
	{
		sz = 0;
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

	if (!rest && *rest == 0)
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
