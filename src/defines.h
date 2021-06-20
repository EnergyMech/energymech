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
#ifndef DEFINES_H
#define DEFINES_H 1

/*
 *  dont export too many symbols...
 */
#ifdef LIBRARY
#define LS	static
#else /* not LIBRARY */
#define LS	/* nothing */
#endif /* LIBRARY */

/*
 *  startup ==
 */
#define STARTUP_RUNNING		0
#define STARTUP_ECHOTOCONSOLE	(startup > 1 && startup < 1000)
#define STARTUP_COMMANDANDEXIT	3
#define STARTUP_TESTRUN		6
#define STARTUP_NORMALSTART	1000
#define STARTUP_SIGSEGV		31337

/*
 *  Channel Crap
 */

#define CHAN_ANY		0
#define CHAN_ACTIVE		1

#define ANY_CHANNEL		NULL
#define GLOBAL_CHANNEL		MATCH_ALL

#define COLLECT_TYPE		1
#define REJOIN_TYPE		2
#define CYCLE_TYPE		4

#define ADD_MODE		1
#define SUB_MODE		2
#define MODE_FORCE		3

#define MODE_PRIVATE		0x0004
#define MODE_SECRET		0x0008
#define MODE_MODERATED		0x0010
#define MODE_TOPICLIMIT		0x0020
#define MODE_INVITEONLY		0x0040
#define MODE_NOPRIVMSGS		0x0080
#define MODE_KEY		0x0100
#define MODE_BAN		0x0200
#define MODE_LIMIT		0x0400
#define MODE_FLAGS		0x07ff

#define CU_VOICE		0x0001
#define CU_CHANOP		0x0002
#define CU_VOICETMP		0x0004
#define CU_NEEDOP		0x0008
#define CU_MODES		0x00ff

#define CU_DEOPPED		0x0100
#define CU_KICKED		0x0200
#define CU_BANNED		0x0400
#define CU_MASSTMP		0x0800
#define CU_KSWARN		0x1000
#define CU_CHANBAN		0x2000

#define SHIT_NOOP		0		/* do nothing */
#define SHIT_CHANOP		1		/* not allowed to be chanop */
#define SHIT_KB			2		/* simple kick and ban */
#define SHIT_PERMABAN		3		/* permanent ban, re-ban if unbanned */
#define SHIT_CHANBAN		4

#define MAXSHITLEVEL		4
#define MAXSHITLEVELSTRING	"4"
#define MAXSHITLEVELCHAR	'4'

#define __STR_ON		"on"
#define __STR_OFF		"off"

/*
 *  channel modequeues
 */
#define QM_RAWMODE	0
#define QM_CHANUSER	1

#define QM_PRI_LOW	100

/*
 *  Bitfield for short_tv being set to 1 or 30 seconds
 */

#define TV_TELNET_NICK		0x0002
#define TV_UCACHE		0x0004
#define TV_SERVCONNECT		0x0008
#define TV_LINEBUF		0x0010
#define TV_BOTNET		0x0020
#define TV_REJOIN		0x0040
#define TV_TRIVIA		0x0080

/* Misc Crap: */

#define EXTRA_CHAR(q)		q==1?"":"s"
#define SPLIT			1

/* Parse Stuff */

#define PA_END			1	/* b00000001 */		/* mask for `end' bit */
#define PA_WHOIS		2	/* b00000010 */
#define PA_LUSERS		4	/* b00000100 */
#define PA_STATS		8	/* b00001000 */
#define PA_USERHOST		16	/* b00010000 */
#define PA_DNS			32	/* b00100000 */		/* for resolving hosts */
#define PA_TYPE			62	/* b00111110 */		/* mask for type bits */

/* DCC Crap: */

#define DCC_SEND		0x0001
#define DCC_RECV		0x0002
#define DCC_WAIT		0x0010
#define DCC_ASYNC		0x0020
#define DCC_ACTIVE		0x0040
#define DCC_TELNET		0x0080
#define DCC_TELNETPASS		0x0100
#define DCC_DELETE		0x0200

/*
 *  format_uh()
 */

#define FUH_USERHOST		1
#define FUH_HOST		2

/* Type of Variable: */

#define INT_VAR			0x01
#define STR_VAR			0x02
#define TOG_VAR			0x04
#define GLOBAL_VAR		0x08
#define PROC_VAR		0x10
#define CHR_VAR			(0x20 | INT_VAR)
#define ANY_VAR			0xff

#define INT_GLOBAL		INT_VAR|GLOBAL_VAR
#define TOG_GLOBAL		TOG_VAR|GLOBAL_VAR
#define TOG_PROC		TOG_VAR|PROC_VAR|GLOBAL_VAR
#define INT_PROC		INT_VAR|PROC_VAR|GLOBAL_VAR
#define STR_PROC		STR_VAR|PROC_VAR|GLOBAL_VAR
#define CHR_PROC		CHR_VAR|PROC_VAR|GLOBAL_VAR
#define STR_GLOBAL		STR_VAR|GLOBAL_VAR
#define CHR_GLOBAL		CHR_VAR|GLOBAL_VAR

#define IsInt(x)		(VarName[x].type & INT_VAR)
#define IsStr(x)		(VarName[x].type & STR_VAR)
#define IsTog(x)		(VarName[x].type & TOG_VAR)
#define IsNum(x)		(VarName[x].type & (INT_VAR|TOG_VAR))
#define IsChar(x)		((VarName[x].type & CHR_VAR) == CHR_VAR)
#define IsProc(x)		(VarName[x].type & PROC_VAR)

/*
 *  is_safepath
 */
#define	FILE_IS_SAFE		1
#define	FILE_MUST_EXIST		1
#define FILE_MAY_EXIST		2
#define FILE_MUST_NOTEXIST	3

/*
 *  see settings.h for the actual setting struct
 */
enum {
	/*
	 *  channel settings
	 */
	TOG_ABK,
	TOG_AOP,
	INT_AUB,
	INT_AVOICE,
#ifdef CHANBAN
	TOG_CHANBAN,
#endif /* CHANBAN */
	INT_CKL,
	TOG_CTL,
#ifdef DYNAMODE
	STR_DYNLIMIT,
#endif /* DYNAMODE */
	TOG_ENFM,
	STR_ENFMODES,
	INT_FL,
	INT_FPL,
	INT_IKT,
	TOG_KS,
	INT_MAL,
	INT_MBL,
	INT_MDL,
	INT_MKL,
	INT_MPL,
	INT_NCL,
	INT_PROT,
	TOG_PUB,
	TOG_RK,
	TOG_SD,
	TOG_SHIT,
	TOG_SO,
#ifdef STATS
	STR_STATS,
#endif /* STATS */
	TOG_TOP,
	/*
	 *  global settings
	 *  Note: first global setting (now: INT_AAWAY) is used below ...
	 */
	INT_AAWAY,
	STR_ALTNICK,
#ifdef BOTNET
	TOG_AUTOLINK,
#endif /* BOTNET */
#ifdef BOUNCE
	INT_BNCPORT,
#endif /* BOUNCE */
	TOG_CC,
	CHR_CMDCHAR,
#ifdef CTCP
	TOG_CTCP,
#endif /* CTCP */
	INT_CTIMEOUT,
#ifdef DCC_FILE
	INT_DCCANON,
	STR_DCCFILES,
	INT_DCCUSER,
#endif /* DCC_FILE */
	TOG_ENFPASS,
	STR_IDENT,
	STR_IRCNAME,
#ifdef NOTIFY
	INT_ISONDELAY,
#endif /* NOTIFY */
#ifdef BOTNET
	STR_LINKPASS,
	INT_LINKPORT,
#endif /* BOTNET */
	INT_MODES,
#ifdef BOTNET
	TOG_NETUSERS,
#endif /* BOTNET */
	TOG_NOIDLE,
#ifdef NOTIFY
	STR_NOTIFYFILE,
#endif /* NOTIFY */
	TOG_ONOTICE,
#ifdef TRIVIA
	CHR_QCHAR,
	INT_QDELAY,
	STR_QFILE,
#endif /* TRIVIA */
#ifdef CTCP
	TOG_RF,
	TOG_RV,
#endif /* CTCP */
#ifdef SEEN
	STR_SEENFILE,
#endif /* SEEN */
	STR_SERVERGROUP,
	TOG_SPY,
	STR_UMODES,
#ifdef UPTIME
	STR_UPHOST,
	STR_UPNICK,
	INT_UPPORT,
#endif /* UPTIME */
#ifdef URLCAPTURE
	INT_URLHISTMAX,
#endif /* URLCAPTURE */
	STR_USERFILE,
	STR_VIRTUAL,
#ifdef WEB
	INT_WEBPORT,
#endif /* WEB */
#ifdef WINGATE
	STR_WINGATE,
	INT_WINGPORT,
#endif /* WINGATE */
	__NULL_VAR__,
	SIZE_VARS
};

/*
 *  why would channel structs contain global vars?
 *  they shouldnt! and now they dont! :)
 */
#define CHANSET_SIZE	INT_AAWAY

/*
 *  For botlinks
 */

#ifdef BOTNET

/* BotNet->status */

#define BN_UNKNOWN		0
#define BN_DEAD			1
#define BN_LINKSOCK		2
#define BN_CONNECT		3
#define BN_BANNERSENT		4
#define BN_WAITAUTH		5
#define BN_WAITLINK		6
#define BN_LINKED		7

#define BNAUTH_PLAINTEXT	0
#define BNAUTH_DES		1
#define BNAUTH_MD5		2
#define BNAUTH_SHA		3

#endif /* BOTNET */

/* for connect status */

#define CN_NOSOCK		0
#define CN_DNSLOOKUP		1
#define CN_TRYING		2
#define CN_CONNECTED		3
#define CN_ONLINE		4
#define CN_DISCONNECT		5
#define CN_BOTDIE		6
#define CN_NEXTSERV		7
#define CN_WINGATEWAIT		8
#define CN_SPINNING		9	/* after exhausting serverlist */

#define SERVERSILENCETIMEOUT	360	/* server connection idle timeout */

/* DCC Kill flags (BYE command) */

#define DCC_NULL		0
#define DCC_COMMAND		1
#define DCC_KILL		2

/* VHOST types */

#define VH_ZERO			0
#define VH_IPALIAS		(1 << 1)
#define VH_IPALIAS_FAIL		(1 << 2)
#define VH_IPALIAS_BOTH		(VH_IPALIAS|VH_IPALIAS_FAIL)
#define VH_WINGATE		(1 << 3)
#define VH_WINGATE_FAIL		(1 << 4)
#define VH_WINGATE_BOTH		(VH_WINGATE|VH_WINGATE_FAIL)

/* server error types */

#define SP_NULL			0
#define SP_NOAUTH		1
#define SP_KLINED		2
#define SP_FULLCLASS		3
#define SP_TIMEOUT		4
#define SP_ERRCONN		5
#define SP_DIFFPORT		6
#define SP_NO_DNS		7
#define SP_THROTTLED		8

/* find_channel() */

#define CH_ACTIVE		0x1
#define CH_OLD			0x2
#define CH_ANY			0x3

/* check_mass() */

#define INDEX_FLOOD		0
#define INDEX_BAN		1
#define INDEX_DEOP		2
#define INDEX_KICK		3
#define INDEX_NICK		4
#define INDEX_CAPS		5
#define INDEX_MAX		6

#define CHK_CAPS		0
#define CHK_PUB			1
#define CHK_PUBLIC		CHK_PUB
#define CHK_DEOP		2
#define CHK_BAN			3
#define CHK_KICK		4

/*
 *  seen selector types
 */
#define SEEN_PARTED		0
#define SEEN_QUIT		1
#define SEEN_NEWNICK		2
#define SEEN_KICKED		3

/*
 *  spying types, source and target types are mixed
 */
#define SPY_FILE		1
#define SPY_DCC			2
#define SPY_CHANNEL		3
#define SPY_STATUS		4
#define SPY_MESSAGE		5
#define SPY_RAWIRC		6
#define SPY_BOTNET		7
#ifdef URLCAPTURE
#define SPY_URL			8
#endif /* URLCAPTURE */
#ifdef HOSTINFO
#define SPY_SYSMON		9
#endif /* HOSTINFO */
#define SPY_RANDSRC		10

#define SPYF_ANY		1
#define SPYF_CHANNEL		(1 << SPY_CHANNEL)
#define SPYF_STATUS		(1 << SPY_STATUS)
#define SPYF_MESSAGE		(1 << SPY_MESSAGE)
#define SPYF_RAWIRC		(1 << SPY_RAWIRC)
#define SPYF_BOTNET		(1 << SPY_BOTNET)
#define SPYF_URL		(1 << SPY_URL)
#define SPYF_RANDSRC		(1 << SPY_RANDSRC)
/*
 *  notify defines
 */
#define NF_OFFLINE		0
#define NF_WHOIS		1
#define NF_MASKONLINE		2	/* anything above NF_MASKONLINE is "online" */
#define NF_NOMATCH		3

/*
 *  uptime defines
 */

#define UPTIME_ENERGYMECH       1	/* http://www.energymech.net		*/
#define UPTIME_EGGDROP		2	/* http://www.eggheads.org		*/
#define UPTIME_MINIMECH		3	/* http://www.energymech.net		*/
#define UPTIME_WINMECH		4	/* http://www.energymech.net		*/
#define UPTIME_RACBOT		5	/* http://www.racbot.org		*/
#define UPTIME_MIRC		6	/* http://www.mirc.com			*/
#define UPTIME_HAL9000		7	/* http://www.2010.org			*/
#define UPTIME_ANABOT		8	/* http://www.sirklabs.hu/ana-liza/	*/
#define UPTIME_ANGELBOT		9	/* unknown				*/
#define UPTIME_FIRECLAW		10	/* http://www.fireclaw.org		*/
#define UPTIME_GARNAX		11	/* http://garnax.mircx.com		*/
#define UPTIME_WINEGGDROP	12	/* http://www.eggheads.org		*/
#define UPTIME_SUPYBOT		14	/* http://supybot.sourceforge.net	*/

#define UPTIME_GENERICDEATH     5000	/* generic death */
#define UPTIME_SIGSEGV          5001
#define UPTIME_SIGBUS           5002
#define UPTIME_SIGTERM          5003
#define UPTIME_SIGINT           5004

#define UPTIMEHOST		"uptime.energymech.net"

#ifdef __CYGWIN__
#define UPTIME_BOTTYPE		UPTIME_WINMECH
#else
#define UPTIME_BOTTYPE		UPTIME_ENERGYMECH
#endif /* __CYGWIN__ */

/*
 *  scripting events
 */
#define MEV_PARSE		0
#define MEV_TIMER		1
#define MEV_COMMAND		2
#define MEV_BOTNET		3
#define MEV_DCC_COMPLETE	4
#define MEV_DNSRESULT		5

/*
 *
 */
#define IRCX_WALLCHOPS		1
#define IRCX_WALLVOICES		2
#define IRCX_IMODE		4
#define IRCX_EMODE		8

/*
 *  stats.c
 */
#define CSTAT_PARTIAL		1

/*
 *  dns.c
 */
#define MAX_NAMESERVERS		4

/*
 *  redirect.c
 */
#define R_NOTICE		0
#define R_PRIVMSG		1
#define R_FILE			2
#define R_BOTNET		3

#endif /* DEFINES_H */

