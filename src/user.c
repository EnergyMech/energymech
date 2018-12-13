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
#define USERLIST_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "usercombo.h"

/*
 *
 *  reading and writing userlists
 *
 */

/*
 *  functions that handle userlist stuff (userlist_cmds[])
 */

void cfg_user(char *rest)
{
	set_mallocdoer(cfg_user);
	cfgUser = Calloc(sizeof(User) + strlen(rest));
	stringcpy(cfgUser->name,rest);
}

#ifdef BOTNET

void cfg_modcount(char *rest)
{
	int	i;

	i = asc2int(rest);
	if (errno || i < 1)
		return;
	cfgUser->modcount = i;
}

#endif /* BOTNET */

void cfg_pass(char *rest)
{
	Free((char**)&cfgUser->pass);
	set_mallocdoer(cfg_pass);
	cfgUser->pass = stringdup(rest);
}

void cfg_mask(char *rest)
{
	addtouser(&cfgUser->mask,rest,TRUE);
}

void cfg_chan(char *rest)
{
	addtouser(&cfgUser->chan,rest,TRUE);
}

LS struct
{
	char	modechar;
	int	modeflag;

} cfg_opt_flags[] =
{
{ 'a',	COMBO_AOP	},
#ifdef BOUNCE
{ 'b',	COMBO_BOUNCE	},
#endif /* BOUNCE */
{ 'e',	COMBO_ECHO	},
#ifdef BOTNET
{ 'L',	COMBO_NOSHARE	},
{ 'R',	COMBO_READONLY	},
#endif /* BOTNET */
#ifdef GREET
{ 'g',	COMBO_GREETFILE	},
{ 'r',	COMBO_RANDLINE	},
#endif /* GREET */
{ 'v',	COMBO_AVOICE	},
{ 0,	0,		}};

void cfg_opt(char *rest)
{
	int	i;

	/*
	 *  `OPT abeLRgrp0vu100'
	 */
	while(*rest)
	{
		for(i=0;cfg_opt_flags[i].modechar;i++)
		{
			if (cfg_opt_flags[i].modechar == *rest)
				cfgUser->x.comboflags |= cfg_opt_flags[i].modeflag;
		}
		if (*rest == 'p' && rest[1] >= '0' && rest[1] <= '4')
			cfgUser->x.x.prot = rest[1] - '0';
		if (*rest == 'u')
		{
			cfgUser->x.x.access = asc2int(rest+1);
			return;
		}
		rest++;
	}
}

void cfg_shit(char *rest)
{
	char	*channel,*mask,*from;
	time_t	backup_now,expire,when;
	int	shitlevel;

	channel = chop(&rest);
	mask    = chop(&rest);
	from    = chop(&rest);

	/*
	 *  quick way of getting the shitlevel
	 *  also, if channel, mask or from is NULL, this will fail (cuz *rest == 0)
	 */
	if (*rest < '0' || *rest > MAXSHITLEVELCHAR)
		return;
	shitlevel = *rest - '0';
	chop(&rest);	/* discard shitlevel argument */

	/*
	 *  convert the expiry time
	 */
	expire = asc2int(chop(&rest));	/* asc2int() can handle NULLs */
	if (errno || expire < now)
		return;

	/*
	 *  convert time when the shit was added
	 */
	when = asc2int(chop(&rest));
	if (errno || *rest == 0)	/* if *rest == 0, the reason is missing */
		return;

	/*
	 *  finally, add the sucker
	 */
	backup_now = now;
	now = when;
	add_shit(from,channel,mask,rest,shitlevel,expire);
	now = backup_now;
}

void cfg_kicksay(char *rest)
{
	Client	*backup;

	backup = CurrentDCC;
	CurrentDCC = (Client*)&CoreClient;
	do_kicksay((char*)CoreUser.name,NULL,rest,0);
	CurrentDCC = backup;
}

#ifdef GREET

void cfg_greet(char *rest)
{
	Free((char**)&cfgUser->greet);
	set_mallocdoer(cfg_greet);
	cfgUser->greet = stringdup(rest);
}

#endif /* GREET */

#ifdef NOTE

void cfg_note(char *rest)
{
	Strp	*sp,**np;

	np = &cfgUser->note;
	while(*np)
		np = &(*np)->next;
	*np = sp = Calloc(sizeof(Strp) + strlen(rest));
	/* Calloc sets to zero sp->next = NULL; */
	stringcpy(sp->p,rest);
}

#endif /* NOTE */

void user_sync(void)
{
	User	*user;

	user = add_user(cfgUser->name,cfgUser->pass,0);
	/* memcpy copies all access and userflags */
	memcpy(&user->mask,&cfgUser->mask,((char*)&user->pass - (char*)&user->mask));

#ifdef BOTNET
	user->tick = global_tick++;
#endif /* BOTNET */

	Free((char**)&cfgUser->pass);
	Free((char**)&cfgUser);
}

#define FL_ONEARG	1
#define FL_NEEDUSER	2	/* userfile */
#define FL_NEWUSER	4	/* userfile */

typedef struct CommandStruct
{
	char	*name;
	void	(*function)(char *);
	int	flags;

} ConfCommand;

LS const ConfCommand userlist_cmds[] =
{
/*
 *  users
 */
{ "USER",	cfg_user,	FL_NEWUSER  | FL_ONEARG	},
{ "PASS",	cfg_pass,	FL_NEEDUSER | FL_ONEARG	},
{ "MASK",	cfg_mask,	FL_NEEDUSER | FL_ONEARG	},
{ "CHAN",	cfg_chan,	FL_NEEDUSER | FL_ONEARG },
{ "OPT",	cfg_opt,	FL_NEEDUSER		},
{ "SHIT",	cfg_shit,	0			},
{ "KICKSAY",	cfg_kicksay,	0			},
#ifdef GREET
{ "GREET",	cfg_greet,	FL_NEEDUSER		},
#endif /* GREET */
#ifdef NOTE
{ "NOTE",	cfg_note,	FL_NEEDUSER		},
#endif /* NOTE */
#ifdef BOTNET
{ "MODCOUNT",	cfg_modcount,	FL_NEEDUSER		},
#endif /* BOTNET */
{ NULL, }};

int read_userlist_callback(char *line)
{
	char	*command;
	int	i;

	fix_config_line(line);
	command = chop(&line);
	for(i=0;userlist_cmds[i].name;i++)
	{
		if (!stringcasecmp(command,userlist_cmds[i].name))
			break;
	}
	if (userlist_cmds[i].name)
	{
		if (!cfgUser && (userlist_cmds[i].flags & FL_NEEDUSER))
		{
#ifdef DEBUG
			debug("[RUC] cfgUser is NULL for command that requires it to be set (%s)\n",command);
#endif /* DEBUG */
			return(FALSE);
		}
		if ((userlist_cmds[i].flags & FL_NEWUSER) && cfgUser)
			user_sync();
		if (userlist_cmds[i].flags & FL_ONEARG)
		{
			if ((line = chop(&line)) == NULL)
				return(FALSE);
		}
		userlist_cmds[i].function(line);
	}
	return(FALSE);
}

int read_userlist(char *filename)
{
	User	*user,*u2;
	User	*olduserlist;
	User	*newuserlist;
	int	in;
#ifdef DEBUG
	int	r;

	if (!filename)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE filename empty\n");
#endif /* NEWBIE */
		debug("(read_userlist) filename is NULL\n");
		return(FALSE);
	}
	if (*filename == '<') /* read only userfile */
		filename++;
	if ((r = is_safepath(filename,FILE_MAY_EXIST)) != FILE_IS_SAFE)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE filename is unsafe\n");
#endif /* NEWBIE */
		debug("(read_userlist) %s: unsafe filename (%i)...\n",filename,r);
		return(FALSE);
	}
	if ((in = open(filename,O_RDONLY)) < 0)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE \"%s\": %s\n",filename,strerror(errno));
#endif /* NEWBIE */
		debug("(read_userlist) failed to open \"%s\": %s\n",filename,strerror(errno));
		return(FALSE);
	}
#else /* ifdef DEBUG */
	if (!filename)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE filename empty\n");
#endif /* NEWBIE */
		return(FALSE);
	}
	if (*filename == '<') /* read only userfile */
		filename++;
	if (is_safepath(filename,FILE_MAY_EXIST) != FILE_IS_SAFE)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE filename is unsafe\n");
#endif /* NEWBIE */
		return(FALSE);
	}
	if ((in = open(filename,O_RDONLY)) < 0)
	{
#ifdef NEWBIE
		if (startup)
			to_file(1,"read_userlist: USERFILE \"%s\": %s\n",filename,strerror(errno));
#endif /* NEWBIE */
		return(FALSE);
	}
#endif /* ifdef DEBUG */

	olduserlist = current->userlist;
	cfgUser = current->userlist = NULL;

	readline(in,&read_userlist_callback);		/* readline closes in */

	/*
	 *  save the last user
	 */
	if (cfgUser)
		user_sync();

	newuserlist = current->userlist;
	current->userlist = olduserlist;

	for(user=newuserlist;user;user=user->next)
	{
		u2 = find_handle(user->name);		/* find user in old userlist, may be NULL */
		reset_userlink(u2,user);
	}

	/*
	 *  remove the old userlist
	 */
	while(current->userlist)
		remove_user(current->userlist);

	/*
	 *  re-apply shitlist that we just loaded
	 */
	check_shit();

	current->userlist = newuserlist;
	current->ul_save = 0;
	return(TRUE);
}

int write_userlist(char *filename)
{
	KickSay	*ks;
	Shit	*shit;
	Strp	*ump;
	User	*user;
	char	*p,flags[7];
	int	i,f;
#ifdef DEBUG
	int	dodeb,r;
#endif /* DEBUG */

	if (!filename)
		return(FALSE);

	if (!current->ul_save)
		return(TRUE);

#ifdef DEBUG
	if (*filename == '<') /* we dont write to read only userfiles */
	{
		debug("(write_userlist) %s: writing to read only userfile is prohibited...\n",filename);
		return(FALSE);
	}
	if ((r = is_safepath(filename,FILE_MAY_EXIST)) != FILE_IS_SAFE)
	{
		debug("(write_userlist) %s: unsafe filename (%i)...\n",filename,r);
		return(FALSE);
	}
#else
	if (*filename == '<') /* we dont write to read only userfiles */
		return(FALSE);
	if (is_safepath(filename,FILE_MAY_EXIST) != FILE_IS_SAFE)
		return(FALSE);
#endif

	if ((f = open(filename,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) < 0)
		return(FALSE);

	/*
	 *  reset the change-counter
	 */
	current->ul_save = 0;

#ifdef DEBUG
	dodeb = dodebug;
	dodebug = FALSE;
#endif /* DEBUG */

	for(user=current->userlist;user;user=user->next)
	{
		to_file(f,"\nuser\t\t%s\n",user->name);
		for(ump=user->mask;ump;ump=ump->next)
			to_file(f,"mask\t\t%s\n",ump->p);
		for(ump=user->chan;ump;ump=ump->next)
			to_file(f,"chan\t\t%s\n",ump->p);
		/*
		 *  `OPT aegrp0vu100'
		 */
		p = flags;
		for(i=0;cfg_opt_flags[i].modeflag;i++)
		{
			if (user->x.comboflags & cfg_opt_flags[i].modeflag)
				*(p++) = cfg_opt_flags[i].modechar;
		}
		*p = 0;
		to_file(f,"opt\t\t%sp%iu%i\n",flags,user->x.x.prot,user->x.x.access);
		/*
		 *  `PASS <password>'
		 */
		if (user->pass)
			to_file(f,"pass\t\t%s\n",user->pass);
#ifdef GREET
		if (user->greet)
			to_file(f,"greet\t\t%s\n",user->greet);
#endif /* GREET */
#ifdef NOTE
		for(ump=user->note;ump;ump=ump->next)
			to_file(f,"note\t\t%s\n",ump->p);
#endif /* NOTE */
#ifdef BOTNET
		to_file(f,"modcount\t%i\n",user->modcount);
#endif /* BOTNET */
	}

	to_file(f,"\n");

	for(shit=current->shitlist;shit;shit=shit->next)
	{
		to_file(f,"shit\t\t%s %s %s %i %lu %lu %s\n",
			shit->chan,shit->mask,shit->from,shit->action,
			shit->expire,shit->time,shit->reason);
	}

	to_file(f,"\n");

	for(ks=current->kicklist;ks;ks=ks->next)
	{
		to_file(f,"kicksay\t\t%s %i \"%s\" %s\n",ks->chan,ks->action,ks->mask,ks->reason);
	}

	close(f);

#ifdef DEBUG
	dodebug = dodeb;
#endif /* DEBUG */
	return(TRUE);
}

/*
 *  adding and removing masks from user records
 *  rehash_chanusers() executed if mask is added or removed
 *  addtouser() add channel or mask to a user
 *  remfromuser() remove a channel or mask from a user
 */

void rehash_chanusers(void)
{
	Chan *chan;
	ChanUser *cu;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		for(cu=chan->users;cu;cu=cu->next)
			cu->user = get_user(get_nuh(cu),chan->name);
	}
}

void addtouser(Strp **pp, const char *string, int rehash)
{
	Strp	*um;

	while(*pp)
	{
		um = *pp;
		if (!stringcasecmp(um->p,string))
			return;
		pp = &um->next;
	}

	set_mallocdoer(addtouser);
	*pp = um = (Strp*)Calloc(sizeof(Strp) + strlen(string));
	stringcpy(um->p,string);
	if (rehash)
		rehash_chanusers();
}

int remfromuser(Strp **pp, const char *string)
{
	Strp	*um;

	while(*pp)
	{
		um = *pp;
		if (!stringcasecmp(um->p,string))
		{
			*pp = um->next;
			Free((char**)&um);
			rehash_chanusers();
			return(TRUE);
		}
		pp = &um->next;
	}
	return(FALSE);
}

/*
 *  make duplicates of a user on other local bots
 */
void mirror_user(User *user)
{
	Mech	*backup,*anybot;
	User	*newuser,*olduser;
	Strp	*notes;

#ifdef BOTNET
	/* dont mirror noshare users */
	if (user->x.x.noshare)
		return;
#endif /* BOTNET */
	backup = current;
	for(anybot=botlist;anybot;anybot=anybot->next)
	{
		if (anybot == backup) /* dont try to copy to myself, bad things will happen */
			continue;
		for(olduser=anybot->userlist;olduser;olduser=olduser->next)
		{
			if (!stringcasecmp(user->name,olduser->name))
			{
#ifdef BOTNET
				/* dont overwrite "better" users */
				if (olduser->modcount >= user->modcount)
					return;
				/* dont overwrite read only users */
				if (olduser->x.x.readonly)
					return;
#endif /* BOTNET */
				break;
			}
		}
#ifdef DEBUG
		debug("(mirror_user) mirroring user %s[%i] to local bot %s(%i)\n",
			user->name,user->x.x.access,nullstr(anybot->nick),anybot->guid);
#endif /* DEBUG */

		current = anybot;
		if (olduser)
		{
#ifdef NOTE
			/* hang on to notes */
			notes = olduser->note;
			olduser->note = NULL;
#endif /* NOTE */
			remove_user(olduser); /* uses current->userlist */
			/* authlist/chanuserlist/dcclist is now a minefield */
		}
		newuser = add_user(user->name,user->pass,user->x.x.access); /* uses current->userlist */
		if (olduser)
		{
#ifdef NOTE
			newuser->note = notes;
#endif /* NOTE */
#ifdef DEBUG
			debug("(1)\n");
#endif /* DEBUG */
			reset_userlink(olduser,newuser); /* uses current->userlist */
			/* authlist/chanuserlist/dcclist should now be safe again. */
		}
#ifdef DEBUG
		debug("(2)\n");
#endif /* DEBUG */
		dupe_strp(user->mask,&newuser->mask); /* copy masks */
		dupe_strp(user->chan,&newuser->chan); /* copy channels */
		/* do not copy notes (creates spam) */
#ifdef DEBUG
		debug("(3)\n");
#endif /* DEBUG */
		newuser->x.comboflags = user->x.comboflags;
#ifdef BOTNET
		newuser->x.x.readonly = 0; /* dont copy the RO flag */
		newuser->modcount = user->modcount;
		newuser->tick = user->tick; /* is this proper??? */
#endif /* BOTNET */
	}
	current = backup; /* assume my old identity */
#ifdef DEBUG
	debug("(mirror_user) %s[%i] finished\n",user->name,user->x.x.access);
#endif /* DEBUG */
}

void mirror_userlist(void)
{
	User	*user;

#ifdef DEBUG
	debug("(mirror_userlist) mirroring userlist of %s(%i)\n",nullstr(current->nick),current->guid);
#endif /* DEBUG */

	for(user=current->userlist;user;user=user->next)
	{
#ifdef BOTNET
		if (!user->x.x.noshare)
#endif /* BOTNET */
		mirror_user(user);
	}
}
/*
 *  adding, removing, matching and searching for user records
 */

void reset_userlink(User *old, User *new)
{
	Auth	*auth,*nx_auth;
	Chan	*chan;
	ChanUser *cu;
	Client	*client,*nx_client;
	Spy	*spy;

	/*
	 *  auth list
	 */
	for(auth=current->authlist;auth;)
	{
		nx_auth = auth->next;
		if (auth->user == old)
		{
			if (new)
				auth->user = new;
			else
				remove_auth(auth);
		}
		auth = nx_auth;
	}

	/*
	 *  client list
	 */
	for(client=current->clientlist;client;)
	{
		nx_client = client->next;
		if (client->user == old)
		{
			if (new)
				client->user = new;
			else
				delete_client(client);
		}
		client = nx_client;
	}

	/*
	 *  spy list
	 */
	if (new)
	{
		for(spy=current->spylist;spy;spy=spy->next)
		{
			if (spy->dest == old->name)
				spy->dest = new->name;
		}
	}

	/*
	 *  channel userlists
	 */
	for(chan=current->chanlist;chan;chan=chan->next)
	{
		for(cu=chan->users;cu;cu=cu->next)
		{
			if (cu->user == old)
				cu->user = new;
		}
	}
}

void remove_user(User *user)
{
	User	**pp;
	Strp	*ump,*nxt;

	pp = &current->userlist;
	for(;(*pp);pp=&(*pp)->next)
	{
		if (*pp == user)
		{
			*pp = user->next;
#ifdef GREET
			Free((char**)&user->greet);
#endif /* GREET */
			for(ump=user->mask;ump;)
			{
				nxt = ump->next;
				Free((char**)&ump);
				ump = nxt;
			}
#ifdef NOTE
			for(ump=user->note;ump;)
			{
				nxt = ump->next;
				Free((char**)&ump);
				ump = nxt;
			}
#endif /* NOTE */
			Free((char**)&user);
			current->ul_save++;
			return;
		}
	}
}

User *add_user(char *handle, char *pass, int axs)
{
	User	*user;
	char	*p;

#ifdef DEBUG
	debug("(add_user) handle = %s; pass = %s; axs = %i\n",
		nullstr(handle),nullstr(pass),axs);
#endif /* DEBUG */

	set_mallocdoer(add_user);
	user = (User*)Calloc(sizeof(User) + StrlenX(handle,pass,NULL)); /* StrlenX() tolerates pass being NULL, Strlen2() does not. */
	user->x.x.access = axs;
	user->next = current->userlist;
	current->userlist = user;

	/*
	 *  "name\0pass\0"
	 */
	p = stringcpy(user->name,handle) + 1;
	if (pass)
	{
		user->pass = p;
		stringcpy(user->pass,pass);
	}
	current->ul_save++;
	return(user);
}

/*
 *  find the user record for a named handle
 */
User *find_handle(const char *handle)
{
	User 	*user;

	for(user=current->userlist;user;user=user->next)
	{
		if (!stringcasecmp(handle,user->name))
			return(user);
	}
	return(NULL);
}

int userhaschannel(const User *user, const char *channel)
{
	Strp	*ump;

	if (!channel)
		return(TRUE);
	for(ump=user->chan;ump;ump=ump->next)
	{
		if (*ump->p == '*' || !stringcasecmp(ump->p,channel))
			return(TRUE);
	}
	return(FALSE);
}

/*
 *  Find the user that best matches the userhost
 */
User *get_user(const char *userhost, const char *channel)
{
	Strp	*ump;
	User	*user,*save;
	int	num,best;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
	{
		if (userhaschannel(CurrentDCC->user,channel))
			return(CurrentDCC->user);
		return(NULL);
	}

	save = NULL;
	best = 0;
	for(user=current->userlist;user;user=user->next)
	{
		if (userhaschannel(user,channel))
		{
			for(ump=user->mask;ump;ump=ump->next)
			{
				num = num_matches(ump->p,userhost);
				if (num > best)
				{
					best = num;
					save = user;
				}
			}
		}
	}
	return(save);
}

/*
 *  highest channel on a given channel
 */
int get_useraccess(const char *userhost, const char *channel)
{
	User	*user;

	if (is_bot(userhost))
		return(BOTLEVEL);

	if ((user = get_user(userhost,channel)) == NULL)
		return(0);
	return(user->x.x.access);
}

/*
 *  highest access regardless of channel
 */
int get_maxaccess(const char *userhost)
{
	Strp	*ump;
	User	*user;
	int	uaccess;

	if (CurrentDCC && CurrentDCC->user->name == userhost)
		return(CurrentDCC->user->x.x.access);

	if (is_bot(userhost))
		return(BOTLEVEL);

	uaccess = 0;
	for(user=current->userlist;user;user=user->next)
	{
		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!matches(ump->p,userhost))
				uaccess = (user->x.x.access > uaccess) ? user->x.x.access : uaccess;
		}
	}
	return(uaccess);
}

int is_bot(const char *userhost)
{
	Mech	*bot;

	for(bot=botlist;bot;bot=bot->next)
	{
		if (!nickcmp(userhost,bot->nick))
			return(TRUE);
	}
	return(FALSE);
}

/*
 *  FIXME: does this apply to local bots?
 */
int get_protaction(Chan *chan, char *userhost)
{
	Strp	*ump;
	User	*user;
	int	prot;

	prot = 0;
	for(user=current->userlist;user;user=user->next)
	{
		if (userhaschannel(user,chan->name))
		{
			for(ump=user->mask;ump;ump=ump->next)
			{
				if (!matches(userhost,ump->p))
				{
					if (user->x.x.prot > prot)
						prot = user->x.x.prot;
					break;
				}
			}
		}
	}
	return(prot);
}

int usercanmodify(const char *from, const User *user)
{
	User	*u;
	Strp	*fmp,*ump;
	int	ua;

	ua = (user->x.x.access >= 100) ? 100 : user->x.x.access;

	for(u=current->userlist;u;u=u->next)
	{
		if (!CurrentDCC || CurrentDCC->user != u)
		{
			for(fmp=u->mask;fmp;fmp=fmp->next)
			{
				if (!matches(fmp->p,from))
					break;
			}
			if (!fmp)
				continue;
		}
		for(fmp=u->chan;fmp;fmp=fmp->next)
		{
			if (*fmp->p == '*' && u->x.x.access >= ua)
				return(TRUE);
			for(ump=user->chan;ump;ump=ump->next)
			{
				if (!stringcasecmp(ump->p,fmp->p))
				{
					if (u->x.x.access >= ua)
						return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}

/*
 *
 *  user commands related to userlist
 *
 */

void do_access(COMMAND_ARGS)
{
	const char *chan;
	char	*nuh;
	int	level;

	chan = get_channel(to,&rest);
	if (rest && ((nuh = chop(&rest))))
	{
		if (*nuh == current->setting[CHR_CMDCHAR].char_var)
		{
			nuh++;
			level = access_needed(nuh);
			if (level < 0)
				return;
			else
			if (level > 200)
				to_user(from,"The command \"%s\" has been disabled",nuh);
			else
				to_user(from,"The access level needed for \"%s\" is: %i",nuh,level);
			return;
		}
		if ((nuh = nick2uh(from,nuh)) == NULL)
			return;
	}
	else
	{
		nuh = from;
	}
	to_user(from,"Immortality Level for %s",nuh);
	to_user(from,"Channel: %s  Access: %i",chan,get_useraccess(nuh,chan));
}

#ifdef BOTNET
#define BOTNET_FMT	"%s/%s/"
#else
#define BOTNET_FMT	/* nothing */
#endif /* BOTNET */

#ifdef BOUNCE
#define BOUNCE_FMT	"%s/"
#else
#define BOUNCE_FMT	/* nothing */
#endif /* BOUNCE */

char *userlist_outputfmt[4] =
{
	"Chan%s\t: %s",
	"%s\t  %s",
	"Mask%s\t: %s",
	"%s\t  %s"
};

void do_userlist(COMMAND_ARGS)
{
	Strp	*ump;
	User	*user;
	char	*channel,*mask,*extra;
	int	minlevel = 0;
	int	maxlevel = BOTLEVEL;
	int	chanonly = FALSE;
	int	count;
	int	n;
#ifdef NOTE
	int	sz;
#endif /* NOTE */

	mask = channel = NULL;
	count = 0;

	if (*rest)
	{
		if (*rest == '+' && rest[1] >= '0' && rest[1] <= '9')
		{
			minlevel = asc2int(rest+1);
		}
		else
		if (*rest == '-' && rest[1] >= '0' && rest[1] <= '9')
		{
			maxlevel = asc2int(rest+1);
		}
		else
		if (*rest == '-' && (*rest|32) == 'b')
		{
			minlevel = BOTLEVEL;
		}
		else
		if (*rest == '-' && (*rest|32) == 'c')
		{
			chanonly = TRUE;
		}
		else
		if (ischannel(rest))
		{
			channel = rest;
		}
		else
		if (STRCHR(rest,'*') != NULL)
		{
			mask = rest;
		}
		else
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}

#ifdef DEBUG
	debug("(do_userlist) mask=%s minlevel=%i maxlevel=%i chanonly=%s\n",
		(mask) ? mask : "NOMASK",minlevel,maxlevel,(chanonly) ? "Yes" : "No");
#endif /* DEBUG */

	for(user=current->userlist;user;user=user->next)
	{
		if (user->x.x.access < minlevel)
			continue;
		if (user->x.x.access > maxlevel)
			continue;
		if (channel && (userhaschannel(user,channel) == FALSE))
			continue;
		if (chanonly && userhaschannel(user,MATCH_ALL) == TRUE)
			continue;
		ump = NULL;
		if (mask)
		{
			for(ump=user->mask;ump;ump=ump->next)
			{
				if (matches(mask,ump->p))
					break;
			}
		}
		if (ump)
			continue;

		table_buffer("User\t: %-11s   [%3i/%s/%s/" BOTNET_FMT BOUNCE_FMT "%s/P%i]",
			user->name,user->x.x.access,
			(user->x.x.aop)      ?  "AO" : "--",
			(user->x.x.avoice)   ?  "AV" : "--",
#ifdef BOTNET
			(user->x.x.readonly) ?  "RO" : "--",
			(user->x.x.noshare)  ?  "NS" : "--",
#endif /* BOTNET */
#ifdef BOUNCE
			(user->x.x.bounce)   ? "BNC" : "---",
#endif /* BOUNCE */
			(user->pass)         ?  "PW" : "--",
			user->x.x.prot);

		n = 0;
		ump = user->chan;
		extra = NULL;
		while(ump)
		{
			if (!extra)
				extra = (ump->next) ? "s" : "";
			table_buffer(userlist_outputfmt[n],extra,ump->p);
			extra = "";
			n |= 1;
			ump = ump->next;
			if (!ump && n < 2)
			{
				n = 2;
				extra = NULL;
				ump = user->mask;
			}
		}
#ifdef GREET
		if (user->greet)
		{
			table_buffer("Greet\t: %s%s",user->greet,
				(user->x.x.greetfile) ? " (greetfile)" :
				((user->x.x.randline) ? " (random line from file)" : ""));
		}
#endif /* GREET */
#ifdef NOTE
		if ((ump = user->note))
		{
			sz = n = 0;
			for(;ump;ump=ump->next)
			{
				if (*ump->p == 1)
					n++;
				else
					sz += strlen(ump->p);
			}
			table_buffer("Message\t: %i message%s (%i bytes)",n,(n == 1) ? "" : "s",sz);
		}
#endif /* NOTE */
		table_buffer(" ");
		count++;
	}
	table_send(from,0);
	to_user(from,"Total of %i users",count);
}

#define _ADD	'+'
#define _SUB	'-'

void do_user(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	Mech	*anybot;
	User	*user;
	Strp	*ump;
	char	*handle,*pt,*mask,*nick,*chan,*anum,*pass,*encpass;
	char	mode;
	char	tmpmask[NUHLEN];
	int	change;
	int	newaccess,uaccess;
	union	usercombo combo;
	int	ch,ho;

	mode = 0;
	mask = NULL;

	/*
	 *  chop wont return NULL (on_msg checks CARGS)
	 */
	handle = chop(&rest);
	if (*handle == '-' || *handle == '+')
	{
		if (handle[1] == 0)
		{
			mode = *handle;
			handle = chop(&rest);
		}
		else
		{
			mode = *handle;
			handle++;
		}
	}

	if (!handle)
		goto usage;
	user = find_handle(handle);

	if (mode == '+')
	{
		/*
		 *  Usage: USER + <handle> <channel|*> <nick|mask> <level> [password]
		 */
		/*
		 *  dont create duplicate handles
		 */
		if (user)
		{
			to_user(from,"User \"%s\" already exists",handle);
			return;
		}
		chan = chop(&rest);
		nick = chop(&rest);
		anum = chop(&rest);
		pass = chop(&rest);

		newaccess = asc2int(anum);
		if (errno)
			goto usage;

		if (*chan == '*')
			chan = MATCH_ALL;
		else
		if (!ischannel(chan))
			goto usage;

		if ((uaccess = get_useraccess(from,chan)) < cmdaccess)
			return;
		/*
		 *  check access level
		 */
		if ((newaccess != BOTLEVEL) && ((newaccess < 0) || (newaccess > OWNERLEVEL)))
		{
			to_user(from,"access must be in the range 0 - %i",OWNERLEVEL);
			return;
		}
		if ((uaccess != OWNERLEVEL) && (newaccess > uaccess))
		{
			to_user(from,"access denied on %s",chan);
			return;
		}
		/*
		 *  convert and check nick/mask
		 */
		if ((mask = nick2uh(from,nick)) == NULL) /* nick2uh uses nuh_buf */
			return;
		stringcpy(tmpmask,mask);
#ifdef DEBUG
		debug("(do_user) nick2uh(from \"%s\", nick \"%s\") = mask \"%s\"\n",from,nick,tmpmask);
#endif /* DEBUG */
#ifdef NEWBIE
		if (!matches(tmpmask,"!@"))
		{
			to_user(from,"Problem adding %s (global mask)",tmpmask);
			return;
		}
#endif /* NEWBIE */
		format_uh(tmpmask,FUH_USERHOST); /* format_uh uses local temporary buffer but copies result back into tmpmask */
		/*
		 *  dont duplicate users
		 */
		if (get_useraccess(tmpmask,chan))
		{
			to_user(from,"%s (%s) on %s is already a user",nick,tmpmask,chan);
			return;
		}
		/*
		 *  encrypt password
		 */
		encpass = (pass) ? makepass(pass) : NULL;
		/*
		 *  passwords for bots are never used
		 */
		if (newaccess == BOTLEVEL)
			encpass = NULL;

		/*
		 *  add_user() touches current->ul_save for us
		 */
		user = add_user(handle,encpass,newaccess);
		addtouser(&user->mask,tmpmask,FALSE);	/* does not run rehash_chanusers(), does not clobber nuh_buf */
		addtouser(&user->chan,chan,TRUE);	/* clobbers nuh_buf */
#ifdef DEBUG
		debug("(do_user) from %s, handle %s,\n\tmask %s, chan %s\n",from,handle,tmpmask,chan);
#endif /* DEBUG */
		to_user(from,"%s has been added as %s on %s",handle,tmpmask,chan);
		to_user(from,"Access level: %i%s%s",newaccess,(pass) ? "  Password: " : "",(pass) ? pass : "");
#ifdef NEWUSER_SPAM
		if ((newaccess != BOTLEVEL) && find_nuh(nick))
		{
			/*
			 *  yay! its a nick! we can spam them!
			 */
			char	cmdchar = current->setting[CHR_CMDCHAR].char_var;

			to_server("NOTICE %s :%s has blessed you with %i levels of immortality\n",
				nick,CurrentNick,newaccess);
			to_server("NOTICE %s :My command character is %c\n",nick,cmdchar);
			to_server("NOTICE %s :Use \026%c%s\026 for command help\n",nick,cmdchar,C_HELP);
			if (encpass)
			{
				to_server("NOTICE %s :Password necessary for doing commands: %s\n",nick,pass);
				to_server("NOTICE %s :If you do not like your password, use \026%c%s\026 to change it\n",
					nick,cmdchar,C_PASSWD);
			}
		}
#endif /* NEWUSER_SPAM */
		return;
	}

	if (!user)
	{
		to_user(from,TEXT_UNKNOWNUSER,handle);
		return;
	}
	if (!usercanmodify(from,user))
	{
		to_user(from,TEXT_USEROWNSYOU,user->name);
		return;
	}

	/*
	 *  ".user - handle" passed access tests, remove it.
	 */
	if (mode == '-')
	{
		to_user(from,"User %s has been purged",handle);
#ifdef BOTNET
		botnet_relay(NULL,"UD%i %s\n",user->modcount,user->name);
#endif /* BOTNET */
		/*
		 *  delete all references to the user record
		 */
		reset_userlink(user,NULL);
		/*
		 *  remove_user() touches current->ul_save for us
		 */
		remove_user(user);
		return;
	}

	combo.comboflags = user->x.comboflags;
	change = 0;
	ch = ho = 0;

	while((pt = chop(&rest)))
	{
		switch(*pt++)
		{
		case '+':
			mode = TRUE;
			break;
		case '-':
			mode = FALSE;
			break;
		default:
			goto usage;
		}

#ifdef BOTNET
		if (!stringcasecmp(pt,"NS"))
			combo.x.noshare = mode;
		else
		if (!stringcasecmp(pt,"RO"))
			combo.x.readonly = mode;
		else
#endif /* BOTNET */
#ifdef BOUNCE
		if (!stringcasecmp(pt,"BNC"))
			combo.x.bounce = mode;
		else
#endif /* BOUNCE */
		if (!stringcasecmp(pt,"AV"))
			combo.x.avoice = mode;
		else
		if (!stringcasecmp(pt,"AO"))
			combo.x.aop = mode;
		else
		if (!stringcasecmp(pt,"ECHO"))
			combo.x.echo = mode;
		else
		if (!stringcasecmp(pt,"CHAN"))
		{
			ch = (mode) ? _ADD : _SUB;
			mask = chop(&rest);
			if (!mask || *rest)	/* mask needs to be the last option */
				goto usage;
		}
		else
		if (!stringcasecmp(pt,"HOST"))
		{
			ho = (mode) ? _ADD : _SUB;
			mask = chop(&rest);
			if (!mask || *rest)	/* mask needs to be the last option */
				goto usage;
		}
		else
		if (*pt == 'p' || *pt == 'P')
		{
			pt++;
			if ((*pt >= '0') && (*pt <= '4') && (pt[1] == 0))
			{
				combo.x.prot = *pt - '0';
			}
			else
			if ((*pt == 0) && (mode == _SUB))
			{
				combo.x.prot = 0;
			}
			else
				goto usage;
		}
		else
		if ((uaccess = asc2int(pt)) >= 0 && uaccess <= BOTLEVEL && errno == 0)
		{
			combo.x.access = uaccess;
		}
		else
			goto usage;

		change++;
	}
	if (!change)
	{
usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	change = 0;

	/*
	 *  make the actual changes
	 */
	if (ch == _SUB && remfromuser(&user->chan,mask))
	{
		change++;
	}
	else
	if (ch == _ADD)
	{
		for(ump=user->chan;ump;ump=ump->next)
		{
			if (!stringcasecmp(ump->p,mask))
			{
				to_user(from,"Channel %s already exists for %s",mask,user->name);
				return;
			}
		}
#ifdef NEWBIE
		if (!ischannel(mask) && *mask != '*')
		{
			to_user(from,TEXT_CHANINVALID);
			return;
		}
#endif /* NEWBIE */
		addtouser(&user->chan,mask,TRUE);
		change++;
	}
	else
	if (ho == _SUB && remfromuser(&user->mask,mask))
	{
		change++;
	}
	else
	if (ho == _ADD)
	{
		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!stringcasecmp(ump->p,mask))
			{
				to_user(from,"Mask %s already exists for %s",mask,user->name);
				return;
			}
		}
#ifdef NEWBIE
		/*
		 *  newbies dont know what they're doing
		 */
		if (!matches(mask,"!@"))
		{
			to_user(from,"Problem adding %s (global mask)",mask);
			return;
		}
/* With ipv6 and other funky crap, this is no longer suitable */
/*
		if (matches("*@?*.?*",mask))
		{
			to_user(from,"Problem adding %s (invalid mask)",mask);
			return;
		}
*/
#endif /* NEWBIE */
		addtouser(&user->mask,mask,TRUE);
		change++;
	}

	if (combo.comboflags != user->x.comboflags)
	{
		user->x.comboflags = combo.comboflags;
		change++;
	}
	if (change)
	{
		to_user(from,TEXT_USERCHANGED,user->name);
		current->ul_save++;
#ifdef BOTNET
		user->modcount++;
		user->tick = global_tick++;
#endif /* BOTNET */
		mirror_user(user);
	}
	else
	{
		to_user(from,TEXT_USERNOTCHANGED,user->name);
	}
}

void do_echo(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	char	*tmp;

	if ((tmp = chop(&rest)))
	{
		if (!stringcasecmp(tmp,__STR_ON))
		{
			if (CurrentUser->x.x.echo == FALSE)
			{
				current->ul_save++;
				CurrentUser->x.x.echo = TRUE;
				mirror_user(CurrentUser);
			}
			to_user(from,TEXT_PARTYECHOON);
			return;
		}
		if (!stringcasecmp(tmp,__STR_OFF))
		{
			if (CurrentUser->x.x.echo == TRUE)
			{
				current->ul_save++;
				CurrentUser->x.x.echo = FALSE;
				mirror_user(CurrentUser);
			}
			to_user(from,TEXT_PARTYECHOOFF);
			return;
		}
	}
	usage(from);	/* usage for CurrentCmd->name */
}


void change_pass(User *user, char *pass)
{
	User	*new,**uptr;
	char	*enc;

	enc = makepass(pass);
	if (strlen(user->pass) <= strlen(enc))
	{
		stringcpy(user->pass,enc);
#ifdef BOTNET
		user->modcount++;
#endif /* BOTNET */
	}
	/*
	 *  password is stuck in a solid malloc in a linked list
	 *  add_user() touches current->ul_save for us
	 */
	new = add_user(user->name,makepass(pass),0);
	memcpy(&new->mask,&user->mask,((char*)&user->pass - (char*)&user->mask));
#ifdef BOTNET
	new->modcount++;
	new->tick = global_tick++;
#endif /* BOTNET */

	/*
	 *  unlink the old user record
	 */
	uptr = &new->next;
	while(*uptr)
	{
		if (*uptr == user)
		{
			*uptr = user->next;
			break;
		}
		uptr = &(*uptr)->next;
	}
	reset_userlink(user,new);
	Free((char**)&user);
	mirror_user(new);
}

void do_passwd(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*pass,*savedpass;

	if ((user = get_user(from,NULL)) == NULL)
		return;

	savedpass = pass = chop(&rest);
	if (user->pass)
	{
		pass = chop(&rest);
		if (!pass || !*pass)
		{
			usage(from);	/* usage for CurrentCmd->name */
			return;
		}
	}
	if (strlen(pass) < MINPASSCHARS)
	{
		to_user(from,TEXT_PASS_SHORT);
		return;
	}
	if (strlen(pass) > MAXPASSCHARS)
	{
		to_user(from,TEXT_PASS_LONG);
		return;
	}
	if (user->pass && !passmatch(savedpass,user->pass))
	{
		to_user(from,TEXT_PASS_INCORRECT);
		return;
	}
	to_user(from,TEXT_PASS_NEWSET);
	/*
	 *  all is well
	 *  change_pass() -> add_user() and current->ul_save is touched
	 */
	change_pass(user,pass);
}

void do_setpass(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS
	 */
	User	*user;
	char	*nick,*pass;

	nick = chop(&rest);
	pass = chop(&rest);

	if (!nick || !pass)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	if (strlen(pass) < MINPASSCHARS)
	{
		to_user(from,"password must be at least %i characters long",MINPASSCHARS);
		return;
	}
	if (strlen(pass) >= MAXPASSCHARS)
		pass[MAXPASSCHARS] = 0;
	if ((user = find_handle(nick)) == NULL)
	{
		to_user(from,TEXT_UNKNOWNUSER,nick);
		return;
	}
	if (!usercanmodify(from,user))
	{
		to_user(from,TEXT_USEROWNSYOU,user->name);
		return;
	}
	if (!stringcasecmp(pass,"none"))
	{
		user->pass = NULL;
		to_user(from,"password for %s has been removed",user->name);
		return;
	}
	to_user(from,"new password for %s has been set",user->name);
	/*
	 *  all is well
	 *  change_pass() -> add_user() and current->ul_save is groped
	 */
	change_pass(user,pass);
}

void do_save(COMMAND_ARGS)
{
	char	*fname;

	fname = current->setting[STR_USERFILE].str_var;
	current->ul_save = 1;
	if (!write_userlist(fname))
	{
		to_user(from,(fname) ? ERR_NOSAVE : ERR_NOUSERFILENAME,fname);
	}
	else
	{
		to_user(from,TEXT_LISTSAVED,fname);
	}
#ifdef SEEN
	if (seenfile && !write_seenlist())
	{
		to_user(from,TEXT_SEENNOSAVE,seenfile);
	}
#endif /* SEEN */
#ifdef NOTIFY
	if (current->notifylist)
		write_notifylog();
#endif /* NOTIFY */
#ifdef TRIVIA
	write_triviascore();
#endif /* TRIVIA */
}

void do_load(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	char	*fname;

	fname = current->setting[STR_USERFILE].str_var;
	if (!read_userlist(fname))
	{
		purge_shitlist();
		purge_kicklist();
		to_user(from,(fname) ? ERR_NOREAD : ERR_NOUSERFILENAME,fname);
	}
	else
	{
		to_user(from,TEXT_LISTREAD,fname);
	}
#ifdef SEEN
	if (seenfile && !read_seenlist())
	{
		to_user(TEXT_SEENNOLOAD,seenfile);
	}
#endif /* SEEN */
#ifdef TRIVIA
	read_triviascore();
#endif /* TRIVIA */
	mirror_userlist();
}
