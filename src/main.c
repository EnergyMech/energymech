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
#ifdef MEGA_C
#define MAIN_C
#include "config.h"
#include "defines.h"
#include "structs.h"
#include "global.h"
#ifdef TCL
#include <tcl.h>
#endif
#include "h.h"
#include "text.h"
#include "mcmd.h"
#undef MAIN_C
#define VARS_C
#include "settings.h"
#undef VARS_C

#include "alias.c"
#include "auth.c"
#include "bounce.c"
#include "calc.c"
#include "channel.c"
#include "core.c"
#include "ctcp.c"
#include "debug.c"
#undef DEBUG_C
#include "dns.c"
#include "function.c"
#include "greet.c"
#include "help.c"
#include "hostinfo.c"
#include "io.c"
#include "irc.c"
#include "lib/string.c"
#include "net.c"
#include "note.c"
#include "ons.c"
#include "parse.c"
#include "partyline.c"
#if defined(TCL) && defined(PERL)
#undef STRINGIFY
#endif
#include "perl.c"
#include "prot.c"
#include "python.c"
#include "reset.c"
#include "seen.c"
#include "shit.c"
#include "spy.c"
#include "tcl.c"
#include "toybox.c"
#include "uptime.c"
#include "user.c"
#include "vars.c"
#include "web.c"
#else /* ifdef MEGA_C */

#define MAIN_C
#include "config.h"

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#endif /* ifdef-else MEGA_C */

/*
 *  we generally try to mess around as little as possible here
 */
void mech_exec(void)
{
	char	newprog[4096];		/* we're bloating the stack, so what? */
	char	*argv[5],*envp[2];
	int	i;

	argv[1] = argv[2] = argv[3] = argv[4] = NULL;

	if (respawn)
	{
		sprintf(newprog,"%s %i",executable,respawn);
		argv[0] = newprog;
	}
	else
	{
		argv[0] = executable;
	}

	i = 1;

	if (makecore)
		argv[i++] = "-c";

#ifdef DEBUG
	if (debug_on_exit)
		argv[i++] = "-X";
#endif /* DEBUG */

	envp[0] = mechresetenv;
	envp[1] = NULL;

#ifdef DEBUG
	debug("execve( %s, argv = { %s %s %s %s %s }, envp = { %s } )\n",
		nullstr(executable),
		nullstr(argv[0]),nullstr(argv[1]),nullstr(argv[2]),nullstr(argv[3]),nullstr(argv[4]),
		nullstr(envp[0]));
#endif /* DEBUG */

#ifdef __profiling__
#ifdef SIGPROF
	signal(SIGPROF,SIG_IGN);
#endif /* SIGPROF */
#endif /* __profiling__ */

	execve(executable,argv,envp);

#ifdef DEBUG
	debug("execve() failed!\n");
	if (debug_on_exit)
		run_debug();
#endif /* DEBUG */
	exit(1);
}

LS int r_ct;
LS char r_str[MSGLEN];

int randstring_count(char *line)
{
	r_ct++;
	return(FALSE);
}

int randstring_getline(char *line)
{
	if (--r_ct == 0)
	{
		stringcpy(r_str,line);
		return(TRUE);
	}
	return(FALSE);
}

char *randstring(const char *file)
{
	int	in;

	if ((in = open(file,O_RDONLY)) < 0)
		return(NULL);

	r_ct = 0;
	readline(in,&randstring_count);				/* readline closes in */

	if ((in = open(file,O_RDONLY)) < 0)
		return(NULL);

	r_ct = RANDOM(1,r_ct);
	readline(in,&randstring_getline);			/* readline closes in */

	return(r_str);
}

/*
 *  Signal handlers
 *  ~~~~~~~~~~~~~~~
 *  SIGHUP	Read and execute all commands in `mech.msg' file.
 *  SIGCHLD	Take care of zombies
 *  SIGALRM	Ignore ALRM signals
 *  SIGPIPE	Ignore PIPE signals
 *  SIGINT	Exit gracefully on ^C
 *  SIGILL	Illegal instruction (debug: report) quit
 *  SIGABRT	abort(3) (debug: report) quit
 *  SIGBUS	(Try to) Exit/restart gracefully on bus faults
 *  SIGSEGV	(Try to) Exit/restart gracefully on segmentation violations
 *  SIGTERM	Exit gracefully when killed
 *  SIGUSR1	Jump (a) bot to a new server
 *  SIGUSR2	Call run_debug() (dump `everything' to a debug file)
 */

LS struct
{
	uint32_t sighup:1,
		sigint:1,
		sigusr1:1;

} sched_sigs;

int sig_hup_callback(char *line)
{
	on_msg((char*)CoreUser.name,current->nick,line);
	return(FALSE);
}

void do_sighup(void)
{
	int	in;

	sched_sigs.sighup = FALSE;

	CurrentShit = NULL;
	CurrentChan = NULL;
	CurrentUser = (User*)&CoreUser;
	CurrentDCC  = (Client*)&CoreClient;
	*CurrentNick = 0;

	if ((in = open(MSGFILE,O_RDONLY)) >= 0)
	{
		readline(in,&sig_hup_callback);			/* readline closes in */
		unlink(MSGFILE);
	}

	CurrentDCC  = NULL;
}

void sig_hup(int crap)
{
#ifdef DEBUG
	debug("(sighup)\n");
#endif /* DEBUG */
	signal(SIGHUP,sig_hup);
	/*
	 *  Schedule the doing of greater things
	 */
	sched_sigs.sighup = TRUE;
}

#ifndef __linux__

/*
 *  Figured it out the hard way:
 *  DONT PUT THE signal() CALL BEFORE THE ACTUAL wait()!!
 *  Fucked up SunOS will hang/sigsegv!
 */
void sig_child(int crap)
{
#ifdef DEBUG
	debug("(sig_child)\n");
#endif /* DEBUG */

	while(1)
	{
		if (waitpid(-1,NULL,WNOHANG) <= 0)
			break;
	}
	signal(SIGCHLD,sig_child);
}

void sig_alrm(int signum)
{
#ifdef DEBUG
	debug("(sigalrm)\n");
#endif /* DEBUG */
	signal(SIGALRM,sig_alrm);
}

void sig_pipe(int dummy)
{
#ifdef DEBUG
	debug("(sigpipe)\n");
#endif /* DEBUG */
	signal(SIGPIPE,sig_pipe);
}

#endif /* not __linux__ */

void do_sigusr1(void)
{
	sched_sigs.sigusr1 = FALSE;

	if (current->connect == CN_ONLINE)
		to_server(TEXT_SIGUSR1);
	else
	{
		if (current->sock != -1)
		{
#ifdef IDWRAP
			unlink_identfile();
#endif /* IDWRAP */
			close(current->sock);
		}
		current->sock = -1;
	}
}

/*
 *  SIGUSR1 -- reconnect first bot to a new server
 */
void sig_usr1(int crap)
{
#ifdef DEBUG
	debug("(sigusr1)\n");
#endif /* DEBUG */

	sched_sigs.sigusr1 = TRUE;
	signal(SIGUSR1,sig_usr1);
}

#ifdef DEBUG

/*
 *  SIGUSR2 -- dump debug info
 */
void sig_usr2(int crap)
{
	time(&now);

	debug("(sigusr2)\n");
	signal(SIGUSR2,sig_usr2);
	run_debug();
}

#endif /* DEBUG */

/*
 *  signals that cause suicide...
 */

#ifdef UPTIME
#define UP_ARGS		, int uptype
#define UP_CALL(x)	, x
#else
#define UP_ARGS		/* nothing */
#define UP_CALL(x)	/* nothing */
#endif

void sig_suicide(char *text UP_ARGS)
{
#ifdef TRIVIA
	/*
	 *  trivia is global and is saved in the same way (and places) as session
	 */
	write_triviascore();
#endif /* TRIVIA */

#ifdef SESSION
	write_session();
#endif /* SESSION */

#ifdef UPTIME
	uptime_death(uptype);
#endif /* UPTIME */

	kill_all_bots(text);
	/* NOT REACHED */
}

void do_sigint(void)
{
	/*
	 *  We dont care about resetting sched_sigs.sigint,
	 *  we're committing suicide here!
	 */

	sig_suicide(TEXT_SIGINT /* comma */ UP_CALL(UPTIME_SIGINT));
	/* NOT REACHED */
}

void sig_int(int signum)
{
	if (sched_sigs.sigint)
	{
		/*
		 *  something locked up, SIGINT twice
		 */
#ifdef DEBUG
		debug("(sigint) (twice)\n");
#endif /* DEBUG */
		do_sigint();
		/* NOT REACHED */
	}
#ifdef DEBUG
	debug("(sigint)\n");
#endif /* DEBUG */

	sched_sigs.sigint = TRUE;
	signal(SIGINT,sig_int);
}

/*
 *  SIGILL, Illegal instruction
 */
#ifdef DEBUG
void sig_ill(int crap)
{
	debug("(sigill)\n");
}
#endif /* DEBUG */

/*
 *  SIGABRT, abort(3)
 */
#ifdef DEBUG
void sig_abrt(int crap)
{
	debug("(sigabrt)\n");
}
#endif /* DEBUG */

/*
 *  SIGBUS is a real killer and cant be scheduled.
 */
void sig_bus(int crap)
{
	time(&now);

	respawn++;
	if (respawn > 10)
		mechexit(1,exit);

#ifdef DEBUG
	debug("(sigbus)\n");
#endif /* DEBUG */

	do_exec = TRUE;
	sig_suicide(TEXT_SIGBUS /* comma */ UP_CALL(UPTIME_SIGBUS));
	/* NOT REACHED */
}

/*
 *  SIGSEGV shows no mercy, cant schedule it.
 */
#if defined(__linux__) && defined(__x86_64__) && defined(DEBUG) && !defined(__STRICT_ANSI__)
#include <sys/ucontext.h>
void sig_segv(int crap, siginfo_t *si, void *uap)
{
	mcontext_t *mctx;
	greg_t	*rsp,*rip; /* general registers */

	time(&now);
	startup = STARTUP_SIGSEGV;

	debug("(sigsegv) trying to access "mx_pfmt"\n",(mx_ptr)si->si_addr);
	mctx = &((ucontext_t *)uap)->uc_mcontext;
	rsp = &mctx->gregs[15];	/* RSP, 64-bit stack pointer */
	rip = &mctx->gregs[16]; /* RIP, 64-bit instruction pointer */

	debug("(sigsegv) Stack pointer: "mx_pfmt", Instruction pointer: "mx_pfmt"\n",(mx_ptr)*rsp,(mx_ptr)*rip);
	debug("(sigsegv) sig_segv() = "mx_pfmt"\n",(mx_ptr)sig_segv);
	debug("(sigsegv) do_crash() = "mx_pfmt"\n",(mx_ptr)do_crash);

	if (debug_on_exit)
	{
		run_debug();
		debug_on_exit = FALSE;
	}

	respawn++;
	if (respawn > 10)
		mechexit(1,exit);

	do_exec = TRUE;
	sig_suicide(TEXT_SIGSEGV /* comma */ UP_CALL(UPTIME_SIGSEGV));
	/* NOT REACHED */
}

#else /* defined(__linux__) && defined(__x86_64__) && defined(DEBUG) && !defined(__STRICT_ANSI__) */

void sig_segv(int signum)
{
	time(&now);

#ifdef DEBUG
	if (debug_on_exit)
	{
		run_debug();
		debug_on_exit = FALSE;
	}
#endif /* DEBUG */

	respawn++;
	if (respawn > 10)
		mechexit(1,exit);

	do_exec = TRUE;
	sig_suicide(TEXT_SIGSEGV /* comma */ UP_CALL(UPTIME_SIGSEGV));
	/* NOT REACHED */
}

#endif /* else defined(__linux__) && defined(__x86_64__) && defined(DEBUG) && !defined(__STRICT_ANSI__) */

/*
 *  SIGTERM
 */
void sig_term(int signum)
{
#ifdef __profiling__
	exit(0);
#endif /* __profiling__ */

	time(&now);

#ifdef DEBUG
	debug("(sigterm)\n");
#endif /* DEBUG */

	sig_suicide(TEXT_SIGTERM /* comma */ UP_CALL(UPTIME_SIGTERM));
	/* NOT REACHED */
}

/*
 *
 *  The main loop
 *
 */

#ifdef __GNUC__
LS void doit(void) __attribute__ ((__noreturn__, __sect(CORE_SEG)));
#endif
void doit(void)
{
	struct	timeval tv;
	Chan	*chan;
	Client	*client;
	SequenceTime this;
	Strp	*qm;
	time_t	last_update;

	last_update = now;

	/*
	 *  init update times so that they dont all run right away
	 */
	this.tenminute = now / 600;
	this.hour = now / 3600;

	/*
	 *  The Main Loop
	 */
mainloop:
	/*
	 *  signal processing
	 */
	for(current=botlist;current;current=current->next)
	{
		if (current->guid == sigmaster)
			break;
	}
	if (!current)
		current = botlist;
	if (sched_sigs.sighup)
		do_sighup();
	if (sched_sigs.sigint)
		do_sigint();
	if (sched_sigs.sigusr1)
		do_sigusr1();

	/*
	 *  check for regular updates
	 */
	if (last_update != now)
	{
		last_update = now;
		update(&this);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	hisock = -1;

#ifdef BOTNET
	select_botnet();
#endif /* BOTNET */

#ifdef WEB
	select_web();
#endif /* WEB */

#ifdef RAWDNS
	select_rawdns();
#endif /* RAWDNS */

#ifdef BOUNCE
	select_bounce();
#endif /* BOUNCE */

#ifdef HOSTINFO
	select_monitor();
#endif

	/*
	 *  unset here, reset if needed in bot loop
	 */
	short_tv &= ~(TV_SERVCONNECT|TV_LINEBUF);
	for(current=botlist;current;current=current->next)
	{
		if (current->sock == -1)
		{
#ifdef RAWDNS
			if (current->connect == CN_DNSLOOKUP)
			{
				Server	*sp;
				char	*host = NULL;

				if ((sp = find_server(current->server)))
				{
					if ((now - current->conntry) > ctimeout)
					{
#ifdef DEBUG
						debug("(doit) RAWDNS timed out (%s)\n",sp->name);
#endif /* DEBUG */
						current->connect = CN_NOSOCK;
						sp->err = SP_NO_DNS;
						goto doit_jumptonext;
					}
					if ((host = poll_rawdns(sp->name)))
					{
						char	hosttemp[strlen(host)+1];

#ifdef DEBUG
						debug("(doit) rawdns: %s ==> %s\n",sp->name,host);
#endif /* DEBUG */
						stringcpy(hosttemp,host);
						host = hosttemp;
						try_server(sp,host);
					}
				}
			}
			else
			{
				if (current->connect == CN_SPINNING)
				{
					if ((now - current->conntry) >= 60)
						connect_to_server();
				}
				else
				{
doit_jumptonext:
					short_tv |= TV_SERVCONNECT;
					if ((now - current->conntry) >= 2)
						connect_to_server();
				}
			}
#else /* ! RAWDNS */
			if (current->connect == CN_SPINNING)
			{
				if ((now - current->conntry) >= 60)
					connect_to_server();
			}
			else
			{
				short_tv |= TV_SERVCONNECT;
				if ((now - current->conntry) >= 2)
					connect_to_server();
			}
#endif /* RAWDNS */
		}
		/* not an else block since previous block might set sock to != -1 */
		if (current->sock != -1)
		{
			if (current->ip.s_addr == 0)
			{
				struct	sockaddr_in sai;
				int	sz;

				sz = sizeof(sai);
				if (getsockname(current->sock,(struct sockaddr *)&sai,&sz) == 0)
					current->ip.s_addr = sai.sin_addr.s_addr;
			}
			if ((current->connect == CN_TRYING) || (current->connect == CN_CONNECTED))
			{
				short_tv |= TV_SERVCONNECT;
				if ((now - current->conntry) > ctimeout)
				{
#ifdef DEBUG
					debug("(doit) {%i} Connection timed out\n",current->sock);
#endif /* DEBUG */
#ifdef IDWRAP
					unlink_identfile();
#endif /* IDWRAP */
					close(current->sock);
					current->sock = -1;
					goto restart_dcc;
				}
				if (current->connect == CN_TRYING)
					FD_SET(current->sock,&write_fds);
			}
			if (current->sendq)
			{
				short_tv |= TV_LINEBUF;
			}
			else
			{
				for(chan=current->chanlist;chan;chan=chan->next)
				{
					if (chan->kicklist || chan->modelist)
					{
						short_tv |= TV_LINEBUF;
						break;
					}
				}
			}
			chkhigh(current->sock);
			FD_SET(current->sock,&read_fds);
		}

		/*
		 *  Clean out DCC_DELETE clients
		 */
restart_dcc:
		for(client=current->clientlist;client;client=client->next)
		{
			if (client->flags == DCC_DELETE)
			{
				delete_client(client);
				goto restart_dcc;
			}
		}

		for(client=current->clientlist;client;client=client->next)
		{
			if (client->flags & DCC_ASYNC)
			{
				chkhigh(client->sock);
				FD_SET(client->sock,&write_fds);
			}
			if (client->sock != -1)
			{
				chkhigh(client->sock);
				FD_SET(client->sock,&read_fds);
			}
		}
	}

#ifdef UPTIME
	if (uptimesock >= 0)
		chkhigh(uptimesock);
#endif /* UPTIME */

	/*
	 *  Longer delay saves CPU but some features require shorter delays
	 */
#ifdef NOTIFY
	tv.tv_sec = (short_tv) ? 1 : 5;
#else /* NOTIFY */
	tv.tv_sec = (short_tv) ? 1 : 30;
#endif /* NOTIFY */
	tv.tv_usec = 0;

	if ((select(hisock+1,&read_fds,&write_fds,0,&tv) == -1) && (errno == EINTR))
		goto mainloop;

	/*
	 *  Update current time
	 */
	time(&now);

	for(current=botlist;current;current=current->next)
	{
		/*
		 *  sendq_time can never be smaller than the current time
		 *  it is important that the check is done before anything
		 *  else that could potentially send output to the server!
		 */
		if (current->sendq_time < now)
			current->sendq_time = now;
	}

	for(current=botlist;current;current=current->next)
	{
		if (current->clientlist)
			process_dcc();

		if (current->sock != -1)
			process_server_input();

		if (current->connect == CN_ONLINE)
		{
			/*
			 *  Keep server idle-timer low to seem like you are chatting
			 */
			if (current->setting[TOG_NOIDLE].int_var)
			{
				if ((now - current->lastantiidle) > PINGSENDINTERVAL)
				{
					to_server("PRIVMSG * :0\n");
					current->lastantiidle = now;
				}
			}
			/*
			 *  check for HIGH priority modes
			 */
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if (chan->modelist && chan->bot_is_op)
					push_modes(chan,8);
			}
			/*
			 *  check for waiting kicks
			 */
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if (chan->kicklist && chan->bot_is_op)
					push_kicks(chan);
			}
			/*
			 *  check for LOW priority modes
			 */
			for(chan=current->chanlist;chan;chan=chan->next)
			{
				if (chan->modelist && chan->bot_is_op)
					push_modes(chan,0);
			}
			/*
			 *  the un-important sendq only sends when sendq_time <= now
			 */
			if ((current->sendq) && (current->sendq_time <= now))
			{
				qm = current->sendq;
				to_server(FMT_PLAINLINE,qm->p);
				current->sendq = qm->next;
				Free((char**)&qm);
			}
		}
	}

	/*
	 *  Check for do_die()'d bots...
	 */
restart_die:
	for(current=botlist;current;current=current->next)
	{
		if (current->connect == CN_BOTDIE)
		{
			signoff(current->from,current->signoff);
			/*
			 *  signoff touches the botlist, so we need to restart
			 */
			goto restart_die;
		}
	}

#ifdef BOTNET
	if (botnetlist)
		process_botnet();
#endif /* BOTNET */

#ifdef BOUNCE
	process_bounce();
#endif /* BOUNCE */

#ifdef CHANBAN
	process_chanbans();
#endif /* CHANBAN */

#ifdef RAWDNS
	process_rawdns();
#endif /* RAWDNS */

#ifdef UPTIME
	process_uptime();
#endif /* UPTIME */

#ifdef WEB
	process_web();
#endif /* WEB */

#ifdef HOSTINFO
	process_monitor();
#endif

#ifdef TRIVIA
	trivia_tick();
#endif /* TRIVIA */

	/*
	 *  Check killsocks
	 */
	if (killsocks)
		killsock(-1);

	goto mainloop;
}

/*
 *  main(), we love it and cant live without it
 */

LS char *bad_exe = "init: Error: Improper executable name\n";

#ifdef __GNUC__
int main(int argc, char **argv, char **envp) __attribute__ ((__sect(INIT_SEG)));
#endif
int main(int argc, char **argv, char **envp)
{
	struct stat st;
	char	*opt;
	int	do_fork = TRUE;
	int	versiononly = FALSE;
#ifdef NEWBIE
	int	n = 0;
#endif

	uptime = time(&now);
	startup = STARTUP_NORMALSTART;

	if ((getuid() == 0) || (geteuid() == 0))
	{
		to_file(1,"init: Do NOT run EnergyMech as root!\n");
		_exit(1);
	}

	stat("..",&st);
	parent_inode = st.st_ino; /* used for is_safepath() */

	srand(now+getpid());

	/*
	 *   Code to detect and recover after a RESET
	 */
	/*
	execve( ./energymech, argv = { ./energymech <NULL> <NULL> <NULL> <NULL> },
		envp = { MECHRESET=d3 f1881:2:X12 } )
	*/
	while(*envp)
	{
		char	*p1;
		const char *p2;

		p1 = *envp;
		p2 = STR_MECHRESET;
		while(*p2)
		{
			if (*p1 != *p2)
				break;
			p1++;
			p2++;
		}
		if (*p2 == 0)
		{
			mechresetenv = p1;
			do_fork = FALSE;
#ifdef DEBUG
			if (*p1 == 'd')
				mechresetenv = recover_debug(p1+1);
#endif /* DEBUG */
			break;
		}
		envp++;
	}

#ifdef DEBUG
	mrrec = calloc(sizeof(aMEA),1);
#endif /* DEBUG */

	if (!*argv || !**argv)
	{
		to_file(1,bad_exe);
		_exit(1);
	}
	if ((opt = STRCHR(*argv,' ')) != NULL)
	{
		*(opt++) = 0;
		respawn = asc2int(opt);
		if (errno)
		{
			to_file(1,bad_exe);
			_exit(1);
		}
	}

	executable = *argv;

	while((argc > 1) && (argv[1][0] == '-'))
	{
		argc--;
		argv++;
		opt = *argv;
		switch(opt[1])
		{
		case 'v':
			versiononly = TRUE;
			break;
		case 'h':
			to_file(1,TEXT_USAGE,executable);
			to_file(1,TEXT_FSWITCH
				  TEXT_CSWITCH
				  TEXT_PSWITCH1
				  TEXT_PSWITCH2
#ifdef DEBUG
				  TEXT_DSWITCH
				  TEXT_OSWITCH
				  TEXT_XSWITCH
#endif /* DEBUG */
				  TEXT_HSWITCH
				  TEXT_VSWITCH);
			_exit(0);
		case 'c':
			makecore = TRUE;
			break;
#ifdef DEBUG
		case 'd':
			dodebug = TRUE;
			do_fork = FALSE;
			break;
		case 'o':
			if (opt[2] != 0)
			{
				debugfile = &opt[2];
			}
			else
			{
				++argv;
				if (!*argv)
				{
					to_file(1,"init: No debugfile specified\n");
					_exit(0);
				}
				debugfile = *argv;
				argc--;
			}
			do_fork = TRUE;
			break;
		case 'p':
			++argv;
			if (*argv)
				to_file(1,"%s\n",makepass(*argv));
			else
				to_file(1,"error: Missing argument for -p <string>\n");
			_exit(0);
		case 'X':
			debug_on_exit = TRUE;
			break;
#endif /* DEBUG */
		case 'e': /* run a single command before exiting */
			startup = 3;
			++argv;
			if (*argv)
			{
				/*void on_msg(char *from, char *to, char *rest)
				on_msg(); */
			}
			else
				to_file(1,"error: Missing argument for -e <command string>\n");
			_exit(0);
		case 't':
			startup = STARTUP_TESTRUN;
			break;
		case 'f':
			if (opt[2] != 0)
			{
				configfile = &opt[2];
			}
			else
			{
				++argv;
				if(!*argv)
				{
					to_file(1,ERR_MISSINGCONF);
					_exit(0);
				}
				configfile = *argv;
				argc--;
			}
			to_file(1,INFO_USINGCONF,configfile);
			break;
		default:
			to_file(1,ERR_UNKNOWNOPT,opt);
			_exit(1);
		}
	}

	servergrouplist = (ServerGroup*)&defaultServerGroup;
	currentservergroup = (ServerGroup*)&defaultServerGroup;

	if (!mechresetenv)
	{
		to_file(1,TEXT_HDR_VERS,VERSION,SRCDATE);
		to_file(1,"Compiled on " GENDATE "\n");
		to_file(1,TEXT_HDR_FEAT,__mx_opts);
	}

#ifdef NEWBIE
	if (stat(configfile,&st));
	{
		if ((st.st_mode & (S_IWGRP|S_IWOTH)) != 0)
		{
			to_file(1,"error: configfile is writeable by others, exiting...\n");
			_exit(0);
		}
		if ((st.st_mode & (S_IRGRP|S_IROTH)) != 0)
			to_file(1,"warning: configfile is readable by others\n");
	}
	if (stat(".",&st));
	{
		if ((st.st_mode & (S_IWGRP|S_IWOTH)) != 0)
		{
			to_file(1,"error: energymech home directory is writeable by others, exiting...\n");
			_exit(0);
		}
	}
#endif /* NEWBIE */

	if (versiononly)
		_exit(0);	/* _exit() here because we dont want a profile file to be written */

#ifdef __linux__
	signal(SIGCHLD,SIG_IGN);
	signal(SIGALRM,SIG_IGN);
	signal(SIGPIPE,SIG_IGN);
#else
	signal(SIGCHLD,sig_child);
	signal(SIGALRM,sig_alrm);
	signal(SIGPIPE,sig_pipe);
#endif
	signal(SIGHUP,sig_hup);
	signal(SIGINT,sig_int);
	signal(SIGBUS,sig_bus);
	signal(SIGTERM,sig_term);
	signal(SIGUSR1,sig_usr1);
#ifdef DEBUG
	signal(SIGUSR2,sig_usr2);
#else /* DEBUG */
	signal(SIGUSR2,SIG_IGN);
#endif /* DEBUG */

#ifdef RAWDNS
	memset(&ia_ns,0,sizeof(ia_ns));
	ia_default.s_addr = LOCALHOST_ULONG;
#endif /* RAWDNS */

	memset(&__internal_users,0,sizeof(User)*2);
	CoreUser.x.x.access = 100;
	LocalBot.x.x.access = 200;
	LocalBot.x.x.aop = 1;
	LocalBot.chan = CoreUser.chan = (Strp*)&CMA;

	readcfgfile();

#ifdef UPTIME
	init_uptime();
#endif /* UPTIME */

#ifndef I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS
	if (spawning_lamer > 4)
	{
		to_file(1,"init: I refuse to run more than 4 bots!\n");
		_exit(1);
	}
#endif /* I_HAVE_A_LEGITIMATE_NEED_FOR_MORE_THAN_4_BOTS */

	for(current=botlist;current;current=current->next)
	{
		if ((opt = current->setting[STR_USERFILE].str_var))
#ifndef NEWBIE
			read_userlist(opt);
	}
#else
		{
			if (stat(opt,&st));
			{
				if ((st.st_mode & (S_IWGRP|S_IWOTH)) != 0)
				{
					to_file(1,"error: userfile(%s) is writeable by others, exiting...\n",opt);
					_exit(0);
				}
				if ((st.st_mode & (S_IRGRP|S_IROTH)) != 0)
					to_file(1,"warning: userfile(%s) is readable by others\n",opt);
			}
			read_userlist(opt);
		}
		if (current->userlist == NULL)
		{
			to_file(1,"init: No userlist loaded for %s\n",nullstr(current->nick));
			n++;
		}
	}
	if (n)
	{
		_exit(1);
	}
#endif /* NEWBIE */

	for(current=botlist;current;current=current->next)
	{
		mirror_userlist();
	}

#ifdef SEEN
	read_seenlist();
#endif /* SEEN */

	if (!mechresetenv)
		to_file(1,INFO_RUNNING);

	if (do_fork && startup != 666)
	{
		close(0);
		close(1);
		close(2);

		switch(fork())
		{
		case 0:
			break;
		default:
#ifdef DEBUG
			debug_on_exit = FALSE;
#endif /* DEBUG */
		case -1:
			mechexit(0,_exit);
		}
		setsid();
	}

	/*
	 *  save pid to `mech.pid'
	 */
	if ((do_fork = open(PIDFILE,O_WRONLY|O_CREAT|O_TRUNC,NEWFILEMODE)) >= 0)
	{
		to_file(do_fork,"%i\n",(int)(getpid()));
		close(do_fork);
	}

#ifdef CTCP
	memset(&ctcp_slot,0,sizeof(ctcp_slot));
#endif /* CTCP */

#ifdef BOTNET
	last_autolink = now + 30 + (rand() >> 27);	/* + 0-31 seconds */
#endif /* BOTNET */

	if (mechresetenv)
		recover_reset();

	/*
	 *  wait until after recover_reset() cuz it might change makecore
	 */
	if (!makecore)
	{
#if defined(__linux__) && defined(__x86_64__) && defined(DEBUG) && !defined(__STRICT_ANSI__)
		struct sigaction s;
		s.sa_flags = SA_SIGINFO;
		sigemptyset(&s.sa_mask);
		s.sa_sigaction = sig_segv;
		if (sigaction(SIGSEGV, &s, NULL) < 0)
		{
			debug("(main) binding SIGSEGV handler failed: %s\n",strerror(errno));
		}
#else
		signal(SIGSEGV,sig_segv);
#endif
#ifdef DEBUG
		signal(SIGILL,sig_ill);
		signal(SIGABRT,sig_abrt);
#endif /* DEBUG */
	}

#ifdef HOSTINFO
	monitor_fs(executable);
#endif

	if (startup == STARTUP_TESTRUN)
	{
		to_file(1,"init: test run completed, exiting...\n");
		_exit(0);
	}
	startup = STARTUP_RUNNING;
#ifdef DEBUG
	debug("(main) entering doit()...\n");
#endif
	doit();
}
