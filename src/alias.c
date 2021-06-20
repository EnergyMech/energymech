/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2020 proton

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
#define ALIAS_C
#include "config.h"
#ifdef ALIAS
#include "defines.h"
#include "structs.h"
#ifdef TEST
#define MAIN_C
#endif
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#ifdef TEST

#include "debug.c"

char result[MSGLEN];
char *input = "alias one two three four five   six seven eight nine ten";

void *Calloc(int size)
{
	return(calloc(1,size));
}

void testcase(const char *test, const char *expect)
{
	afmt(result,test,input);
	if (strcmp(result,expect) == 0) debug("testcase SUCCESS: test \"%s\" -> got \"%s\"\n",test,expect);
	else debug("testcase FAIL: test \"%s\", expected \"%s\", got \"%s\"\n",test,expect,result);
}

int main(int argc, char **argv, char **envp)
{
	char *format = argv[1];

	dodebug = 1;
	stringcpy(CurrentNick,"noob");
	if (format == NULL)
	{
		/* testcases */
		testcase("cmd","cmd");
		testcase("cmd cmd","cmd cmd");
		testcase("cmd $$","cmd $");
		testcase("cmd $$0","cmd $0");
		testcase("cmd $$$0","cmd $alias");
		testcase("cmd $$$0$$","cmd $alias$");
		testcase("cmd $1","cmd one");
		testcase("cmd $0","cmd alias");
		testcase("cmd $1-2","cmd one two");
		testcase("cmd $1 $2","cmd one two");
		testcase("cmd $2 $1","cmd two one");
		testcase("cmd $8-","cmd eight nine ten");
		testcase("cmd $4-5","cmd four five");
		testcase("cmd $5-6","cmd five   six");
		testcase("cmd $~","cmd noob");
		testcase("cmd $one $two","cmd $one $two");
		exit(0);
	}
	debug("input = %s\n",input);
	debug("format = %s\n",format);
	afmt(result,format,input);
	debug("result = %s\n",result);
	exit(0);
}

#endif /* TEST */

/*
 *   copy_to = buffer to put resulting new command into
 *   src = Alias format string
 *   input = input from user
 */
void afmt(char *copy_to, const char *src, const char *input)
{
#define BUFTAIL	(copy_to+MSGLEN-1)		/* avoid buffer overflows */
	const char *argstart,*argend;
	char	*dest;
	int	startnum,endnum,spc;

	dest = copy_to;
	while(*src)
	{
		if (src[0] == '$' && src[1] == '$')
			src++;
		else
		if (*src == '$' && src[1] == '~')
		{
			src += 2;
			argstart = CurrentNick;
			while(*argstart && dest <= BUFTAIL)
				*(dest++) = *(argstart++);
		}
		else
		if (*src == '$' && (attrtab[(uchar)src[1]] & NUM) == NUM)
		{
			src++;
			startnum = endnum = 0;
			while(attrtab[(uchar)*src] & NUM)
				startnum = (startnum * 10) + (*(src++) - '0');
			if (*src == ' ' || *src == 0)
				endnum = startnum;
			else
			if (*src == '-')
			{
				src++;
				if ((attrtab[(uchar)*src] & NUM) != NUM)
					endnum = 9999;
				else
				while(attrtab[(uchar)*src] & NUM)
					endnum = (endnum * 10) + (*src++ - '0');
			}

			argstart = input;
			if (startnum)
			for(spc=0;*argstart;argstart++)
			{
				if (*argstart == ' ')
				{
					while(*argstart == ' ') /* skip multiple spaces */
						argstart++;
					if (++spc >= startnum)
						break;
				}
			}

			for(argend=argstart,spc=startnum;*argend;argend++)
			{
				if (*argend == ' ')
				{
					if (++spc > endnum)
						break;
					while(*argend == ' ') /* skip multiple spaces */
						argend++;
				}
			}
#ifdef DEBUG
#ifndef TEST
			debug("(afmt) args #%i-#%i, characters %i-%i\n",startnum,endnum,argstart-input,argend-input);
#endif /* ifndef TEST */
#endif /* DEBUG */
			while(*argstart && argstart < argend && dest <= BUFTAIL)
				*(dest++) = *(argstart++);
			continue;
		}
		if (dest >= BUFTAIL)
			break;
		*(dest++) = *(src++);
	}
	*dest = 0;
#ifdef DEBUG
#ifndef TEST
	debug("(afmt) start %i end %i spc %i\n",startnum,endnum,spc);
#endif /* ifndef TEST */
#endif /* DEBUG */
}

#ifndef TEST

/*
 *
 *  associated commands
 *
 */
/*
help:ALIAS
usage:ALIAS <newcommand> <command [arguments ...]>
file:../help/ALIAS
begin:

Create or replace a command alias.

Arguments in the form $#, $#- and $#-# will
be replaced with the corresponding argument from input.

$0     The zeroeth argument (the aliased command).
$1     The first argument.
$1-    All arguments starting with the first.
$3-    All arguments starting with the third.
$1-2   The first and second argument.
$3-5   The third, fourth and fifth argument.
$$     A literal $ character.
$~     Nickname of the user doing the command.

Example:
ALIAS MEEP SAY $2 $1 $3-

Doing ``MEEP one two three four''
Would become ``SAY two one three four''

Example 2:
ALIAS MEEP SAY #home $~ did $0

Doing ``MEEP one two three four''
Would become ``SAY #home nickname did MEEP''

Aliases may call other aliases and aliases can be used
to replace built in commands. Aliases can recurse a
maximum of 20 times (to prevent infinite loops).

See also: unalias
:end
*/
void do_alias(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	Alias	*alias;
	char	*cmd;

	if (!*rest)
	{
		/* list all aliases */
		if (!aliaslist)
			to_user(from,TEXT_NOALIASES);
		else
		{
			if (partyline_only_command(from))
				return;
			to_user(from,"\037Alias\037              \037Format\037");
			for(alias=aliaslist;alias;alias=alias->next)
			{
				to_user(from,"%-18s %s",alias->alias,alias->format);
			}
		}
		return;
	}

	cmd = chop(&rest);
	if (!*rest)
	{
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}
	for(alias=aliaslist;alias;alias=alias->next)
	{
		if (!stringcasecmp(alias->alias,cmd))
		{
			Free(&alias->format);
			set_mallocdoer(do_alias);
			alias->format = stringdup(rest);
			to_user(from,"Replaced alias: %s --> %s",cmd,rest);
#ifdef DEBUG
			debug("(do_alias) Replaced alias: %s --> %s\n",cmd,rest);
#endif
			return;
		}
	}
	set_mallocdoer(do_alias);
	alias = (Alias*)Calloc(sizeof(Alias)+strlen(cmd));
	stringcpy(alias->alias,cmd);
	set_mallocdoer(do_alias);
	alias->format = stringdup(rest);
	alias->next = aliaslist;
	aliaslist = alias;
	to_user(from,"Added alias: %s --> %s",cmd,rest);
#ifdef DEBUG
	debug("(do_alias) Added alias: %s --> %s\n",cmd,rest);
#endif
}

/*
help:UNALIAS
usage:UNALIAS <alias>
file:../help/UNALIAS
begin:

Remove an existing alias.

See also: alias
:end
*/
void do_unalias(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS + CARGS
	 */
	Alias	*alias,**ap;

	for(ap=&aliaslist;*ap;ap=&(*ap)->next)
	{
		if (!stringcasecmp(rest,(*ap)->alias))
		{
			alias = *ap;
			*ap = alias->next;
			to_user(from,"Removed alias: %s (--> %s)",alias->alias,alias->format);
			Free(&alias->format);
			Free((void*)&alias);
			return;
		}
	}
	to_user(from,"Couldnt find matching alias");
}

#endif /* not TEST */
#endif /* ALIAS */
