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
#define HOSTINFO_C
#include "config.h"

#ifdef HOSTINFO
#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#include <sys/utsname.h>

void do_hostinfo(COMMAND_ARGS)
{
	struct utsname un;

	if (uname(&un) == 0)
		to_user(from,"%s %s %s",un.sysname,un.release,un.machine);
}

char	vmpeak[32];
char	vmsize[32];
char	vmrss[32];
char	vmdata[32];
char	vmstk[32];
char	vmexe[32];
char	vmlib[32];

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

	to_user(from,"VM %s (Max %s), RSS %s [ Code %s, Data %s, Libs %s, Stack %s ]",
		vmsize,vmpeak,vmrss,vmexe,vmdata,vmlib,vmstk);
}

void do_cpuinfo(COMMAND_ARGS)
{
}

#endif /* HOSTINFO */
