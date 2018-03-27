/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2018 proton

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
#ifndef SETTINGS_H
#define SETTINGS_H 1
#ifdef VARS_C

#define DEFAULTCMDCHAR	'-'

#define ZERO		0
#define INTCAST(x)	.v.num=x
#define CMDCHAR		.v.chr=DEFAULTCMDCHAR
#define VNULL		.v.str=NULL
#define TOGPROC(x)	.v.numptr=&x
#define CHRPROC(x)	.v.str=&x
#define INTPROC(x)	.v.numptr=&x
#define STRPROC(x)	.v.strptr=&x

LS const Setting VarName[SIZE_VARS] =
{
/*
 *  all channel settings in the beginning
 */
/* TYPE		UACCES	MIN	DEFAULT		NAME		MAX		*/
{ TOG_VAR,	40,	0,	ZERO,		"ABK",		1		},
{ TOG_VAR,	40,	0,	ZERO,		"AOP",		1		},	/* autoop enable */
{ INT_VAR,	40,	0,	ZERO,		"AUB",		86400		},
{ INT_VAR,	40,	0,	INTCAST(1),	"AVOICE",	2		},
#ifdef CHANBAN
{ TOG_VAR,	40,	0,	ZERO,		"CHANBAN",	1,		},	/* chanban enable */
#endif /* CHANBAN */
{ INT_VAR,	40,	0,	ZERO,		"CKL",		20		},
{ TOG_VAR,	40,	0,	ZERO,		"CTL",		1		},
#ifdef DYNAMODE
{ STR_VAR,	40,	0,	VNULL,		"DYNLIMIT",	1		},	/* settings for dynamode: `delay:window:minwin' */
#endif /* DYNAMODE */
{ TOG_VAR,	40,	0,	ZERO,		"ENFM",		1		},
{ STR_VAR,	40,	0,	VNULL,		"ENFMODES"			},	/* modes to enforce, +ENFM to enable */
{ INT_VAR,	40,	0,	INTCAST(6),	"FL",		20		},	/* number of lines that counts as a text flood */
{ INT_VAR,	40,	0,	ZERO,		"FPL",		2		},
{ INT_VAR,	40,	0,	INTCAST(0),	"IKT",		40320		},	/* idle-kick: minutes of idle-time (max 4 weeks) */
{ TOG_VAR,	40,	0,	ZERO,		"KS",		1		},	/* kicksay enable */
{ INT_VAR,	40,	0,	INTCAST(90),	"MAL",		200		},
{ INT_VAR,	40,	2,	INTCAST(7),	"MBL",		20		},
{ INT_VAR,	40,	2,	INTCAST(7),	"MDL",		20		},
{ INT_VAR,	40,	2,	INTCAST(7),	"MKL",		20		},
{ INT_VAR,	40,	0,	INTCAST(1),	"MPL",		2		},	/* mass action levels: 0=off, 1=kick, 2=kickban */
{ INT_VAR,	40,	2,	INTCAST(20),	"NCL",		20		},
{ INT_VAR,	40,	0,	INTCAST(4),	"PROT",		4		},	/* max enforced protection level */
{ TOG_VAR,	40,	0,	INTCAST(1),	"PUB",		1		},	/* public commands */
{ TOG_VAR,	40,	0,	ZERO,		"RK",		1		},	/* revenge kick enable */
{ TOG_VAR,	40,	0,	ZERO,		"SD",		1		},	/* server-op deop enable */
{ TOG_VAR,	40,	0,	INTCAST(1),	"SHIT",		1		},	/* shitlist enable */
{ TOG_VAR,	40,	0,	ZERO,		"SO",		1		},	/* safe-op enable */
#ifdef STATS
{ STR_VAR,	80,	0,	VNULL,		"STATS"				},	/* statistics log file */
#endif /* STATS */
{ TOG_VAR,	40,	0,	ZERO,		"TOP",		1		},
/*
 *  all global variables
 */
/* TYPE		UACCES	MIN	DEFAULT			NAME		MAX	*/
{ INT_GLOBAL,	40,	0,	ZERO,			"AAWAY",	1440	},	/* set auto-away after ___ minutes */
{ STR_GLOBAL,	90,	0,	VNULL,			"ALTNICK"		},	/* alternative nick */
#ifdef BOTNET
{ TOG_PROC,	90,	0,	TOGPROC(autolink),	"AUTOLINK",	1	},	/* establish links automagically */
#endif /* BOTNET */
#ifdef BOUNCE
{ INT_PROC,	100,	0,	INTPROC(bounce_port),	"BNCPORT",	65535,	(&new_port_bounce) },	/* irc proxy port to listen on */
#endif /* BOUNCE */
{ TOG_GLOBAL,	90,	0,	INTCAST(1),		"CC",		1	},	/* require command char */
{ CHR_GLOBAL,	90,	1,	CMDCHAR,		"CMDCHAR",	255	},	/* command char */
#ifdef CTCP
{ TOG_GLOBAL,	90,	0,	INTCAST(1),		"CTCP",		1	},	/* ctcp replies enable */
#endif /* CTCP */
{ INT_PROC,	100,	10,	INTPROC(ctimeout),	"CTIMEOUT",	3600	},	/* how long to wait between connect attempts */
#ifdef DCC_FILE
{ INT_GLOBAL,	80,	0,	ZERO,			"DCCANON",	100	},	/* anonymous (non user) DCC slots */
{ STR_GLOBAL,	80,	0,	VNULL,			"DCCFILES"		},	/* string with space separated masks for auto-accepted filenames */
{ INT_GLOBAL,	80,	0,	INTCAST(4),		"DCCUSER",	100	},	/* user DCC slots */
#endif /* DCC_FILE */
{ TOG_GLOBAL,	80,	0,	ZERO,			"ENFPASS",	1	},	/* disallow users with no passwords */
{ STR_GLOBAL,	90,	0,	VNULL,			"IDENT"			},	/* register with this in the `user' field */
{ STR_GLOBAL,	90,	0,	VNULL,			"IRCNAME"		},	/* register with this in the `real name' field */
#ifdef NOTIFY
{ INT_GLOBAL,	80,	10,	INTCAST(30),		"ISONDELAY",	600	},	/* seconds between each ISON */
#endif /* NOTIFY */
#ifdef BOTNET
{ STR_PROC,	90,	0,	STRPROC(linkpass),	"LINKPASS"		},	/* local process linkpass */
{ INT_PROC,	100,	0,	INTPROC(linkport),	"LINKPORT",	65535	},	/* listen on <linkport> for botnet connections */
#endif /* BOTNET */
{ INT_GLOBAL,	80,	1,	INTCAST(3),		"MODES",	20	},	/* max number of channel modes to send */
#ifdef BOTNET
{ TOG_GLOBAL,	90,	0,	INTCAST(1),		"NETUSERS",	1	},	/* this bot accepts shared users (on by default) */
#endif /* BOTNET */
{ TOG_GLOBAL,	80,	0,	ZERO,			"NOIDLE",	1	},	/* dont idle */
#ifdef NOTIFY
{ STR_GLOBAL,	80,	0,	VNULL,			"NOTIFYFILE"		},	/* read notify settings from <notifyfile> */
#endif /* NOTIFY */
{ TOG_GLOBAL,	90,	0,	ZERO,			"ONOTICE",	1	},	/* ircd has /notice @#channel */
#ifdef TRIVIA
{ CHR_PROC,	80,	0,	CHRPROC(triv_qchar),	"QCHAR"			},	/* use <qchar> as mask char when displaying answer */
{ INT_PROC,	80,	1,	INTPROC(triv_qdelay),	"QDELAY",	3600	},	/* seconds between each question */
{ STR_PROC,	80,	0,	STRPROC(triv_qfile),	"QFILE"			},	/* load questions from <qfile> */
#endif /* TRIVIA */
#ifdef CTCP
{ TOG_GLOBAL,	80,	0,	ZERO,			"RF",		1	},	/* random ctcp finger reply */
{ TOG_GLOBAL,	80,	0,	ZERO,			"RV",		1	},	/* random ctcp version reply */
#endif /* CTCP */
#ifdef SEEN
{ STR_PROC,	90,	0,	STRPROC(seenfile),	"SEENFILE"		},	/* load/save seen database from <seenfile> */
#endif /* SEEN */
{ STR_GLOBAL,	80,	0,	VNULL,			"SERVERGROUP"		},	/* connect bot to a certain group of servers */
{ TOG_GLOBAL,	90,	0,	ZERO,			"SPY",		1	},	/* send info about executed commands to status channel */
{ STR_GLOBAL,	90,	0,	VNULL,			"UMODES"		},	/* send these modes on connect */
#ifdef UPTIME
{ STR_PROC,	100,	0,	STRPROC(uptimehost),	"UPHOST"		},	/* send uptime packets to <uphost> */
{ STR_PROC,	100,	0,	STRPROC(uptimenick),	"UPNICK"		},	/* send <upnick> as identifier instead of bots nick */
{ INT_PROC,	100,	0,	INTPROC(uptimeport),	"UPPORT",	65535	},	/* send packets to port <upport> */
#endif /* UPTIME */
#ifdef URLCAPTURE
{ INT_PROC,	100,	0,	INTPROC(urlhistmax),	"URLHIST",	100	},	/* max # of url's to keep in history */
#endif /* URLCAPTURE */
{ STR_GLOBAL,	90,	0,	VNULL,			"USERFILE"		},	/* what file to load/save userlist from/to */
{ STR_GLOBAL,	90,	0,	VNULL,			"VIRTUAL", 	0,	(&var_resolve_host) },	/* visual host */
#ifdef WEB
{ INT_PROC,	100,	0,	INTPROC(webport),	"WEBPORT",	65535	},	/* httpd should listen on... */
#endif /* WEB */
#ifdef WINGATE
{ STR_GLOBAL,	90,	0,	VNULL,			"WINGATE",	0,	(&var_resolve_host) },	/* wingate hostname */
{ INT_GLOBAL,	90,	0,	ZERO,			"WINGPORT",	65535	},	/* wingate port */
#endif /* WINGATE */
{ 0, }};

#undef ZERO
#undef INTCAST
#undef CHARCAST

#else /* VARS_C */

extern const Setting VarName[];

#endif /* VARS_C */
#endif /* SETTINGS_H */
