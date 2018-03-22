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

typedef struct BigC
{
	struct	BigC *next;

	int	width;
	Strp	*data;
	char	chars[1];

} BigC;

LS char *fontname = NULL;
LS BigC *newchar;

LS BigC *fontlist = NULL;
LS int charlines;
LS int charheight;
LS int spacewidth;
LS int kerning;

LS BigC *orig_fontlist = NULL;
LS int orig_charlines;
LS int orig_charheight;
LS int orig_spacewidth;
LS int orig_kerning;

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
		pp = &newchar->data;
		while(*pp)
			pp = &(*pp)->next;
		set_mallocdoer(read_bigcharset_callback);
		*pp = sp = (Strp*)Calloc(sizeof(Strp) + sz);
		/* Calloc sets to zero sp->next = NULL; */
		Strcpy(sp->p,rest);
		return(FALSE);
	}

	opt = chop(&rest);
	n = NULL;

	if (!Strcasecmp(opt,"chars") && charheight)
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
		Strcpy(newchar->chars,opt);
	}
	else
	if (!Strcasecmp(opt,"spacewidth"))
	{
		n = &spacewidth;
	}
	else
	if (!Strcasecmp(opt,"charheight"))
	{
		n = &charheight;
	}
	else
	if (!Strcasecmp(opt,"kerning"))
	{
		n = &kerning;
	}
	else
	if (!Strcasecmp(opt,"fontname"))
	{
		opt = chop(&rest);
		if (fontname && !Strcasecmp(fontname,opt))
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
		fontname = Strdup(opt);
	}

	if (n)
	{
		*n = a2i(rest);
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
		Strcat(fname,FONT_EXTENSION);
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
		while(bigc->data)
		{
			sp = bigc->data;
			bigc->data = sp->next;
			Free((char**)&sp);
		}
		Free((char**)&bigc);
	}

	return(0);
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

	Strcpy(output,BIGSAY_DEFAULTFONT);

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
					temp = sp->p;
					temp2 = tail;
					while(*temp && tail < OEND)
						*(tail++) = *(temp++);
					//temp = Strcat(tail,sp->p);
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
	char	*filename,*message;

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

#endif /* TOYBOX */
