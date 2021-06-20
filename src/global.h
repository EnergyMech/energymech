/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2018 proton

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
#ifndef GLOBAL_H
#define GLOBAL_H 1

#ifdef MAIN_C

#define MDEF(x)		= x
#define BEG		LS

#else /* MAIN_C */

#define MDEF(x)
#define BEG		extern

#endif /* MAIN_C */

#include "githash.h"

/*
 *
 */

#define DEFAULTCMDCHAR			'-'
#define MECHUSERLOGIN			"v3.energymech.net"

BEG const char VERSION[]		MDEF("3.1p" GITHASH);
BEG const char SRCDATE[]		MDEF("April 14th, 2018");
#ifdef __CYGWIN__
BEG const char BOTCLASS[]		MDEF("WinMech");
#else /* ! CYGWIN */
BEG const char BOTCLASS[]		MDEF("EnergyMech");
#endif /* CYGWIN */
BEG const char BOTLOGIN[]		MDEF("emech");

BEG const char NULLSTR[]		MDEF("<NULL>");

BEG const char ERR_CHAN[]		MDEF("I'm not on %s");
BEG const char ERR_FILEOPEN[]		MDEF("Couldn't open the file %s");
BEG const char ERR_INIT[]		MDEF("init: Warning:");
BEG const char ERR_NICK[]		MDEF("Invalid nickname: %s");
BEG const char ERR_NOCHANNELS[]		MDEF("I'm not active on any channels");
BEG const char ERR_NOTOPPED[]		MDEF("I'm not opped on %s");
BEG const char ERR_UNKNOWN_COMMAND[]	MDEF("Squeeze me?");

BEG const char SPYSTR_RAWIRC[]		MDEF("rawirc");
BEG const char SPYSTR_MESSAGE[]		MDEF("message");
BEG const char SPYSTR_STATUS[]		MDEF("status");
BEG const char SPYSTR_BOTNET[]		MDEF("botnet");
#ifdef URLCAPTURE
BEG const char SPYSTR_URL[]		MDEF("url");
#endif /* URLCAPTURE */
#if defined URLCAPTURE || defined HOSTINFO
BEG const char SPYSTR_SYSMON[]		MDEF("sysmon");
#endif /* URLCAPTURE */
BEG const char SPYSTR_RANDSRC[]		MDEF("randsrc");

BEG const char STR_MECHRESET[]		MDEF("MECHRESET=");

BEG const char FMT_6XSTRTAB[]		MDEF("%s\t%s\t%s\t%s\t%s\t%s");
#define FMT_4XSTRTAB			&FMT_6XSTRTAB[6]
#define FMT_3XSTRTAB			&FMT_6XSTRTAB[9]
#define FMT_PLAIN			&FMT_6XSTRTAB[15]

BEG Mech	*botlist		MDEF(NULL);
BEG Mech	*current;

BEG char	*executable;
BEG char	*configfile		MDEF(CFGFILE);
BEG char	*mechresetenv		MDEF(NULL);

BEG time_t	uptime			MDEF(0);
BEG int		do_exec			MDEF(FALSE);	/* call mech_exec on mechexit */
BEG int		makecore		MDEF(FALSE);
BEG int		respawn			MDEF(0);
BEG int		sigmaster		MDEF(0);
BEG int		ctimeout		MDEF(30);	/* proc var */
BEG int		startup			MDEF(TRUE);
BEG ino_t	parent_inode;

BEG KillSock	*killsocks		MDEF(NULL);

BEG Server	*serverlist		MDEF(NULL);
BEG ServerGroup	*servergrouplist	MDEF(NULL);
BEG ServerGroup	*currentservergroup	MDEF(NULL);
BEG int		servergroupid		MDEF(0);
BEG int		serverident		MDEF(1);

BEG char	CurrentNick[NUHLEN];
BEG Client	*CurrentDCC		MDEF(NULL);
BEG Chan	*CurrentChan		MDEF(NULL);
BEG User	*CurrentUser		MDEF(NULL);
BEG Shit	*CurrentShit		MDEF(NULL);
BEG const OnMsg	*CurrentCmd		MDEF(NULL);
BEG User	*cfgUser		MDEF(NULL);
BEG const char	*global_from		MDEF(NULL);

BEG User	__internal_users[2];
#define CoreUser (__internal_users[0])
#define LocalBot (__internal_users[1])

/*
 *  generic output buffer, can be used as buffer in any `leaf' function
 *  (functions that do not call any other non-trivial functions)
 */
BEG char	globaldata[MAXLEN];
BEG char	nick_buf[MAXHOSTLEN];
BEG char	nuh_buf[NUHLEN];

/*
 *  select() stuff.
 */

BEG fd_set	read_fds;
BEG fd_set	write_fds;
BEG int		hisock;
BEG int		short_tv;

/*
 *  current UNIX timestamp
 */

BEG time_t	now;

/*
 *  defined features
 */

#ifdef ALIAS

BEG Alias	*aliaslist		MDEF(NULL);

#endif /* ALIAS */

#ifdef BOTNET

BEG const char	UNKNOWNATUNKNOWN[]	MDEF("unknown@unknown");
#define		UNKNOWN			(&UNKNOWNATUNKNOWN[8])

BEG BotNet	*botnetlist		MDEF(NULL);
BEG NetCfg	*netcfglist		MDEF(NULL);
BEG char	*linkpass		MDEF(NULL);	/* proc var */
BEG int		linkport		MDEF(0);	/* proc var */
BEG int		autolink		MDEF(0);	/* proc var */
BEG time_t	last_autolink		MDEF(0);
BEG NetCfg	*autolink_cfg		MDEF(NULL);
BEG int		global_tick		MDEF(0);

#endif /* BOTNET */

#ifdef BOUNCE

BEG ircLink	*bnclist		MDEF(NULL);
BEG int		bounce_sock		MDEF(-1);
BEG int		bounce_port		MDEF(0);	/* proc var */

#endif /* BOUNCE */

#ifdef CTCP

BEG time_t	ctcp_slot[CTCP_SLOTS];

#endif /* CTCP */

#ifdef DEBUG

BEG char	debugbuf[MAXLEN];
BEG char	*debugfile		MDEF(NULL);
BEG int		dodebug			MDEF(FALSE);
BEG int		debug_fd		MDEF(-1);
BEG int		debug_on_exit		MDEF(FALSE);
BEG aMEA	*mrrec;
BEG void	*mallocdoer;

#endif /* DEBUG */

#ifdef HOSTINFO
BEG FileMon	*filemonlist		MDEF(NULL);
#endif /* HOSTINFO */

#ifdef NOTE

BEG Note	*notelist		MDEF(NULL);

#endif /* NOTE */

#ifdef RAWDNS

BEG dnsList	*dnslist		MDEF(NULL);
BEG dnsAuthority *dnsroot		MDEF(NULL);
BEG struct in_addr ia_ns[MAX_NAMESERVERS];
BEG struct in_addr ia_default;

#ifdef SESSION
BEG Strp	*dnsrootfiles		MDEF(NULL);
#endif /* SESSION */
#endif /* RAWDNS */

#ifdef REDIRECT

BEG struct
{
	char	*to;
	int	method;
#ifdef BOTNET
	int	guid;
#endif /* BOTNET */

} redirect;

#endif /* REDIRECT */

#ifdef SCRIPTING

BEG Hook	*hooklist		MDEF(NULL);

#endif /* SCRIPTING */

#ifdef SEEN

BEG char	*seenfile		MDEF(NULL);	/* proc var */
BEG Seen	*seenlist		MDEF(NULL);

#endif /* SEEN */

BEG char *fontname			MDEF(NULL);
BEG BigC *fontlist			MDEF(NULL);
BEG int charlines;
BEG int charheight;
BEG int spacewidth;
BEG int kerning;

#ifdef TRIVIA

BEG int		triv_qdelay		MDEF(30);	/* proc var */
BEG char	*triv_qfile		MDEF(NULL);	/* proc var */
BEG char	triv_qchar		MDEF('*');	/* proc var */
BEG TrivScore	*scorelist		MDEF(NULL);

#endif /* TRIVIA */

#ifdef UPTIME

BEG int		uptimeport		MDEF(9969);	/* proc var */
BEG char	*uptimehost		MDEF(NULL);	/* proc var */
BEG char	*uptimenick		MDEF(NULL);	/* proc var */
BEG int		uptimesock;
BEG uint32_t	uptimeip		MDEF((uint32_t)-1);
BEG uint32_t	uptimecookie;
BEG uint32_t	uptimeregnr		MDEF(0);
BEG time_t	uptimelast		MDEF(0);
BEG const char	*defaultuptimehost	MDEF("uptime.energymech.net");

#endif /* UPTIME */

#ifdef URLCAPTURE

BEG int		urlhistmax		MDEF(20);	/* proc var */
BEG Strp	*urlhistory		MDEF(NULL);

#endif /* URLCAPTURE */

#ifdef WEB

BEG int		websock			MDEF(-1);
BEG int		webport			MDEF(0);

#endif /* WEB */

#ifndef I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS

BEG int		spawning_lamer		MDEF(0);

#endif /* I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS */

/*
 *  attrtab defines
 */
#define NUM	0x01
#define NICK	0x02
#define FIRST	0x04
#define CRLF	0x08

#define FNICK	(NICK|FIRST)
#define NNICK	(NICK|NUM)

#if defined(MAIN_C) || defined(MAKETABLES)

/*
 *  tolowertab blatantly ripped from ircu2.9.32
 */
LS const uchar tolowertab[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	' ',  '!',  '"',  '#',  '$',  '%',  '&',  0x27,
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/*
 *  be wary, this is not a normal upper-to-lower table...
 */
LS const uchar nickcmptab[256] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	' ',  0x00, '"',  '#',  '$',  '%',  '&',  0x27,		/* <-- observe! the '!' changed to 0x00 */
	'(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
	'0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',
	'8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
	'@',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  '_',
	'`',  'a',  'b',  'c',  'd',  'e',  'f',  'g',
	'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
	'p',  'q',  'r',  's',  't',  'u',  'v',  'w',
	'x',  'y',  'z',  '{',  '|',  '}',  '~',  0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
	0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
	0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
	0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
	0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
	0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

LS const uchar attrtab[256] =
{
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x00 - 0x07 */
	0,	0,	CRLF,	0,	0,	CRLF,	0,	0,	/* 0x08 - 0x0F */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x10 - 0x17 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x18 - 0x1F */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x20 - 0x27 */
	0,	0,	0,	0,	0,	NICK,	0,	0,	/* 0x28 - 0x2F */
	NNICK,	NNICK,	NNICK,	NNICK,	NNICK,	NNICK,	NNICK,	NNICK,	/* 0x30 - 0x37 */
	NNICK,	NNICK,	0,	0,	0,	0,	0,	0,	/* 0x38 - 0x3F */
	0,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x40 - 0x47 */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x48 - 0x4F */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x50 - 0x57 */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x58 - 0x5F */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x60 - 0x67 */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x68 - 0x6F */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	/* 0x70 - 0x77 */
	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	FNICK,	0,	0,	/* 0x78 - 0x7F */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x80 - 0x87 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x88 - 0x8F */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x90 - 0x97 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0x98 - 0x9F */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xA0 - 0xA7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xA8 - 0xAF */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xB0 - 0xB7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xB8 - 0xBF */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xC0 - 0xC7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xC8 - 0xCF */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xD0 - 0xD7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xD8 - 0xDF */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xE0 - 0xE7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xE8 - 0xEF */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xF0 - 0xF7 */
	0,	0,	0,	0,	0,	0,	0,	0,	/* 0xF8 - 0xFF */
};

#endif /* MAIN_C || MAKETABLES */

#ifdef MAIN_C

/*
 *  user struct for the core client
 */
LS const Strp CMA =
{
	NULL,
	"*"
};

/*
 *  client struct for the core client
 */
LS ShortClient CoreClient =
{
	NULL,			/* next */
	(User*)&CoreUser,	/* user */
	-1,			/* socket */
	0,			/* flags */
	0,			/* inputcount */
	0			/* lasttime */
};

LS ShortChan CoreChan =
{
	NULL,
	NULL
};

typedef struct coreServerGroup
{
	ServerGroup	*next;
	int		servergroup;
	char		name[8];
} coreServerGroup;

LS coreServerGroup defaultServerGroup =
{
	NULL,			/* next */
	0,			/* servergroup */
	"default"		/* name */
};

LS struct
{
	const char *string;
	const int id;

} meventstrings[] = {
{ "parse",		MEV_PARSE },
{ "timer",		MEV_TIMER },
{ "command",		MEV_COMMAND },
{ "botnet",		MEV_BOTNET },
{ "dcc_complete",	MEV_DCC_COMPLETE },
{ "dnsresult",		MEV_DNSRESULT }};

#else /* MAIN_C */

extern const uchar tolowertab[];
extern const uchar nickcmptab[];
extern const uchar attrtab[];
extern const User xxCoreUser;
extern const User xxLocalBot;
extern ShortClient CoreClient;
extern ShortChan CoreChan;
extern ServerGroup defaultServerGroup;

#endif /* MAIN_C */

#endif /* GLOBAL_H */
