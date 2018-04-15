/*

    EnergyMech, IRC bot software
    Copyright (c) 2001-2009 proton

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
#define TCL_C
#include "config.h"

#ifdef TCL

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#include <tcl.h>

LS Tcl_Interp *energymech_tcl = NULL;

#define	tclv_READ	TCL_TRACE_READS
#define tclv_WRITE	TCL_TRACE_WRITES
#define tclv_RDWR	(tclv_READ|tclv_WRITE)

#define TVINFO_UNTYPED	0x0ffff
#define	TVINFO_INT	0x10000
#define	TVINFO_CHAR	0x20000

enum {
	TVINFO_pointer,
	TVINFO_guid,
	TVINFO_nick,
	TVINFO_wantnick,
	TVINFO_server,
	TVINFO_nextserver,
	TVINFO_currentchan,
};

typedef struct Tcl_TVInfo
{
	int	type;
	int	flags;
	char	*name;
	const char *data;
	Tcl_Obj	*n1;

} Tcl_TVInfo;

LS Tcl_TVInfo vinfolist[] =
{
{ TVINFO_pointer	| TVINFO_CHAR,	tclv_READ,	"mech_currentnick",	CurrentNick	},
{ TVINFO_guid		| TVINFO_INT,	tclv_READ,	"mech_guid"				},
{ TVINFO_nick		| TVINFO_CHAR,	tclv_READ,	"mech_nick"				},
{ TVINFO_wantnick	| TVINFO_CHAR,	tclv_RDWR,	"mech_wantnick"				},
{ TVINFO_server		| TVINFO_INT,	tclv_READ,	"mech_server"				},
{ TVINFO_nextserver	| TVINFO_INT,	tclv_RDWR,	"mech_nextserver"			},
{ TVINFO_currentchan	| TVINFO_CHAR,	tclv_READ,	"mech_currentchan"			},
{ TVINFO_pointer	| TVINFO_CHAR,	tclv_READ,	"mech_version",		VERSION		},
{ TVINFO_pointer	| TVINFO_CHAR,	tclv_READ,	"mech_srcdate",		SRCDATE		},
{ TVINFO_pointer	| TVINFO_CHAR,	tclv_READ,	"mech_class",		BOTCLASS	},
{ 0, }};

/*
 *
 *
 *
 */

char *tcl_var_read(Tcl_TVInfo *vinfo, Tcl_Interp *I, char *n1, char *n2, int flags)
{
	Tcl_Obj	*obj;
	union {
		int i;
		const char *c;
	} rdata;

	switch(vinfo->type & TVINFO_UNTYPED)
	{
	case TVINFO_pointer:
		rdata.c = vinfo->data;
		break;
	case TVINFO_guid:
		rdata.i = (current) ? current->guid : -1;
		break;
	case TVINFO_nick:
		rdata.c = (current) ? current->nick : "(undefined variable)";
		break;
	case TVINFO_wantnick:
		rdata.c = (current) ? current->wantnick : "(undefined variable)";
		break;
	case TVINFO_server:
		rdata.i = (current) ? current->server : -1;
		break;
	case TVINFO_nextserver:
		rdata.i = (current) ? current->nextserver : -1;
		break;
	case TVINFO_currentchan:
		rdata.c = (current && current->activechan) ? current->activechan->name : "(undefined variable)";
		break;
	default:
		return("(undefined variable)");
	}

	if (vinfo->type & TVINFO_INT)
	{
		obj = Tcl_NewIntObj(rdata.i);
	}
	else
	/* if (vinfo->type & TVINFO_CHAR) */
	{
		obj = Tcl_NewStringObj((char*)rdata.c,strlen(rdata.c));
	}

	Tcl_ObjSetVar2(energymech_tcl,vinfo->n1,NULL,obj,TCL_GLOBAL_ONLY);

	return(NULL);
}

char *tcl_var_write(Tcl_TVInfo *vinfo, Tcl_Interp *I, char *n1, char *n2, int flags)
{
	return("not yet implemented");
}

/*
 *
 *
 *
 */

int tcl_timer_jump(Hook *hook)
{
	return(0);
}

int tcl_parse_jump(char *from, char *rest, Hook *hook)
{
	Tcl_Obj	*tcl_result;
	int	i;

#ifdef DEBUG
	debug("(tcl_parse_jump) %s %s %s\n",
		nullstr(hook->self),nullstr(from),nullstr(rest));
#endif /* DEBUG */

	if (from)
		nickcpy(CurrentNick,from);
	else
		*CurrentNick = 0;

	Tcl_SetVar(energymech_tcl,"_from",from,0);
	Tcl_SetVar(energymech_tcl,"_rest",rest,0);

	i = 0;
	if (Tcl_VarEval(energymech_tcl,hook->self," $_from $_rest",NULL) == TCL_OK)
	{
		tcl_result = Tcl_GetObjResult(energymech_tcl);
		Tcl_GetIntFromObj(energymech_tcl,tcl_result,&i);
	}
#ifdef DEBUG
	debug("(tcl_parse_jump) result = %i\n",i);
#endif /* DEBUG */
	return(i);
}

#ifdef DCC_FILE

void tcl_dcc_complete(Client *client, int cps)
{
	Tcl_Obj	*obj;
	Tcl_Obj	*vname;
	Hook	*hook;

	vname = Tcl_NewStringObj("_cps",3);
	for(hook=hooklist;hook;hook=hook->next)
	{
		if (hook->flags == MEV_DCC_COMPLETE &&
			hook->guid && current && hook->guid == current->guid)
		{
			Tcl_SetVar(energymech_tcl,"_filetarget",client->whom,0);
			Tcl_SetVar(energymech_tcl,"_filename",client->filename,0);
			obj = Tcl_NewIntObj(cps);
			Tcl_ObjSetVar2(energymech_tcl,vname,NULL,obj,TCL_GLOBAL_ONLY);
			Tcl_VarEval(energymech_tcl,hook->self," $_filetarget $_filename $_cps",NULL);
#ifdef DEBUG
			debug("(tcl_dcc_complete) filetarget %s, filename %s\n",client->whom,client->filename);
#endif /* DEBUG */
		}
	}
}

#endif /* DCC_FILE */

int tcl_hook(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Hook	*hook;
	HookTimer hooktimer;
	char	*type,*command,*self;
	int	guid,mode;
	int	sz1,sz2;

	guid = 0;
	if (objc == 5)
	{
		if (Tcl_GetIntFromObj(energymech_tcl,objv[4],&guid) == TCL_ERROR)
			return(TCL_ERROR);
	}
	else
	if (objc != 4)
	{
		return(TCL_ERROR);
	}

	type	= Tcl_GetStringFromObj(objv[1],&mode);
	command	= Tcl_GetStringFromObj(objv[2],&sz1);
	self	= Tcl_GetStringFromObj(objv[3],&sz2);

	if (!mode || !sz1 || !sz2)
		return(TCL_ERROR);

	if (!stringcasecmp(type,"command"))
		mode = MEV_COMMAND;
	else
	if (!stringcasecmp(type,"dcc_complete"))
		mode = MEV_DCC_COMPLETE;
	else
	if (!stringcasecmp(type,"parse"))
		mode = MEV_PARSE;
	else
	if (!stringcasecmp(type,"timer"))
	{
		if (compile_timer(&hooktimer,command) < 0)
			return(TCL_ERROR);
		mode = MEV_TIMER;
		sz1  = sizeof(HookTimer);
	}
	else
	{
		return(TCL_ERROR);
	}

	set_mallocdoer(tcl_hook);
	hook = (Hook*)Calloc(sizeof(Hook) + sz1 + sz2);
	hook->guid = guid;
	hook->flags = mode;
	hook->next = hooklist;
	hooklist = hook;

	hook->type.any = (void*)(stringcpy(hook->self,self) + 1);

	switch(mode)
	{
	case MEV_COMMAND:
	case MEV_PARSE:
		stringcpy(hook->type.command,command);
		hook->func = tcl_parse_jump;
		break;
	default:
	/* case MEV_TIMER: */
		memcpy(hook->type.timer,&hooktimer,sizeof(HookTimer));
		hook->func = tcl_timer_jump;
		break;
	}

#ifdef DEBUG
	debug("(tcl_hook) hooked %s `%s' --> %s\n",
		nullstr(type),nullstr(command),nullstr(self));
#endif /* DEBUG */

	return(TCL_OK);
}

int tcl_unhook(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	return(TCL_ERROR);
}

int tcl_userlevel(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Obj	*tcl_result;
	char	*nuh,*chan;
	int	n;

	if (!current)
		return(TCL_ERROR);

	chan = NULL;

	if (objc == 3)
	{
		if ((chan = Tcl_GetStringFromObj(objv[2],NULL)) == NULL)
			return(TCL_ERROR);
	}
	else
	if (objc != 2)
		return(TCL_ERROR);

	if ((nuh = Tcl_GetStringFromObj(objv[1],NULL)) == NULL)
		return(TCL_ERROR);

	n = get_useraccess(nuh,chan);

	tcl_result = Tcl_GetObjResult(energymech_tcl);
	Tcl_SetIntObj(tcl_result,n);
	return(TCL_OK);
}

#ifdef DEBUG

int tcl_debug(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	char	*text;

	if (objc != 2)
		return(TCL_ERROR);

	if ((text = Tcl_GetStringFromObj(objv[1],NULL)) == NULL)
		return(TCL_ERROR);

	debug("(tcl_debug) %s\n",text);

	return(TCL_OK);
}

#endif /* DEBUG */

int tcl_to_server(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Strp	*sp,**pp;
	Tcl_Obj	*tcl_result;
	char	*line;
	int	n,sz,sendqmax;

	if (!current)
		return(TCL_ERROR);

	sendqmax = -1;
	if (objc == 3)
	{
		if (Tcl_GetIntFromObj(energymech_tcl,objv[2],&sendqmax) == TCL_ERROR)
			return(TCL_ERROR);
	}
	else
	if (objc != 2)
		return(TCL_ERROR);

	if ((line = Tcl_GetStringFromObj(objv[1],&sz)) == NULL)
		return(TCL_ERROR);

#ifdef DEBUG
	debug("(tcl_to_server) max = %i; line = %s",sendqmax,line);
#endif /* DEBUG */

	if (sendqmax >= 0)
	{
		n = 0;
		pp = &current->sendq;
		while(*pp)
		{
			n++;
			pp = &(*pp)->next;
		}
		if (sendqmax && n >= sendqmax)
		{
			n = -n;
		}
		else
		if (sz)
		{
			*pp = sp = (Strp*)Calloc(sizeof(Strp) + sz);
			/* Calloc sets to zero sp->next = NULL; */
			stringcpy(sp->p,line);
		}
	}
	else
	{
		if ((n = write(current->sock,line,sz)) < 0)
		{
#ifdef DEBUG
			debug("(tcl_to_server) {%i} errno = %i\n",current->sock,errno);
#endif /* DEBUG */
			close(current->sock);
			current->sock = -1;
			current->connect = CN_NOSOCK;
			return(TCL_ERROR);
		}
		current->sendq_time += 2;
	}
	tcl_result = Tcl_GetObjResult(energymech_tcl);
	Tcl_SetIntObj(tcl_result,n);
	return(TCL_OK);
}

int tcl_to_file(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Obj	*tcl_result;
	char	*text;
	int	fd,sz;
	int	r;

	if (objc != 3)
		return(TCL_ERROR);

	if ((Tcl_GetIntFromObj(energymech_tcl,objv[1],&fd) == TCL_ERROR)
		|| ((text = Tcl_GetStringFromObj(objv[2],&sz)) == NULL))
		return(TCL_ERROR);

	r = write(fd,text,sz);

	tcl_result = Tcl_GetObjResult(energymech_tcl);
	Tcl_SetIntObj(tcl_result,r);
	return(TCL_OK);
}

#ifdef DCC_FILE

int tcl_dcc_sendfile(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Obj	*tcl_result;
	char	*filename,*target;
	int	sz;

	if (objc != 3)
		return(TCL_ERROR);
	if ((target = Tcl_GetStringFromObj(objv[1],NULL)) == NULL)
		return(TCL_ERROR);
	if ((filename = Tcl_GetStringFromObj(objv[2],NULL)) == NULL)
		return(TCL_ERROR);

	if ((sz = dcc_sendfile(target,filename)) < 0)
		return(TCL_ERROR);

	tcl_result = Tcl_GetObjResult(energymech_tcl);
	Tcl_SetIntObj(tcl_result,sz);
	return(TCL_OK);
}

#endif /* DCC_FILE */

#ifdef RAWDNS

int tcl_dns_jump(char *host, char *resolved, Hook *hook)
{
	Tcl_Obj	*tcl_result;
	int	i;

#ifdef DEBUG
	debug("(tcl_dns_jump) %s %s %s\n",
		nullstr(hook->self),nullstr(host),nullstr(resolved));
#endif /* DEBUG */

	Tcl_SetVar(energymech_tcl,"_host",host,0);
	Tcl_SetVar(energymech_tcl,"_resolved",resolved,0);

	i = 0;
	if (Tcl_VarEval(energymech_tcl,hook->self," $_host $_resolved",NULL) == TCL_OK)
	{
		tcl_result = Tcl_GetObjResult(energymech_tcl);
		Tcl_GetIntFromObj(energymech_tcl,tcl_result,&i);
	}
#ifdef DEBUG
	debug("(tcl_dns_jump) result = %i\n",i);
#endif /* DEBUG */
	return(i);
}

int tcl_dns(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[])
{
	Tcl_Obj	*tcl_result;
	Hook	*hook;
	char	*host,*callback;

	if (objc != 3)
		return(TCL_ERROR);
	if ((host = Tcl_GetStringFromObj(objv[1],NULL)) == NULL)
		return(TCL_ERROR);
	if ((callback = Tcl_GetStringFromObj(objv[2],NULL)) == NULL)
		return(TCL_ERROR);

#ifdef DEBUG
	debug("(tcl_dns) resolving: %s (callback %s)\n",host,callback);
#endif /* DEBUG */

	set_mallocdoer(tcl_dns);
	hook = (Hook*)Calloc(sizeof(Hook) + strlen(host));
	hook->guid = (current) ? current->guid : 0;
	hook->flags = MEV_DNSRESULT;
	hook->next = hooklist;
	hooklist = hook;
	hook->type.host = stringcpy(hook->self,callback) + 1;
	stringcpy(hook->type.host,host);
	hook->func = tcl_dns_jump;

	rawdns(host);
	return(TCL_OK);
}

#endif /* RAWDNS */

/*
 *
 *
 *
 */

LS struct
{
	char	*cmdname;
	void	*func;

} tcl2mech[] =
{
{	"to_server",		tcl_to_server		},
{	"to_file",		tcl_to_file		},
{	"mech_userlevel",	tcl_userlevel		},
{	"mech_hook",		tcl_hook		},
{	"mech_unhook",		tcl_unhook		},
#ifdef DEBUG
{	"mech_debug",		tcl_debug		},
#endif /* DEBUG */
#ifdef DCC_FILE
{	"mech_dcc_sendfile",	tcl_dcc_sendfile	},
#endif /* DCC_FILE */
#ifdef RAWDNS
{	"mech_dns",		tcl_dns			},
#endif /* RAWDNS */
{ NULL, }};

void init_tcl(void)
{
#ifdef DEBUG
	void	*res;
	int	resi;
#endif /* DEBUG */
	int	i;

	if ((energymech_tcl = Tcl_CreateInterp()) == NULL)
		return;

	if (Tcl_Init(energymech_tcl) != TCL_OK)
	{
		Tcl_DeleteInterp(energymech_tcl);
		energymech_tcl = NULL;
		return;
	}

#ifdef DEBUG
	Tcl_SetVar(energymech_tcl,"define_debug","1",0);
#endif /* DEBUG */

	for(i=0;tcl2mech[i].cmdname;i++)
	{
#ifdef DEBUG
		res = 
#endif /* DEBUG */
		Tcl_CreateObjCommand(energymech_tcl,tcl2mech[i].cmdname,tcl2mech[i].func,NULL,NULL);
#ifdef DEBUG
		debug("(init_tcl) create tcl command: %s (%s)\n",tcl2mech[i].cmdname,(res) ? "SUCCESS" : "FAIL");
#endif /* DEBUG */
	}

	/*
	 *  trace list of variables
	 */
	for(i=0;vinfolist[i].name;i++)
	{
		/*
		 *  make the variable name into a tcl object
		 */
		vinfolist[i].n1 = Tcl_NewStringObj(vinfolist[i].name,strlen(vinfolist[i].name));
		Tcl_IncrRefCount(vinfolist[i].n1);
		/*
		 *  trace read ops
		 */
#ifdef DEBUG
		resi =
#endif /* DEBUG */
		Tcl_TraceVar(energymech_tcl,vinfolist[i].name,TCL_TRACE_READS | TCL_GLOBAL_ONLY,
			(Tcl_VarTraceProc*)tcl_var_read,&vinfolist[i]);
#ifdef DEBUG
		debug("(init_tcl) trace tcl variable (read): %s (%s)\n",
			vinfolist[i].name,(resi == TCL_OK) ? "SUCCESS" : "FAIL");
#endif /* DEBUG */
		/*
		 *  trace write ops
		 */
		if ((vinfolist[i].flags & tclv_WRITE) == 0)
			continue;
#ifdef DEBUG
		resi =
#endif /* DEBUG */
		Tcl_TraceVar(energymech_tcl,vinfolist[i].name,TCL_TRACE_WRITES | TCL_GLOBAL_ONLY,
			(Tcl_VarTraceProc*)tcl_var_write,(ClientData)&vinfolist[i]);
#ifdef DEBUG
		debug("(init_tcl) trace tcl variable (write): %s (%s)\n",
			vinfolist[i].name,(resi == TCL_OK) ? "SUCCESS" : "FAIL");
#endif /* DEBUG */
	}
}

/*
 *
 *
 *
 */


void do_tcl(COMMAND_ARGS)
{
	int	res;

	if (!energymech_tcl)
	{
		init_tcl();
		if (!energymech_tcl)
			return;
	}

#ifdef PLEASE_HACK_MY_SHELL
	if (CurrentCmd->name == C_TCL)
	{
		res = Tcl_Eval(energymech_tcl,rest);
		to_user(from,"tcl command %s",(res == TCL_OK) ? "executed ok" : "failed");
	}
	else
#endif /* PLEASE_HACK_MY_SHELL */
	{
		res = Tcl_EvalFile(energymech_tcl,rest);
		to_user(from,"tcl script %s",(res == TCL_OK) ? "loaded ok" : "failed to load");
#ifdef DEBUG
		debug("(do_tcl) tcl script \"%s\" %s\n",rest,(res == TCL_OK) ? "loaded ok" : "failed to load");
#endif /* DEBUG */
	}
}

#endif /* TCL */
