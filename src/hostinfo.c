/*

    EnergyMech, IRC bot software
    Copyright (c) 2018 proton

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

char	omni[224]; // 32*7
#define vmpeak	&omni[0]
#define vmsize	&omni[32]
#define vmrss	&omni[64]
#define vmdata	&omni[96]
#define vmstk	&omni[128]
#define vmexe	&omni[160]
#define vmlib	&omni[192]
/*
char	vmpeak[32];
char	vmsize[32];
char	vmrss[32];
char	vmdata[32];
char	vmstk[32];
char	vmexe[32];
char	vmlib[32];
*/

struct // statusvalues
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

char *cpufrom;
int sentmodel;
int physid,cpus,cores,addsiblings;

/*
proton@endemic:~/energymech/src> cat /proc/loadavg
0.00 0.00 0.00 1/178 6759
processor       : 0
vendor_id       : GenuineIntel
cpu family      : 6
model           : 23
model name      : Intel(R) Core(TM)2 Quad  CPU   Q8200  @ 2.33GHz
stepping        : 7
microcode       : 0x705
cpu MHz         : 2024.267
cache size      : 2048 KB
physical id     : 0
siblings        : 4
core id         : 0
cpu cores       : 4
*/

int parse_proc_cpuinfo(char *line)
{
	char	*src,*dst;
	int	spc;

	if (sentmodel == 0 && strncmp(line,"model name\t: ",13) == 0)
	{
		spc = 0;
		src = line+13;
		dst = omni;
		while(*src)
		{
			if (*src == ' ')
			{
				if (spc++)
				{
					src++;
					continue;
				}
			}
			else
				spc = 0;
			*dst++ = *src++;
		}
		*dst = 0;
#ifdef DEBUG
		debug("(parse_proc_cpuinfo) model name = %s\n",omni);
#endif
		to_user_q(cpufrom,"Cpu: %s\n",omni);
		sentmodel++;
	}
	else
	if (strncmp(line,"physical id\t: ",14) == 0)
	{
		spc = asc2int(line+14);
		if (errno == 0 && spc != physid)
		{
			cpus++;
			addsiblings = 1;
			physid = spc;
		}
	}
	else
	if (addsiblings == 1 && strncmp(line,"siblings\t: ",11) == 0)
	{
		spc = asc2int(line+11);
		addsiblings = 0;
		cores += spc;
	}
}

/*
help:HOSTINFO:(no arguments)

Equivalent to ``uname -orm''

See also: meminfo, cpuinfo
*/
void do_hostinfo(COMMAND_ARGS)
{
	struct utsname un;

	if (uname(&un) == 0)
		to_user_q(from,"%s %s %s",un.sysname,un.release,un.machine);
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
	readline(fd,&parse_proc_status);	// readline closes fd

	to_user_q(from,"VM %s (Max %s), RSS %s [ Code %s, Data %s, Libs %s, Stack %s ]",
		vmsize,vmpeak,vmrss,vmexe,vmdata,vmlib,vmstk);
}

/*
help:CPUINFO:(no arguments)

See also: hostinfo, meminfo
*/
void do_cpuinfo(COMMAND_ARGS)
{
	char	*a1,*a2,*a3;
	int	fd,n;

	if ((fd = open("/proc/cpuinfo",O_RDONLY)) < 0)
#ifdef DEBUG
	{
		debug("(do_cpuinfo) /proc/cpuinfo: %s\n",strerror(errno));
		return;
	}
#else
		return;
#endif

	debug("%s\n",from);
	cpufrom = from;
	sentmodel = 0;
	physid = -1;
	cpus = cores = addsiblings = 0;
	readline(fd,&parse_proc_cpuinfo); // readline closes fd

	if ((fd = open("/proc/loadavg",O_RDONLY)) < 0)
#ifdef DEBUG
	{
		debug("(do_cpuinfo) /proc/loadavg: %s\n",strerror(errno));
		return;
	}
#else
		return;
#endif
	n = read(fd,gsockdata,MSGLEN-2);
	gsockdata[n] = 0;
	close(fd);

	rest = gsockdata;
	a1 = chop(&rest);
	a2 = chop(&rest);
	a3 = chop(&rest);

	if (!a3 || !*a3)
		return;

	to_user_q(from,"Load: %s(1m) %s(5m) %s(15m) [%i physical cpu%s, %i core%s]\n",
		a1,a2,a3,cpus,(cpus == 1) ? "" : "s",cores,(cores == 1) ? "" : "s");
#ifdef DEBUG
	debug("(do_cpuinfo) Load: %s(1m) %s(5m) %s(15m) [%i physical cpu%s, %i core%s]\n",
		a1,a2,a3,cpus,(cpus == 1) ? "" : "s",cores,(cores == 1) ? "" : "s");
#endif
}

#endif /* HOSTINFO */
