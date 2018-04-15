/*

    EnergyMech, IRC bot software
    Copyright (c) 2009 Nemesis128 <stnsls@gmail.com>

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

/* Python scripting backend for Energymech

by Nemesis128 <stnsls@gmail.com>

*/

#define PYTHON_C
#include "config.h"

#ifdef PYTHON

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#define PYVAR_guid          0
#define PYVAR_currentnick   1
#define PYVAR_botnick       2
#define PYVAR_wantnick      3
#define PYVAR_userhost      4
#define PYVAR_server        5
#define PYVAR_nextserver    6
#define PYVAR_currentchan   7

#if PY_MAJOR_VERSION >= 3

char *python_unicode2char(PyUnicodeObject *obj)
{	/* try to get a valid char* from PyUnicode object.
	   returned str must be free'd afterwards. */
	int sz = PyUnicode_GET_SIZE(obj);

	wchar_t *wcs = (wchar_t*) malloc(sizeof(wchar_t) * sz);
	PyUnicode_AsWideChar(obj, wcs, sz);
	char *cs = (char*) malloc(sizeof(char) * (sz + 1));
	wcstombs(cs, wcs, sz);
	cs[sz] = '\0';
	free(wcs);
	return cs;
}

#endif /* Py3k */

static PyObject *python_error; /* emech exception object */

static PyObject *python_getvar(PyObject *self, PyObject *args)
{	/* get some global var */
	int vartype;

#ifdef DEBUG
	if (!current)
	{
		PyErr_SetString(python_error, "(python_getvar) No current bot?!");
		return NULL;
	}
#endif /* DEBUG */

	if (!PyArg_ParseTuple(args, "i", &vartype))
		return NULL;

	switch(vartype)
	{
	case PYVAR_guid:
		return Py_BuildValue("i", current ? current->guid : -1);
	case PYVAR_currentnick:
		return Py_BuildValue("s", CurrentNick);
	case PYVAR_botnick:
		return Py_BuildValue("s", current && current->nick ? current->nick : "");
	case PYVAR_wantnick:
		return Py_BuildValue("s", current && current->wantnick ? current->wantnick : "");
	case PYVAR_userhost:
		return Py_BuildValue("s", current && current->userhost ? current->userhost : "");
	case PYVAR_server:
		return Py_BuildValue("i", current && current->server ? current->server : -1);
	case PYVAR_nextserver:
		return Py_BuildValue("i", current && current->nextserver ? current->nextserver : -1);
	case PYVAR_currentchan:
		return Py_BuildValue("s", current && current->activechan ? current->activechan->name : "");
	default:
		PyErr_SetString(python_error, "(python_getvar) invalid var");
		return NULL;
	}
}

/* From channel.c */

static PyObject *python_is_chanuser(PyObject *self, PyObject *args, PyObject *keywds)
/* Return True if $nick is on $chan, else False */
{
	char *chan, *nick;
	static char *kwlist[] = {"chan", "nick", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &chan, &nick))
		return NULL;

	Chan *ch = (Chan*) find_channel(chan, CHAN_ANY);

	if (!ch)
		Py_RETURN_FALSE;

	ChanUser *cu = find_chanuser(ch, nick);
	if (!cu)
		Py_RETURN_FALSE;
	Py_RETURN_TRUE;
}

#if 0

static PyObject *python_do_join(PyObject *self, PyObject *args, PyObject *keywds)
/* Join a channel */
{
	char *from, *to="", *rest;
	int cmdaccess = 100;
	static char *kwlist[] = {"from", "rest", "cmdaccess", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|i", kwlist, &from, &rest, &cmdaccess))
		return NULL;

	do_join(from, to, rest, cmdaccess);
	Py_RETURN_NONE;
}

static PyObject *python_do_part(PyObject *self, PyObject *args, PyObject *keywds)
/* Part a channel */
{
	char *from, *to, *rest="";
	int cmdaccess = 100;
	static char *kwlist[] = {"from", "to", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &from, &to))
		return NULL;
	do_part(from, to, rest, cmdaccess);
	Py_RETURN_NONE;
}

static PyObject *python_do_cycle(PyObject *self, PyObject *args, PyObject *keywds)
/* Cycle a channel */
{
	char *from, *to, *rest="";
	int cmdaccess = 100;
	static char *kwlist[] = {"from", "to", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &from, &to))
		return NULL;

	do_cycle(from, to, rest, cmdaccess);
	Py_RETURN_NONE;
}

static PyObject *python_do_wall(PyObject *self, PyObject *args, PyObject *keywds)
/* Wall */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_wall(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_mode(PyObject *self, PyObject *args, PyObject *keywds)
/* Mode */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_mode(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_invite(PyObject *self, PyObject *args, PyObject *keywds)
/* Invite */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_invite(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_sayme(PyObject *self, PyObject *args, PyObject *keywds)
/* Say /me */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_sayme(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_topic(PyObject *self, PyObject *args, PyObject *keywds)
/* Set topic */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_topic(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_idle(PyObject *self, PyObject *args, PyObject *keywds)
/* Check idle time */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_idle(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

#endif /* 0 */

static PyObject *python_find_nuh(PyObject *self, PyObject *args, PyObject *keywds)
/* Find nuh */
{
	char *nick, *nuh;
	static char *kwlist[] = {"nick", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "s", kwlist, &nick))
		return NULL;

	if ((nuh = find_nuh(nick)) == NULL)
		Py_RETURN_NONE;
	else
		return Py_BuildValue("s", nuh);
}

/* From core.c */

#if 0

static PyObject *python_do_die(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *from="", *to="", *rest="";
    int cmdaccess=0;
    static char *kwlist[] = {"from", "rest", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|s", kwlist, &from, &rest))
        return NULL;
    do_die(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_shutdown(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *from="", *to="", *rest="";
    int cmdaccess=0;
    static char *kwlist[] = {"from", "rest", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|s", kwlist, &from, &rest))
        return NULL;
    do_shutdown(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

static PyObject *python_do_server(PyObject *self, PyObject *args, PyObject *keywds)
/* Connect server */
{
    char *from, *to, *rest;
    int cmdaccess = 100;
    static char *kwlist[] = {"from", "to", "rest", "cmdaccess", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sss|i", kwlist,
        &from, &to, &rest, &cmdaccess))
        return NULL;
    do_server(from, to, rest, cmdaccess);
    Py_RETURN_NONE;
}

#endif /* 0 */

PyObject *python_hook(PyObject *self, PyObject *args, PyObject *keywds)
{
	Hook *hook;
	HookTimer hooktimer;
	PyObject *cb, *funcname;
	char *type, *command, *cbname;
	int guid = 0, mode, sz1, sz2;
	static char *kwlist[] = {"type", "command", "callback", "guid", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, keywds, "ssO|i", kwlist, &type, &command, &cb, &guid))
		return NULL;

	/* check callback function */
	if (!PyFunction_Check(cb))
	{
		PyErr_SetString(python_error, "(python_hook) callback is not a function");
		return NULL;
	}
	funcname = PyObject_GetAttrString(cb, "__name__"); /* new ref */
	if (!funcname)
	{
		PyErr_SetString(python_error, "(python_hook) cant get function name");
		return NULL;
	}

#if PY_MAJOR_VERSION >= 3
	cbname = python_unicode2char((PyUnicodeObject*) funcname);
#else
	cbname = PyString_AsString(funcname); /* not a copy! */
#endif /* Py3k */

    mode = strlen(type);
    sz1 = strlen(command);
    sz2 = strlen(cbname);
    if (!mode || !sz1 || !sz2)
    {
        Py_DECREF(funcname);
#if PY_MAJOR_VERSION >= 3
        free(cbname);
#endif /* Py3k */
        PyErr_SetString(python_error, "(python_hook) invalid params");
        return NULL;
    }

	if (!stringcasecmp(type, "command"))
		mode = MEV_COMMAND;
	else
	if (!stringcasecmp(type, "dcc_complete"))
		mode = MEV_DCC_COMPLETE;
	else
	if (!stringcasecmp(type, "parse"))
		mode = MEV_PARSE;
	else
	if (!stringcasecmp(type, "timer"))
	{
		if (compile_timer(&hooktimer, command) < 0)
		{
			Py_DECREF(funcname);
#if PY_MAJOR_VERSION >= 3
			free(cbname);
#endif /* Py3k */
			PyErr_SetString(python_error, "(python_hook) cant compile timer");
			return NULL;
		}
		mode = MEV_TIMER;
		sz1  = sizeof(HookTimer);
	}
	else
	{
		Py_DECREF(funcname);
#if PY_MAJOR_VERSION >= 3
		free(cbname);
#endif /* Py3k */
        PyErr_SetString(python_error, "(python_hook) invalid hook type");
        return NULL;
    }

    set_mallocdoer(python_hook);
    hook = (Hook*) Calloc(sizeof(Hook) + sz1 + sz2);
    hook->guid = guid;
    hook->flags = mode;
    hook->next = hooklist;
    hooklist = hook;

    hook->type.any = (void*) (stringcpy(hook->self, cbname) + 1);

    switch(mode)
    {
    case MEV_COMMAND:
    case MEV_PARSE:
        stringcpy(hook->type.command, command);
        hook->func = python_parse_jump;
        break;
    default:
    /* case MEV_TIMER: */
        memcpy(hook->type.timer, &hooktimer, sizeof(HookTimer));
        hook->func = python_timer_jump;
        break;
    }

#ifdef DEBUG
    debug("(python_hook) hooked %s `%s' --> %s\n",
        nullstr(type), nullstr(command), nullstr(cbname));
#endif /* DEBUG */

    Py_DECREF(funcname);
#if PY_MAJOR_VERSION >= 3
    free(cbname);
#endif /* Py3k */
    Py_RETURN_NONE;
}

PyObject *python_unhook(PyObject *self, PyObject *args, PyObject *keywds)
{
    return Py_BuildValue("i", 1);
}

int python_parse_jump(char *from, char *rest, Hook *hook)
{
    int i;
    PyObject *mainmod, *cb, *res;

#ifdef DEBUG
    debug("(python_parse_jump) %s %s %s\n",
        nullstr(hook->self), nullstr(from), nullstr(rest));
#endif /* DEBUG */

    if (from)
        nickcpy(CurrentNick, from);
    else
        *CurrentNick = 0;

    /* get callback object */
    mainmod = PyImport_AddModule("__main__");
    if (!mainmod)
    {
#ifdef DEBUG
        debug("(python_parse_jump) cant get main module\n");
#endif /* DEBUG */
        return 0; /* python exception has been set */
    }
    cb = PyDict_GetItemString(PyModule_GetDict(mainmod), hook->self);
    if (!PyFunction_Check(cb))
    {
#ifdef DEBUG
        debug("(python_parse_jump) invalid callback function\n");
#endif /* DEBUG */
        return 0;
    }

    /* call our function */
    res = PyObject_CallFunction(cb, "ss", from, rest); /* new ref */
#if PY_MAJOR_VERSION >= 3
    i = PyLong_AsLong(res);
#else
    i = PyInt_AS_LONG(res);
#endif /* Py3k */

#ifdef DEBUG
    debug("(python_parse_jump) result = %d\n", i);
#endif
    Py_DECREF(res);
    return i;
}

int python_timer_jump(Hook *hook)
{
    return 0;
}

#ifdef DCC_FILE

void python_dcc_complete(Client *client, int cps)
{
    PyObject *mainmod = NULL, *cb, *res;
    Hook *hook;

    for (hook = hooklist; hook; hook = hook->next)
    {
        if (hook->flags == MEV_DCC_COMPLETE &&
            hook->guid && current && hook->guid == current->guid)
        {
            /* get callback object */
            if (!mainmod)
                mainmod = PyImport_AddModule("__main__");
            if (!mainmod)
            {
#ifdef DEBUG
                debug("(python_dcc_complete) cant get main module\n");
#endif /* DEBUG */
                return; /* python exception has been set */
            }
            cb = PyDict_GetItemString(PyModule_GetDict(mainmod), hook->self);
            if (!PyFunction_Check(cb))
            {
#ifdef DEBUG
                debug("(python_dcc_complete) invalid callback function\n");
#endif /* DEBUG */
                return;
            }

            /* call our function */
            res = PyObject_CallFunction(cb, "ssi",
                client->whom, client->filename, cps); /* new ref */
#ifdef DEBUG
#if PY_MAJOR_VERSION >= 3
            debug("(python_dcc_complete) result = %d\n", PyLong_AsLong(res));
#else
            debug("(python_dcc_complete) result = %d\n", PyInt_AS_LONG(res));
#endif /* Py3k */
#endif /* DEBUG */
            Py_DECREF(res);
        }
    }
}

#endif /* DCC_FILE */

PyObject *python_userlevel(PyObject *self, PyObject *args, PyObject *keywds)
{
#ifdef DEBUG
    if (!current)
    {
        PyErr_SetString(python_error, "(python_userlevel) No current bot?!");
        return NULL;
    }
#endif /* DEBUG */
    char *nuh, *chan;
    int n;
    static char *kwlist[] = {"nuh", "chan", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|s", kwlist, &nuh, &chan))
        return NULL;
    n = get_useraccess(nuh, chan);
    return Py_BuildValue("i", n);
}

PyObject *python_to_server(PyObject *self, PyObject *args, PyObject *keywds)
{
#ifdef DEBUG
    if (!current)
    {
        PyErr_SetString(python_error, "(python_to_server) No current bot?!");
        return NULL;
    }
#endif /* DEBUG */
    Strp *sp, **pp;
    char *line;
    int sendqmax = -1, n, sz;
    static char *kwlist[] = {"line", "sendqmax", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|i", kwlist, &line, &sendqmax))
        return NULL;
#ifdef DEBUG
    debug("(python_to_server) max = %i; line = %s\n", sendqmax, line);
#endif /* DEBUG */
    sz = strlen(line);
    if (sendqmax >= 0)
    {
        n = 0;
        pp = &current->sendq;
        while (*pp)
        {
            n++;
            pp = &(*pp)->next;
        }
        if (sendqmax && n >= sendqmax) n = -n;
        else
        if (sz)
        {
		make_strp(pp,line);
        }
    }
    else
    {
        if ((n = write(current->sock, line, sz)) < 0)
        {
#ifdef DEBUG
            debug("(python_to_server) {%i} errno = %i\n", current->sock, errno);
#endif /* DEBUG */
            close(current->sock);
            current->sock = -1;
            current->connect = CN_NOSOCK;
            PyErr_SetString(python_error, "Socket error");
            return NULL;
        }
        current->sendq_time += 2;
    }
    return Py_BuildValue("i", n);
}

PyObject *python_to_file(PyObject *self, PyObject *args, PyObject *keywds)
{
    int fd, r;
    char *txt;
    static char *kwlist[] = {"fd", "text", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "is", kwlist, &fd, &txt))
        return NULL;
    r = write(fd, txt, strlen(txt));
    return Py_BuildValue("i", r);
}

#ifdef DCC_FILE

static PyObject *python_dcc_sendfile(PyObject *self, PyObject *args, PyObject *keywds)
{
    char *filename, *target;
    int sz;
    static char *kwlist[] = {"file", "target", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss", kwlist, &filename, &target))
        return NULL;
    if ((sz = dcc_sendfile(target, filename)) < 0)
    {
        PyErr_SetString(python_error, "(python_dcc_sendfile) error!");
        return NULL;
    }
    return Py_BuildValue("i", sz);
}

#endif /* DCC_FILE */

#ifdef DEBUG

PyObject *python_debug(PyObject *self, PyObject *args)
{
    PyObject *o;
    if (!PyArg_ParseTuple(args, "O", &o))
        return NULL;
    o = PyObject_Repr(o);
    if (!o)
    {
        debug("(python_debug) object has no __repr__ method\n");
        Py_RETURN_NONE;
    }
#if PY_MAJOR_VERSION >= 3
    char *cs;
    cs = python_unicode2char((PyUnicodeObject*) o);
    debug("(python_debug) %s\n", cs);
    free(cs);
#else
    debug("(python_debug) %s\n", PyString_AsString(o));
#endif /* Py3k */
    Py_RETURN_NONE;
}

#endif /* DEBUG */

#ifdef RAWDNS

int python_dns_jump(char *host, char *resolved, Hook *hook)
{
    PyObject *mainmod, *cb, *res;
    int i = 0;
#ifdef DEBUG
    debug("(python_dns_jump) %s %s %s\n",
        nullstr(hook->self), nullstr(host), nullstr(resolved));
#endif /* DEBUG */

    /* get callback object */
    mainmod = PyImport_AddModule("__main__");
    if (!mainmod)
    {
#ifdef DEBUG
        debug("(python_dns_jump) cant get main module\n");
#endif /* DEBUG */
        return 0; /* python exception has been set */
    }
    cb = PyDict_GetItemString(PyModule_GetDict(mainmod), hook->self);
    if (!PyFunction_Check(cb))
    {
#ifdef DEBUG
        debug("(python_dns_jump) invalid callback function\n");
#endif /* DEBUG */
        return 0;
    }

    /* call our function */
    res = PyObject_CallFunction(cb, "ss", host, resolved); /* new ref */
#if PY_MAJOR_VERSION >= 3
    i = PyLong_AsLong(res);
#else
    i = PyInt_AS_LONG(res);
#endif /* Py3k */

#ifdef DEBUG
    debug("(python_dns_jump) result = %d\n", i);
#endif /* DEBUG */
    Py_DECREF(res);
    return i;
}

static PyObject *python_dns(PyObject *self, PyObject *args, PyObject *keywds)
{
    PyObject *cb, *funcname;
    Hook    *hook;
    char    *host, *cbname;

    static char *kwlist[] = {"host", "callback", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sO", kwlist, host, cb))
        return NULL;

    /* check callback function */
    if (!PyFunction_Check(cb))
    {
        PyErr_SetString(python_error, "(python_dns) callback is not a function");
        return NULL;
    }
    funcname = PyObject_GetAttrString(cb, "__name__"); /* new ref */
    if (!funcname)
    {
        PyErr_SetString(python_error, "(python_dns) cant get function name");
        return NULL;
    }

#if PY_MAJOR_VERSION >= 3
    cbname = python_unicode2char((PyUnicodeObject*) funcname);
#else
    cbname = PyString_AsString(funcname); /* not a copy! */
#endif /* Py3k */

#ifdef DEBUG
    debug("(python_dns) resolving: %s (callback %s)\n", host, cbname);
#endif /* DEBUG */

    set_mallocdoer(python_dns);
    hook = (Hook*)Calloc(sizeof(Hook) + strlen(host));
    hook->guid = (current) ? current->guid : 0;
    hook->flags = MEV_DNSRESULT;
    hook->next = hooklist;
    hooklist = hook;
    hook->type.host = stringcpy(hook->self, cbname) + 1;
    stringcpy(hook->type.host, host);
    hook->func = python_dns_jump;

    rawdns(host);
#if PY_MAJOR_VERSION >= 3
    free(cbname);
#endif /* Py3k */
    Py_DECREF(funcname);
    Py_RETURN_NONE;
}

#endif /* RAWDNS */

static PyObject *python_execute(PyObject *self, PyObject *args)
{
    PyObject *cmd, *from, *to, *rest;
    char *c_cmd, *c_from, *c_to, *c_rest;
    int cmdaccess, i;
    void (*found)(char*, const char*,char*, const int) = NULL;

    if (!PyArg_ParseTuple(args, "OOOOi", &cmd, &from, &to, &rest, &cmdaccess))
        return NULL;

    /* check for 4 strings */
#if PY_MAJOR_VERSION >= 3
    if (!PyUnicode_CheckExact(cmd)
        || !(PyUnicode_CheckExact(from))/* || Py_None == from)*/
        || !PyUnicode_CheckExact(to)
        || !PyUnicode_CheckExact(rest))
#else
    if (!PyString_CheckExact(cmd)
        || !(PyString_CheckExact(from))/* || Py_None == from)*/
        || !PyString_CheckExact(to)
        || !PyString_CheckExact(rest))
#endif /* Py3k */
    {
        PyErr_SetString(python_error, "(python_execute) invalid arguments");
        return NULL;
    }

    /* extract vars */
#if PY_MAJOR_VERSION >= 3
    c_cmd = python_unicode2char((PyUnicodeObject*) cmd);
    if (from != Py_None) c_from = python_unicode2char((PyUnicodeObject*) from);
    c_to = python_unicode2char((PyUnicodeObject*) to);
    c_rest = python_unicode2char((PyUnicodeObject*) rest);
#else
    c_cmd = PyString_AsString(cmd); /* not a copy */
    if (from != Py_None) c_from = PyString_AsString(from);
    c_to = PyString_AsString(to);
    c_rest = PyString_AsString(rest);
#endif /* Py3k */

    /*
    if (from == Py_None)
    {
        from = LocalBot..;
    }*/

    /* check command exists */
    for (i = 0; mcmd[i].name; ++i)
    {
        if (!stringcmp(mcmd[i].name, c_cmd))
        {
            found = mcmd[i].func;
            break;
        }
    }
    if (!found)
    {
#ifdef DEBUG
        debug("(python_execute) squeeze me (%s) !?!\n", c_cmd);
#endif /* DEBUG */
#if PY_MAJOR_VERSION >= 3
        free(c_cmd); if (from != Py_None) free(c_from); free(c_to); free(c_rest);
#endif /* Py3k */
        Py_RETURN_NONE;
    }

#ifdef DEBUG
    debug("(python_execute) (%s) (%s) (%s) (%s) (%d)\n", c_cmd, c_from, c_to, c_rest, cmdaccess);
#endif /* DEBUG */

    (*found)(c_from, c_to, c_rest, cmdaccess);

#if PY_MAJOR_VERSION >= 3
    free(c_cmd); if (from != Py_None) free(c_from); free(c_to); free(c_rest);
#endif /* Py3k */

    Py_RETURN_NONE;
}

static struct PyMethodDef pythonMethods[] =
{
{"getvar", (PyCFunction) python_getvar, METH_VARARGS, "Get variable"},
{"userlevel", (PyCFunction) python_userlevel, METH_VARARGS|METH_KEYWORDS, "User level"},
/* From channel.c */
{"is_chanuser", (PyCFunction) python_is_chanuser, METH_VARARGS|METH_KEYWORDS, "Check nick is in channel"},
#if 0
{"join", (PyCFunction) python_do_join, METH_VARARGS|METH_KEYWORDS, "Join a channel"},
{"part", (PyCFunction) python_do_part, METH_VARARGS|METH_KEYWORDS, "Part a channel"},
{"cycle", (PyCFunction) python_do_cycle, METH_VARARGS|METH_KEYWORDS, "Cycle a channel"},
{"wall", (PyCFunction) python_do_wall, METH_VARARGS|METH_KEYWORDS, "Do wall"},
{"mode", (PyCFunction) python_do_mode, METH_VARARGS|METH_KEYWORDS, "Do mode"},
{"invite", (PyCFunction) python_do_invite, METH_VARARGS|METH_KEYWORDS, "Do invite"},
{"sayme", (PyCFunction) python_do_sayme, METH_VARARGS|METH_KEYWORDS, "Do sayme"},
{"topic", (PyCFunction) python_do_topic, METH_VARARGS|METH_KEYWORDS, "Do topic"},
{"idle", (PyCFunction) python_do_idle, METH_VARARGS|METH_KEYWORDS, "Do idle"},
#endif /* 0 */
{"find_nuh", (PyCFunction) python_find_nuh, METH_VARARGS|METH_KEYWORDS, "Find nuh"},
/* From core.c */
#if 0
{"die", (PyCFunction) python_do_die, METH_VARARGS|METH_KEYWORDS, "Die"},
{"shutdown", (PyCFunction) python_do_shutdown, METH_VARARGS|METH_KEYWORDS, "Shutdown"},
{"server", (PyCFunction) python_do_server, METH_VARARGS|METH_KEYWORDS, "Connect server"},
#endif /* 0 */
/* Scripting stuff */
{"hook", (PyCFunction) python_hook, METH_VARARGS|METH_KEYWORDS, "Create hook"},
{"unhook", (PyCFunction) python_unhook, METH_VARARGS|METH_KEYWORDS, "Destroy hook"},
#ifdef DCC_FILE
{"dcc_sendfile", (PyCFunction) python_dcc_sendfile, METH_VARARGS|METH_KEYWORDS, "DCC send file"},
#endif /* DCC_FILE */
{"to_server", (PyCFunction) python_to_server, METH_VARARGS|METH_KEYWORDS, "Write line to server"},
{"to_file", (PyCFunction) python_to_file, METH_VARARGS|METH_KEYWORDS, "Write text to file"},
#ifdef DEBUG
{"debug", (PyCFunction) python_debug, METH_VARARGS, "Emech debug"},
#endif /* DEBUG */
#ifdef RAWDNS
{"dns", (PyCFunction) python_dns, METH_VARARGS|METH_KEYWORDS, "DNS"},
#endif /* RAWDNS */
{"execute", (PyCFunction) python_execute, METH_VARARGS, "Execute internal command"},
{NULL, NULL, 0, NULL} /* sentinel */
};

#if PY_MAJOR_VERSION >= 3

struct PyModuleDef pythonModule =
{
    PyModuleDef_HEAD_INIT,
    "emech", /* module name */
    "EnergyMech", /* module docstring */
    -1, /* size of per-interpreter state of the module,
        or -1 if the module keeps state in global variables. */
    pythonMethods
};

#endif /* Py3k */

PyMODINIT_FUNC pythonInit(void)
{
    PyObject *m;

#if PY_MAJOR_VERSION == 3
	if ((m = PyModule_Create(&pythonModule)) == NULL)
		return NULL;
#else
    m = Py_InitModule4("emech", pythonMethods,
        "EnergyMech", (PyObject*) NULL, PYTHON_API_VERSION);
#endif /* Py3k */

    python_error = PyErr_NewException("emech.Error", NULL, NULL);
    Py_INCREF(python_error);
    PyModule_AddObject(m, "Error", python_error);

#ifdef DEBUG
    PyModule_AddIntConstant(m, "define_debug", 1);
#else
    PyModule_AddIntConstant(m, "define_debug", 0);
#endif /* DEBUG */

    PyModule_AddStringConstant(m, "MEV_COMMAND", "command");
    PyModule_AddStringConstant(m, "MEV_PARSE", "parse");
    PyModule_AddStringConstant(m, "MEV_TIMER", "timer");
    PyModule_AddStringConstant(m, "DCC_COMPLETE", "dcc_complete");

    PyModule_AddIntConstant(m, "OK", 0);
    PyModule_AddIntConstant(m, "ERROR", 1);

    PyModule_AddIntConstant(m, "GUID", PYVAR_guid);
    PyModule_AddIntConstant(m, "CURRENTNICK", PYVAR_currentnick);
    PyModule_AddIntConstant(m, "BOTNICK", PYVAR_botnick);
    PyModule_AddIntConstant(m, "WANTNICK", PYVAR_wantnick);
    PyModule_AddIntConstant(m, "USERHOST", PYVAR_userhost);
    PyModule_AddIntConstant(m, "SERVER", PYVAR_server);
    PyModule_AddIntConstant(m, "NEXTSERVER", PYVAR_nextserver);
    PyModule_AddIntConstant(m, "CURRENTCHAN", PYVAR_currentchan);

    PyModule_AddStringConstant(m, "VERSION", (char *)VERSION);
    PyModule_AddStringConstant(m, "SRCDATE", (char *)SRCDATE);
    PyModule_AddStringConstant(m, "BOTCLASS", (char *)BOTCLASS);

    if (PyErr_Occurred())
    {
        PyErr_Print();
        Py_FatalError("Cant initialize module emech!");
    }

#if PY_MAJOR_VERSION >= 3
    return m;
#endif /* Py3k */
}

void init_python(void)
{
    if (Py_IsInitialized()) return;

    PyImport_AppendInittab((char*) "emech", pythonInit);

#if PY_MAJOR_VERSION >= 3
    Py_SetProgramName(L"energymech");
    wchar_t *argv2[] = {L""};
#else
    Py_SetProgramName("energymech");
    char *argv2[] = {""};
#endif /* Py3k */

    Py_Initialize();
    PySys_SetArgv(1, argv2);
}

#ifdef PLEASE_HACK_MY_SHELL

void do_python(COMMAND_ARGS)
{
    init_python();
    int res = PyRun_SimpleString(rest);
    to_user(from, "python command %s", (res == 0) ? "executed ok" : "failed");
}

#endif /* PLEASE_HACK_MY_SHELL */

void do_pythonscript(COMMAND_ARGS)
{
    init_python();
    FILE *f = fopen(rest, "r");
    if (f == NULL)
    {
        to_user(from, "python script %s not existing or unreadable", rest);
        return;
    }
    int res = PyRun_SimpleFileEx(f, rest, 1);
    to_user(from,"python script %s", (res == 0) ? "loaded ok" : "failed to load");
}

void free_python(void)
{
    if (Py_IsInitialized()) Py_Finalize();
}

#endif /* PYTHON */
