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
#define FUNCTION_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#ifdef TEST
#define MAIN_C
#endif
#include "global.h"
#include "h.h"
#include "text.h"

#ifndef TEST

LS char timebuf[24];		/* max format lentgh == 20+1, round up to nearest longword -> 24 */
LS char idlestr[36];		/* max format lentgh == 24+1, round up to nearest longword -> 28 */
LS const char monlist[12][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
LS const char daylist[7][4] = {	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

/*
 *  memory allocation routines
 */

#ifdef DEBUG

void *Calloc(int size)
{
	aME	*mmep;
	aMEA	*mp;
	int	i;

#ifdef __GNUC__
	if (mallocdoer == NULL)
	{
		mallocdoer = __builtin_return_address(0);
		debug("(Calloc) mallocdoer = "mx_pfmt"\n",(mx_ptr)mallocdoer);
		mallocdoer = NULL;
	}
#endif /* GNUC */

	mmep = NULL;
	mp = mrrec;
	while(!mmep)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == NULL)
			{
				mmep = &mp->mme[i];
				break;
			}
		}
		if (!mmep)
		{
			if (mp->next == NULL)
			{
				mp->next = calloc(sizeof(aMEA),1);
				mmep = &mp->next->mme[0];
			}
			else
				mp = mp->next;
		}
	}

	/*
	 *  heap check +4 bytes
	 */
	if ((mmep->area = (void*)calloc(size+4,1)) == NULL)
	{
		run_debug();
		exit(1);
	}
	mmep->size = size;
	mmep->when = now;
	mmep->doer = mallocdoer;
	mallocdoer = NULL;
	return((void*)mmep->area+4);
}

void Free(char **mem)
{
	aME	*mmep;
	aMEA	*mp;
	int	*src;
	int	i;

	if (*mem == NULL)
		return;

	src = (int*)((*mem)-4);
	mmep = NULL;
	mp = mrrec;
	while(!mmep)
	{
		for(i=0;i<MRSIZE;i++)
		{
			if (mp->mme[i].area == src)
			{
				mmep = &mp->mme[i];
				break;
			}
		}
		if (!mmep)
		{
			if (mp->next == NULL)
			{
				debug("(Free) PANIC: Free(0x"mx_pfmt"); Unregistered memory block\n",(mx_ptr)src);
				run_debug();
				/*exit(1);	/* overreacting. just ignore it and accept the leak.  */
				return;
			}
			mp = mp->next;
		}
	}

	if (*src)
		debug("Free: Heap corruption at 0x"mx_pfmt"\n",(mx_ptr)(*mem)-4);
	mmep->area = NULL;
	mmep->size = 0;
	mmep->when = (time_t)0;
	free((*mem)-4);
	*mem = NULL;
}

#else /* DEBUG */

void *Calloc(int size)
{
	void	*tmp;

	if ((tmp = (void*)calloc(size,1)) == NULL)
		exit(1);
	return((void*)tmp);
}

/*
 *  Free() can be called with NULL's
 */
void Free(char **mem)
{
	if (*mem)
	{
		free(*mem);
		*mem = NULL;
	}
}

#endif /* DEBUG */

Strp *make_strp(Strp **pp, const char *string)
{
	set_mallocdoer(make_strp);
	*pp = (Strp*)Calloc(sizeof(Strp) + strlen(string));
	stringcpy((*pp)->p,string);
	return(*pp);
}

Strp *append_strp(Strp **pp, const char *string)
{
	while((*pp) != NULL)
		pp = &((*pp)->next);
	make_strp(pp,string);
	return(*pp);
}

Strp *prepend_strp(Strp **pp, const char *string)
{
	Strp	*sp;

	make_strp(&sp,string);
	sp->next = *pp;
	*pp = sp;
	return(sp);
}

/*
 *  Free() a whole linked list of any suitable type (Strp, Banlist)
 */
void purge_linklist(void **list)
{
	Strp	*sp,*nxt;

	sp = *list;
	while(sp)
	{
		nxt = sp->next;
		Free((char**)&sp);
		sp = nxt;
	}
	*list = NULL;
}

/*
 *  duplicate a list of Strp
 */
void dupe_strp(Strp *sp, Strp **pp)
{
	while(sp)
	{
		make_strp(pp,sp->p);
		pp = &((*pp)->next);
		sp = sp->next;
	}
}

Strp *e_table = NULL;

void table_buffer(const char *format, ...)
{
	Strp	**sp;
	va_list	msg;
	int	sz;

	va_start(msg,format);
	sz = sizeof(Strp) + vsprintf(globaldata,format,msg);
	va_end(msg);

	for(sp=&e_table;*sp;sp=&(*sp)->next)
		;

	set_mallocdoer(table_buffer);
	*sp = (Strp*)Calloc(sz);
	/* Calloc sets to zero (*sp)->next = NULL; */
	stringcpy((*sp)->p,globaldata);
}

void table_send(const char *from, const int space)
{
	char	message[MAXLEN];
	Strp	*sp,*next;
	char	*src,*o,*end;
	int	i,u,g,x,z[6];

	z[0] = z[1] = z[2] = z[3] = z[4] = z[5] = 0;
	for(sp=e_table;sp;sp=sp->next)
	{
		u = i = 0;
		src = o = sp->p;
		while(*src)
		{
			if (*src == '\037' || *src == '\002')
				u++;
			if (*src == '\t' || *src == '\r')
			{
				x = (src - o) - u;
				if (x > z[i])
					z[i] = x;
				i++;
				o = src+1;
				u = 0;
			}
			src++;
		}
	}

	for(sp=e_table;sp;sp=next)
	{
		next = sp->next;

		o = message;
		src = sp->p;
		g = x = i = 0;
		while(*src)
		{
			if (g)
			{
				end = src;
				while(*end && *end != '\t' && *end != '\r')
					end++;
				g -= (end - src);
				while(g-- > 0)
					*(o++) = ' ';
			}
			if (*src == '\037' || *src == '\002')
				x++;
			if (*src == '\t' || *src == '\r')
			{
				if (*src == '\r')
					g = z[i+1];
				src++;
				x += (z[i++] + space);
				while(o < (message + x))
					*(o++) = ' ';
			}
			else
				*(o++) = *(src++);
		}
		*o = 0;
		to_user(from,FMT_PLAIN,message);

		Free((char**)&sp);
	}
	e_table = NULL;
}

char *getuh(char *nuh)
{
	char	*s;

	s = nuh;
	while(*s)
	{
		if (*s == '!')
		{
			nuh = s + 1;
			/*
			 *  We have to grab everything from the first '!' since some
			 *  braindamaged ircds allow '!' in the "user" part of the nuh
			 */
			break;
		}
		s++;
	}
	return(nuh);
}

/*
 *  caller is responsible for:
 *
 *    src != NULL
 *   *src != NULL
 *
 */
char *get_token(char **src, const char *token_sep)
{
	const char *s;
	char	*token = NULL;

	/*
	 *  skip past any token_sep chars in the beginning
	 */

	if (0)		/* is this legal C? */
a:	++(*src);
	s = token_sep;
	while(**src && *s)
	{
		if (*(s++) == **src)
			goto a;
	}

	if (token || **src == 0)
		return(token);

	token = *src;

	/*
	 *  find the next token_sep char
	 */
	do {
		s = token_sep;
		do {
			if (*s == **src)
			{
				**src = 0;
				goto a;
			}
			++s;
		}
		while(*s);
		(*src)++;
	}
	while(**src);

	return(token);
}

/*
 *  time to string routines
 */

char *logtime(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%s %i %i %02i:%02i:%02i",	/* max format length: 20+1 */
		monlist[btime->tm_mon],btime->tm_mday,btime->tm_year+1900,
		btime->tm_hour,btime->tm_min,btime->tm_sec);
	return(timebuf);
}

char *time2str(time_t when)
{
	struct	tm *btime;

	if (!when)
		return(NULL);

	btime = localtime(&when);
	sprintf(timebuf,"%02i:%02i:%02i %s %02i %i",	/* max format length: 20+1 */
		btime->tm_hour,btime->tm_min,btime->tm_sec,monlist[btime->tm_mon],
		btime->tm_mday,btime->tm_year+1900);
	return(timebuf);
}

char *time2away(time_t when)
{
	struct	tm *btime;
	char	ampm;

	if (!when)
		return(NULL);

	btime = localtime(&when);
	if (btime->tm_hour < 12)
	{
		if (btime->tm_hour == 0)
			btime->tm_hour = 12;
		ampm = 'a';
	}
	else
	{
		if (btime->tm_hour != 12)
			btime->tm_hour -= 12;
		ampm = 'p';
	}

	sprintf(timebuf,"%i:%02i%cm %s %s %i",		/* max format length: 18+1 */
		btime->tm_hour,btime->tm_min,ampm,daylist[btime->tm_wday],
		monlist[btime->tm_mon],btime->tm_mday);
	return(timebuf);
}

char *time2medium(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%02i:%02i",			/* max format length: 5+1 */
		btime->tm_hour,btime->tm_min);
	return(timebuf);
}

char *time2small(time_t when)
{
	struct	tm *btime;

	btime = localtime(&when);
	sprintf(timebuf,"%s %02i",			/* max format length: 6+1 */
		monlist[btime->tm_mon],btime->tm_mday);
	return(timebuf);
}

char *idle2str(time_t when, int small)
{
	char	*dst;
	int	n,z[4];

	z[0] = when / 86400;
	z[1] = (when -= z[0] * 86400) / 3600;
	z[2] = (when -= z[1] * 3600) / 60;
	z[3] = when % 60;

	/* 32 : "9999 days, 24 hours, 59 minutes" */
	/* xx : "24 hours, 59 minutes" */
	/* xx : "59 minutes, 59 seconds" */
	/* xx : "59 seconds" */
	if (small)
	{
		char *f[] = {"day","hour","minute","second"};

		*idlestr = 0;
		for(n=0;n<4;n++)
		{
			if (*idlestr || z[n])
			{
				dst = STREND(idlestr);
				sprintf(dst,"%s%i %s%s",(*idlestr) ? ", " : "",z[n],f[n],(z[n]==1) ? "" : "s");
			}
		}
	}
	else
		/* 18+1 (up to 9999 days) */
		sprintf(idlestr,"%i day%s %02i:%02i:%02i",z[0],EXTRA_CHAR(z[0]),z[1],z[2],z[3]);
	return(idlestr);
}

const char *get_channel(const char *to, char **rest)
{
	const char *channel;

	if (*rest && ischannel(*rest))
	{
		channel = chop(rest);
	}
	else
	{
		if (!ischannel(to) && current->activechan)
			channel = current->activechan->name;
		else
			channel = to;
	}
	return(channel);
}

const char *get_channel2(const char *to, char **rest)
{
	const char *channel;

	if (*rest && (**rest == '*' || ischannel(*rest)))
	{
		channel = chop(rest);
	}
	else
	{
		if (!ischannel(to) && current->activechan)
			channel = current->activechan->name;
		else
			channel = to;
	}
	return(channel);
}

char *cluster(char *hostname)
{
	char	mask[NUHLEN];
	char	*p,*host;
	char	num,dot;

	host = p = hostname;
	num = dot = 0;
	while(*p)
	{
		if (*p == '@')
		{
			host = p + 1;
			num = dot = 0;
		}
		else
		if (*p == '.')
			dot++;
		else
		if (*p < '0' || *p > '9')
			num++;
		p++;
	}

	if (!num && (dot == 3))
	{
		/*
		 *  its a numeric IP address
		 *  1.2.3.4 --> 1.2.*.*
		 */
		p = mask;
		while(*host)
		{
			if (*host == '.')
			{
				if (num)
					break;
				num++;
			}
			*(p++) = *(host++);
		}
		stringcpy(p,".*.*");
	}
	else
	{
		/*
		 *  its not a numeric mask
		 */
		p = mask;
		*(p++) = '*';
		num = (dot >= 4) ? 2 : 1;
		while(*host)
		{
			if (*host == '.')
				dot--;
			if (dot <= num)
				break;
			host++;
		}
		stringcpy(p,host);
	}
	stringcpy(hostname,mask);
	return(hostname);
}

/*
 *  type   output
 *  ~~~~   ~~~~~~
 *  0,1    *!*user@*.host.com
 *  2      *!*@*.host.com
 */
char *format_uh(char *userhost, int type)
{
	char	tmpmask[NUHLEN];
	char	*u,*h;

	if (STRCHR(userhost,'*'))
		return(userhost);

	stringcpy(tmpmask,userhost);

	h = tmpmask;
	    get_token(&h,"!");	/* discard nickname */
	u = get_token(&h,"@");

	if (*h == 0)
		return(userhost);

	if (u && (type < 2))
	{
		if ((type = strlen(u)) > 9)
			u += (type - 9);
		else
		if (*u == '~')
			u++;
	}
	sprintf(userhost,"*!*%s@%s",(u) ? u : "",cluster(h));
	return(userhost);
}

/*
 *  NOTE! beware of conflicts in the use of nuh_buf, its also used by find_nuh()
 */
char *nick2uh(char *from, char *userhost)
{
	if (STRCHR(userhost,'!') && STRCHR(userhost,'@'))
	{
		stringcpy(nuh_buf,userhost);
	}
	else
	if (!STRCHR(userhost,'!') && !STRCHR(userhost,'@'))
	{
		/* find_nuh() stores nickuserhost in nuh_buf */
		if (find_nuh(userhost) == NULL)
		{
			if (from)
				to_user(from,"No information found for %s",userhost);
			return(NULL);
		}
	}
	else
	{
		stringcpy(nuh_buf,"*!");
		if (!STRCHR(userhost,'@'))
			stringcat(nuh_buf,"*@");
		stringcat(nuh_buf,userhost);
	}
	return(nuh_buf);
}

void deop_ban(Chan *chan, ChanUser *victim, char *mask)
{
	if (!mask)
		mask = format_uh(get_nuh(victim),FUH_USERHOST);
	send_mode(chan,85,QM_CHANUSER,'-','o',victim);
	send_mode(chan,90,QM_RAWMODE,'+','b',mask);
}

void deop_siteban(Chan *chan, ChanUser *victim)
{
	char	*mask;

	mask = format_uh(get_nuh(victim),FUH_HOST);
	deop_ban(chan,victim,mask);
}

void screwban_format(char *userhost)
{
	int	sz,n,pos;

#ifdef DEBUG
	debug("(screwban_format) %s\n",userhost);
#endif /* DEBUG */

	if ((sz = strlen(userhost)) < 8)
		return;

	n = RANDOM(4,sz);
	while(--n)
	{
		pos = RANDOM(0,(sz - 1));
		if (!STRCHR("?!@*",userhost[pos]))
		{
			userhost[pos] = (RANDOM(0,3) == 0) ? '*' : '?';
		}
	}
}

void deop_screwban(Chan *chan, ChanUser *victim)
{
	char	*mask;
	int	i;

	for(i=2;--i;)
	{
		mask = format_uh(get_nuh(victim),FUH_USERHOST);
		screwban_format(mask);
		deop_ban(chan,victim,mask);
	}
}

int is_nick(const char *nick)
{
	uchar	*p;

	p = (uchar*)nick;
	if ((attrtab[*p] & FNICK) != FNICK)
		return(FALSE);
	while(*p)
	{
		if ((attrtab[*p] & NICK) != NICK)
			return(FALSE);
		p++;
	}
	return(TRUE);
}

/* asc2int requires the whole string *anum to be a valid numeric */
int asc2int(const char *anum)
{
	int	res = 0, neg;

	errno = EINVAL;

	if (!anum || !*anum)
		return(-1);

	neg = (*anum == '-') ? 1 : 0;
	anum += neg;

	while(*anum)
	{
		if (0 == (attrtab[(uchar)*anum] & NUM))
			return(-1);
		res = (res * 10) + *(anum++) - '0';
	}
	errno = 0;
	return((neg) ? -res : res);
}

/* get_number returns as soon as a non numeric character is encountered */
int get_number(const char *rest)
{
	const char *src = NULL;
	int	n = 0;

	while(*rest)
	{
		if (0 == (attrtab[(uchar)*rest] & NUM))
			n = (n * 10) + (*(src = rest) - '0');
		else
		if (src)
			return(n);
		rest++;
	}
	return(n);
}

void fix_config_line(char *text)
{
	char	*s,*space;

	space = NULL;
	for(s=text;*s;s++)
	{
		if (*s == '\t')
			*s = ' ';
		if (!space && *s == ' ')
			space = s;
		if (space && *s != ' ')
			space = NULL;
	}
	if (space)
		*space = 0;
}

#endif /* ifndef TEST */

/*
 *  mask matching
 *  returns 0 for match
 *  returns 1 for non-match
 */
int matches(const char *mask, const char *text)
{
	const uchar *m = (uchar*)mask;
	const uchar *n = (uchar*)text;
	const uchar *orig = (uchar*)mask;
	int	wild,q;

	q = wild = 0;

	if (!m || !n)
		return(TRUE);

loop:
	if (!*m)
	{
		if (!*n)
			return(FALSE);
		for (m--;((*m == '?') && (m > orig));m--)
			;
		if ((*m == '*') && (m > orig) && (m[-1] != '\\'))
			return(FALSE);
		if (wild)
		{
			m = (uchar *)mask;
			n = (uchar *)++text;
		}
		else
			return(TRUE);
	}
	else
	if (!*n)
	{
		while(*m == '*')
			m++;
		return(*m != 0);
	}

	if (*m == '*')
	{
		while (*m == '*')
			m++;
		wild = 1;
		mask = (char *)m;
		text = (char *)n;
	}

	if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
	{
		m++;
		q = 1;
	}
	else
		q = 0;

	if ((tolowertab[(uchar)*m] != tolowertab[(uchar)*n]) && ((*m != '?') || q))
	{
		if (wild)
		{
			m = (uchar *)mask;
			n = (uchar *)++text;
		}
		else
			return(TRUE);
	}
	else
	{
		if (*m)
			m++;
		if (*n)
			n++;
	}
	goto loop;
}

int num_matches(const char *mask, const char *text)
{
	const char *p = mask;
	int	n;

	n = !matches(mask,text);

	if (n)
	{
		do {
			if (*p != '*')
				n++;
		}
		while(*++p);
	}
	return(n);
}

int is_safepath(const char *path, int filemustexist)
{
	struct stat st;
	char	tmp[PATH_MAX];
	const char *src;
	char	*dst;
	int	mo,dir_r,orr,oerrno;

#ifdef TEST
	memset(&st,0,sizeof(st));
#endif
	if (*(src = path) == '/') /* dont allow starting at root, only allow relative paths */
		return(-1);

	if (strlen(path) >= PATH_MAX)
		return(-6);

	orr = lstat(path,&st);
	oerrno = errno;

	if (filemustexist == FILE_MUST_EXIST && orr == -1 && errno == ENOENT)
		return(-2);
	if (filemustexist == FILE_MUST_NOTEXIST && orr == 0)
		return(-3);

	mo = st.st_mode; /* save mode for later */
	dir_r = -1;

	for(dst=tmp;*src;)
	{
		if (*src == '/')
		{
			*dst = 0;
			if ((dir_r = lstat(tmp,&st)) == -1 && errno == ENOENT)
				return(-7);
			if (!(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) /* disallow all except regular files and directories */
				return(-4);
			if (st.st_ino == parent_inode) /* disallow traversing below bots homedir */
				return(-5);
		}
		if (dst == tmp + PATH_MAX-1)
			return(-6);
		*dst++ = *src++;
	}
	if (filemustexist != FILE_MUST_EXIST && orr == -1 && oerrno == ENOENT)
		return(TRUE);
	return (S_ISREG(mo)) ? TRUE : FALSE;
}

#ifdef TEST

#include "debug.c"

#define P0	"verylongexcessivelengthfilenamegibberish"
#define P1	P0 "/" P0 "/" P0 "/" P0 "/" P0 "/" P0 "/" P0 "/" P0 "/" P0 "/" P0 "/"
#define P2	P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P1 P0

char *tostr[] = { "ZERO", "FILE_MUST_EXIST", "FILE_MAY_EXIST", "FILE_MUST_NOTEXIST" };

void *Calloc(int size)
{
	return(calloc(1,size));
}

void testcase(const char *str, int expected, int filemustexist)
{
	int	r;

	r = is_safepath(str,filemustexist);
	if (r == expected)
	{
		debug("testcase SUCCESS: testpath %s %s -> result %i\n",
			(strlen(str)>50) ? "(very long string)" : str,tostr[filemustexist],r);
	}
	else
	{
		debug("testcase FAIL: testpath %s(%i) %s -> result %i, expected %i\n",
			(strlen(str)>50) ? "(very long string)" : str,
			strlen(str),tostr[filemustexist],r,expected);/*(r) ? "TRUE" : "FALSE",(expected) ? "TRUE" : "FALSE"); */
	}
}

const char teststr[] = "dingchat";

void teststring(void)
{
	char	str[100];

	stringcpy(str,teststr);
	if (stringcmp(str,teststr))
		debug("teststring FAIL: stringcpy(str,%s) != %s (%s)\n",teststr,teststr,str);
	else
		debug("teststring SUCCESS: stringcpy(str,%s) == %s\n",teststr,teststr);

	stringcpy_n(str+4,"xoom",2);
	if (stringcmp(str,"dingxo"))
		debug("teststring FAIL: stringcpy_n(str+4,%s,2) != %s (%s)\n","xoom","dingxo",str);
	else
		debug("teststring SUCCESS: stringcpy_n(str+4,%s,2) == %s\n","xoom","dingxo");

	stringcat(str,"free");
	if (stringcmp(str,"dingxofree"))
		debug("teststring FAIL: stringcat(str,%s) != %s (%s)\n","free","dingxofree",str);
	else
		debug("teststring SUCCESS: stringcat(str,%s) == %s\n","free","dingxofree");
}

int main(int argc, char **argv, char **envp)
{
	struct stat st;
	int	r;

	dodebug = 1;
        stat("../..",&st);
        parent_inode = st.st_ino; /* used for is_safepath() */

	debug("PATH_MAX = %i\n",PATH_MAX);
	if (argv[1] == NULL)
	{
		testcase("/etc/passwd",-1,FILE_MAY_EXIST);
		testcase("../../../../../../../../../../../etc/passwd",-5,FILE_MAY_EXIST);
		testcase("../anyparentfile",1,FILE_MAY_EXIST);
		testcase("../../anyparentparentfile",-5,FILE_MAY_EXIST);
		testcase("function.c",TRUE,FILE_MUST_EXIST);
		testcase("./function.c",TRUE,FILE_MUST_EXIST);
		testcase("function.c",-3,FILE_MUST_NOTEXIST);
		testcase("./function.c",-3,FILE_MUST_NOTEXIST);
		testcase("nosuchfile",1,FILE_MAY_EXIST);
		testcase("nosuchfile",1,FILE_MUST_NOTEXIST);
		testcase("./nosuchfile",1,FILE_MUST_NOTEXIST);
		testcase("../../nosuchfile",-5,FILE_MUST_NOTEXIST);
		testcase(P2,-6,FILE_MAY_EXIST);
		teststring();
		exit(0);
	}

	r = is_safepath(argv[1],FILE_MAY_EXIST);
	debug("testpath %s -> result %s\n",argv[1],(r) ? "TRUE" : "FALSE");
}

#endif /* TEST */

