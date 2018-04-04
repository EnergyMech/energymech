/*

    EnergyMech, IRC bot software
    Copyright (c) 2000-2018 proton

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
#ifndef TEXT_H
#define TEXT_H 1

/*
 *  These are more or less globally used..
 */

/*
#define FMT_PLAIN		"%s"
#define FMT_6XSTRTAB		"%s\t%s\t%s\t%s\t%s\t%s"
#define FMT_4XSTRTAB		"%s\t%s\t%s\t%s"
#define FMT_3XSTRTAB		"%s\t%s\t%s"
*/

#define FMT_PLAINLINE		"%s\n"
#define MATCH_ALL		"*"

#define TEXT_NOTINSERVLIST	"(not in serverlist)"
#define TEXT_NONE		"(none)"

#define TEXT_LISTSAVED		"Lists saved to file %s"
#define TEXT_LISTREAD		"Lists read from file %s"
#define ERR_NOSAVE		"Lists could not be saved to file %s"
#define ERR_NOREAD		"Lists could not be read from file %s"
#define ERR_NOUSERFILENAME	"No userfile has been set"

#define TEXT_UNKNOWNUSER	"Unknown user: %s"

/*
 *  alias.c
 */
#define TEXT_NOALIASES		"No aliases has been set"

/*
 *  channel.c
 */
#define TEXT_SENTWALLOP		"Sent wallop to %s"

#define TEXT_TOPICCHANGED	"Topic changed on %s"
#define TEXT_CHANINVALID	"Invalid channel name"

/*
 *  ctcp.c
 */
#define TEXT_DCC_ONLY		"Multiline output from \"%s\" command requires DCC chat"
#define TEXT_DCC_GOODBYE	"Hasta la vista!"

#define TEXT_NOTCONNECTED	"(not connected)"
#define TEXT_WHOMUSERLINE	"%s\tu%i\t%s (idle %i min, %i sec)"
#define TEXT_WHOMSELFLINE	"\037%s\037\t%s\t%s"
#define TEXT_WHOMBOTLINE	FMT_3XSTRTAB
#define TEXT_WHOMBOTGUID	"%s\t%s\t%s [%s] [%i]"

/*
 *  core.c
 */
#define TEXT_EMPTYSERVLIST	"No servers in serverlist!"
#define TEXT_NOSERVMATCHP	"No matching entries was found for %s:%i"
#define TEXT_NOSERVMATCH	"No matching entries was found for %s:*"
#define TEXT_SERVERDELETED	"Server has been deleted: %s:%i"
#define TEXT_MANYSERVMATCH	"Several entries for %s exists, please specify port also"
/* do_core() */
#define TEXT_CURRNICKWANT	"Current nick\t%s (Wanted: %s) [guid #%i]"
#define TEXT_CURRNICKHAS	"Current nick\t%s [guid #%i]"
#define TEXT_USERLISTSTATS	"Users in userlist\t%i (%i Superuser%s, %i Bot%s)"
#define TEXT_ACTIVECHANS	"Active channels\t%s"
#define TEXT_MOREACTIVECHANS	"\t%s"

#define TEXT_VIRTHOST		"Virtual host\t%s (IP Alias%s)"
#define TEXT_VIRTHOSTWINGATE	"Virtual host\t%s:%i (WinGate%s)"
#define TEXT_VHINACTIVE		" - Inactive"

#define TEXT_CURRSERVER		"Current Server\t%s:%i"
#define TEXT_CURRSERVERNOT	"Current Server\t" TEXT_NOTINSERVLIST
#define TEXT_TRYNEWSERVER	"Trying new server, brb..."
#define TEXT_SWITCHSERVER	"Switching servers..."
#define TEXT_SERVERONTIME	"Server Ontime\t%s"
#define TEXT_BOTMODES		"Mode\t+%s"

#define TEXT_CURRENTTIME	"Current Time\t%s"
#define TEXT_BOTSTARTED		"Started\t%s"
#define TEXT_BOTUPTIME		"Uptime\t%s"
#define TEXT_BOTVERSION		"Version\t%s (%s)"
#define TEXT_BOTFEATURES	"Features\t%s"

#define TEXT_CSERV		"Current Server: %s:%i"
#define TEXT_CSERVNOT		"Current Server: " TEXT_NOTINSERVLIST

#define TEXT_AGO		" ago"
#define TEXT_CURRENT		" (current)"
#define TEXT_NEVER		"(never)"

#define TEXT_SP_NOAUTH		"(no authorization)"
#define TEXT_SP_KLINED		"(K-lined)"
#define TEXT_SP_FULLCLASS	"(connection class full)"
#define TEXT_SP_TIMEOUT		"(connection timed out)"
#define TEXT_SP_ERRCONN		"(unable to connect)"
#define TEXT_SP_DIFFPORT	"(use a different port)"
#define TEXT_SP_NO_DNS		"(DNS problem)"

#define TEXT_NOLONGERAWAY	"No longer set /away"
#define TEXT_NOWSETAWAY		"Now set /away"

#define TEXT_NAMETOOLONG	"Hostname exceeds maximum length"

#define TEXT_SHUTDOWNBY		"Shutdown initiated by %s[100], flatlining ..."
#define TEXT_SHUTDOWNCOMPLETE	"Shutdown Complete"

/*
 *  main.c
 */
#define TEXT_SIGINT		"Lurking interrupted by luser ... er, owner. (SIGINT)"
#define TEXT_SIGSEGV		"Mary had a little signal segmentation fault (SIGSEGV)"
#define TEXT_SIGBUS		"Another one drives the bus! (SIGBUS)"
#define TEXT_SIGTERM		"What have I done to deserve this?? aaaaaarrghhh! (SIGTERM)"
#define TEXT_SIGUSR1		"QUIT :Switching servers... (SIGUSR1)\n"

#define TEXT_USAGE		"Usage: %s [switches [args]]\n"
#define TEXT_FSWITCH		" -f <file>     read configuration from <file>\n"
#define TEXT_CSWITCH		" -c            make core file instead of coredebug/reset\n"
#define TEXT_HSWITCH		" -h            show this help\n"
#define TEXT_VSWITCH		" -v            show EnergyMech version\n"
#define TEXT_PSWITCH1		" -p <string>   encrypt <string> using the password hashing algorithm,\n"
#define TEXT_PSWITCH2		"               output the result and then quit.\n"

#define TEXT_DSWITCH		" -d            start mech in debug mode\n"
#define TEXT_OSWITCH		" -o <file>     write debug output to <file>\n"
#define TEXT_XSWITCH		" -X            write a debug file before exit\n"

#define TEXT_HDR_VERS		"EnergyMech %s, %s\n"
#define TEXT_HDR_FEAT		"Features: %s\n"

#define ERR_MISSINGCONF		"init: No configfile specified\n"
#define ERR_UNKNOWNOPT		"init: Unknown option %s\n"
#define ERR_SAMEUSERLIST	"init: Error: UserList for %s matches the userlist for %s\n"
#define ERR_SAMEUSERLIST2	"             Bots can not share the same userlist, please specify a new one.\n"

#define INFO_USINGCONF		"init: Using config file: %s\n"
#define INFO_RUNNING		"init: EnergyMech running...\n"


#define TEXT_ALREADYSHITTED	"%s is in my shitlist already for this channel"
#define TEXT_SHITLOWACCESS	"Unable to shit %s, insufficient access"
#define TEXT_DEFAULTSHIT	"Leave Lamer!"
#define TEXT_HASSHITTED		"The user has been shitted as %s on %s"
#define TEXT_SHITEXPIRES	"The shit entry will expire: %s"


#define TEXT_SEENNOSAVE		"SeenList could not be saved to file %s"
#define TEXT_SEENNOLOAD		"SeenList could not be loaded from file %s"

/*
 *  ons.c
 */
#define TEXT_LASTHDR		str_underline("Last %i Commands")

#define KICK_BANNED		"banned"
#define KICK_REVENGE		"revenge"
#define KICK_CAPS		"excessive caps"
#define KICK_MASSMODES		"massmodes"
#define KICK_NICKFLOOD		"nickflood"
#define KICK_TEXTFLOOD		"textflood"
#define KICK_BAD_IDENT		"bad ident"
#define KICK_DEFAULT		"Requested Kick"

/*
 * telnet
 */
#define TEXT_ENTERNICKNAME	"Please enter your nickname."
#define TEXT_ENTERPASSWORD	"Please enter your password."

/*
 *  shit.c
 */
#define TEXT_CLEAREDSHITLIST	"Shitlist has been cleared"

/*
 *  user.c
 */
#define TEXT_PASS_SHORT		"password too short"
#define TEXT_PASS_LONG		"password too long"
#define TEXT_PASS_INCORRECT	"password incorrect"
#define TEXT_PASS_NEWSET	"new password has been set"

#define TEXT_PARTYECHOON	"Partyline echo is now On"
#define TEXT_PARTYECHOOFF	"Partyline echo is now Off"

#define TEXT_USERCHANGED	"User %s has been modified"
#define TEXT_USERNOTCHANGED	"User %s is unmodified"

#define TEXT_NOACCESSON		"Access denied (you have no access on %s)"
#define TEXT_USEROWNSYOU	"Access denied (%s has higher access than you)"

#endif /* TEXT_H */
