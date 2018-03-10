/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2009 proton

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
#define NET_C
#include "config.h"

#ifdef BOTNET

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#if defined(SHACRYPT) || defined(MD5CRYPT)
char *CRYPT_FUNC(const char *, const char *);
#endif

const char banneropt[] = "BB%i %i PTA"
#ifdef SHACRYPT
	" SHA"
#endif /* SHACRYPT */
#ifdef MD5CRYPT
	" MD5"
#endif /* MD5CRYPT */
	"\n";

#ifdef TELNET
char *telnetprompt = TEXT_ENTERNICKNAME;
#endif /* TELNET */
/*
 *  this is a partial copy of the BotNet struct
 */
LS struct
{
	struct	BotNet *next;

	int	sock;
	int	status;
	int	has_data;

} linksock = { NULL, -1, BN_LINKSOCK, 0 };

/*
 *  command lists
 */
typedef struct LinkCmd
{
	char	c1;
	char	c2;
	void	(*func)(BotNet *, char *);

} LinkCmd;

LS const LinkCmd basicProto[] =
{
{ 'B', 'A', basicAuth		},
{ 'B', 'B', basicBanner		},
{ 'B', 'K', basicAuthOK		},
{ 'B', 'L', basicLink		},
{ 'B', 'Q', basicQuit		},
{ 'C', 'O', netchanNeedop	},
{ 'P', 'A', partyAuth		},
#ifdef REDIRECT
{ 'P', 'C', partyCommand	},
#endif /* REDIRECT */
{ 'P', 'M', partyMessage	},
{ 'U', 'T', ushareTick		},
{ 'U', 'U', ushareUser		},
{ 'U', 'D', ushareDelete	},
{  0,    0, NULL		},
};

LS int deadlinks = FALSE;

/*
 *
 *  misc
 *
 */

Mech *get_netbot(void)
{
	Mech	*netbot,*bot;
	int	uid;

	netbot = NULL;
	uid = INT_MAX;
	for(bot=botlist;bot;bot=bot->next)
	{
		if (bot->guid < uid)
		{
			uid = bot->guid;
			netbot = bot;
		}
	}
	return(netbot);
}

void reset_linkable(int guid)
{
	NetCfg	*cfg;

	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == guid)
		{
#ifdef DEBUG
			debug("(reset_linkable) guid %i reset to linkable\n",guid);
#endif /* DEBUG */
			cfg->linked = FALSE;
			return;
		}
	}
}

void botnet_deaduplink(BotNet *bn)
{
	bn->status = BN_DEAD;
	deadlinks = TRUE;
	reset_linkable(bn->guid);
}

NetCfg *find_netcfg(int guid)
{
	NetCfg	*cfg;

	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == guid)
			return(cfg);
	}
	return(NULL);
}

BotInfo *make_botinfo(int guid, int hops, char *nuh, char *server, char *version)
{
	BotInfo	*new;

	set_mallocdoer(make_botinfo);
	new = (BotInfo*)Calloc(sizeof(BotInfo) + Strlen(nuh,server,version,NULL));

	new->guid = guid;
	new->hops = hops;

	new->server = Strcat(new->nuh,nuh) + 1;
	new->version = Strcat(new->server,server) + 1;
	Strcpy(new->version,version);

	return(new);
}

void botnet_relay(BotNet *source, char *format, ...)
{
	BotNet	*bn;
	va_list msg;
	int	sz = 0;

	for(bn=botnetlist;bn;bn=bn->next)
	{
		if ((bn == source) || (bn->status != BN_LINKED))
			continue;

		if (!sz)
		{
			va_start(msg,format);
			vsprintf(gsockdata,format,msg);
			va_end(msg);
			sz = strlen(gsockdata);
		}

		if (write(bn->sock,gsockdata,sz) < 0)
			botnet_deaduplink(bn);
#ifdef DEBUG
		debug("[bnr] {%i} %s",bn->sock,gsockdata);
#endif /* DEBUG */
	}
}

void botnet_refreshbotinfo(void)
{
	Server	*sv;

	sv = find_server(current->server);
	botnet_relay(NULL,"BL%i 0 %s!%s %s:%i %s %s\n",	current->guid,current->nick,
		(current->userhost) ? current->userhost : UNKNOWNATUNKNOWN,
		(sv) ? ((*sv->realname) ? sv->realname : sv->name) : UNKNOWN,
		(sv) ? sv->port : 0,BOTCLASS,VERSION);
#ifdef DEBUG
	debug("(botnet_refreshbotinfo) sent refreshed information to botnet\n");
#endif /* DEBUG */
}

__attr(CORE_SEG, __regparm (2))
void botnet_binfo_relay(BotNet *source, BotInfo *binfo)
{
	botnet_relay(source,"BL%i %i %s %s %s\n",binfo->guid,(binfo->hops + 1),
		(binfo->nuh) ? binfo->nuh : UNKNOWNATUNKNOWN,
		(binfo->server) ? binfo->server : UNKNOWN,
		(binfo->version) ? binfo->version : "-");
}

__attr(CORE_SEG, __regparm (2))
void botnet_binfo_tofile(int sock, BotInfo *binfo)
{
	to_file(sock,"BL%i %i %s %s %s\n",binfo->guid,(binfo->hops + 1),
		(binfo->nuh) ? binfo->nuh : UNKNOWNATUNKNOWN,
		(binfo->server) ? binfo->server : UNKNOWN,
		(binfo->version) ? binfo->version : "-");
}

void botnet_dumplinklist(BotNet *bn)
{
	BotInfo *binfo;
	BotNet	*bn2;
	Mech	*bot;
	Server	*sv;

	/*
	 *  send link lines
	 */
	for(bot=botlist;bot;bot=bot->next)
	{
		/*
		 *  tell the other side who the local bots are
		 */
		sv = find_server(bot->server);
		to_file(bn->sock,"BL%i %c %s!%s %s:%i %s %s\n",bot->guid,
			(bot == bn->controller) ? '0' : '1',bot->nick,
			(bot->userhost) ? bot->userhost : UNKNOWNATUNKNOWN,
			(sv) ? ((*sv->realname) ? sv->realname : sv->name) : UNKNOWN,
			(sv) ? sv->port : 0,BOTCLASS,VERSION);
	}
	for(bn2=botnetlist;bn2;bn2=bn2->next)
	{
		if ((bn2 == bn) || (bn2->status != BN_LINKED) || !(bn2->list_complete))
			continue;
		for(binfo=bn2->botinfo;binfo;binfo=binfo->next)
			botnet_binfo_tofile(bn->sock,binfo);
	}
	/*
	 *  tell remote end to sync
	 */
	to_file(bn->sock,"BL-\n");
}

int connect_to_bot(NetCfg *cfg)
{
	BotNet	*bn;
	int	s;

#ifdef DEBUG
	debug("(connect_to_bot) NetCfg* "mx_pfmt" = { \"%s\", guid = %i, port = %i, ... }\n",
		(mx_ptr)cfg,nullstr(cfg->host),cfg->guid,cfg->port);
#endif /* DEBUG */

	if ((s = SockConnect(cfg->host,cfg->port,FALSE)) < 0)
		return(-1);

	/*
	 *  set linked status
	 */
	cfg->linked = TRUE;

	set_mallocdoer(connect_to_bot);
	bn = (BotNet*)Calloc(sizeof(BotNet));

	bn->sock = s;
	bn->status = BN_CONNECT;
	bn->when = now;
	bn->guid = cfg->guid;

	bn->next = botnetlist;
	botnetlist = bn;

	return(0);
}

/*
 *
 *  protocol routines
 *
 */

void basicAuth(BotNet *bn, char *rest)
{
	NetCfg	*cfg;
	char	*pass;
	int	authtype = -1;

	if (bn->status != BN_WAITAUTH)
		return;

	if ((pass = chop(&rest)))
	{
		if (!Strcmp(pass,"PTA"))
			authtype = BNAUTH_PLAINTEXT;
#ifdef SHACRYPT
		if (!Strcmp(pass,"SHA"))
			authtype = BNAUTH_SHA;
#endif /* SHACRYPT */
#ifdef MD5CRYPT
		if (!Strcmp(pass,"MD5"))
			authtype = BNAUTH_MD5;
#endif /* MD5CRYPT */
	}

	/*
	 *  find the other bots' password so we can check if it matches
	 */
	pass = NULL;
	for(cfg=netcfglist;cfg;cfg=cfg->next)
	{
		if (cfg->guid == bn->guid)
		{
			pass = cfg->pass;
			break;
		}
	}
	if (!pass || !*pass)
		goto badpass;

	switch(authtype)
	{
	case BNAUTH_PLAINTEXT:
		if (Strcmp(pass,rest))
			goto badpass;
		break;
#ifdef SHACRYPT
	case BNAUTH_SHA:
		if (linkpass && *linkpass)
		{
			char	*enc,temppass[24 + Strlen2(pass,linkpass)]; // linkpass is never NULL

			/* "mypass theirpass REMOTEsid LOCALsid" */
			sprintf(temppass,"%s %s %i %i",linkpass,pass,bn->rsid,bn->lsid);
#ifdef DEBUG
			debug(">> sha pass exchange: \"%s\"\n",temppass);
#endif /* DEBUG */
			enc = CRYPT_FUNC(temppass,rest);
#ifdef DEBUG
			debug("(basicAuth) their = %s, mypass = %s :: sha = %s\n",
				pass,linkpass,enc);
#endif /* DEBUG */
			if (!Strcmp(enc,rest))
				break;
		}
#endif /* SHACRYPT */
#ifdef MD5CRYPT
	case BNAUTH_MD5:
		if (linkpass && *linkpass)
		{
			char	*enc,temppass[24 + Strlen2(pass,linkpass)]; // linkpass is never NULL

			/* "mypass theirpass REMOTEsid LOCALsid" */
			sprintf(temppass,"%s %s %i %i",linkpass,pass,bn->rsid,bn->lsid);
#ifdef DEBUG
			debug(">> md5 pass exchange: \"%s\"\n",temppass);
#endif /* DEBUG */
			enc = CRYPT_FUNC(temppass,rest);
#ifdef DEBUG
			debug("(basicAuth) their = %s, mypass = %s :: md5 = %s\n",
				pass,linkpass,enc);
#endif /* DEBUG */
			if (!Strcmp(enc,rest))
				break;
		}
#endif /* MD5CRYPT */
	default:
	badpass:
		/*
		 *  we can/should use deaduplink here since we set cfg->linked = TRUE in basicBanner()
		 */
		botnet_deaduplink(bn);
#ifdef DEBUG
		debug("(basicAuth) bad password [ guid = %i ]\n",bn->guid);
#endif /* DEBUG */
		return;
	}

	to_file(bn->sock,"BK\n");
	bn->status = BN_LINKED;
	botnet_dumplinklist(bn);
#ifdef DEBUG
	debug("(basicAuth) bn->tick = 0\n");
#endif /* DEBUG */
	bn->tick = 0;
	bn->tick_last = now - 580; /* 10 minutes (10*60) - 20 seconds */
}

void basicAuthOK(BotNet *bn, char *rest)
{
	if (bn->status != BN_WAITLINK)
		return;

	bn->status = BN_LINKED;
	botnet_dumplinklist(bn);
#ifdef DEBUG
	debug("(basicAuthOK) bn->tick = 0\n");
#endif /* DEBUG */
	bn->tick = 0;
	bn->tick_last = now - 580; /* 10 minutes (10*60) - 20 seconds */
}

void basicBanner(BotNet *bn, char *rest)
{
	Mech	*netbot;
	NetCfg	*cfg;
	char	*p;
	int	guid;
	int	authtype = -1;

	/*
	 *  we're only prepared to listen to banners in the first connection phase
	 */
	if (bn->status != BN_BANNERSENT && bn->status != BN_UNKNOWN)
		return;

	/*
	 *  find out who's calling
	 */
	p = chop(&rest);
	guid = a2i(p);
	/*
	 *  bad guid received
	 */
	if (errno)
	{
		if (bn->guid)
		{
			/*
			 *  we know who its supposed to be, ergo sum, we've set cfg->linked = TRUE, lets undo that
			 */
			reset_linkable(bn->guid);
		}
		bn->status = BN_DEAD;
		deadlinks = TRUE;
		return;
	}

	if (bn->guid && bn->guid != guid)
	{
		/*
		 *  its not who we expected!
		 */
#ifdef DEBUG
		debug("(basicBanner) {%i} calling guid %i but got guid %i!\n",
			bn->sock,bn->guid,guid);
#endif /* DEBUG */
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  either (bn->guid == 0) || (bn->guid == guid), we dont need to check
	 */
	bn->guid = guid;

	/*
	 *  prevent circular linking
	 */
	if (bn->status == BN_UNKNOWN)
	{
		/*
		 *  they are connecting to us
		 */
		if ((cfg = find_netcfg(guid)))
		{
			if (cfg->linked)
			{
				/*
				 *  we already think this remote bot is connected and it's still connecting to us!
				 */
				bn->status = BN_DEAD;
				deadlinks = TRUE;
#ifdef DEBUG
				debug("(basicBanner) {%i} guid %i (connecting to us) is already linked!\n",
					bn->sock,bn->guid);
#endif /* DEBUG */
				return;
			}
			/*
			 *  its not linked? well it is now!
			 */
			cfg->linked = TRUE;
		}
	}

	/*
	 *  get a session number
	 */
	p = chop(&rest);
	bn->rsid = a2i(p);
	if (errno)
	{
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  parse banner options
	 */
	while((p = chop(&rest)))
	{
		if (!Strcmp(p,"PTA"))
			bn->opt.pta = TRUE;
#ifdef SHACRYPT
		if (!Strcmp(p,"SHA"))
			bn->opt.sha = TRUE;
#endif /* SHACRYPT */
#ifdef MD5CRYPT
		if (!Strcmp(p,"MD5"))
			bn->opt.md5 = TRUE;
#endif /* MD5CRYPT */
	}

	/*
	 *  update timestamp
	 */
	bn->when = now;

	/*
	 *  if the remote bot initiated the connection we need a valid pass from them
	 *  before we send our own password to validate
	 */
	if (bn->status == BN_UNKNOWN)
	{
		bn->controller = netbot = get_netbot();
		to_file(bn->sock,banneropt,netbot->guid,bn->lsid);
		bn->status = BN_WAITAUTH;
		return;
	}

	/*
	 *  we're the one that initiated the connection, we now send our password
	 */
	if (!linkpass || !*linkpass)
	{
		botnet_deaduplink(bn);
		return;
	}

	/*
	 *  select authentication method
	 */
	if (bn->opt.pta && (BNAUTH_PLAINTEXT > authtype))
		authtype = BNAUTH_PLAINTEXT;
#ifdef MD5CRYPT
	if (bn->opt.md5 && (BNAUTH_MD5 > authtype))
		authtype = BNAUTH_MD5;
#endif /* MD5CRYPT */
#ifdef SHACRYPT
	if (bn->opt.sha && (BNAUTH_SHA > authtype))
		authtype = BNAUTH_SHA;
#endif /* SHACRYPT */

	switch(authtype)
	{
	case BNAUTH_PLAINTEXT:
		to_file(bn->sock,"BAPTA %s\n",linkpass);
		break;
#ifdef SHACRYPT
	case BNAUTH_SHA:
		if ((cfg = find_netcfg(guid)))
		{
			if (cfg->pass && *cfg->pass)
			{
				char	*enc,salt[8];
				char	temppass[24 + Strlen2(cfg->pass,linkpass)]; // linkpass(procvar) is not NULL

				/* "theirpass mypass LOCALsid REMOTEsid" */
				sprintf(temppass,"%s %s %i %i",cfg->pass,linkpass,bn->lsid,bn->rsid);
#ifdef DEBUG
				debug(">> sha pass exchange: \"%s\"\n",temppass);
#endif /* DEBUG */
				sprintf(salt,"$6$%04x",(rand() >> 16));
				enc = CRYPT_FUNC(temppass,salt);
				to_file(bn->sock,"BASHA %s\n",enc);
				break;
			}
		}
#endif /* SHACRYPT */
#ifdef MD5CRYPT
	case BNAUTH_MD5:
		if ((cfg = find_netcfg(guid)))
		{
			if (cfg->pass && *cfg->pass)
			{
				char	*enc,salt[8];
				char	temppass[24 + Strlen2(cfg->pass,linkpass)]; // linkpass(procvar) is not NULL

				/* "theirpass mypass LOCALsid REMOTEsid" */
				sprintf(temppass,"%s %s %i %i",cfg->pass,linkpass,bn->lsid,bn->rsid);
#ifdef DEBUG
				debug(">> md5 pass exchange: \"%s\"\n",temppass);
#endif /* DEBUG */
				sprintf(salt,"$1$%04x",(rand() >> 16));
				enc = CRYPT_FUNC(temppass,salt);
				to_file(bn->sock,"BAMD5 %s\n",enc);
				break;
			}
		}
#endif /* MD5CRYPT */
	default:
		botnet_deaduplink(bn);
		return;
	}
	bn->status = BN_WAITLINK;
}

void basicLink(BotNet *bn, char *version)
{
	BotInfo	*binfo,*delete,**pp;
	NetCfg	*cfg;
	char	*nuh,*server;
	int	guid,hops;

	if (bn->status != BN_LINKED)
		return;

	/*
	 *  BL-
	 */
	if (*version == '-')
	{
		/*
		 *  check all links for conflicts
		 */
		binfo = (bn->botinfo) ? bn->botinfo->next : NULL;
		for(;binfo;binfo=binfo->next)
		{
			if ((cfg = find_netcfg(binfo->guid)) == NULL)
				continue;
			if (cfg->linked == TRUE)
				break;
		}
		if (binfo)
		{
#ifdef DEBUG
			debug("(basicLink) circular linking: guid == %i\n",binfo->guid);
#endif /* DEBUG */
			/*
			 *  drop everything we've gotten sofar...
			 */
			botnet_deaduplink(bn);
			return;
		}

		/*
		 *  we're done, we're fine, we're linked!
		 */
		for(binfo=bn->botinfo;binfo;binfo=binfo->next)
		{
			botnet_binfo_relay(bn,binfo);
			check_botinfo(binfo,NULL);
			if ((cfg = find_netcfg(binfo->guid)) == NULL)
				continue;
			cfg->linked = TRUE;
		}
		bn->list_complete = TRUE;
		return;
	}

	/*
	 *  BL<guid> <hops> <nick>!<userhost> <server> <version>
	 */
	nuh = chop(&version);
	guid = a2i(nuh);
	if (errno)
		return;

	nuh = chop(&version);
	hops = a2i(nuh);
	if (errno)
		return;

	nuh = chop(&version);
	server = chop(&version);

	if (!*version)
		return;

	binfo = make_botinfo(guid,hops,nuh,server,version);

	if (bn->botinfo == NULL)
		send_global(SPYSTR_BOTNET,"connecting to %s [guid %i]",nickcpy(NULL,nuh),bn->guid);
	pp = &bn->botinfo;
	while(*pp)
	{
		delete = *pp;
		if (guid == delete->guid)
		{
			*pp = delete->next;
			Free((char**)&delete);
			break;
		}
		pp = &delete->next;
	}
	binfo->next = *pp;
	*pp = binfo;

	if (bn->list_complete)
	{
		if ((cfg = find_netcfg(guid)))
			cfg->linked = TRUE;
		/*
		 *  broadcast the new link
		 */
		botnet_binfo_relay(bn,binfo);
		check_botinfo(binfo,NULL);
	}
}

void basicQuit(BotNet *bn, char *rest)
{
	BotInfo	*binfo,**pp_binfo;
	int	guid;

	if (bn->status != BN_LINKED)
		return;

	guid = a2i(rest);
	if (errno)
		return;

	pp_binfo = &bn->botinfo;
	while(*pp_binfo)
	{
		binfo = *pp_binfo;
		if (binfo->guid == guid)
		{
			send_global(SPYSTR_BOTNET,"quit: %s (from guid %i)",rest,bn->guid);
			*pp_binfo = binfo->next;
			reset_linkable(guid);
			Free((char**)&binfo);
			break;
		}
		pp_binfo = &binfo->next;
	}
	botnet_relay(bn,"BQ%s\n",rest);
}

/*
 *
 */

void partyAuth(BotNet *bn, char *rest)
{
	User	*user;
	Strp	*ump;
	char	*name,*userhost,*checksum;
	int	m;

	name = chop(&rest);
	userhost = chop(&rest);
	if ((checksum = chop(&rest)) == NULL)
		checksum = "";

	for(current=botlist;current;current=current->next)
	{
		for(user=current->userlist;user;user=user->next)
		{
			for(ump=user->mask;ump;ump=ump->next)
			{
				if (!matches(ump->p,userhost))
				{
#ifdef DEBUG
					debug("(partyAuth) testing auth on %s\n",user->name);
#endif /* DEBUG */
					m = 0;
					if (user->pass)
					{
						sprintf(gsockdata,"%s %s %s",userhost,user->name,user->pass);
						m = passmatch(gsockdata,checksum);
					}
					if (m)
					{
						make_auth(userhost,user);
					}
				}
			}
		}
	}
}

#ifdef REDIRECT

int commandlocal(int dg, int sg, char *from, char *command)
{
	Client	mydcc;
	User	*user;
	Strp	*sp;
	char	tempdata[MSGLEN];
	char	*p1,*p2,*uh;

#ifdef DEBUG
	debug("(commandlocal) %i %i %s %s\n",dg,sg,from,command);
#endif /* DEBUG */

	uh = getuh(from);
	nickcpy(CurrentNick,from);
	for(current=botlist;current;current=current->next)
	{
		if (dg != -1 && current->guid != dg)
			continue;

		user = get_authuser(from,ANY_CHANNEL);
		if (!user && (user = find_handle(CurrentNick)))
		{
			for(sp=user->mask;sp;sp=sp->next)
			{
				if (!Strcmp(sp->p,uh))
					break;
			}
		}
		if (user)
		{
			mydcc.user  = CurrentUser = user;
			CurrentDCC  = &mydcc;
			CurrentShit = NULL;
			CurrentChan = NULL;

			redirect.method = R_BOTNET;
			redirect.guid = sg;
			set_mallocdoer(commandlocal);
			redirect.to = Strdup(CurrentNick);

			p1 = tempdata;
			p2 = Strcpy(p1,from);
			p2++; /* skip past '0' */
			*p2 = current->setting[CHR_CMDCHAR].char_var;
			Strcpy((*p2 == *command) ? p2 : p2+1,command);

			on_msg(p1,current->nick,p2);
			CurrentDCC = NULL;
		}
		if (dg == -1)
			return(TRUE);
	}
	return(FALSE);
}

/*
 *  PC<targetguid> <sourceguid> <sourceuserhost> <command>
 */
void partyCommand(BotNet *bn, char *rest)
{
	BotNet	*bn2;
	BotInfo	*binfo;
	char	*dguid,*sguid,*userhost;
	int	idguid,isguid;

#ifdef DEBUG
	debug("(partyCommand) %s\n",rest);
#endif /* DEBUG */

	dguid = chop(&rest);
	sguid = chop(&rest);
	if ((userhost = chop(&rest)) == NULL || *rest == 0)
		return;

	isguid = a2i(sguid);
	if (errno)
		return;

	idguid = a2i(dguid);
	if (errno)	/* == "*" */
	{
		commandlocal(-1,isguid,userhost,rest);
	}
	else
	{
		if (commandlocal(idguid,isguid,userhost,rest))
			return;
		for(bn2=botnetlist;bn2;bn2=bn2->next)
		{
			if (bn2->status != BN_LINKED)
				continue;
			for(binfo=bn2->botinfo;binfo;binfo=binfo->next)
			{
				if (idguid == binfo->guid)
				{
					to_file(bn2->sock,"PC%s %s %s %s\n",dguid,sguid,userhost,rest);
					return;
				}
			}
		}
	}
	botnet_relay(bn,"PC%s %s %s %s\n",dguid,sguid,userhost,rest);
}

#endif /* REDIRECT */

/*
 *  PM<targetguid> <targetuserhost> <source> <message>
 */
void partyMessage(BotNet *bn, char *rest)
{
	BotNet	*bn2;
	BotInfo	*binfo;
	Client	*client;
	char	*src,*dst,*userhost;
	int	guid;

	dst = chop(&rest);
	userhost = chop(&rest);
	if ((src = chop(&rest)) == NULL || *rest == 0)
		return;

	guid = a2i(dst);
	if (errno)	/* == "*" */
	{
		Strncpy(CurrentNick,src,NUHLEN-1);
		/*
		 *  partyline_broadcast() uses CurrentNick for the first %s in the format
		 */
		if (*rest == 1)
			partyline_broadcast(NULL,"* %s %s\n",rest+1);
		else
			partyline_broadcast(NULL,"<%s> %s\n",rest);
		botnet_relay(bn,"PM* * %s %s\n",src,rest);
	}
	else
	{
		for(current=botlist;current;current=current->next)
		{
			if (guid == current->guid)
			{
				if ((client = find_client(userhost)))
				{
					to_file(client->sock,"[%s] %s\n",src,rest);
					return;
				}
			}
		}
		for(bn2=botnetlist;bn2;bn2=bn2->next)
		{
			if (bn2->status != BN_LINKED)
				continue;
			for(binfo=bn2->botinfo;binfo;binfo=binfo->next)
			{
				if (guid == binfo->guid)
				{
					to_file(bn2->sock,"PM%s %s %s %s\n",dst,userhost,src,rest);
					return;
				}
			}
		}
		botnet_relay(bn,"PM%s %s %s %s\n",dst,userhost,src,rest);
	}
}

/*
 *
 */

void ushareUser(BotNet *bn, char *rest)
{
	User	*user,tempuser;
	char	c,*handle,*pass;
	int	i,tick,modcount,uaccess;

	c = *(rest++);
	tick = a2i(chop(&rest));
	if (errno)
		return;
	if ((c != '-' && bn->tick >= tick) && (c != '+' && bn->tick != tick))
		return;
	switch(c)
	{
	case '+':
		if (tick > bn->tick)
		{
#ifdef DEBUG
			debug("(ushareUser) bumping tick to %i\n",tick);
#endif /* DEBUG */
			bn->tick = tick;
		}
		/* `UU+tick modcount access handle chan pass' */
		/*  UU+0 4 100 proton * $1$3484$AxMkHvZijkeqb8hA6h9AB/ */
		i = 0;
		modcount = a2i(chop(&rest));
		i += errno;
		uaccess = a2i(chop(&rest));
		i += errno;
		handle = chop(&rest);
		pass = chop(&rest);
		if (i == 0 && handle && pass && *pass)
		{
			if (!Strcmp(pass,"none"))
				pass = NULL;
			i = 0;
			bn->addsession = (rand() | 1);
			for(current=botlist;current;current=current->next)
			{
				/* accept users from other bots */
				if (current->setting[TOG_NETUSERS].int_var == 0)
					continue;
				user = find_handle(handle);
				if (user && user->x.x.readonly)
					continue;
				if (user && user->modcount < modcount)
				{
					/* user with higher modcount overwrites */
					remove_user(user);
					user = NULL;
				}
				if (!user)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ re-creating\n",handle);
#endif /* DEBUG */
					user = add_user(handle,pass,uaccess);
					user->modcount = modcount;
					user->guid = bn->guid;		/* we got new user from <guid> this session */
					user->tick = global_tick;
					user->addsession = bn->addsession;
					i++;
				}
			}
			if (i)
			{
#ifdef DEBUG
				debug("(ushareUser) bn->tick = %i\n",tick);
#endif /* DEBUG */
				bn->tick = tick;
				global_tick++;
			}
		}
		break;
	case '-':
#ifdef DEBUG
		debug("(ushareUser) ticking to next user %i ++\n",bn->tick);
#endif /* DEBUG */
		for(current=botlist;current;current=current->next)
			for(user=current->userlist;user;user=user->next)
				if (user->guid == bn->guid && user->addsession)
				{
					user->addsession = 0;
					mirror_user(user); // copy to other local bots
				}
		bn->addsession = 0;
		bn->tick++;
		to_file(bn->sock,"UT%i\n",bn->tick);
		bn->tick_last = now;
		break;
	case '*':
	case '#':
		if ((rest = chop(&rest)) == NULL)
			return;
		for(current=botlist;current;current=current->next)
		{
			for(user=current->userlist;user;user=user->next)
			{
				if (user->guid == bn->guid && user->addsession == bn->addsession)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ mask/chan %s\n",user->name,rest);
#endif /* DEBUG */
					addtouser((c == '*') ? &user->mask : &user->chan,rest,TRUE);
				}
			}
		}
		break;
	case '!':
		user = cfgUser;
		cfgUser = memset(&tempuser,0,sizeof(User));
		cfg_opt(chop(&rest));
		cfgUser = user;
		for(current=botlist;current;current=current->next)
		{
			for(user=current->userlist;user;user=user->next)
			{
				if (user->guid == bn->guid && user->addsession == bn->addsession)
				{
#ifdef DEBUG
					debug("(ushareUser) user %s ++ touching flags\n",user->name);
					debug("(ushareUser) %s %s %s prot%i\n",(tempuser.x.x.aop) ? "aop" : "",(tempuser.x.x.echo) ? "echo" : "",
						(tempuser.x.x.avoice) ? "avoice" : "",tempuser.x.x.prot);
#endif /* DEBUG */
					user->x.x.aop = tempuser.x.x.aop;
					user->x.x.echo = tempuser.x.x.echo;
					user->x.x.avoice = tempuser.x.x.avoice;
#ifdef BOUNCE
					user->x.x.bounce = tempuser.x.x.bounce;
#endif /* BOUNCE */
				}
			}
		}
		break;
	}
#ifdef DEBUG
	current = NULL;
#endif /* DEBUG */
}

void ushareTick(BotNet *bn, char *rest)
{
	Strp	*pp;
	Mech	*mech;
	User	*user,*senduser;
	int	i;

	i = a2i(rest);
	if (errno)
		return;
#ifdef DEBUG
	debug("(ushareTick) remote bot ticked %i\n",i);
#endif /* DEBUG */
	senduser = 0;
	for(mech=botlist;mech;mech=mech->next)
	{
		for(user=mech->userlist;user;user=user->next)
		{
			if (i <= user->tick && !user->x.x.noshare)
			{
				if (!senduser)
					senduser = user;
				/* user->guid != bn->guid :: dont send users back to the bot we got them from */
				if (user->tick < senduser->tick && user->guid != bn->guid)
					senduser = user;
			}
		}
	}
	if (senduser)
	{
		char	*p,temp[8];

		user = senduser;
#ifdef DEBUG
		debug("(ushareTick) user %s, user->tick = %i (%i)\n",user->name,user->tick,i);
#endif /* DEBUG */
		to_file(bn->sock,"UU+%i %i %i %s %s\n",user->tick,user->modcount,
			user->x.x.access,user->name,(user->pass) ? user->pass : "none");
		*(p = temp) = 0;
		if (user->x.x.aop)
			*(p++) = 'a';
#ifdef BOUNCE
		if (user->x.x.bounce)
			*(p++) = 'b';
#endif /* BOUNCE */
		if (user->x.x.echo)
			*(p++) = 'e';
		if (user->x.x.avoice)
			*(p++) = 'v';
		*(p++) = 'p';
		*(p++) = '0' + user->x.x.prot;
		*p = 0;
		to_file(bn->sock,"UU!%i %s\n",user->tick,temp);
		for(pp=user->mask;pp;pp=pp->next)
		{
			to_file(bn->sock,"UU*%i %s\n",user->tick,pp->p);
		}
		for(pp=user->chan;pp;pp=pp->next)
		{
			to_file(bn->sock,"UU#%i %s\n",user->tick,pp->p);
		}
		to_file(bn->sock,"UU-%i\n",user->tick);
		return;
	}
}

void ushareDelete(BotNet *bn, char *rest)
{
	User	*user;
	char	*orig;
	int	modcount;

	orig = rest;
	modcount = a2i(chop(&rest));
	if (errno)
		return;
	for(current=botlist;current;current=current->next)
	{
		if (current->setting[TOG_NETUSERS].int_var)
		{
			user = find_handle(rest);
			if (user && user->modcount == modcount)
			{
				reset_userlink(user,NULL);
				remove_user(user);
			}
		}
	}
	unchop(orig,rest);
	botnet_relay(bn,"UD%s\n",orig);
}

/*
 *
 */

void botnet_parse(BotNet *bn, char *rest)
{
	int	i;

#ifdef TELNET
	if (bn->status == BN_UNKNOWN)
	{
		if (!Strcmp(rest,telnetprompt))
			return;
		if (*rest != 'B')
		{
			if (check_telnet(bn->sock,rest))
			{
				bn->sock = -1;
				bn->status = BN_DEAD;
				deadlinks = TRUE;
				return;
			}
		}
	}
#endif /* TELNET */

	if (*rest != 'B' && bn->status != BN_LINKED)
		return;

	for(i=0;basicProto[i].func;i++)
	{
		if (basicProto[i].c1 == rest[0] && basicProto[i].c2 == rest[1])
		{
			basicProto[i].func(bn,rest+2);
			return;
		}
	}

	/*
	 *  relay to bots that know/want the protocol
	 */
}

void botnet_newsock(void)
{
	BotNet	*bn;
	int	s;

	/*
	 *  accept the connection
	 */
	if ((s = SockAccept(linksock.sock)) < 0)
		return;

#ifdef DEBUG
	debug("(botnet_newsock) {%i} new socket\n",s);
#endif /* DEBUG */

#ifdef TELNET
	to_file(s,FMT_PLAINLINE,telnetprompt);
#endif /* TELNET */

	set_mallocdoer(botnet_newsock);
	bn = (BotNet*)Calloc(sizeof(BotNet));

	bn->sock = s;
	bn->status = BN_UNKNOWN;
	bn->lsid = rand();
	bn->when = now;

	bn->next = botnetlist;
	botnetlist = bn;

	/*
	 *  crude... but, should work
	 */
	last_autolink = now + AUTOLINK_DELAY;
}

/*
 *
 */

void select_botnet(void)
{
	BotNet	*bn;

	/*
	 *  handle incoming connections
	 */
	if (linkport && linksock.sock == -1)
	{
		linksock.sock = SockListener(linkport);
		if (linksock.sock != -1)
		{
			linksock.next = botnetlist;
			botnetlist = (BotNet*)&linksock;
#ifdef DEBUG
			debug("(doit) {%i} Linksocket is active (%i)\n",linksock.sock,linkport);
#endif /* DEBUG */
		}
	}

	/*
	 *  autolink
	 */
	if (autolink && (now > last_autolink))
	{
		last_autolink = now + AUTOLINK_DELAY;

		if (autolink_cfg)
			autolink_cfg = autolink_cfg->next;
		if (!autolink_cfg)
			autolink_cfg = netcfglist;

		if (autolink_cfg && !autolink_cfg->linked &&
			autolink_cfg->host && autolink_cfg->pass)
		{
			/*
			 *  this thing isnt linked yet!
			 */
			connect_to_bot(autolink_cfg);
		}
	}

	short_tv &= ~TV_BOTNET;
	for(bn=botnetlist;bn;bn=bn->next)
	{
		chkhigh(bn->sock);
		if (bn->status == BN_CONNECT)
		{
			FD_SET(bn->sock,&write_fds);
			short_tv |= TV_BOTNET;
		}
		else
		{
			FD_SET(bn->sock,&read_fds);
		}
	}
}

void process_botnet(void)
{
	BotNet	*bn,**pp;
	BotInfo	*binfo;
	Mech	*netbot;
	char	*rest,linebuf[MSGLEN];

	for(bn=botnetlist;bn;bn=bn->next)
	{
		/*
		 *  usersharing tick, 10 minute period
		 */
		if (bn->status == BN_LINKED && (bn->tick_last + 600) < now)
		{
#ifdef DEBUG
			debug("(process_botnet) {%i} periodic ushare tick\n",bn->sock);
#endif /* DEBUG */
			bn->tick_last = now;
			to_file(bn->sock,"UT%i\n",bn->tick);
		}

		/*
		 *
		 */
		if (bn->has_data)
			goto has_data;

		/*
		 *  outgoing connection established
		 */
		if (FD_ISSET(bn->sock,&write_fds))
		{
			bn->lsid = rand();
			bn->controller = netbot = get_netbot();
			if (to_file(bn->sock,banneropt,netbot->guid,bn->lsid) < 0)
			{
				botnet_deaduplink(bn);
			}
			else
			{
				bn->status = BN_BANNERSENT;
				bn->when = now;
			}
			/* write_fds is only set for sockets where reading is not needed */
			continue;
		}

		/*
		 *  incoming data read
		 */
		if (FD_ISSET(bn->sock,&read_fds))
		{
			/*
			 *  ye trusty old linksock
			 */
			if (bn->status == BN_LINKSOCK)
				botnet_newsock();
			else
			{
		has_data:
				/*
				 *  Commands might change the botnetlist,
				 *  so returns here are needed
				 */
				rest = sockread(bn->sock,bn->sockdata,linebuf);
				bn->has_data = (rest) ? TRUE : FALSE;
				if (rest)
				{
					botnet_parse(bn,rest);
					if (!deadlinks)
						goto has_data; /* process more lines if link list is unchanged */
					goto clean;
				}
				switch(errno)
				{
				default:
#ifdef DEBUG
					debug("(process_botnet) {%i} sockread() errno = %i\n",bn->sock,errno);
#endif /* DEBUG */
					botnet_deaduplink(bn);
				case EAGAIN:
				case EINTR:
					goto clean;
				}
			}
		}

		if ((bn->status == BN_CONNECT) && ((now - bn->when) > LINKTIME))
		{
#ifdef DEBUG
			debug("(process_botnet) {%i} Life is good; but not for this guy (guid == %i). Timeout!\n",
				bn->sock,bn->guid);
#endif /* DEBUG */
			botnet_deaduplink(bn);
		}
	}
clean:
	/*
	 *  quit/delete BN_DEAD links
	 */
	if (!deadlinks)
		return;

	pp = &botnetlist;
	while(*pp)
	{
		bn = *pp;
		if (bn->status == BN_DEAD)
		{
			*pp = bn->next;
			send_global(SPYSTR_BOTNET,"quit: guid %i",bn->guid);
#ifdef DEBUG
			debug("(process_botnet) botnet quit: guid %i\n",bn->guid);
#endif /* DEBUG */
			while((binfo = bn->botinfo))
			{
				bn->botinfo = binfo->next;
#ifdef DEBUG
				debug("(process_botnet) botnet quit: guid %i child of %i on socket %i\n",
					binfo->guid,bn->guid,bn->sock);
#endif /* DEBUG */
				if (bn->list_complete)
				{
					send_global(SPYSTR_BOTNET,"quit: guid %i (child of %i)",
						binfo->guid,bn->guid);
					botnet_relay(bn,"BQ%i\n",binfo->guid);
					reset_linkable(binfo->guid);
				}
				Free((char**)&binfo);
			}
			if (bn->list_complete)
			{
				botnet_relay(bn,"BQ%i\n",bn->guid);
			}
			close(bn->sock);
			Free((char**)&bn);
			continue;
		}
		pp = &bn->next;
	}
	deadlinks = FALSE;
}

/*
 *
 *  commands related to botnet
 *
 */

void do_link(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	NetCfg	*cfg,**pp;
	char	*guid,*pass,*host,*port;
	int	iguid,iport;
	int	mode;

	/*
	 *  list all the known links
	 */
	if (!*rest)
	{
		table_buffer("guid\tpass\thost\tport");
		for(cfg=netcfglist;cfg;cfg=cfg->next)
		{
			table_buffer("%i\t%s\t%s\t%i",cfg->guid,(cfg->pass) ? cfg->pass : "",
				(cfg->host) ? cfg->host : "",cfg->port);
		}
		table_send(from,2);
		return;
	}

	guid = chop(&rest);
	if (*guid == '+' || *guid == '-')
		mode = *guid++;
	else
		mode = 0;

	iguid = a2i(guid);
	if (errno)
	{
usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	pp = &netcfglist;
	while((cfg = *pp))
	{
		if (cfg->guid == iguid)
			break;
		pp = &cfg->next;
	}

	if (CurrentUser == &CoreUser || mode == '+')
	{
		if (cfg)
		{
			to_user(from,"guid %i already exists",iguid);
			return;
		}
		pass = chop(&rest);
		host = chop(&rest);
		port = chop(&rest);

		iport = a2i(port);
		if (!pass || (host && !port) || (port && (errno || iport < 1 || iport > 65535)))
			goto usage;

		set_mallocdoer(do_link);
		cfg = (NetCfg*)Calloc(sizeof(NetCfg) + Strlen(pass,host,NULL)); // host might be NULL, Strlen() handles NULLs, Strlen2() does not.

		cfg->guid = iguid;
		cfg->port = iport;
		cfg->host = Strcat(cfg->pass,pass) + 1;

		if (host)
			Strcpy(cfg->host,host);
		else
			cfg->host = NULL;

		cfg->next = netcfglist;
		netcfglist = cfg;
		return;
	}

	if (!cfg)
	{
		to_user(from,"unknown guid: %i",iguid);
		return;
	}

	if (mode == '-')
	{
		*pp = cfg->next;
		Free((char**)&cfg);
		return;
	}

	if (!cfg->host)
	{
		to_user(from,"unknown host/port for guid %i",iguid);
		return;
	}

	if (cfg->linked)
	{
		to_user(from,"guid %i is already connected",iguid);
		return;
	}

	if (connect_to_bot(cfg) < 0)
	{
		to_user(from,"unable to create connection");
		return;
	}

	send_global(SPYSTR_BOTNET,"connecting to guid %i [%s:%i]",iguid,cfg->host,cfg->port);
}

#ifdef REDIRECT

void do_cmd(COMMAND_ARGS)
{
	Mech	*backup;
	char	tempdata[MAXLEN];
	char	*target,*orig = rest;
	int	guid;

	target = chop(&rest);
	guid = a2i(target);
	if (errno)
	{
		unchop(orig,rest);
		rest = orig;
		target = MATCH_ALL;
	}
	else
	if (*rest == 0)
	{
		usage(from);
		return;
	}

	if (STRCHR(from,'!'))
		sprintf(tempdata,"%s %i %s %s",target,current->guid,from,rest);
	else
		sprintf(tempdata,"%s %i %s!%s %s",target,current->guid,from,CurrentUser->mask->p,rest);
	backup = current;
	partyCommand(NULL,tempdata);
	current = backup;
}

#endif /* REDIRECT */
#endif /* BOTNET */
