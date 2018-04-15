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
#define AUTH_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifndef MD5CRYPT
#ifndef SHACRYPT

/*
 *  simple password encryption
 */

char	pctab[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuwvxuz0123456789+-";

#define CIPHER(a1,a2,a3,a4,b1,b2,b3,b4) \
{					\
	a2 =  a2 ^  a1;			\
	b2 =  b2 ^  b1;			\
	a3 =  a2 ^  a1;			\
	b3 =  b2 ^  b1;			\
	b3 >>= 2;			\
	b3 |= ((a3 & 3) << 30);		\
	a3 >>= 2;			\
	a2 =  a3 ^  a2;			\
	b2 =  b3 ^  b2;			\
	a4 = ~a4 ^  a2;			\
	b4 = -b4 ^  b2;			\
	a2 =  a4 ^ ~a2;			\
	b2 =  b4 ^ -b2;			\
	b1 >>= 1;			\
	b1 |= ((a1 & 1) << 31);		\
	a1 >>= 1;			\
}

char *cipher(char *arg)
{
	static	char res[40];
	uint32_t B1a,B2a,B3a,B4a;
	uint32_t B1b,B2b,B3b,B4b;
	uchar	*ptr;
	uint32_t R1;
	int	i;

	if (!arg || !*arg)
		return(NULL);

	B1a = B2a = B3a = B4a = 0;
	B1b = B2b = B3b = B4b = 0;
	ptr = arg;

	while(*ptr)
	{
		R1 = *ptr;
		for(i=0;i<8;i++)
		{
			if (R1 & 1)
			{
				B1a |= 0x80008000;
				B1b |= 0x80008000;
			}
			R1  >>= 1;
			CIPHER(B1a,B2a,B3a,B4a,B1b,B2b,B3b,B4b);
		}
		ptr++;
	}
	while((B1a) || (B1b))
	{
		CIPHER(B1a,B2a,B3a,B4a,B1b,B2b,B3b,B4b);
	}

	for(i=0;i<10;i++)
	{
		res[i] = pctab[(B4b & 0x3f)];
		B4b >>= 6;
		B4b |= ((B4a & 0x3f) << 26);
		B4a >>= 6;
	}
	res[i] = 0;
	return(res);
}

char *makepass(char *plain)
{
	return(cipher(plain));
}

int passmatch(char *plain, char *encoded)
{
	return(!stringcmp(cipher(plain),encoded));
}

#endif /* not SHACRYPT */
#endif /* not MD5CRYPT */

#ifdef SHACRYPT

/*
 *  use SHA512 to hash passwords
 */

char *CRYPT_FUNC(const char *, const char *);

char *makepass(char *plain)
{
	char	salt[8];

	sprintf(salt,"$6$%04x",(rand() >> 16));
	return(CRYPT_FUNC(plain,salt));
}

int passmatch(char *plain, char *encoded)
{
	char	*enc;

	if (matches("$6$????$*",encoded))
		return(FALSE);
	enc = CRYPT_FUNC(plain,encoded);
	return(!stringcmp(enc,encoded));
}

#endif /* SHACRYPT */

#ifndef SHACRYPT
#ifdef MD5CRYPT

/*
 *  use MD5 to hash passwords
 */

char *CRYPT_FUNC(const char *, const char *);

char *makepass(char *plain)
{
	char	salt[8];

	sprintf(salt,"$1$%04x",(rand() >> 16));
	return(CRYPT_FUNC(plain,salt));
}

int passmatch(char *plain, char *encoded)
{
	char	*enc;

	if (matches("$1$????$*",encoded))
		return(FALSE);
	enc = CRYPT_FUNC(plain,encoded);
	return(!stringcmp(enc,encoded));
}

#endif /* MD5CRYPT */
#endif /* not SHACRYPT */
/*
 *
 */
void delete_auth(char *userhost)
{
	Auth	*auth,**pp;

	pp = &current->authlist;
	while(*pp)
	{
		if (!stringcasecmp(userhost,(*pp)->nuh))
		{
			auth = *pp;
			*pp = auth->next;
			Free((char**)&auth);
			/*
			 *  dont return yet, there might be more auths
			 */
			continue;
		}
		pp = &(*pp)->next;
	}
}

void remove_auth(Auth *auth)
{
	Auth	**pp;

	pp = &current->authlist;
	while(*pp)
	{
		if (*pp == auth)
		{
			*pp = auth->next;
			Free((char**)&auth);
			/*
			 *  when removing a DCC/telnet auth the connection should also be removed
			 */
			return;
		}
		pp = &(*pp)->next;
	}
}

/*
 *  register nick-changes in auths
 */
void change_authnick(char *nuh, char *newnuh)
{
	Auth	*auth,*oldauth,**pp;
	char	*n1,*n2;

	pp = &current->authlist;
	while(*pp)
	{
		auth = *pp;
		if (!nickcmp(nuh,auth->nuh))
		{
			for(n1=nuh;*n1 != '!';n1++);
			for(n2=newnuh;*n2 != '!';n2++);
			if ((n1 - nuh) >= (n2 - newnuh))
			{
#ifdef DEBUG
				debug("(change_authnick) auth->nuh = `%s'; was `%s' (stringcpy) (L1 = %i, L2 = %i)\n",
					newnuh,nuh,(int)(n1 - nuh),(int)(n2 - newnuh));
#endif /* DEBUG */
				stringcpy(auth->nuh,newnuh);
			}
			else
			{
#ifdef DEBUG
				debug("(change_authnick) auth->nuh = `%s'; was `%s' (re-alloc)\n",newnuh,nuh);
#endif /* DEBUG */
				/*
				 *  de-link the old auth record
				 */
				oldauth = auth;
				*pp = auth->next;

				set_mallocdoer(change_authnick);
				auth = (Auth*)Calloc(sizeof(Auth) + strlen(newnuh));
				auth->user = oldauth->user;
				auth->active = now;
				auth->next = current->authlist;
				current->authlist = auth;
				stringcpy(auth->nuh,newnuh);
				Free((char**)&oldauth);
			}
			return;
		}
		pp = &(*pp)->next;
	}
}

LS User *au_user;
LS const char *au_userhost;
LS const char *au_channel;
LS int au_access;

void aucheck(User *user)
{
	Strp	*ump;

	if (au_channel)
	{
		for(ump=user->chan;ump;ump=ump->next)
		{
			if (*ump->p == '*' || !stringcasecmp(au_channel,ump->p))
				break;
		}
		if (!ump)
			return;
	}

	for(ump=user->mask;ump;ump=ump->next)
	{
		if (!matches(ump->p,au_userhost))
		{
			if (au_access < user->x.x.access)
			{
				au_access = user->x.x.access;
				au_user = user;
			}
			return;
		}
	}
}

User *get_authuser(const char *userhost, const char *channel)
{
	User	*user;
	Auth	*auth;

	au_user = NULL;
	au_userhost = userhost;
	au_channel = channel;
	au_access = 0;

	/*
	 *  people who are authenticated
	 */
	for(auth=current->authlist;auth;auth=auth->next)
	{
		if (au_access < auth->user->x.x.access)
		{
			if (!stringcasecmp(userhost,auth->nuh))
			{
				aucheck(auth->user);
			}
		}
	}
	if (current->setting[TOG_ENFPASS].int_var)
		return(au_user);
	/*
	 *  people who dont need a password
	 */
	for(user=current->userlist;user;user=user->next)
	{
		if (!user->pass && (au_access < user->x.x.access))
		{
			aucheck(user);
		}
	}
	return(au_user);
}

int get_authaccess(const char *userhost, const char *channel)
{
	User	*user;
	Strp	*ump;

	if (userhost == CoreUser.name)
		return(100);
	if (CurrentDCC && CurrentDCC->user->name == userhost)
	{
		user = CurrentDCC->user;
		if (!channel)
			return(user->x.x.access);
		for(ump=user->chan;ump;ump=ump->next)
		{
			if (*ump->p == '*' || !stringcasecmp(ump->p,channel))
				return(user->x.x.access);
		}
		return(0);
	}
	if (is_bot(userhost))
		return(BOTLEVEL);

	get_authuser(userhost,channel);
	return(au_access);
}

int make_auth(const char *userhost, const User *user)
{
	Auth	*auth;
	Chan	*chan;
	ChanUser *cu;

	for(auth=current->authlist;auth;auth=auth->next)
	{
		if ((auth->user == user) && !stringcasecmp(auth->nuh,userhost))
			return(TRUE);
	}

#ifdef DEBUG
	debug("(make_auth) %s = %s\n",userhost,user->name);
#endif /* DEBUG */
	/*
	 *  there is no matching auth for this user, we add one
	 */
	set_mallocdoer(make_auth);
	auth = (Auth*)Calloc(sizeof(Auth) + strlen(userhost));
	auth->user = (User*)user;
	auth->active = now;
	stringcpy(auth->nuh,userhost);

	auth->next = current->authlist;
	current->authlist = auth;

	for(chan=current->chanlist;chan;chan=chan->next)
	{
		if (!chan->bot_is_op)
			continue;
		if (!chan->setting[TOG_AOP].int_var)
			continue;
		if ((cu = find_chanuser(chan,userhost)) == NULL)
			continue;
		if (cu->user && cu->user->x.x.aop)
		{
			send_mode(chan,120,QM_CHANUSER,'+','o',(void*)cu);
		}
	}
	return(FALSE);
}

/*
 *
 *  authentication commands
 *
 */

/*
help:AUTH
help:VERIFY
usage:AUTH <password>
usage:VERIFY <password>
file:../help/AUTH
file:../help/VERIFY
begin:

Authenticate yourself with the bot.

See also: passwd, setpass
:end
*/
void do_auth(COMMAND_ARGS)
{
#ifdef BOTNET
	char	*checksum;
#endif /* BOTNET */
	Strp	*ump;
	User	*user;
	char	*pass;
	int	hostmatch;

	if ((pass = chop(&rest)) == NULL)
		return;

	/*
	 *  chop chop
	 */
	if (strlen(pass) > MAXPASSCHARS)
		pass[MAXPASSCHARS] = 0;

	hostmatch = FALSE;
	/*
	 *  find a matching password
	 */
	for(user=current->userlist;user;user=user->next)
	{
		for(ump=user->mask;ump;ump=ump->next)
		{
			if (!matches(ump->p,from))
			{
				if (user->pass && passmatch(pass,user->pass))
					goto listcheck;
				hostmatch = TRUE;
			}
		}
	}

	/*
	 *  if there was a matching host, they get a message...
	 */
	if (hostmatch)
		to_user(from,TEXT_PASS_INCORRECT);
	return;

listcheck:
#ifdef BOTNET
	checksum = "";
	if (user->pass)
	{
		sprintf(globaldata,"%s %s %s",from,user->name,user->pass);
		checksum = makepass(globaldata);
	}
	botnet_relay(NULL,"PA%s %s %s\n",user->name,from,checksum);
#endif /* BOTNET */
	if (make_auth(from,user))
	{
		to_user(from,"You have already been authorized");
		return;
	}
	to_user(from,"You are now officially immortal");
}
