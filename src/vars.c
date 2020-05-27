/*

    EnergyMech, IRC bot software
    Parts Copyright (c) 1997-2009 proton

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
#define VARS_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"
#include "settings.h"

void set_str_varc(Chan *channel, int which, char *value)
{
	char	*temp,**dst;

	if (value && *value)
	{
		set_mallocdoer(set_str_varc);
		temp = stringdup(value);
	}
	else
		temp = NULL;
	dst = (channel) ? (char**)&channel->setting[which] : (char**)&current->setting[which];
	if (*dst)
		Free(dst);
	*dst = temp;
}

/*
 *  The rest
 */
int find_setting(const char *name)
{
	int	i;

	for(i=0;VarName[i].name;i++)
	{
		if (!stringcasecmp(name,VarName[i].name))
			return(i);
	}
	return(-1);
}

void copy_vars(UniVar *dst, UniVar *src)
{
	int	i;

	for(i=0;i<CHANSET_SIZE;i++)
	{
		if (IsStr(i))
		{
			if (src[i].str_var)
			{
				set_mallocdoer(copy_vars);
				dst[i].str_var = stringdup(src[i].str_var);
			}
		}
		else
		{
			dst[i].int_var = src[i].int_var;
		}
	}
}

void set_binarydefault(UniVar *dst)
{
	int	i;

	for(i=0;VarName[i].name;i++)
		dst[i].str_var = VarName[i].v.str;
}

void delete_vars(UniVar *vars, int which)
{
	while(--which)
	{
		if (IsStr(which) && !IsProc(which))
		{
#ifdef DEBUG
			debug("(delete_vars) deleting string var `%s'\n",VarName[which].name);
#endif /* DEBUG */
			Free((char**)&vars[which].str_var);
		}
	}
#ifdef DEBUG
	debug("(delete_vars) all done\n");
#endif /* DEBUG */
}

void var_resolve_host(const struct Setting *varname)
{
}

/*
 *  support functions for esay
 */
char	*ec_end;
char	*ec_dest;
char	*ec_src;

void nobo_strcpy(const char *src)
{
	while(*src && ec_dest < ec_end)
		*(ec_dest++) = *(src++);
	*ec_dest = 0;
}

void ec_access(char *from, const char *to)
{
	char	num[20];

	sprintf(num,"%i",get_useraccess(from,to));
	nobo_strcpy(num);
}

void ec_capabilities(char *from, const char *to)
{
	nobo_strcpy(__mx_opts);
}

void ec_cc(char *from, const char *to)
{
	nobo_strcpy((current->activechan) ? current->activechan->name : TEXT_NONE);
}

void ec_channels(char *from, const char *to)
{
	Chan	*chan;
	int	n;

	if ((chan = current->chanlist) == NULL)
	{
		nobo_strcpy(ERR_NOCHANNELS);
		return;
	}
	for(n=0;chan;chan=chan->next)
	{
		if (chan->active)
		{
			if (n && ec_dest < ec_end)
				*(ec_dest++) = ' ';
			if (chan->bot_is_op && ec_dest < ec_end)
				*(ec_dest++) = '@';
			nobo_strcpy(chan->name);
		}
	}
}

void ec_time(char *from, const char *to)
{
	nobo_strcpy(time2away(now));
}

void ec_set(char *from, const char *to)
{
	Chan	*chan;
	UniVar	*varval;
	char	num[20];
	char	*src;
	int	which,uaccess;

	src = ec_src;
	while(*src && *src != ')')
		src++;
	if (*src != ')')
		return;
	*(src++) = 0;

	if ((which = find_setting(ec_src)) >= 0)
	{
		if (which >= CHANSET_SIZE)
		{
			uaccess = get_useraccess(from,ANY_CHANNEL);
			varval = &current->setting[which];
		}
		else
		if ((chan = find_channel_ny(to)))
		{
			uaccess = get_useraccess(from,to);
			varval = &chan->setting[which];
		}
		else
		{
			nobo_strcpy("(unknown channel)");
			return;
		}
		if (uaccess < VarName[which].uaccess)
			return;

		if (IsProc(which))
			varval = varval->proc_var;

		if (IsChar(which))
		{
			num[0] = varval->char_var;
			num[1] = 0;
			nobo_strcpy(num);
		}
		else
		if (IsInt(which))
		{
			sprintf(num,"%i",varval->int_var);
			nobo_strcpy(num);
		}
		else
		if (IsTog(which))
		{
			nobo_strcpy((varval->int_var) ? __STR_ON : __STR_OFF);
		}
		else
		if (IsStr(which))
		{
			nobo_strcpy(nullstr(varval->str_var));
		}
	}
	else
	{
		nobo_strcpy("(unknown setting)");
	}
	ec_src = src;
}

void ec_on(char *from, const char *to)
{
	nobo_strcpy(idle2str(now - current->ontime,FALSE));
}

void ec_server(char *from, const char *to)
{
	Server	*sv;
	char	*s;

	if ((sv = find_server(current->server)))
		s = (*sv->realname) ? sv->realname : sv->name;
	else
		s = TEXT_NOTINSERVLIST;
	nobo_strcpy(s);
}

void ec_up(char *from, const char *to)
{
	nobo_strcpy(idle2str(now - uptime,FALSE));
}

void ec_ver(char *from, const char *to)
{
	nobo_strcpy(BOTCLASS);
	nobo_strcpy(" ");
	nobo_strcpy(VERSION);
}

void ec_guid(char *from, const char *to)
{
	char	tmp[32];

	sprintf(tmp,"%i",current->guid);
	nobo_strcpy(tmp);
}

LS const struct
{
	void	(*func)(char *, const char *);
	char	name[12];
	char	len;

} ecmd[] =
{
	{ ec_access,		"$access",	7	},
	{ ec_capabilities,	"$cap",		4	},
	{ ec_cc,		"$cc",		3	},
	{ ec_channels,		"$channels",	9	},
	{ ec_time,		"$time",	5	},
	{ ec_set,		"$var(",	5	},
	{ ec_on,		"$on",		3	},
	{ ec_server,		"$server",	7	},
	{ ec_up,		"$up",		3	},
	{ ec_ver,		"$ver",		4	},
	{ ec_guid,		"$guid",	5	},
	{ NULL,			"",		0	},
};

/*
 *
 *  commands for variables
 *
 */
void do_esay(COMMAND_ARGS)
{
	/*
	 *  on_msg checks CAXS + CARGS
	 */
	char	output[MSGLEN];
	char	c,*chp;
	int	i,n;

	ec_end = output + MSGLEN - 50;
	ec_src = rest;
	rest = STREND(rest);
	ec_dest = output;
	c = 0;
	chp = NULL;

	while(*ec_src)
	{
		if (*ec_src != '$')
		{
			if (ec_dest < ec_end)
				*(ec_dest++) = *(ec_src);
			ec_src++;
			continue;
		}
		for(i=0;ecmd[i].len;i++)
		{
			if ((rest - ec_src) >= ecmd[i].len)
			{
				chp = ec_src + ecmd[i].len;
				c = *chp;
				*chp = 0;
			}
			n = stringcasecmp(ecmd[i].name,ec_src);
			if (c)
			{
				*chp = c;
				c = 0;
			}
			if (!n)
			{
				ec_src += ecmd[i].len;
				ecmd[i].func(from,to);
				break;
			}
		}
		if (!ecmd[i].len)
		{
			if (ec_dest < ec_end)
				*(ec_dest++) = *(ec_src);
			ec_src++;
		}
	}
	*ec_dest = 0;
	to_user_q(from,FMT_PLAIN,output);
}

void do_set(COMMAND_ARGS)
{
	/*
	 *  on_msg checks:
	 */
	Chan	*chan;
	UniVar	*univar,*varval;
	char	tmp[MSGLEN];
	char	*pp,*name;
	const char *channel;
	int	n,which,i,sz,limit,uaccess;

	/*
	 *
	 */
	channel = get_channel2(to,&rest);
	chan = find_channel_ny(channel);
	name = chop(&rest);

	/*
	 *  empty args, its "set" or "set #channel"
	 */
	if (!name)
	{
		if (!chan)
		{
			to_user(from,ERR_CHAN,channel);
			return;
		}

		if (!CurrentDCC)
			return;

		i = CHANSET_SIZE;
		limit = SIZE_VARS - 1;
		univar = current->setting;
		*tmp = 0;
		if ((uaccess = get_useraccess(from,GLOBAL_CHANNEL)))
			to_user(from,str_underline("Global settings"));
second_pass:
		for(;i<limit;i++)
		{
			if (uaccess < VarName[i].uaccess)
				continue;

			varval = (IsProc(i)) ? current->setting[i].proc_var : &univar[i];

			sz = Strlen2(tmp,VarName[i].name); /* VarName[i].name is never NULL */

			if (IsStr(i))
			{
				sz += (varval->str_var) ? strlen(varval->str_var) : 7;
			}

			if (sz > 58)
			{
				to_user(from,FMT_PLAIN,tmp);
				*tmp = 0;
			}

			if (IsInt(i))
			{
				pp = tolowercat(tmp,VarName[i].name);
				sprintf(pp,(IsChar(i)) ? "=`%c' " : "=%i ",varval->int_var);
			}
			else
			if (IsStr(i))
			{
				pp = tolowercat(tmp,VarName[i].name);
				sprintf(pp,(varval->str_var) ? "=\"%s\" " : "=(unset) ",varval->str_var);
			}
			else
			if (IsTog(i))
			{
				pp = stringcat(tmp,(varval->int_var) ? "+" : "-");
				pp = tolowercat(pp,VarName[i].name);
				pp[0] = ' ';
				pp[1] = 0;
			}
		}
		if (*tmp && tmp[1])
			to_user(from,FMT_PLAIN,tmp);

		if (limit != CHANSET_SIZE)
		{
			to_user(from,"\037Channel settings: %s\037",(chan) ? chan->name : rest);
			i = 0;
			limit = CHANSET_SIZE;
			univar = chan->setting;
			*tmp = 0;
			uaccess = get_useraccess(from,(chan) ? chan->name : rest);
			goto second_pass;
		}
		return;
	}

	/*
	 *  alter a setting
	 */
	if ((which = find_setting(name)) == -1)
	{
set_usage:
		usage(from);	/* usage for CurrentCmd->name */
		return;
	}

	if ((which < CHANSET_SIZE) && *channel != '*')
	{
		if (!chan)
		{
			to_user(from,ERR_CHAN,channel);
			return;
		}
		/*
		 *  its a channel setting
		 */
		channel = chan->name;
		varval = &chan->setting[which];
	}
	else
	{
		/*
		 *  its a global setting
		 */
		channel = MATCH_ALL;
		varval = &current->setting[which];
	}

	if (VarName[which].uaccess > get_authaccess(from,channel))
		return;

	/*
	 *  Check each type and process `rest' if needed.
	 */
	n = 0;
	if (IsChar(which))
	{
		if (rest[1])
			goto set_usage;
	}
	else
	if (IsNum(which))
	{
		if (IsTog(which))
		{
			if (!stringcasecmp(rest,__STR_ON))
			{
				n = 1;
				goto num_data_ok;
			}
			else
			if (!stringcasecmp(rest,__STR_OFF))
			{
				/* n is 0 by default */
				goto num_data_ok;
			}
		}
		n = asc2int((rest = chop(&rest)));
		if (errno || n < VarName[which].min || n > VarName[which].max)
		{
			to_user(from,"Possible values are %i through %i",VarName[which].min,VarName[which].max);
			return;
		}
	}
num_data_ok:
	/*
	 *
	 */
	if ((which < CHANSET_SIZE) && *channel == '*')
	{
		for(chan=current->chanlist;chan;chan=chan->next)
		{
			if (IsNum(which))
			{
				chan->setting[which].int_var = n;
			}
			else
			if (IsStr(which))
			{
				Free((char**)&chan->setting[which].str_var);
				if (*rest)
				{
					set_mallocdoer(do_set);
					chan->setting[which].str_var = stringdup(rest);
				}
			}
		}
		channel = "(all channels)";
	}
	else
	{
		if (IsProc(which))
			varval = varval->proc_var;

		if (IsChar(which))
			varval->char_var = *rest;
		else
		if (IsNum(which))
			varval->int_var = n;
		else
		{
			if (varval->str_var)
				Free((char**)&varval->str_var);
			if (*rest)
			{
				set_mallocdoer(do_set);
				varval->str_var = stringdup(rest);
			}
		}
	}
	to_user(from,"Var: %s   On: %s   Set to: %s",VarName[which].name,
		(which >= CHANSET_SIZE) ? "(global)" : channel,(*rest) ? rest : NULLSTR);
	if (VarName[which].func)
		VarName[which].func(&VarName[which]);
}
