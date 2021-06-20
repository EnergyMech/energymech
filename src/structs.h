/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2020 proton

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
#ifndef STRUCTS_H
#define STRUCTS_H 1

typedef union usercombo
{
	struct
	{
		uint32_t access:8,		/* access level (0-200)		[0-255]	*/
			prot:3,			/* protlevel (0-4) 		[0-7]	*/
#ifdef BOTNET
			noshare:1,		/* dont share this user over botnet	*/
			readonly:1,		/* botnet cannot alter this user	*/
#endif /* BOTNET */
#ifdef GREET
			greetfile:1,		/* greeting is filename			*/
			randline:1,		/* grab random line from filename	*/
#endif /* GREET */
#ifdef BOUNCE
			bounce:1,		/* user has access to bouncer		*/
#endif /* BOUNCE */
			echo:1,			/* partyline echo of own messages	*/
			aop:1,			/* auto-opping				*/
			avoice:1;		/* auto-voicing				*/
	} x;
	uint32_t	comboflags;

} usercombo;

typedef struct OnMsg
{
	const char	*name;
	void		(*func)(char *, const char *, char *, const int);
	uint32_t	defaultaccess:8,	/* defaultaccess */
			dcc:1,
			cc:1,
			pass:1,
			args:1,
			nopub:1,
			nocmd:1,
			gaxs:1,
			caxs:1,
			redir:1,
			lbuf:1,
			cbang:1,
			acchan:1,
			supres:1; /* -- 21 bits */
	const char	*cmdarg;

} OnMsg;

typedef unsigned char OnMsg_access;

#ifndef GENCMD_C

typedef struct ircLink
{
	struct		ircLink *next;

	int		servsock;
	int		usersock;

	int		status;
	time_t		active;

	char		*userLine;
	char		*nickLine;

	char		*nick;			/* which nick to speak to */
	char		*handle;

#ifdef IDWRAP
	char		*idfile;
#endif /* IDWRAP */

	char		servmem[MSGLEN];
	char		usermem[MSGLEN];

} ircLink;

typedef struct
{
	int		tenminute;
	int		hour;

} SequenceTime;

typedef struct DEFstruct
{
	union
	{
		int	id;
		void	*func;

	} v;
	char		*idstr;

} DEFstruct;

typedef struct Alias
{
	struct		Alias *next;

	char		*format;
	char		alias[1];

} Alias;

typedef struct nfLog
{
	struct		nfLog *next;

	time_t		signon;
	time_t		signoff;

	char		*realname;
	char		userhost[2];

} nfLog;

typedef struct Notify
{
	struct		Notify *next;

	int		status;
	time_t		checked;	/* when nick was last checked */

	nfLog		*log;		/* online + offline + userhost + realname */

	char		*info;
	char		*endofmask;
	char		*mask;
	char		nick[3];

} Notify;

typedef struct IReq
{
	struct		IReq *next;

	time_t		when;
	int		t;

	char		*nick;
	char		from[2];

} IReq;

typedef struct Seen
{
	struct		Seen *next;

	time_t		when;
	int		t;

	/*
	 *  dont mess with the order of these
	 */
	char		*pa;
	char		*pb;
	char		*userhost;
	char		nick[4];

} Seen;

typedef union UniVar
{
	union		UniVar *proc_var;

	char		*str_var;
	char		char_var;
	int		int_var;

} UniVar;

typedef struct Setting
{
	uchar		type;
	uchar		uaccess;		/* user access to touch/view this setting */
	short		min;
	union
	{
			int	num;
			int	*numptr;
			char	chr;
			char	*str;
			char	**strptr;

	} v;
	char		*name;
	int		max;
	void		(*func)(const struct Setting *);

} Setting;

typedef struct Strp
{
	struct		Strp *next;
	char		p[1];

} Strp;

typedef struct KickSay
{
	struct		KickSay *next;

	int		action;

	/*
	 *  dont mess with the order of these
	 */
	char		*chan;
	char		*reason;
	char		mask[5];

} KickSay;

typedef struct Shit
{
	struct		Shit *next;

	int		action;
	time_t		time;
	time_t		expire;

	/*
	 *  dont mess with the order of these
	 */
	char		*chan;
	char		*from;
	char		*reason;
	char		mask[4];

} Shit;


/*
 *  this struct is put to use in global.h
 */
typedef struct User
{
	struct		User *next;		/* linked list */

	Strp		*mask;
	Strp		*chan;

	union		usercombo x;

#ifdef GREET
	char		*greet;
#endif /* GREET */

#ifdef NOTE
	Strp		*note;
#endif /* NOTE */

#ifdef BOTNET
	int		modcount;
	uint16_t	guid;
	int		tick;
	int		addsession;
#endif /* BOTNET */

	/*
	 *  dont mess with the order of these
	 */
	char		*pass;
	char		name[2];

} User;

typedef struct Client
{
	struct		Client *next;

	User		*user;

	int		sock;
	int		flags;
	int		inputcount;		/* used to enforce input limit	*/
	time_t		lasttime;               /* can be used for idletime	*/

#ifdef DCC_FILE
	int		fileno;
	int		fileend;
	time_t		start;
	char		*whom;
	char		filename[2];
#endif /* DCC_FILE */

	char		sockdata[MSGLEN];	/* input buffer			*/

} Client;

typedef struct ShortClient
{
	struct		Client *next;

	User		*user;

	int		sock;
	int		flags;
	int		inputcount;		/* used to enforce input limit	*/
	time_t		lasttime;               /* can be used for idletime	*/

} ShortClient;

typedef struct Auth
{
	struct		Auth *next;

	time_t		active;
	User		*user;
	char		nuh[1];

} Auth;

typedef struct ChanUser
{
	struct		ChanUser *next;

	User		*user;
	Shit		*shit;

	int		flags;

	uint8_t		action_num[INDEX_MAX];
	time_t		action_time[INDEX_MAX];
	time_t		idletime;

#ifdef CHANBAN
	time_t		lastwhois;	/* time when the last whois was made to check channels */
#endif /* CHANBAN */

	char		*nick;		/* nick can change without the user reconnecting */
	char		userhost[1];	/* ident & host is static	*/

} ChanUser;

typedef struct qKick
{
	struct		qKick *next;

	char		*reason;
	char		nick[2];

} qKick;

typedef struct qMode
{
	struct		qMode *next;

	int		pri;		/* urgency factor */
	int		type;

	void		*data;		/* nick (ChanUser), key, banmask, limit */
	char		plusminus;	/* +/- */
	char		modeflag;	/* ov, iklmnpst */

} qMode;

typedef struct Ban
{
	struct		Ban *next;

	time_t		time;
#ifdef IRCD_EXTENSIONS
	uint32_t	imode:1,
			emode:1;
#endif /* IRCD_EXTENSIONS */

	char		*bannedby;
	char		banstring[2];

} Ban;

typedef struct ChanStats
{
	int		userseconds;	/* number of users on channel * number of seconds (one hour, 3600 seconds)	*/
	int		LHuserseconds;
	int		users;
	time_t		lastuser;	/* last time a user joined or left (part, quit, kick)				*/

	int		flags;

	int		userpeak;
	int		userlow;

	int		joins;
	int		parts;
	int		kicks;
	int		quits;

	int		privmsg;
	int		notice;

} ChanStats;

typedef struct ShortChan
{
	struct		ShortChan *next;
	const char	*name;

} ShortChan;

typedef struct Chan
{
	struct		Chan *next;

	char		*name;			/* name of the channel			*/
	char		*key;			/* channel key, if any			*/
	char		*topic;			/* channel topic, if any		*/
	char		*kickedby;		/* n!u@h of whomever kicks the bot	*/

	Ban		*banlist;		/* banlist				*/
	qKick		*kicklist;		/* KICK sendq				*/
	qMode		*modelist;		/* MODE sendq				*/

	ChanUser	*users;			/* users				*/
	ChanUser	*cacheuser;		/* cache for find_chanuser()		*/

	int		limit;			/* channel limit			*/

	uint32_t	bot_is_op:1,		/* set if the bot is opped		*/
			private:1,		/* channel mode: +p			*/
			secret:1,		/* channel mode: +s			*/
			moderated:1,		/* channel mode: +m			*/
			topprot:1,		/* channel mode: +t			*/
			limitmode:1,		/* channel mode: +l			*/
			invite:1,		/* channel mode: +i			*/
			nomsg:1,		/* channel mode: +n			*/
			keymode:1,		/* channel mode: +k			*/
			hiddenkey:1,		/* Undernet screwup			*/
			sync:1,			/* join sync status			*/
			wholist:1,		/* first /WHO				*/
			active:1,		/* active or inactive channel		*/
			rejoin:1;		/* trying to rejoin it?			*/

	int		this10,this60;
	int		last10,last60;

#ifdef DYNAMODE
	time_t		lastlimit;
#endif /* DYNAMODE */

#ifdef STATS
	ChanStats	*stats;
#endif /* STATS */

	UniVar		setting[CHANSET_SIZE];	/* channel vars				*/

} Chan;

typedef struct Spy
{
	struct		Spy *next;

	int		t_src;
	int		t_dest;

	union
	{
		Client		*dcc;		/* DCC */
		int		destbot;	/* botnet */
		time_t		delay;		/* randsrc */

	} data;

	const char	*src;
	char		*dest;
	char		p[2];

} Spy;

typedef struct Server
{
	struct		Server *next;

	int		ident;
	int		usenum;
	int		servergroup;
	int		port;
	int		err;
	time_t		lastconnect;
	time_t		lastattempt;
	time_t		maxontime;

	char		realname[NAMEBUF];
	char		name[NAMEBUF];
	char		pass[PASSLEN];

} Server;

typedef struct ServerGroup
{
	struct		ServerGroup *next;

	int		servergroup;
	char		name[1];

} ServerGroup;

typedef struct FileMon
{
	struct		FileMon *next;
	int		fd;
	time_t		nospam;
	char		filename[1];

} FileMon;

typedef struct Mech
{
	struct		Mech *next;

	uint16_t	guid;			/* globally uniqe ID		*/
	int		connect;
	int		sock;
	struct in_addr	ip;			/* for DCC			*/
	int		server;			/* ident of my current server	*/
	int		nextserver;

#ifdef BOTNET
	const char	*supres_cmd;
	int		supres_crc;
#endif

	/*
	 *  Line buffer for non-essential stuff
	 */
	Strp		*sendq;
	time_t		sendq_time;

	/*
	 *  Basic bot information
	 */
	char		*nick;			/* current nickname		*/
	char		*wantnick;		/* wanted nickname		*/
	char		*userhost;
	int		vhost_type;

	uint32_t	reset:1,
			rejoin:1,
			away:1;

	User		*userlist;
	time_t		ul_save;		/* increment for each change, save if needed */

	Chan		*chanlist;
	Chan		*activechan;

	Client		*clientlist;
	Auth		*authlist;
	Shit		*shitlist;
	KickSay		*kicklist;
	IReq		*parselist;
	Spy		*spylist;
	int		spy;

#ifdef NOTIFY
	Notify		*notifylist;
	time_t		lastnotify;		/* ... */
#endif /* NOTIFY */

	time_t		lastreset;		/* last time bot was reset		*/
	time_t		lastantiidle;		/* avoid showing large idle times	*/
	time_t		lastrejoin;		/* last time channels were reset	*/
#ifdef CHANBAN
	time_t		lastchanban;		/* last time a chanban check was run	*/
#endif /* CHANBAN */

	time_t		conntry;		/* when connect try started		*/
						/* re-used for server activity once connected */
	int		heartbeat;		/* handle server timeout stuff		*/
	time_t		activity;		/* Away timer (AAWAY)			*/

	time_t		ontime;			/* how long the bot has been connected	*/

#ifdef IRCD_EXTENSIONS
	int		ircx_flags;
#endif /* IRCD_EXTENSIONS */

	/*
	 *  Buffers for do_die() command.
	 */
	char		*signoff;
	char		*from;

#ifdef IDWRAP
	char		*identfile;
#endif /* IDWRAP */

	/* big buffers at the end */
	UniVar		setting[SIZE_VARS];	/* global vars + channel defaults */
	char		modes[32];
	char		*lastcmds[LASTCMDSIZE];

	/*
	 *  Server socket input buffer
	 */
	char		sockdata[MSGLEN];

} Mech;

typedef struct aME
{
	void		*area;
	void		*doer;
	int		size;
	time_t		when;
	char		touch;

} aME;

typedef struct aMEA
{
	struct		aMEA *next;
	aME		mme[MRSIZE];

} aMEA;

typedef struct KillSock
{
	struct		KillSock *next;
	time_t		time;
	int		sock;

} KillSock;

typedef struct Note
{
	struct		Note *next;

	time_t		start;

	char		*user;
	char		*to;
	char		from[3];

} Note;

typedef struct PartyUser
{
	struct		PartyUser *next;

} PartyUser;

typedef struct BotInfo
{
	struct		BotInfo *next;

	uint16_t	guid;
	int		hops;

	char		*version;
	char		*server;
	char		nuh[4];

} BotInfo;

typedef struct BotNet
{
	struct		BotNet *next;

	int		sock;
	int		status;
	int		has_data;

	/*
	 *  do not touch the above vars!
	 *  they are copied partially in that order in net.c
	 */

	uint16_t	guid;		/* remote bot guid	*/
	int		lsid;		/* local session id	*/
	int		rsid;		/* remote session id	*/

	struct
	{
	uint32_t	pta:1,		/* plain text auth	*/
			sha:1,		/* SHA */
			md5:1;		/* MD5 */

	} opt;

	Mech		*controller;

	int		tick;		/* tick of the remote bot */
	int		addsession;
	time_t		tick_last;

	time_t		when;

	struct		BotInfo *botinfo;
	int		list_complete;

	char		sockdata[MSGLEN];

} BotNet;

typedef struct NetCfg
{
	struct		NetCfg *next;

	uint16_t	guid;
	uint16_t	port;
	uint16_t	linked; /*:1;*/

	char		*host;
	char		pass[2];

} NetCfg;

typedef struct WebDoc
{
	struct		WebDoc *next;

	char		*url;
	void		(*proc)();

} WebDoc;

typedef struct WebSock
{
	struct		WebSock *next;

	int		sock;
	int		status;
	time_t		when;

	WebDoc		*docptr;
	Mech		*ebot;
	Chan		*echan;
	char		*url;

	char		sockdata[MSGLEN];

} WebSock;

typedef struct
{
	time_t		last;
	time_t		next;
	uint32_t	second1;	/*:30;*/
	uint32_t	second2;	/*:30;*/
	uint32_t	minute1;	/*:30;*/
	uint32_t	minute2;	/*:30;*/
	uint32_t	hour;		/*:24;*/
	uint32_t	weekday;	/*:7;*/

} HookTimer;

typedef struct Hook
{
	struct	Hook *next;

	int		(*func)();
	uint16_t	guid;	/* guid filter */
	int		flags;
	union {
	void		*any;
	char		*command;
	char		*host;
	HookTimer	*timer;
	} type;
	char		self[2];

} Hook;

typedef struct TrivScore
{
	struct		TrivScore *next;

	int		score_wk;
	int		score_last_wk;
	int		week_nr;

	int		score_mo;
	int		score_last_mo;
	int		month_nr;

	char		nick[1];

} TrivScore;

typedef struct dnsAuthority
{
	struct dnsAuthority *next;
	struct	in_addr ip;
	unsigned char count;
	char	hostname[1];

} dnsAuthority;

typedef struct dnsList
{
	struct		dnsList *next;
	time_t		when;
	struct in_addr	ip;
	uint16_t	id;
	int		findauth;
	dnsAuthority	*auth;
	dnsAuthority	*auth2;
	char		*cname;
	char		host[1];

} dnsList;

typedef struct dnsQuery
{
	uint16_t	qid;		/* query id */
	uint16_t	flags;
	uint16_t	questions;
	uint16_t	answers;
	uint16_t	authorities;
	uint16_t	resources;

} dnsQuery;

typedef struct BigC
{
	struct	BigC *next;

	int	width;
	Strp	*data;
	char	chars[1];

} BigC;

#endif /* ifndef GENCMD_C */

#endif /* STRUCTS_H */
