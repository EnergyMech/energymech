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
#ifndef H_H
#define H_H 1

#define ischannel(x)		(*x == '#')

#define nullstr(x)		((x)) ? (x) : NULLSTR
#define nullbuf(x)		(x && *x) ? x : NULLSTR

#define chkhigh(x)		if (x > hisock) { hisock = x; }

#define COMMAND_ARGS		char *from, const char *to, char *rest, const int cmdaccess

#define STRCHR			stringchr
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

/* __x86_64__ automatically compiles for regparm optimization */
#if !defined(__profiling__) && defined(__i386__)
# define __regparm(x)		regparm(x)
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

/* alias.c */

LS void afmt(char *, const char *, const char *)		__page(CORE_SEG);
LS void do_alias(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_unalias(COMMAND_ARGS)				__page(CMD1_SEG);

/* auth.c */

LS char *cipher(char *);
LS char *makepass(char *);
LS int passmatch(char *, char *);
LS void delete_auth(char *);
LS void remove_auth(Auth *);
LS void change_authnick(char *, char *);
LS void aucheck(User *);
LS User *get_authuser(const char *, const char *)		__page(CORE_SEG);
LS int get_authaccess(const char *, const char *)		__page(CORE_SEG);
LS int make_auth(const char *, const User *);
LS void do_auth(COMMAND_ARGS)					__page(CMD1_SEG);

/* bounce.c */

void bounce_parse(ircLink *, char *);
void bounce_cleanup(void);
void new_port_bounce(const struct Setting *);
void select_bounce(void);
void process_bounce(void);

/* calc.c */

LS void do_convert(COMMAND_ARGS)				__page(CMD1_SEG);

/* channel.c */

void check_idlekick(void);
LS Chan *find_channel(const char *, int)			__attr(CORE_SEG, __regparm(2));
LS Chan *find_channel_ac(const char *)				__attr(CORE_SEG, __regparm(1));
LS Chan *find_channel_ny(const char *)				__attr(CORE_SEG, __regparm(1));
void remove_chan(Chan *);
void join_channel(char *, char *);
void reverse_topic(Chan *, char *, char *);
void cycle_channel(Chan *);
int reverse_mode(char *, Chan *, int, int);
void chan_modestr(Chan *, char *);
char *find_nuh(char *);
Ban *make_ban(Ban **, char *, char *, time_t);
void delete_ban(Chan *, char *);
void delete_modemask(Chan *, char *, int);
void channel_massmode(const Chan *, char *, int, char, char);
void channel_massunban(Chan *, char *, time_t);
LS ChanUser *find_chanuser(Chan *, const char *);
LS ChanUser *find_chanbot(Chan *, const char *);
LS void remove_chanuser(Chan *, const char *);
LS void make_chanuser(char *, char *);
LS void purge_chanusers(Chan *);
LS char *get_nuh(const ChanUser *);
LS void do_join(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_part(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_cycle(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_forget(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_channels(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_wall(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_mode(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_names(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_cchan(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_invite(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_sayme(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_who(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_topic(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_showidle(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_idle(COMMAND_ARGS)					__page(CMD1_SEG);

/* core.c */

void unlink_identfile(void);
int conf_callback(char *line);
void readcfgfile(void);
int write_session(void);
void setbotnick(Mech *bot, char *nick);
Mech *add_bot(int guid, char *nick);
void signoff(char *from, char *reason);
void kill_all_bots(char *reason)					__attr(RARE_SEG, __noreturn__);;
Server *add_server(char *host, int port, char *pass);
ServerGroup *getservergroup(const char *name);
ServerGroup *getservergroupid(int id);
Server *find_server(int id);
int try_server(Server *sp, char *hostname);
void connect_to_server(void);
void register_with_server(void);
int sub_compile_timer(int, uint32_t *, uint32_t *, char *);
int compile_timer(HookTimer *, char *);
void update(SequenceTime *this);
void process_server_input(void);
void do_version(COMMAND_ARGS)					__page(CMD1_SEG);
void do_core(COMMAND_ARGS)					__page(CMD1_SEG);
void do_die(COMMAND_ARGS)					__page(RARE_SEG);
void do_shutdown(COMMAND_ARGS)					__page(RARE_SEG);
void do_servergroup(COMMAND_ARGS)				__page(CMD1_SEG);
void do_server(COMMAND_ARGS)					__page(CMD1_SEG);
void do_cserv(COMMAND_ARGS)					__page(CMD1_SEG);
void do_away(COMMAND_ARGS)					__page(CMD1_SEG);
void do_do(COMMAND_ARGS)					__page(CMD1_SEG);
void do_nick(COMMAND_ARGS)					__page(CMD1_SEG);
void do_time(COMMAND_ARGS)					__page(CMD1_SEG);
void do_upontime(COMMAND_ARGS)					__page(CMD1_SEG);
void do_msg(COMMAND_ARGS)					__page(CMD1_SEG);

/* ctcp.c */

Client *find_client(const char *nick);
void delete_client(Client *client);
int dcc_sendfile(char *target, char *filename);
void dcc_pushfile(Client *client, off_t where);
int dcc_freeslots(int uaccess);
void parse_dcc(Client *client);
void process_dcc(void);
void ctcp_dcc(char *from, char *to, char *rest);
void ctcp_finger(char *from, char *to, char *rest);
void ctcp_ping(char *from, char *to, char *rest);
void ctcp_version(char *from, char *to, char *rest);
void on_ctcp(char *from, char *to, char *rest);
void do_ping_ctcp(COMMAND_ARGS)					__page(CMD1_SEG);
void do_send(COMMAND_ARGS)					__page(CMD1_SEG);

/* debug.c */

void strflags(char *dst, const DEFstruct *flagsstruct, int flags);
const char *strdef(const DEFstruct *dtab, int num);
const char *funcdef(const DEFstruct *dtab, void *func);
void memreset(void);
void memtouch(const void *addr);
const char *proc_lookup(void *addr, int size);
char *atime(time_t when);
void debug_server(Server *sp, char *pad);
void debug_settings(UniVar *setting, int type);
void debug_memory(void);
void debug_botinfo(BotInfo *binfo);
void debug_botnet(void);
void debug_core(void);
void debug_rawdns(void);
char *uint32tobin(int limit, uint32_t x);
void debug_scripthook(void);
void run_debug(void);
int wrap_debug(void);
void do_debug(COMMAND_ARGS)					__page(CMD1_SEG);
void do_crash(COMMAND_ARGS)					__page(RARE_SEG);
void debug(char *format, ...);

/* dns.c */

void init_rawdns(void);
struct in_addr dnsroot_lookup(const char *hostname);
const char *get_dns_token(const char *src, const char *packet, char *dst, int sz);
int make_query(char *packet, const char *hostname);
struct in_addr *get_stored_ip(const char *ipdata);
void dns_hook(char *host, char * resolved);
void parse_query(int psz, dnsQuery *query);
void rawdns(const char *hostname);
void select_rawdns(void);
void process_rawdns(void);
char *poll_rawdns(char *hostname);
int read_dnsroot(char *line);
uint32_t rawdns_get_ip(const char *host);
void do_dnsroot(COMMAND_ARGS)					__page(CMD1_SEG);
void do_dnsserver(COMMAND_ARGS)					__page(CMD1_SEG);
void do_dns(COMMAND_ARGS)					__page(CMD1_SEG);

/* dynamode.c */
/* function.c */

LS void *Calloc(int)						__attr(CORE_SEG, __regparm(1));
LS void Free(char **)						__attr(CORE_SEG, __regparm(1));
LS Strp *make_strp(Strp **, const char *)			__attr(CORE_SEG, __regparm(2));
LS Strp *append_strp(Strp **, const char *)			__attr(CORE_SEG, __regparm(2));
LS Strp *prepend_strp(Strp **, const char *)			__attr(CORE_SEG, __regparm(2));
LS void purge_linklist(void **)					__attr(CORE_SEG, __regparm(1));
LS void dupe_strp(Strp *, Strp **)				__attr(CORE_SEG, __regparm(2));
LS const int StrlenX(const char *, ...)				__attr(CORE_SEG, __regparm(1));
LS const int Strlen2(const char *, const char *)		__attr(CORE_SEG, __regparm(2));
LS char *getuh(char *)						__page(CORE_SEG);
LS char *get_token(char **, const char *)			__page(CORE_SEG);
LS char *logtime(time_t)					__page(CORE_SEG);
LS char *time2str(time_t)					__page(CORE_SEG);
LS char *time2away(time_t)					__page(CORE_SEG);
LS char *time2medium(time_t)					__page(CORE_SEG);
LS char *time2small(time_t)					__page(CORE_SEG);
LS char *idle2str(time_t, int)					__page(CORE_SEG);
LS const char *get_channel(const char *, char **)		__page(CORE_SEG);
LS const char *get_channel2(const char *, char **)		__page(CORE_SEG);
LS char *cluster(char *)					__page(CORE_SEG);
LS char *format_uh(char *, int)					__page(CORE_SEG);
LS char *nick2uh(char *, char *)				__page(CORE_SEG);
LS void deop_ban(Chan *, ChanUser *, char *)			__page(CORE_SEG);
LS void deop_siteban(Chan *, ChanUser *)			__page(CORE_SEG);
LS void screwban_format(char *)					__page(CORE_SEG);
LS void deop_screwban(Chan *, ChanUser *)			__page(CORE_SEG);
LS int is_nick(const char *)					__attr(CORE_SEG, __regparm(1));
LS int asc2int(const char *)					__attr(CORE_SEG, __regparm(1));
LS int get_number(const char *)					__attr(CORE_SEG, __regparm(1));
LS void fix_config_line(char *)					__page(CFG1_SEG);
LS int matches(const char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS int num_matches(const char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS void table_buffer(const char *, ...)				__page(CMD1_SEG);
LS void table_send(const char *, const int)			__page(CMD1_SEG);
LS int is_safepath(const char *, int)				__attr(CORE_SEG, __regparm(2));

/* greet.c */

void greet(void);
void do_greet(COMMAND_ARGS)					__page(CMD1_SEG);

/* help.c */

void print_help(char *from, char *line, int len);
int do_help_callback(char *line);
void usage_command(char *to, const char *arg);
void usage(char *to);
void do_help(COMMAND_ARGS)					__page(CMD1_SEG);
void do_usage(COMMAND_ARGS)					__page(CMD1_SEG);

/* hostinfo.c */

int monitor_fs(const char *);
void select_monitor();
void process_monitor();
int parse_proc_status(char *line)				__page(CMD1_SEG);
int parse_proc_cpuinfo(char *line)				__page(CMD1_SEG);
void do_hostinfo(COMMAND_ARGS)					__page(CMD1_SEG);
void do_meminfo(COMMAND_ARGS)					__page(CMD1_SEG);
void do_cpuinfo(COMMAND_ARGS)					__page(CMD1_SEG);
void do_filemon(COMMAND_ARGS)					__page(CMD1_SEG);

/* io.c */

LS uint32_t get_ip(const char *)					__page(CORE_SEG);
LS void SockFlags(int)							__page(CORE_SEG);
LS int SockOpts(void)							__page(CORE_SEG);
LS int SockListener(int)						__page(CORE_SEG);
LS int SockConnect(char *, int, int)					__page(CORE_SEG);
LS int SockAccept(int)							__page(CORE_SEG);
int to_file(int sock, const char *format, ...);
void to_server(char *format, ...);
void to_user_q(const char *, const char *, ...);
void to_user(const char *, const char *, ...);
char *sockread(int, char *, char *);
void readline(int, int (*)(char *));
void remove_ks(KillSock *);
int killsock(int);
LS void do_clearqueue(COMMAND_ARGS)					__page(CMD1_SEG);

/* irc.c */

LS void make_ireq(int, const char *, const char *)		__page(CMD1_SEG);
void send_pa(int type, const char *nick, const char *format, ...);
void do_irclusers(COMMAND_ARGS)					__page(CMD1_SEG);
void do_ircstats(COMMAND_ARGS)					__page(CMD1_SEG);
void do_ircwhois(COMMAND_ARGS)					__page(CMD1_SEG);

/* kicksay.c */

KickSay *find_kicksay(char *text, char *channel);
void check_kicksay(Chan *chan, ChanUser *doer, char *text);
void remove_kicksay(KickSay *kick);
void purge_kicklist(void);
void do_kicksay(COMMAND_ARGS)					__page(CMD1_SEG);
void do_rkicksay(COMMAND_ARGS)					__page(CMD1_SEG);

/* lib/string.c */

LS char *chop(char **)						__attr(CORE_SEG, __regparm(1));
LS void unchop(char *, const char *)				__attr(CORE_SEG, __regparm(2));
LS int stringcasecmp(const char *, const char *)		__attr(CORE_SEG, __regparm(2));
LS int stringcmp(const char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS int nickcmp(const char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS char *nickcpy(char *, const char *)				__attr(CORE_SEG, __regparm(2));
LS void stringcpy_n(char *, const char *, int)			__attr(CORE_SEG, __regparm(3));
LS char *stringcpy(char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS char *stringchr(const char *, int)				__attr(CORE_SEG, __regparm(2));
LS char *stringdup(const char *)				__attr(CORE_SEG, __regparm(1));
LS char *stringcat(char *, const char *)			__attr(CORE_SEG, __regparm(2));
LS char *tolowercat(char *, const char *)			__attr(CORE_SEG, __regparm(2));

/* main.c */

void mech_exec(void);
int randstring_count(char *line);
int randstring_getline(char *line);
LS char *randstring(const char *)				__page(CORE_SEG);
LS int sig_hup_callback(char *)					__page(RARE_SEG);		/* rare */
LS void do_sighup(void)						__page(CMD1_SEG);
LS void sig_hup(int)						__page(RARE_SEG);		/* rare */
LS void sig_child(int)						__page(RARE_SEG);		/* rare */
LS void sig_alrm(int)						__page(RARE_SEG);		/* rare */
LS void sig_pipe(int)						__page(CORE_SEG);
LS void do_sigusr1(void)					__page(CMD1_SEG);
LS void sig_usr1(int)						__page(CMD1_SEG);
LS void sig_usr2(int)						__page(DBUG_SEG);		/* DEBUG */
LS void sig_suicide()						__attr(RARE_SEG, __noreturn__);	/* rare */
LS void do_sigint(void)						__page(RARE_SEG);		/* rare */
LS void sig_int(int)						__page(RARE_SEG);		/* rare */
LS void sig_ill(int)						__page(RARE_SEG);
LS void sig_abrt(int)						__page(RARE_SEG);
LS void sig_bus(int)						__page(CMD1_SEG);
#if defined(__linux__) && defined(__x86_64__) && defined(DEBUG) && !defined(__STRICT_ANSI__)
LS void sig_segv(int, siginfo_t *, void *)			__attr(RARE_SEG, __noreturn__);
#else
LS void sig_segv(int)						__attr(RARE_SEG, __noreturn__);
#endif
LS void sig_term(int)						__attr(RARE_SEG, __noreturn__);	/* rare */
LS void doit(void);
LS int main(int argc, char **argv, char **envp);

/* net.c */

Mech *get_netbot(void);
void reset_linkable(int guid);
void botnet_deaduplink(BotNet *bn);
NetCfg *find_netcfg(int guid);
BotInfo *make_botinfo(int guid, int hops, char *nuh, char *server, char *version);
void botnet_relay(BotNet *source, char *format, ...);
void botnet_refreshbotinfo(void);
void botnet_binfo_relay(BotNet *source, BotInfo *binfo);
void botnet_binfo_tofile(int sock, BotInfo *binfo);
void botnet_dumplinklist(BotNet *bn);
int connect_to_bot(NetCfg *cfg);
LS void check_botjoin(Chan *chan, ChanUser *cu);
LS void check_botinfo(BotInfo *binfo, const char *channel);
void basicAuth(BotNet *bn, char *rest);
void basicAuthOK(BotNet *bn, char *rest);
void basicBanner(BotNet *bn, char *rest);
void basicLink(BotNet *bn, char *version);
void basicQuit(BotNet *bn, char *rest);
LS void netchanNeedop(BotNet *source, char *rest);
LS void netchanSuppress(BotNet *, char *)			__page(CORE_SEG);
void partyAuth(BotNet *bn, char *rest);
int commandlocal(int dg, int sg, char *from, char *command);
void partyCommand(BotNet *bn, char *rest);
void partyMessage(BotNet *bn, char *rest);
void ushareUser(BotNet *bn, char *rest);
void ushareTick(BotNet *bn, char *rest);
void ushareDelete(BotNet *bn, char *rest);
void parse_botnet(BotNet *bn, char *rest);
void botnet_newsock(void);
void select_botnet(void);
void process_botnet(void);
void do_link(COMMAND_ARGS)					__page(CMD1_SEG);
void do_cmd(COMMAND_ARGS)					__page(CMD1_SEG);

/* note.c */

int catch_note(char *from, char *to, char *rest);
void do_note(COMMAND_ARGS)					__page(CMD1_SEG);
void do_read(COMMAND_ARGS)					__page(CMD1_SEG);

/* notify.c */

void purge_notify(void);
int mask_check(Notify *nf, char *userhost);
void send_ison(void);
void catch_ison(char *rest);
void catch_whois(char *nick, char *userhost, char *realname);
int notifylog_callback(char *rest);
void read_notifylog(void);
void write_notifylog(void);
int notify_callback(char *rest);
int read_notify(char *filename);
void nfshow_brief(Notify *nf);
void nfshow_full(Notify *nf);
void sub_notifynick(char *from, char *rest);
void do_notify(COMMAND_ARGS)					__page(CMD1_SEG);

/* ons.c */

LS uint32_t makecrc(const char *)				__page(CORE_SEG);
LS void send_suppress(const char *, const char *)		__page(CORE_SEG);
LS void on_kick(char *from, char *rest);
LS void on_join(Chan *chan, char *from);
LS void on_nick(char *from, char *newnick);
LS void on_msg(char *from, char *to, char *rest);
LS void on_mode(char *from, char *channel, char *rest);
LS void common_public(Chan *chan, char *from, char *spyformat, char *rest);
LS void on_action(char *from, char *to, char *rest);
LS int access_needed(char *name);
LS void do_chaccess(COMMAND_ARGS)				__page(CMD1_SEG);
LS void do_last(COMMAND_ARGS)					__page(CMD1_SEG);

/* parse.c */

LS void parse_error(char *from, char *rest);
LS void parse_invite(char *from, char *rest);
LS void parse_join(char *from, char *rest);
LS void parse_mode(char *from, char *rest);
LS void parse_notice(char *from, char *rest);
LS void parse_part(char *from, char *rest);
LS void parse_ping(char *from, char *rest);
LS void parse_pong(char *from, char *rest);
LS void parse_privmsg(char *from, char *rest);
LS void parse_quit(char *from, char *rest);
LS void parse_topic(char *from, char *rest);
LS void parse_wallops(char *from, char *rest);
LS void parse_213(char *from, char *rest);
LS void parse_219(char *from, char *rest);
LS void parse_251(char *from, char *rest);
LS void parse_252(char *from, char *rest);
LS void parse_253(char *from, char *rest);
LS void parse_254(char *from, char *rest);
LS void parse_255(char *from, char *rest);
LS void parse_301(char *from, char *rest);
LS void parse_303(char *from, char *rest);
LS void parse_311(char *from, char *rest);
LS void parse_312(char *from, char *rest);
LS void parse_313(char *from, char *rest);
LS void parse_315(char *from, char *rest);
LS void parse_317(char *from, char *rest);
LS void parse_318(char *from, char *rest);
LS void parse_319(char *from, char *rest);
LS void parse_324(char *from, char *rest);
LS void parse_352(char *from, char *rest);
LS void parse_367(char *from, char *rest);
LS void parse_376(char *from, char *rest);
LS void parse_401(char *from, char *rest);
LS void parse_433(char *from, char *rest);
LS void parse_451(char *from, char *rest);
LS void parse_471(char *from, char *rest);
LS void parse_473(char *from, char *rest);
LS void parse_346(char *from, char *rest);
LS void parse_348(char *from, char *rest);
LS void parse_368(char *from, char *rest);
LS void parse_005(char *from, char *rest);
LS uint32_t stringhash(char *s);
LS void parse_server_input(char *rest);

/* partyline.c */

LS int check_telnet(int, char *)					__page(CMD1_SEG);
LS void check_telnet_pass(Client *, char *)				__page(CMD1_SEG);
LS int partyline_only_command(const char *)				__page(CMD1_SEG);
LS void partyline_broadcast(const Client *, const char *, const char *) __page(CMD1_SEG);
LS void partyline_banner(Client *)					__page(CMD1_SEG);
LS void dcc_chat(char *)						__page(CMD1_SEG);
LS void whom_printbot(char *, BotInfo *, char *)			__page(CMD1_SEG);
LS void do_whom(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_chat(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_bye(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_boot(COMMAND_ARGS)						__page(CMD1_SEG);

/* perl.c */

LS void do_perl(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_perlscript(COMMAND_ARGS)					__page(CMD1_SEG);

/* prot.c */

LS void send_kick(Chan *chan, const char *nick, const char *format, ...);
LS void push_kicks(Chan *chan);
LS void unmode_chanuser(Chan *chan, ChanUser *cu);
LS void send_mode(Chan *chan, int pri, int type, char plusminus, char modeflag, void *data);
LS int mode_effect(Chan *chan, qMode *mode);
LS void push_modes(Chan *chan, int lowpri);
LS void update_modes(Chan *chan);
LS int check_mass(Chan *chan, ChanUser *doer, int type);
LS void mass_action(Chan *chan, ChanUser *doer);
LS void prot_action(Chan *chan, char *from, ChanUser *doer, char *target, ChanUser *victim);
LS void process_chanbans(void);
LS void chanban_action(char *, char *, Shit *);
LS void check_dynamode(Chan *)						__page(CORE_SEG);
LS void do_opdeopme(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_opvoice(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_kickban(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_unban(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_banlist(COMMAND_ARGS)					__page(CMD1_SEG);

/* python.c */

#ifdef PYTHON

#if defined(DEBUG_C) || defined(MEGA_C)
PyObject *python_hook(PyObject *self, PyObject *args, PyObject *keywds);
PyObject *python_unhook(PyObject *self, PyObject *args, PyObject *keywds);
#endif
/*
char *python_unicode2char(PyUnicodeObject *obj);
PyObject *python_userlevel(PyObject *self, PyObject *args, PyObject *keywds);
PyObject *python_to_server(PyObject *self, PyObject *args, PyObject *keywds);
PyObject *python_to_file(PyObject *self, PyObject *args, PyObject *keywds);
static PyObject *python_dcc_sendfile(PyObject *self, PyObject *args, PyObject *keywds);
PyObject *python_debug(PyObject *self, PyObject *args);
PyMODINIT_FUNC pythonInit(void);
*/
int python_parse_jump(char *, char *, Hook *);
int python_timer_jump(Hook *hook);
void python_dcc_complete(Client *client, int cps);
int python_dns_jump(char *host, char *resolved, Hook *hook);
void init_python(void);
void free_python(void);
void do_python(COMMAND_ARGS)						__page(CMD1_SEG);
void do_pythonscript(COMMAND_ARGS)					__page(CMD1_SEG);

#endif /* PYTHON */

/* reset.c */

char *recover_client(char *env);
char *recover_debug(char *env);
char *recover_server(char *env);
void recover_reset(void);
void do_reset(COMMAND_ARGS)						__page(RARE_SEG);

/* seen.c */

int write_seenlist(void);
int read_seenlist_callback(char *rest);
int read_seenlist(void);
void make_seen(char *nick, char *userhost, char *pa, char *pb, time_t when, int t);
void do_seen(COMMAND_ARGS);

/* shit.c */

void shit_action(Chan *chan, ChanUser *cu);
void check_shit(void);
void remove_shit(Shit *shit);
void purge_shitlist(void);
Shit *add_shit(char *from, char *chan, char *mask, char *reason, int axs, int expire);
Shit *find_shit(const char *userhost, const char *channel);
Shit *get_shituser(char *userhost, char *channel);
int get_shitaction(const char *userhost, const char *chan);
void do_shit(COMMAND_ARGS)						__page(CMD1_SEG);
void do_rshit(COMMAND_ARGS)						__page(CMD1_SEG);
void do_shitlist(COMMAND_ARGS)						__page(CMD1_SEG);
void do_clearshit(COMMAND_ARGS)						__page(CMD1_SEG);

/* spy.c */

void send_spy(const char *src, const char *format, ...);
void send_global(const char *src, const char *format, ...);
void spy_typecount(Mech *bot);
int spy_source(char *from, int *t_src, const char **src);
char *urlhost(const char *);
LS void urlcapture(const char *)					__page(CORE_SEG);
int begin_redirect(char *, char *);
void send_redirect(char *);
void end_redirect(void);
void stats_loghour(Chan *chan, char *filename, int hour);
void stats_plusminususer(Chan *chan, int plusminus);
void do_spy(COMMAND_ARGS)						__page(CMD1_SEG);
void do_rspy(COMMAND_ARGS)						__page(CMD1_SEG);
void do_info(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_urlhist(COMMAND_ARGS)					__page(CMD1_SEG);

/* tcl.c */
#ifdef TCL

/*
LS char *tcl_var_read(Tcl_TVInfo *vinfo, Tcl_Interp *I, char *n1, char *n2, int flags);
LS char *tcl_var_write(Tcl_TVInfo *vinfo, Tcl_Interp *I, char *n1, char *n2, int flags);
*/
LS int tcl_timer_jump(Hook *hook);
LS int tcl_parse_jump(char *from, char *rest, Hook *hook);
LS void tcl_dcc_complete(Client *client, int cps);
#if defined(DEBUG_C) || defined(MEGA_C)
LS int tcl_hook(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
#endif
/*
LS int tcl_unhook(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_userlevel(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_debug(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_to_server(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_to_file(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_dcc_sendfile(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
LS int tcl_dns_jump(char *host, char *resolved, Hook *hook);
LS int tcl_dns(void *foo, Tcl_Interp *I, int objc, Tcl_Obj *CONST objv[]);
*/
LS void init_tcl(void);
LS void do_tcl(COMMAND_ARGS)						__page(CMD1_SEG);

#endif /* TCL */

/* toybox.c */

LS int read_bigcharset_callback(char *);
LS int read_bigcharset(char *);
LS int read_ascii(char *);
LS void trivia_week_toppers(void);
LS void hint_one(void);
LS void hint_two(void);
LS void hint_three(void);
LS void trivia_cleanup(void);
LS void trivia_check(Chan *, char *);
LS void trivia_no_answer(void);
LS char *random_question(char *);
LS void trivia_question(void);
LS void trivia_tick(void);
LS void write_triviascore(void);
LS int trivia_score_callback(char *);
LS void read_triviascore(void);
LS void do_bigsay(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_random_msg(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_randtopic(COMMAND_ARGS)					__page(CMD1_SEG);
LS void do_8ball(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_ascii(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_rand(COMMAND_ARGS)						__page(CMD1_SEG);
LS void do_trivia(COMMAND_ARGS)						__page(CMD1_SEG);


/* uptime.c */

void init_uptime(void);
void send_uptime(int type);
void uptime_death(int type);
void process_uptime(void);
void do_upsend(COMMAND_ARGS)						__page(CMD1_SEG);

/* user.c */

LS void cfg_user(char *)					__page(CFG1_SEG);
void cfg_modcount(char *);
LS void cfg_pass(char *)					__page(CFG1_SEG);
LS void cfg_mask(char *)					__page(CFG1_SEG);
LS void cfg_chan(char *)					__page(CFG1_SEG);
LS void cfg_opt(char *)						__page(CFG1_SEG);
LS void cfg_shit(char *)					__page(CFG1_SEG);
void cfg_kicksay(char *);
LS void cfg_greet(char *)					__page(CFG1_SEG);
LS void cfg_note(char *)					__page(CFG1_SEG);
void user_sync(void);
int read_userlist_callback(char *);
int read_userlist(char *);
int write_userlist(char *);
void rehash_chanusers(void);
LS void addtouser(Strp **, const char *, int)			__attr(CORE_SEG, __regparm(3));
LS int remfromuser(Strp **, const char *)			__attr(CORE_SEG, __regparm(2));
void mirror_user(User *)					__page(CORE_SEG);
void mirror_userlist(void)					__page(CORE_SEG);
void reset_userlink(User *, User *);
void remove_user(User *);
User *add_user(char *, char *, int);
User *find_handle(const char *);
int userhaschannel(const User *, const char *);
User *get_user(const char *, const char *);
int get_useraccess(const char *, const char *);
int get_maxaccess(const char *);
int is_bot(const char *);
int get_protaction(Chan *, char *);
int usercanmodify(const char *, const User *);
void change_pass(User *, char *)					__page(CMD1_SEG);
void do_access(COMMAND_ARGS)						__page(CMD1_SEG);
void do_userlist(COMMAND_ARGS)						__page(CMD1_SEG);
void do_user(COMMAND_ARGS)						__page(CMD1_SEG);
void do_echo(COMMAND_ARGS)						__page(CMD1_SEG);
void do_passwd(COMMAND_ARGS)						__page(CMD1_SEG);
void do_setpass(COMMAND_ARGS)						__page(CMD1_SEG);
void do_save(COMMAND_ARGS)						__page(CMD1_SEG);
void do_load(COMMAND_ARGS)						__page(CMD1_SEG);

/* vars.c */

void set_str_varc(Chan *, int, char *);
int find_setting(const char *);
void copy_vars(UniVar *, UniVar *);
void set_binarydefault(UniVar *);
void delete_vars(UniVar *, int);
void var_resolve_host(const struct Setting *);
void nobo_strcpy(const char *);
void ec_access(char *, const char *);
void ec_capabilities(char *, const char *);
void ec_cc(char *, const char *);
void ec_channels(char *, const char *);
void ec_time(char *, const char *);
void ec_set(char *, const char *);
void ec_on(char *, const char *);
void ec_server(char *, const char *);
void ec_up(char *, const char *);
void ec_ver(char *, const char *);
void do_esay(COMMAND_ARGS)						__page(CMD1_SEG);
void do_set(COMMAND_ARGS)						__page(CMD1_SEG);

/* web.c */

char *webread(int, char *, char *);
void eml_fmt(WebSock *, char *);
void web_raw(WebSock *, char *);
void web_botstatus(WebSock *, char *);
void web_debug(WebSock *, char *);
void web_404(WebSock *, char *);
void parse_web(WebSock *, char *);
void select_web(void);
void process_web(void);

#endif /* H_H */

