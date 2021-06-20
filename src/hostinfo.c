/*

    EnergyMech, IRC bot software
    Copyright (c) 2020 proton

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
#define HOSTINFO_C
#include "config.h"
#ifdef HOSTINFO
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include <sys/utsname.h>
#include <sys/inotify.h>

/*
Emulate this, but use the same memory space:
char	vmpeak[32];
char	vmsize[32];
char	vmrss[32];
char	vmdata[32];
char	vmstk[32];
char	vmexe[32];
char	vmlib[32];
*/
char	omni[224]; /* 32*7 */
#define vmpeak	&omni[0]
#define vmsize	&omni[32]
#define vmrss	&omni[64]
#define vmdata	&omni[96]
#define vmstk	&omni[128]
#define vmexe	&omni[160]
#define vmlib	&omni[192]

struct /* statusvalues */
{
	const char *key;
	const int klen;
	char	*valbuf;

} sv[] =
{
{ "VmPeak:",	7,	vmpeak },
{ "VmSize:",	7,	vmsize },
{ "VmRSS:",	6,	vmrss },
{ "VmData:",	7,	vmdata },
{ "VmStk:",	6,	vmstk },
{ "VmExe:",	6,	vmexe },
{ "VmLib:",	6,	vmlib },
{ NULL, 0, NULL }
};

#ifdef DEBUG

struct
{
	int	value;
	char	*str;

} in2str[] =
{
{ IN_ACCESS,		"IN_ACCESS" },		/* File was accessed (read) */
{ IN_ATTRIB,		"IN_ATTRIB" },		/* Metadata changed, e.g., permissions, timestamps, extended attributes, link count, UID, GID, etc. */
{ IN_CLOSE_WRITE,	"IN_CLOSE_WRITE" },	/* File opened for writing was closed */
{ IN_CLOSE_NOWRITE,	"IN_CLOSE_NOWRITE" },	/* File not opened for writing was closed */
{ IN_CREATE,		"IN_CREATE" },		/* File/directory created in watched directory */
{ IN_DELETE,		"IN_DELETE" },		/* File/directory deleted from watched directory */
{ IN_DELETE_SELF,	"IN_DELETE_SELF" },	/* Watched file/directory was itself deleted */
{ IN_MODIFY,		"IN_MODIFY" },		/* File was modified */
{ IN_MOVE_SELF,		"IN_MOVE_SELF" },	/* Watched file/directory was itself moved */
{ IN_MOVED_FROM,	"IN_MOVED_FROM" },	/* Generated for the directory containing the old filename when a file is renamed */
{ IN_MOVED_TO,		"IN_MOVED_TO" },	/* Generated for the directory containing the new filename when a file is renamed */
{ IN_OPEN,		"IN_OPEN" },		/* File was opened */
{ 0,			NULL		}
};

char *inomask2str(uint32_t mask, char *dst)
{
	const char *src;
	int	i,n = 0;

	for(i=0;in2str[i].str;i++)
	{
		if ((mask & in2str[i].value) == in2str[i].value)
		{
			if (n)
				dst[n++] = '|';
			src = in2str[i].str;
			for(;src[n];n++)
				dst[n] = src[n];
		}
	}
	dst[n] = 0;
	return(dst);
}

#endif /* DEBUG */

int monitor_fs(const char *file)
{
	FileMon	*fmon,*fnew;
	int	ino;

	if ((ino = inotify_init()) < 0)
		return(-1);
	inotify_add_watch(ino,file,IN_ALL_EVENTS);
#ifdef DEBUG
	debug("(monitor_fs) added notifier on %s [fd %i]\n",file,ino);
#endif
        set_mallocdoer(monitor_fs);
	fnew = Calloc(sizeof(FileMon) + strlen(file));
	fnew->fd = ino;
	fnew->nospam = now;
	stringcpy(fnew->filename,file);

	fnew->next = filemonlist;
	filemonlist = fnew;
	return(0);
}

int parse_proc_status(char *line)
{
	const char *key;
	char	*dest,*limit;
	int	i;

	key = chop(&line);
#ifdef DEBUG
	debug("pps key = %s (%s)\n",key,line);
#endif
	if (key == NULL)
		return(FALSE);
	for(i=0;sv[i].key;i++)
	{
		if (strncmp(key,sv[i].key,sv[i].klen) == 0)
		{
#ifdef DEBUG
			debug("(parse_proc_status) key %s -> %s\n",key,line);
#endif /* DEBUG */
			dest = sv[i].valbuf;
			limit = sv[i].valbuf + 31;
			while(*line == ' ')
				line++;
			while(*line && dest <= limit)
				*(dest++) = *(line++);
			*dest = 0;
		}
	}
	return(FALSE);
}

int havemodel,bogo,procct,physid,cpus,cores,siblings;

/*
proton@endemic:~/energymech/src> cat /proc/loadavg
0.00 0.00 0.00 1/178 6759

processor       : 0
model name      : Intel(R) Core(TM)2 Quad  CPU   Q8200  @ 2.33GHz
physical id     : 0
siblings        : 4 <-- total number of cores in all cpus
core id         : 0
cpu cores       : 4
bogomips        : 4533.39

*/

const char *cfind(const char *s1, const char *s2)
{
	while(*s2)
	{
		while(*s1 == ' ' || *s1 == '\t')
			s1++;
		if (tolowertab[(uchar)(*s1)] != tolowertab[(uchar)(*s2)])
			return(NULL);
		s1++;
		s2++;
	}
	while(*s1 == ' ' || *s1 == '\t' || *s1 == ':')
		s1++;
	return(s1);
}

int parse_proc_cpuinfo(char *line)
{
	const char *src;
	char	*dst = omni,*end = omni+sizeof(omni)-1;
	int	v;

	if ((src = cfind(line,"cpumodel")) || (src = cfind(line,"modelname")))
	{
		if (havemodel == 1)
			return(FALSE);
		*(dst++) = ' '; /* prime with a leading space */
		while(*src && dst < end)
		{
			if (*src != ' ' || dst[-1] != ' ')
				*(dst++) = *src;
			src++;
		}
		*dst = 0;
#ifdef DEBUG
		debug("(parse_proc_cpuinfo) model name = %s\n",omni);
#endif
		havemodel = 1;
	}
	if ((src = cfind(line,"physicalid")) || (src = cfind(line,"core")))
	{
		v = asc2int(src);
		if (errno == 0)
		{
			physid += (v+1);
		}
	}
	else
	if (siblings == 0 && (src = cfind(line,"siblings")))
	{
		cores = siblings = asc2int(src);
	}
	else
	if ((src = cfind(line,"bogomips")))
	{
		bogo++;
		if (bogo != 1)
			return(FALSE);
		dst = vmlib;
		while(*src && dst < end)
		{
			if (*src != ' ' || dst[-1] != ' ')
				*(dst++) = *src;
			src++;
		}
		*dst = 0;
	}
	return(FALSE); /* return false to continue reading lines */
}

void select_monitor()
{
	FileMon	*fmon;

	for(fmon=filemonlist;fmon;fmon=fmon->next)
		if (fmon->fd >= 0)
		{
			FD_SET(fmon->fd,&read_fds);
			chkhigh(fmon->fd);
		}
}

void process_monitor()
{
	FileMon	*fmon;
	struct inotify_event *ivent;
	char	tmp[256];
	int	n;

	for(fmon=filemonlist;fmon;fmon=fmon->next)
	{
		if (fmon->fd >= 0 && FD_ISSET(fmon->fd,&read_fds))
		{
			ivent = (struct inotify_event *)&globaldata;

			n = read(fmon->fd,globaldata,sizeof(struct inotify_event));
			if (ivent->len > 0)
				read(fmon->fd,ivent->name,ivent->len);
			else
				*ivent->name = 0;
#ifdef DEBUG
			debug("ino %i, n %i, sz %i\n",fmon->fd,n,sizeof(in2str));
			debug("wd %i, mask %lu, cookie %lu, len %lu, name %s\n",
				ivent->wd,ivent->mask,ivent->cookie,ivent->len,ivent->name);
			debug("%s\n",inomask2str(ivent->mask,tmp));
			debug("(process_monitor) ino %i bytes read, int wd = %i, uint32_t mask = %s (%lu), "
				"uint32_t cookie = %lu, uint32_t len = %lu, char name = %s\n",
				n,ivent->wd,inomask2str(ivent->mask,tmp),ivent->mask,ivent->cookie,ivent->len,ivent->name);
#endif
			if ((ivent->mask & IN_CLOSE_WRITE) == IN_CLOSE_WRITE)
				return;
			if (fmon->nospam > now-30)
				return;
			fmon->nospam = now;
			send_global(SPYSTR_SYSMON,"Alert: file ``%s'' was touched",fmon->filename);
		}
	}
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------*/

/*
help:HOSTINFO:(no arguments)

Equivalent to ``uname -orm''

See also: meminfo, cpuinfo
*/
void do_hostinfo(COMMAND_ARGS)
{
	char	*h,hostname[256];
	struct utsname un;

	hostname[255] = 0;
	if (gethostname(hostname,250) < 0)
		h = "(hostname error)";
	else
		h = hostname;

	if (uname(&un) == 0)
		to_user_q(from,"%s %s %s %s",h,un.sysname,un.release,un.machine);
}

/*
help:MEMINFO:(no arguments)

Will display memory usage of the energymech process.

VM   Virtual size, size if everything was loaded into memory)
RSS   Resident set size, physical memory actually in use right now.
  Code    Memory allocated for code
  Data    Memory allocated for data
  Libs    Memory used by shared libraries
  Stack   Memory allocated for stack

See also: hostinfo, cpuinfo
*/
void do_meminfo(COMMAND_ARGS)
{
	char	fn[64];
	pid_t	p;
	int	i,fd;

	p = getpid();
	snprintf(fn,sizeof(fn),"/proc/%i/status",p);

	for(i=0;sv[i].key;i++)
		*(sv[i].valbuf) = 0;
	if ((fd = open(fn,O_RDONLY)) < 0)
		return;
	readline(fd,&parse_proc_status);	/* readline closes fd */

	to_user_q(from,"VM %s (Max %s), RSS %s [ Code %s, Data %s, Libs %s, Stack %s ]",
		vmsize,vmpeak,vmrss,vmexe,vmdata,vmlib,vmstk);
}

/*
help:CPUINFO:(no arguments)

See also: hostinfo, meminfo
*/
void do_cpuinfo(COMMAND_ARGS)
{
	char	bogostr[64],cpustr[64];
	char	*a1,*a2,*a3,*dst;
	int	fd,n;
	double	loads[3];

#ifdef DEVELOPING
	a1 = chop(&rest);
	if (a1)
		sprintf(bogostr,"/home/git/cpuinfo/%s",a1);
	else
		stringcpy(bogostr,"/proc/cpuinfo");
	if ((fd = open(bogostr,O_RDONLY)) < 0)
/*
	if ((fd = open("/home/git/cpuinfo/mips3",O_RDONLY)) < 0)
	if ((fd = open("/home/git/cpuinfo/mips2",O_RDONLY)) < 0)
	if ((fd = open("/home/git/cpuinfo/mips1",O_RDONLY)) < 0)
	if ((fd = open("/home/git/cpuinfo/intel1",O_RDONLY)) < 0)
	if ((fd = open("/home/git/cpuinfo/cosmiccow",O_RDONLY)) < 0)
*/
#endif
	if ((fd = open("/proc/cpuinfo",O_RDONLY)) < 0)
#ifdef DEBUG
	{
		debug("(do_cpuinfo) /proc/cpuinfo: %s\n",strerror(errno));
		return;
	}
#else
		return;
#endif

	global_from = from;
	havemodel = bogo = siblings = procct = cpus = cores = physid = 0;
	omni[1] = 0;
	readline(fd,&parse_proc_cpuinfo); /* readline closes fd */

	if (getloadavg(loads,3) < 3)
		return;

#ifdef DEBUG
	debug("(do_cpuinfo) procct %i, physid %i, cores %i, bogo %i\n",procct,physid,cores,bogo);
#endif

	if (cores == 0)
		cores = bogo;
	if (cores && physid && (physid % cores) == 0)
		cpus = (physid / cores)-1;
	if (cores && (cpus == 0 || physid == cores))
		cpus = 1;

	*bogostr = 0;
	*cpustr = 0;
	if (bogo)
		sprintf(bogostr,", %s BogoMips",vmlib);
	if (cpus > 1 || (cores > cpus))
	{
		sprintf(cpustr,", %i physical cpu%s",cpus,(cpus == 1) ? "" : "s");
		if (cores)
			sprintf(STREND(cpustr),", %i core%s",cores,(cores == 1) ? "" : "s");
	}
#ifdef DEBUG
	debug("(do_cpuinfo) %s%s%s, loadavg: %.2f(1m) %.2f(5m) %.2f(15m)",
		omni+1,bogostr,cpustr,(loads[0]),(loads[1]),(loads[2]));
#endif
	to_user_q(from,"%s%s%s, loadavg: %.2f(1m) %.2f(5m) %.2f(15m)",
		omni+1,bogostr,cpustr,(loads[0]),(loads[1]),(loads[2]));
}

void do_filemon(COMMAND_ARGS)
{
	struct	stat st;

#ifdef DEBUG
	debug("(do_filemon) rest = '%s'\n",rest);
#endif

	if (stat(rest,&st) < 0 || monitor_fs(rest) < 0)
	{
		to_user_q(from,"Unable to add file monitor for: ``%s''",rest);
		return;
	}
	to_user_q(from,"File monitor added.");
}

#endif /* HOSTINFO */

