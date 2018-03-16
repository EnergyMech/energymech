/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2018 proton

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
#ifndef H_H
#define H_H 1

#define ischannel(x)		(*x == '#')

#define nullstr(x)		((x)) ? (x) : NULLSTR
#define nullbuf(x)		(x && *x) ? x : NULLSTR

#define chkhigh(x)		if (x > hisock) { hisock = x; }

#define COMMAND_ARGS		char *from, char *to, char *rest, int cmdaccess

#define STRCHR			strchr
#define STREND(x)		STRCHR(x,0)

/*
 *  some default code for socket flags
 */
#ifdef ASSUME_SOCKOPTS

#define unset_closeonexec(x)	fcntl(x,F_SETFD,0);

#else /* ASSUME_SOCKOPTS */

#define	unset_closeonexec(x)	fcntl(x,F_SETFD,(~FD_CLOEXEC) & fcntl(x,F_GETFD));

#endif /* ASSUME_SOCKOPTS */

/*
 *  Dont try this at home kids...
 */
#ifdef __ELF__
#define __sect(x)		__section__ (x)
#else
#define __sect(x)		/* nothing */
#endif
#ifdef __GNUC__

#define __vattr(x)		__attribute__ (( x ))
#define __page(x)		__attribute__ (( __sect(x) ))
#define __attr(x,y)		__attribute__ (( __sect(x), y ))
#define __att2(x,y,z)		__attribute__ (( __sect(x), y, z ))

#else
#define __vattr(x)		/* nothing */
#define __page(x)		/* nothing */
#define __attr(x,y)		/* nothing */
#define __att2(x,y,z)		/* nothing */
#endif

#if !defined(__profiling__) && defined(__i386__)
# define __regparm(x)		regparm (x)
#else
# define __regparm(x)
#endif

#define CORE_SEG	".text.a"
#define CFG1_SEG	".text.b"
#define CMD1_SEG	".text.c"
#define INIT_SEG	".text.d"
#define RARE_SEG	".text.e"
#define DBUG_SEG	".text.f"

#ifdef DEBUG

#define set_mallocdoer(x)	mallocdoer = x;

#define mechexit(x,y)				\
{						\
	if (debug_on_exit)			\
		wrap_debug();			\
	if (do_exec)				\
		mech_exec();			\
	y(x);					\
}

#else /* not DEBUG */

#define set_mallocdoer(x)

#define mechexit(x,y)				\
{						\
	if (do_exec)				\
		mech_exec();			\
	y(x);					\
}

#endif /* DEBUG */
LS int makecrc(const char *);
LS void send_supress(const char *, const char *);
LS void netchanSupress(BotNet *, char *);

LS Chan *find_channel(const char *, int)			__attr(CORE_SEG, __regparm (2) );
LS Chan *find_channel_ac(const char *)				__attr(CORE_SEG, __regparm (1) );
LS Chan *find_channel_ny(const char *)				__attr(CORE_SEG, __regparm (1) );
LS ChanUser *find_chanuser(Chan *, const char *)		__attr(CORE_SEG, __regparm (2) );
LS Client *find_client(const char *)				__page(CORE_SEG);
LS Mech *add_bot(int, char *)					__page(CORE_SEG);
LS KickSay *find_kicksay(char *, char *)			__page(CORE_SEG);
LS Server *add_server(char *, int, char *)			__page(CFG1_SEG);
LS Server *find_server(int)					__page(CORE_SEG);
LS ServerGroup *getservergroupid(int)				__page(CMD1_SEG);
LS Shit *add_shit(char *, char *, char *, char *, int, int)	__page(CMD1_SEG);
LS Shit *find_shit(const char *, const char *)			__page(CORE_SEG);
LS Shit *get_shituser(char *, char *)				__page(CORE_SEG);
LS User *add_user(char *, char *, int)				__page(CFG1_SEG);
LS User *find_handle(const char *)				__page(CORE_SEG);
LS User *get_authuser(char *, char *)				__page(CORE_SEG);
LS User *get_user(const char *, const char *)			__page(CORE_SEG);
LS int get_authaccess(char *, char *)				__page(CORE_SEG);
LS int get_protaction(Chan *, char *)				__page(CORE_SEG);
LS int get_shitaction(const char *, const char *)		__page(CORE_SEG);
LS int get_useraccess(const char *, const char *)		__page(CORE_SEG);
LS int get_maxaccess(const char *)				__page(CORE_SEG);

LS int Strcasecmp(const char *, const char *)			__att2(CORE_SEG, const, __regparm (2) );
LS int Strcmp(const char *, const char *)			__att2(CORE_SEG, const, __regparm (2) );
LS char *Strcat(char *, const char *)				__attr(CORE_SEG, __regparm (2) );
LS char *Strchr(const char *, int)				__att2(CORE_SEG, const, __regparm (2) );
LS char *Strcpy(char *, const char *)				__attr(CORE_SEG, __regparm (2) );
LS char *Strdup(const char *)					__attr(CORE_SEG, __regparm (1) );
LS void Strncpy(char *, const char *, int)			__attr(CORE_SEG, __regparm (3) );
LS char *chop(char **)						__attr(CORE_SEG, __regparm (1) );
LS int get_number(const char *)					__page(CORE_SEG);
LS int nickcmp(const char *, const char *)			__att2(CORE_SEG, const, __regparm (2) );
LS char *nickcpy(char *, const char *)				__attr(CORE_SEG, __regparm (2) );

LS char *cipher(char *)						__page(CMD1_SEG);
LS char *cluster(char *)					__page(CMD1_SEG);
LS char *find_nuh(char *)					__page(CORE_SEG);
LS char *format_uh(char *, int)					__page(CMD1_SEG);
LS char *get_channel(char *, char **)				__attr(CMD1_SEG, __regparm (2) );
LS char *get_channel2(char *, char **)				__page(CMD1_SEG);
LS char *get_nuh(ChanUser *)					__page(CORE_SEG);
LS char *get_token(char **, const char *)			__page(CORE_SEG);
LS char *getuh(char *)						__page(CORE_SEG);
LS char *idle2str(time_t, int)					__page(CORE_SEG);
LS char *makepass(char *)					__page(CMD1_SEG);
LS char *nick2uh(char *, char *)				__page(CMD1_SEG);
LS char *randstring(char *)					__page(CORE_SEG);
LS char *sockread(int, char *, char *)				__page(CORE_SEG);
LS char *logtime(time_t)					__page(CORE_SEG);
LS void table_buffer(const char *, ...)				__attr(CMD1_SEG, format (printf, 1, 2) );
LS void table_send(const char *, const int)			__attr(CMD1_SEG, __regparm (2) );
LS char *time2away(time_t)					__page(CORE_SEG);
LS char *time2medium(time_t)					__page(CORE_SEG);
LS char *time2small(time_t)					__page(CMD1_SEG);
LS char *time2str(time_t)					__page(CMD1_SEG);
LS char *tolowercat(char *dest, const char *src)		__attr(CMD1_SEG, __regparm (2) );

/*
 *  socket.c
 */
LS ulong get_ip(const char *)					__page(CORE_SEG);
LS int SockAccept(int)						__page(CORE_SEG);
LS int SockConnect(char *, int, int)				__page(CORE_SEG);
LS void SockFlags(int)						__page(CORE_SEG);
LS int SockListener(int)					__page(CORE_SEG);
LS int SockOpts(void)						__page(CORE_SEG);

LS int capslevel(char *)					__page(CORE_SEG);
LS int check_mass(Chan *, ChanUser *, int)			__page(CORE_SEG);
LS int compile_timer(HookTimer *, char *)			__page(CORE_SEG);	/* SCRIPTING */
LS int conf_callback(char *)					__page(INIT_SEG);	/* INIT */
LS int do_help_callback(char *)					__page(CMD1_SEG);
LS int find_setting(char *)					__page(CMD1_SEG);
LS int is_bot(const char *)					__page(CORE_SEG);
LS int is_nick(const char *)					__page(CORE_SEG);
LS int killsock(int)						__page(CORE_SEG);
LS int access_needed(char *)					__page(CMD1_SEG);
LS int mode_effect(Chan *, qMode *)				__page(CORE_SEG);
LS int passmatch(char *, char *)				__page(CMD1_SEG);
LS int randstring_count(char *)					__page(CORE_SEG);
LS int randstring_getline(char *)				__page(CORE_SEG);
LS int read_seenlist(void)					__page(CFG1_SEG);
LS int read_seenlist_callback(char *)				__page(CFG1_SEG);
LS int read_userlist(char *)					__page(CFG1_SEG);
LS int read_userlist_callback(char *)				__page(CFG1_SEG);
LS int reverse_mode(char *, Chan *, int, int)			__page(CORE_SEG);
LS int to_file(int, const char *, ...)				__attr(CORE_SEG, format (printf, 2, 3) );
LS int try_server(Server *, char *)				__page(CORE_SEG);
LS int usercanmodify(const char *, const User *)		__attr(CORE_SEG, __regparm (2) );
LS int write_seenlist(void)					__page(CORE_SEG);
LS int write_session(void)					__page(CORE_SEG);
LS int write_userlist(char *)					__page(CORE_SEG);
LS void var_resolve_host(const struct Setting *)		__page(CFG1_SEG);

LS ulong stringhash(char *)					__page(CORE_SEG);

/*
 *  function.c
 */
LS void *Calloc(int)						__attr(CORE_SEG, __regparm (1) );
LS void Free(char **)						__attr(CORE_SEG, __regparm (1) );
LS const int Strlen(const char *, ...)				__page(CORE_SEG);
LS const int Strlen2(const char *, const char *)		__attr(CORE_SEG, __regparm (2) );
LS int matches(const char *, const char *)			__att2(CORE_SEG, const, __regparm (2) );
LS int num_matches(const char *, const char *)			__att2(CORE_SEG, const, __regparm (2) );
LS int a2i(char *)						__attr(CORE_SEG, __regparm (1) );
LS int is_safepath(const char *, int)				__attr(CORE_SEG, __regparm (2) );

LS void afmt(char *, const char *, const char *)		__page(CMD1_SEG);
LS void aucheck(User *)						__attr(CORE_SEG, __regparm (1) );
LS void change_authnick(char *, char *)				__page(CORE_SEG);
LS void change_pass(User *, char *)				__page(CMD1_SEG);
LS void chan_modestr(Chan *, char *)				__page(CMD1_SEG);
LS void channel_massmode(Chan *, char *, int, char, char)	__page(CMD1_SEG);
LS void channel_massunban(Chan *, char *, time_t)		__page(CMD1_SEG);
LS void check_idlekick(void)					__page(CORE_SEG);
LS void check_kicksay(Chan *, ChanUser *, char *)		__page(CORE_SEG);
LS void check_shit(void)					__page(CORE_SEG);
LS void common_public(Chan *, char *, char *, char *)		__page(CORE_SEG);
LS void connect_to_server(void)					__page(CORE_SEG);
LS void copy_vars(UniVar *, UniVar *)				__page(CMD1_SEG);
LS void ctcp_dcc(char *, char *, char *)			__page(CORE_SEG);
LS void cycle_channel(Chan *)					__page(CORE_SEG);
LS void dcc_banner(Client *)					__page(CORE_SEG);
LS void dcc_chat(char *)					__page(CMD1_SEG);
LS int dcc_only_command(char *)					__page(CMD1_SEG);
LS void debug(char *, ...)					__attr(CORE_SEG, format (printf, 1, 2) );
LS void delete_auth(char *)					__page(RARE_SEG);	/* rare */
LS void delete_ban(Chan *, char *)				__page(CORE_SEG);
LS void delete_modemask(Chan *, char *, int)			__page(CORE_SEG);
LS void delete_client(Client *)					__page(CORE_SEG);
LS void delete_vars(UniVar *, int)				__page(CMD1_SEG);
LS void deop_ban(Chan *, ChanUser *, char *)			__page(CMD1_SEG);
LS void deop_screwban(Chan *, ChanUser *)			__page(CMD1_SEG);
LS void deop_siteban(Chan *, ChanUser *)			__page(CMD1_SEG);

/*
 *  user.c
 */
LS void cfg_chan(char *)					__page(CFG1_SEG);
LS void cfg_greet(char *)					__page(CFG1_SEG);
LS void cfg_mask(char *)					__page(CFG1_SEG);
LS void cfg_note(char *)					__page(CFG1_SEG);
LS void cfg_opt(char *)						__page(CFG1_SEG);
LS void cfg_pass(char *)					__page(CFG1_SEG);
LS void cfg_shit(char *)					__page(CFG1_SEG);
LS void cfg_user(char *)					__page(CFG1_SEG);
void mirror_user(User *)					__page(CORE_SEG);
void mirror_userlist(void)					__page(CORE_SEG);
LS void addtouser(Strp **, const char *, int)			__attr(CORE_SEG, __regparm (3) );
LS int remfromuser(Strp **, const char *)			__attr(CORE_SEG, __regparm (2) );

/*
 *  commands
 */
LS void do_8ball(COMMAND_ARGS)					__page(CMD1_SEG);	/* TOYBOX */
LS void do_access(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_alias(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_auth(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_away(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_banlist(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_bigsay(COMMAND_ARGS)					__page(CMD1_SEG);	/* TOYBOX */
LS void do_bye(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_cchan(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_chaccess(COMMAND_ARGS)				__page(CMD1_SEG);	/* DYNCMDACCESS */
LS void do_channels(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_chat(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_clearshit(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_cmd(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_core(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_cserv(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_cycle(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_debug(COMMAND_ARGS)					__page(DBUG_SEG);	/* DEBUG */
LS void do_die(COMMAND_ARGS)					__page(RARE_SEG);	/* rare */
LS void do_do(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_echo(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_esay(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_forget(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_greet(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_help(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_idle(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_invite(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_irclusers(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_ircstats(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_ircuserhost(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_ircwhois(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_join(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_kick(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_kickban(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_kicksay(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_last(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_link(COMMAND_ARGS)					__page(CMD1_SEG);	/* BOTNET */
LS void do_load(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_mode(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_msg(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_names(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_nick(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_note(COMMAND_ARGS)					__page(CMD1_SEG);	/* NOTE */
LS void do_notify(COMMAND_ARGS)					__page(CMD1_SEG);	/* NOTIFY */
LS void do_opdeopme(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_opvoice(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_part(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_passwd(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_ping_ctcp(COMMAND_ARGS)				__page(CMD1_SEG);	/* CTCP */
LS void do_random_msg(COMMAND_ARGS)				__page(CMD1_SEG);	/* TOYBOX */
LS void do_randtopic(COMMAND_ARGS)				__page(CMD1_SEG);	/* TOYBOX */
LS void do_read(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_reset(COMMAND_ARGS)					__page(RARE_SEG);	/* rare */
LS void do_rkicksay(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_rshit(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_rspy(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_save(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_sayme(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_seen(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_server(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_servergroup(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_set(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_setpass(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_shit(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_shitlist(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_showidle(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_showusers(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_shutdown(COMMAND_ARGS)				__page(RARE_SEG);	/* rare */
LS void do_siteban(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_spy(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_time(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_topic(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_trivia(COMMAND_ARGS)					__page(CMD1_SEG);	/* TRIVIA */
LS void do_unalias(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_unban(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_upsend(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_upontime(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_usage(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_user(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_userlist(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_version(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_wall(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_whom(COMMAND_ARGS)					__page(CMD1_SEG);
/*
 *  end of commands
 */
LS void fix_config_line(char *)					__attr(CORE_SEG, __regparm (1) );
LS void greet(void)						__page(CMD1_SEG);
LS void join_channel(char *, char *)				__page(CORE_SEG);
LS void kill_all_bots(char *)					__attr(RARE_SEG, __noreturn__ );		/* rare */
LS int make_auth(const char *, const User *)			__page(CORE_SEG);
LS Ban *make_ban(Ban **, char *, char *, time_t)		__page(CORE_SEG);
LS void make_chanuser(char *, char *)				__attr(CORE_SEG, __regparm (2) );
LS void make_ireq(int, char *, char *)				__page(CMD1_SEG);
LS void mass_action(Chan *, ChanUser *)				__page(CORE_SEG);
LS void mech_exec(void)						__attr(RARE_SEG, __noreturn__ );		/* rare */
LS void on_action(char *, char *, char *)			__page(CORE_SEG);
LS void on_ctcp(char *, char *, char *)				__page(CORE_SEG);
LS void on_join(Chan *, char *)					__page(CORE_SEG);
LS void on_kick(char *, char *)					__page(CORE_SEG);
LS void on_mode(char *, char *, char *)				__page(CORE_SEG);
LS void on_msg(char *, char *, char *)				__page(CORE_SEG);
LS void on_nick(char *, char *)					__page(CORE_SEG);
LS void parse_213(char *, char *)				__page(CMD1_SEG);	/* stats C */
LS void parse_219(char *, char *)				__page(CMD1_SEG);	/* end of stats */
LS void parse_251(char *, char *)				__page(CORE_SEG);
LS void parse_252(char *, char *)				__page(CORE_SEG);
LS void parse_253(char *, char *)				__page(CORE_SEG);
LS void parse_254(char *, char *)				__page(CORE_SEG);
LS void parse_255(char *, char *)				__page(CORE_SEG);
LS void parse_301(char *, char *)				__page(CORE_SEG);
LS void parse_311(char *, char *)				__page(CORE_SEG);
LS void parse_312(char *, char *)				__page(CORE_SEG);
LS void parse_313(char *, char *)				__page(CORE_SEG);
LS void parse_315(char *, char *)				__page(CORE_SEG);
LS void parse_317(char *, char *)				__page(CORE_SEG);
LS void parse_318(char *, char *)				__page(CORE_SEG);
LS void parse_319(char *, char *)				__page(CORE_SEG);
LS void parse_324(char *, char *)				__page(CORE_SEG);
LS void parse_352(char *, char *)				__page(CORE_SEG);
LS void parse_367(char *, char *)				__page(CORE_SEG);
LS void parse_376(char *, char *)				__page(CORE_SEG);
LS void parse_401(char *, char *)				__page(CMD1_SEG);	/* no such nick/channel */
LS void parse_433(char *, char *)				__page(CORE_SEG);
LS void parse_451(char *, char *)				__page(CORE_SEG);
LS void parse_471(char *, char *)				__page(CORE_SEG);
LS void parse_473(char *, char *)				__page(CORE_SEG);
LS void parse_dcc(Client *)					__page(CORE_SEG);
LS void parse_error(char *, char *)				__page(CORE_SEG);
LS void parse_invite(char *, char *)				__page(CMD1_SEG);
LS void parse_join(char *, char *)				__page(CORE_SEG);
LS void parse_mode(char *, char *)				__page(CORE_SEG);
LS void parse_notice(char *, char *)				__page(CORE_SEG);
LS void parse_part(char *, char *)				__page(CORE_SEG);
LS void parse_ping(char *, char *)				__page(CORE_SEG);
#ifdef URLCAPTURE
LS void urlcapture(const char *)				__page(CORE_SEG);
#endif /* URLCAPTURE */
LS void parse_privmsg(char *, char *)				__page(CORE_SEG);
LS void parse_quit(char *, char *)				__page(CORE_SEG);
LS void parse_topic(char *, char *)				__page(CMD1_SEG);
LS void parse_wallops(char *, char *)				__page(CORE_SEG);
LS void parse_server_input(void)				__page(CORE_SEG);
LS void parseline(char *)					__page(CORE_SEG);
LS void partyline_broadcast(Client *, char *, char *)		__page(CORE_SEG);
LS void print_help(char *, char *, int)				__page(CMD1_SEG);
LS void process_dcc(void)					__page(CORE_SEG);
LS void prot_action(Chan *, char *, ChanUser *, char *, ChanUser *) __page(CORE_SEG);
LS void purge_banlist(Chan *)					__page(CORE_SEG);
LS void purge_chanusers(Chan *)					__page(CORE_SEG);
LS void purge_shitlist(void)					__page(RARE_SEG);	/* rare */
LS void purge_kicklist(void)					__page(RARE_SEG);	/* rare */
LS void push_kicks(Chan *)					__page(CORE_SEG);
LS void push_modes(Chan *, int)					__page(CORE_SEG);
LS void readcfgfile(void)					__page(INIT_SEG);	/* INIT */
LS void readline(int s, int (*callback)(char *))		__page(CORE_SEG);
LS void register_with_server(void)				__page(CORE_SEG);
LS void remove_auth(Auth *)					__page(CORE_SEG);
LS void remove_chan(Chan *)					__page(CORE_SEG);
LS void remove_chanuser(Chan *, char *)				__attr(CORE_SEG, __regparm (2) );
LS void remove_kicksay(KickSay *)				__page(CMD1_SEG);
LS void remove_ks(KillSock *)					__page(CORE_SEG);
LS void remove_shit(Shit *)					__page(CMD1_SEG);
LS void remove_user(User *)					__page(CMD1_SEG);
LS void reset_userlink(User *, User *)				__page(CMD1_SEG);
LS void reverse_topic(Chan *, char *, char *)			__page(CORE_SEG);
LS void screwban_format(char *)					__page(CMD1_SEG);
LS void send_global(const char *, const char *, ...)		__attr(CORE_SEG, format (printf, 2, 3) );
LS void send_kick(Chan *, const char *, const char *, ...)	__attr(CORE_SEG, format (printf, 3, 4) );
LS void send_mode(Chan *, int, int, char, char, void *)		__page(CORE_SEG);
LS void send_pa(int, const char *, const char *, ...)		__page(CORE_SEG);
LS void send_spy(const char *, const char *, ...)		__attr(CORE_SEG, format (printf, 2, 3) );
LS void send_uptime(int)					__page(CORE_SEG);
LS void set_binarydefault(UniVar *)				__page(CFG1_SEG);
LS void set_str_varc(Chan *, int, char *)			__page(CFG1_SEG);
LS void setbotnick(Mech *, char *)				__page(CORE_SEG);
LS void shit_action(Chan *, ChanUser *)				__page(CORE_SEG);
LS void signoff(char *, char *)					__page(RARE_SEG);	/* rare */
LS void spy_typecount(Mech *)					__page(CMD1_SEG);
LS void to_server(char *, ...)					__attr(CORE_SEG, format (printf, 1, 2) );
LS void to_user(const char *, const char *, ...)		__attr(CORE_SEG, format (printf, 2, 3) );
LS void to_user_q(const char *, const char *, ...)		__attr(CORE_SEG, format (printf, 2, 3) );
LS void unchop(char *, char *)					__attr(CORE_SEG, __regparm (2) );
LS void unmode_chanuser(Chan *, ChanUser *)			__page(CORE_SEG);
LS void update(SequenceTime *)					__page(CORE_SEG);
LS void update_modes(Chan *)					__page(CORE_SEG);
LS void usage(char *)						__attr(CMD1_SEG, __regparm (1) );
LS void usage_command(char *, const char *)			__page(CMD1_SEG);
LS void user_sync(void)						__page(CFG1_SEG);
LS void whom_printbot(char *, BotInfo *, char *)		__page(CMD1_SEG);

/*
 *  signals
 */
LS int sig_hup_callback(char *)					__page(RARE_SEG);
LS void do_sighup(void)						__page(CMD1_SEG);
LS void do_sigint(void)						__page(RARE_SEG);
LS void do_sigusr1(void)					__page(CMD1_SEG);
LS void sig_alrm(int)						__page(RARE_SEG);
LS void sig_child(int)						__page(RARE_SEG);
LS void sig_bus(int)						__page(CMD1_SEG);
LS void sig_hup(int)						__page(RARE_SEG);
LS void sig_int(int)						__page(RARE_SEG);
LS void sig_pipe(int)						__page(CORE_SEG);
LS void sig_segv(int)						__page(RARE_SEG);
LS void sig_term(int)						__page(RARE_SEG);
LS void sig_usr1(int)						__page(CMD1_SEG);
LS void sig_usr2(int)						__page(DBUG_SEG);		/* DEBUG */
LS void sig_suicide()						__attr(RARE_SEG, __noreturn__);

/*
 *  BOTNET prototypes
 */
#ifdef BOTNET

LS BotInfo *make_botinfo(int, int, char *, char *, char *)	__page(CORE_SEG);
LS ChanUser *find_chanbot(Chan *, char *)			__page(CORE_SEG);
LS Mech *get_netbot(void)					__page(CORE_SEG);
LS NetCfg *find_netcfg(int)					__page(CORE_SEG);
LS int connect_to_bot(NetCfg *cfg)				__page(CORE_SEG);
LS void basicAuth(BotNet *, char *)				__page(CORE_SEG);
LS void basicAuthOK(BotNet *, char *)				__page(CORE_SEG);
LS void basicBanner(BotNet *, char *)				__page(CORE_SEG);
LS void basicLink(BotNet *, char *)				__page(CORE_SEG);
LS void basicQuit(BotNet *, char *)				__page(CORE_SEG);
LS void netchanNeedop(BotNet *, char *)				__page(CORE_SEG);
LS void partyAuth(BotNet *, char *)				__page(CORE_SEG);
LS void partyCommand(BotNet *, char *)				__page(CORE_SEG);
LS void partyMessage(BotNet *, char *)				__page(CORE_SEG);
LS void ushareDelete(BotNet *, char *)				__page(CMD1_SEG);
LS void ushareTick(BotNet *, char *)				__page(CORE_SEG);
LS void ushareUser(BotNet *, char *)				__page(CORE_SEG);
LS void botnet_binfo_relay(BotNet *, BotInfo *)			__attr(CORE_SEG, __regparm (2) );
LS void botnet_binfo_tofile(int, BotInfo *)			__attr(CORE_SEG, __regparm (2) );
LS void botnet_deaduplink(BotNet *)				__page(CORE_SEG);
LS void botnet_dumplinklist(BotNet *)				__page(CORE_SEG);
LS void botnet_newsock(void)					__page(CORE_SEG);
LS void botnet_parse(BotNet *, char *)				__page(CORE_SEG);
LS void botnet_refreshbotinfo(void)				__page(CORE_SEG);
LS void botnet_relay(BotNet *, char *, ...)			__page(CORE_SEG);
LS void check_botinfo(BotInfo *, const char *)			__page(CORE_SEG);
LS void check_botjoin(Chan *, ChanUser *)			__page(CORE_SEG);
LS void select_botnet(void)					__page(CORE_SEG);
LS void process_botnet(void)					__page(CORE_SEG);
LS void reset_linkable(int)					__page(CORE_SEG);

#endif /* BOTNET */

/*
 *  bounce baby bounce!
 */
#ifdef BOUNCE

LS void new_port_bounce(const struct Setting *)			__page(RARE_SEG);
LS void bounce_cleanup(void)					__page(RARE_SEG);	/* rare */
LS void bounce_parse(ircLink *, char *)				__page(CORE_SEG);
LS void process_bounce(void)					__page(CORE_SEG);
LS void select_bounce(void)					__page(CORE_SEG);

#endif /* BOUNCE */

/*
 *
 */
#ifdef CHANBAN

LS void do_chanban(COMMAND_ARGS)				__page(CMD1_SEG);
LS void process_chanbans(void)					__page(CORE_SEG);
LS void chanban_action(char *, char *, Shit *)			__page(CORE_SEG);

#endif /* CHANBAN */

/*
 *
 */
#ifdef CTCP

LS void ctcp_finger(char *, char *, char *)			__page(CORE_SEG);
LS void ctcp_ping(char *, char *, char *)			__page(CORE_SEG);
LS void ctcp_version(char *, char *, char *)			__page(CORE_SEG);

#endif /* CTCP */

/*
 *
 */
#ifdef DEBUG

LS char *atime(time_t)						__page(DBUG_SEG);
LS const char *proc_lookup(void *, int)				__page(DBUG_SEG);
LS const char *strdef(const DEFstruct *, int)			__page(DBUG_SEG);
LS void run_debug(void)						__page(DBUG_SEG);
LS int wrap_debug(void)						__page(DBUG_SEG);
LS void debug_botinfo(BotInfo *)				__page(DBUG_SEG);
LS void debug_botnet(void)					__page(DBUG_SEG);
LS void debug_core(void)					__page(DBUG_SEG);
LS void debug_memory(void)					__page(DBUG_SEG);
LS void debug_server(Server *, char *)				__page(DBUG_SEG);
LS void debug_settings(UniVar *, int)				__page(DBUG_SEG);
LS void memreset(void)						__page(DBUG_SEG);
LS void memtouch(void *)					__page(DBUG_SEG);
LS char *ulong2bin(int, ulong)					__page(DBUG_SEG);

#endif /* DEBUG */

#ifdef DCC_FILE

LS int dcc_sendfile(char *, char *)				__page(CMD1_SEG);
LS void do_send(COMMAND_ARGS)					__page(CMD1_SEG);

#endif /* DCC_FILE */

/*
 *
 */
#ifdef DYNAMODE

LS void check_dynamode(Chan *)					__page(CORE_SEG);

#endif /* DYNAMODE */

#ifdef HOSTINFO

LS void do_hostinfo(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_meminfo(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_cpuinfo(COMMAND_ARGS)				__page(CMD1_SEG);

#endif /* HOSTINFO */
/*
 *
 */
#ifdef IDWRAP

LS void unlink_identfile(void)					__page(CORE_SEG);

#endif /* IDWRAP */

/*
 *
 */
#ifdef NOTE

LS int catch_note(char *, char *, char *)			__page(CMD1_SEG);

#endif /* NOTE */

/*
 *
 */
#ifdef NOTIFY

LS int mask_check(Notify *, char *)				__page(CORE_SEG);
LS int notify_callback(char *)					__page(CMD1_SEG);
LS int notifylog_callback(char *)				__page(CFG1_SEG);
LS int read_notify(char *)					__page(CFG1_SEG);
LS void catch_ison(char *)					__page(CORE_SEG);
LS void catch_whois(char *, char *, char *)			__page(CORE_SEG);
LS void nfshow_brief(Notify *)					__page(CMD1_SEG);
LS void nfshow_full(Notify *)					__page(CMD1_SEG);
LS void parse_303(char *, char *)				__page(CORE_SEG);
LS void purge_notify(void)					__page(CFG1_SEG);
LS void read_notifylog(void)					__page(CFG1_SEG);
LS void send_ison(void)						__page(CORE_SEG);
LS void write_notifylog(void)					__page(CORE_SEG);

#endif /* NOTIFY */

/*
 * Python
 */
#ifdef PYTHON
#if defined(DEBUG_C) || defined(MAIN_C) || defined(PYTHON_C)

LS void do_python(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_pythonscript(COMMAND_ARGS)				__page(CMD1_SEG);
LS int python_parse_jump(char *, char *, Hook *)		__page(CORE_SEG);
LS int python_timer_jump(Hook *)				__page(CORE_SEG);
LS void python_dcc_complete(Client *, int)			__page(CORE_SEG);
LS PyObject *python_hook(PyObject *, PyObject *, PyObject *)	__page(CMD1_SEG);
LS PyObject *python_unhook(PyObject *, PyObject *, PyObject *)	__page(CMD1_SEG);
LS PyObject *python_to_server(PyObject *, PyObject *, PyObject *) __page(CORE_SEG);
LS PyObject *python_to_file(PyObject *, PyObject *, PyObject *)	__page(CORE_SEG);
LS PyObject *python_userlevel(PyObject *, PyObject *, PyObject *) __page(CORE_SEG);
LS PyObject *python_debug(PyObject *, PyObject *)		__page(DBUG_SEG);
LS void init_python(void)					__page(CFG1_SEG);
LS void free_python(void)					__page(CFG1_SEG);

#endif /* defined */
#endif /* PYTHON */

/*
 *
 */
#ifdef RAWDNS

LS void do_dns(COMMAND_ARGS)					__page(CMD1_SEG);
LS void init_rawdns(void)					__page(INIT_SEG);	/* INIT */
LS void process_rawdns(void)					__page(CORE_SEG);
LS void rawdns(const char *)					__page(CORE_SEG);
LS void select_rawdns(void)					__page(CORE_SEG);
LS char *poll_rawdns(char *)					__page(CORE_SEG);
LS void parse_query(int, dnsQuery *)				__page(CORE_SEG);
LS void do_dnsserver(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_dnsroot(COMMAND_ARGS)				__page(CMD1_SEG);
LS int read_dnsroot(char *)					__page(CFG1_SEG);
LS ulong rawdns_get_ip(const char *)				__page(CORE_SEG);

#endif /* RAWDNS */

/*
 *  fun with pipes (aka, REDIRECTS)
 */
#ifdef REDIRECT

LS int begin_redirect(char *, char *)				__page(CORE_SEG);
LS void send_redirect(char *)					__page(CMD1_SEG);
LS void end_redirect(void)					__page(CORE_SEG);

#endif /* REDIRECT */

/*
 *  RESET recover is not a defined feature?
 */
LS char *recover_client(char *)					__page(INIT_SEG);	/* INIT */
LS char *recover_debug(char *)					__page(INIT_SEG);	/* INIT */
LS char *recover_server(char *)					__page(INIT_SEG);	/* INIT */
LS void recover_reset(void)					__page(INIT_SEG);	/* INIT */

/*
 *
 */
#ifdef SEEN

LS void make_seen(char *, char *, char *, char *, time_t, int)	__page(CORE_SEG);	/* SEEN */

#endif /* SEEN */

/*
 *
 */
#ifdef STATS

LS void stats_loghour(Chan *, char *, int)			__page(CORE_SEG);
LS void stats_plusminususer(Chan *, int)			__page(CORE_SEG);
LS void do_info(COMMAND_ARGS)					__page(CMD1_SEG);

#endif /* STATS */

/*
 *  Scripts-R-Us
 */
#ifdef TCL

LS void do_tcl(COMMAND_ARGS)					__page(CMD1_SEG);	/* TCL + PLEASE_HACK_MY_SHELL */
LS char *tcl_var_read()						__page(CORE_SEG);
LS char *tcl_var_write()					__page(CORE_SEG);
LS int tcl_parse_jump()						__page(CORE_SEG);
LS int tcl_timer_jump()						__page(CORE_SEG);
LS void tcl_dcc_complete(Client *, int)				__page(CORE_SEG);
LS int tcl_hook()						__page(CMD1_SEG);
LS int tcl_unhook()						__page(CMD1_SEG);
LS int tcl_to_server()						__page(CORE_SEG);
LS int tcl_to_file()						__page(CORE_SEG);
LS int tcl_userlevel()						__page(CORE_SEG);
LS int tcl_debug()						__page(DBUG_SEG);
LS void init_tcl(void)						__page(CFG1_SEG);

#endif /* TCL */

#ifdef TELNET

int check_telnet(int, char *)					__page(CMD1_SEG);
void check_telnet_pass(Client *, char *)			__page(CMD1_SEG);

#endif /* TELNET */

#ifdef TOYBOX

LS int read_bigcharset(char *)					__page(CMD1_SEG);
LS int read_bigcharset_callback(char *)				__page(CMD1_SEG);

#endif /* TOYBOX */

#ifdef TRIVIA

LS char *random_question(char *)				__page(CORE_SEG);
LS int trivia_score_callback(char *)				__page(CMD1_SEG);
LS void hint_one(void)						__page(CMD1_SEG);
LS void hint_three(void)					__page(CMD1_SEG);
LS void hint_two(void)						__page(CMD1_SEG);
LS void read_triviascore(void)					__page(CMD1_SEG);
LS void trivia_check(Chan *, char *)				__page(CORE_SEG);
LS void trivia_cleanup(void)					__page(CORE_SEG);
LS void trivia_no_answer(void)					__page(CORE_SEG);
LS void trivia_question(void)					__page(CORE_SEG);
LS void trivia_tick(void)					__page(CORE_SEG);
LS void trivia_week_toppers(void)				__page(CORE_SEG);
LS void write_triviascore(void)					__page(CMD1_SEG);

#endif /* TRIVIA */

/*
 *  UPTIME prototypes
 */
#ifdef UPTIME

LS void init_uptime(void)					__page(INIT_SEG);	/* INIT */
LS void process_uptime(void)					__page(CORE_SEG);
LS void uptime_death(int)					__page(RARE_SEG);	/* rare */

#endif /* UPTIME */

#ifdef URLCAPTURE

LS void urlcapture(const char *)				__page(CORE_SEG);
LS void do_urlhist(COMMAND_ARGS)				__page(CMD1_SEG);

#endif /* ifdef URLCAPTURE */

/*
 *  WEB prototypes
 */
#ifdef WEB

LS char *webread(int, char *, char *)				__page(CORE_SEG);
LS void eml_fmt(WebSock *, char *)				__page(CORE_SEG);
LS void select_web(void)					__page(CORE_SEG);
LS void parse(WebSock *, char *)				__page(CORE_SEG);
LS void process_web(void)					__page(CORE_SEG);
LS void web_404(WebSock *, char *)				__page(CORE_SEG);
LS void web_raw(WebSock *, char *)				__page(CORE_SEG);
LS void web_botstatus(WebSock *, char *)			__page(CORE_SEG);
LS void web_debug(WebSock *, char *)				__page(DBUG_SEG);

#endif /* WEB */

#endif /* H_H */
