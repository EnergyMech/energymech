
2.7.6.1 -- June 4th, 2000.

 * Fixed: Strcat() not skipping to end of string

2.7.6 -- February 17th, 2000.

 * Fixed: Lockup/crash bug in WALL command
 * Fixed: Major headache with DIE has been fixed
 * Fixed: Brain-damaged-undernet-coder-kludge for +k
   where the key itself is not sent if the user (bot)
   isnt opped
 * Fixed: The bot will no longer kick/+ban itself if it
   triggers massdeop protection
 * Fixed: You can now delete servers by their real name
   aswell and not just by the name listed in the config
 * Fixed: With AOP enabled, AVOICE would not work at all	[ endorphin ]
 * Fixed: DCC user + linking crash bug
 * Fixed: Major crash bug in the newly optimized
   chanuser handling...
 * Fixed: Ghost chanuser problem when the bot joins a
   channel and someone else joins before the channel is
   synched (/WHO list has been received)

2.7.5 -- February 12th, 2000.

 * Removed: UPDATE command (didnt actually have any real
   effect on anything...)
 * Changed: spymessages are now prefixed with `[message]'
   instead of `$'
 * Changed: The CHANNEL command now shows all the info
   that CSTATS and ALLSTATS used to show, and CSTATS &
   ALLSTATS has been removed
 * Changed: A discusting amount of optimizations...
 * Changed: Temporarily took out the WinGate question
   since it doesnt work
 * Added: configure script now asks if superdebug is to
   be included instead of enabling it by default
 * Added: SIGMASTER setting to decide which bot handles
   signals like, HUP, USR1, etc
 * Changed: Lots of optimizations
 * Fixed: New (since 2.7.3) bug with AVOICE system
 * Changed: Special case get_token()'s split into chop()
   routine for optimizations

2.7.4 -- January 31st, 2000.

 * Fixed: DEBUG mishap (possible crasher) in
   SockConnect() (formerly known as connect_by_number)
 * Fixed: NOIDLE sending refreshes before the bot was
   `officially online'
 * Fixed: SEGV and redundant code in find_userhost()

2.7.3 -- January 19th, 2000.

 * Fixed: WIN and ALS was missing from feature line
 * Changed: Listener socks have been made non-blocking
 * Fixed: Compiling errors re undef'd DEBUG & PIPEUSER
 * Changed: avoice and on_join() heavily optimized
 * Changed: Renamed strcasecmp to Strcasecmp cuz some
   ld tools would bong out when it finds two functions
   with the same name (one internal, one in libc)
 * Added: gcc/gprof profiling support
 * Added: sendnotice() was missing from h.h			[ found by azmodan ]
 * Fixed: The fork bug when superdebug was undef'd
 * Fixed: Bug in find_userhost() (?) caused crashes
   when it was called by Link_needop()
 * Fixed: Unused user records werent skipped in the
   saveuserlist() routine -- until now

2.7.2 -- January 16th, 2000.

 * Fixed: Updated server IPs in sample.set
 * Changed: SeenList memory allocating and handling
   have been fixed & optimized
 * Added: More superdebug debug info
 * Fixed: Memory leaks
 * Fixed: KSLIST now works					[ found by endorphin ]
 * Added: Channel keys are now saved to session file		[ found by detriment ]
 * Fixed: Help command works again
 * Fixed: link.c now compiles OK even if PIPEUSER is		[ azmodan ]
   undefined
 * Fixed: If AVOICE setting was set to 1, it would +v		[ endorphin ]
   everything in sight even though it should only +v AV
   users, its now handled correctly
 * Added: Most 2.6.1b11 beta addons have been merged
   with the current source-tree

2.7.1 -- December 2nd, 1999.

 * Fixed: needinvite now actually works
 * Changed: rewrote my_stricmp and renamed it
   strcasecmp()
 * Fixed: Small bug in time2away which could cause
   garbage to be appended to the "am" and "pm" in some
   time strings.
 * Changed: telnet users no longer need to verify and
   never gets deauthenticated
 * Changed: All f* (buffered file) routines has been
   replaced with open(), sockread(), send_to_socket(),
   and equivalents == less libc calls & better code use
 * Changed: LAST now stops listing "(none)" entries
   after the first one has been printed
 * Added: Channel keys can now be specified in the
   config file like so;
   CHANNEL <channel> <key>
 * Changed: DCC clients are now being deleted in the
   main loop instead of all over the place
 * Changed: DCC and telnet now uses non-blocking IO
 * Changed: freadln() has been removed and sockread()
   is being used to read files instead
 * Added: BinaryDefault settings are now used to avoid
   saving default settings in session files
 * Changed: Rewrote the readcfgfile() function to use
   sockread() instead of freadln()
 * Changed: the usage texts has now been moved into
   the code again. It should be more concurrent aswell
   as one less external file to keep track of
 * Changed: Removed 95% of all time() calls by making
   "now" a global variable. time() is called after
   select() to update it
 * Added: -Wshadow switch to configure script so that
   imporoperly shadowed variables can be weeded out
 * Fixed: SEGV in RKS (a simple swap of two lines)		[ found by mr_c ]

2.7.0 -- October 24th, 1999.

 * Added: Partyline users are now visible across links
 * Changed: New routine for reading server input. This
   breaks wingate support but improves overall quality
   and speed. All server reads are now completely
   converted to non-blocking IO
 * Added: "-o file" option for debug output
 * Changed: Optimizations in the handling of variables
 * Added: More warnings for stray settings in config
   file parser
 * Changed: Global settings are now bot specific, not
   process specific as before. This will require that
   some mech.set files be rewritten (global settings
   moved into the bot configuration part (and possibly
   duplicated for multiple bots)).
 * Fixed: Idle-kick (TOG IK, SET IKT) now works			[ endorphin + proton ]
 * Added: Sanity checking in CMD routines (disallow certain
   commands, like CHAT, CMD and SHUTDOWN)
 * Added: Command table compile-time parser has been merged
   from 2.6.1b10+
 * Added: --location=LOCATION option in configure script
 * Fixed: system() calls to reboot/reset the mech has been
   replaced with an execl() call (in main.c->mech_exec())
 * Fixed: Makefile stub now handles "make mech" like it
   should
 * Fixed: DCC/telnet users who "flood" the partyline
   with more than 2000 bytes (editable in config.h)
   are disconnected (chargen DoS fix)
 * Fixed: Mech now silenty ignores DCC CHAT requests
   originating from ports below 1024 (chargen DoS fix)
 * Fixed: (hopefully) checkmech problem on some machines
   (portable shellscripting is a bitch)
 * Fixed: get_userlevel() returned 100 for the bot itself,
   it now returns 200 which is more appropriate
 * Fixed: "LAST 20" now works as it should
 * Fixed: Shitlist related bugs, does it all work now?
 * Added: SHUTDOWN command
 * Removed: NSL and FINGER commands (this code is much too
   troublesome, it can be made much more simple and secure)
 * Added: TODO file
 * Fixed: CPU-eating bug in connect_to_server()
 * Fixed: SERVERLIST now reports connection errors properly	[ endorphin ]
 * Changed: AVOICE is now a SET (0 = no autovoice,		[ endorphin ]
   1 = autovoice +AV users, 2 = autovoice everyone)
 * Fixed: connect_to_server() would connect to the MOST used
   server in the serverlist, this has been corrected.
 * Changed: connect_to_server() has been optimized		[ proton + endorphin ]
 * Fixed: parse_kick() would remove the kicker from the		[ endorphin ]
   internal channel lists and not the person being kicked
 * Fixed: Port parameter for DELSERVER now works properly	[ endorphin ]
 * Changed: Invalid nicks in randnicks.e was replaced with
   valid ones
 * Fixed: SEGV in ADDSERVER command (user need access 80+)
 * Fixed: SEGV in VERIFY caused by too long passwords being
   allowed. Password must now be between 4 and 50 chars
 * Fixed: SEGV in SERVER command (user need access 80+)
 * Fixed: WHO voice/op bug

2.6.2 -- September 10th, 1999.

 * Added: AutoVoice support.
 * Added: WinGate -w option for SPAWN command.
 * Fixed: PASSWD command no longer requires "old" password
   if no password has been set.
   Usage: PASSWD [oldpassword] <newpassword>
 * Fixed: Very vague infinite-loop possibility in server
   connects existed, it has been fixed.
 * Fixed: NOIDLE not working					[ starlite ]
 * Fixed: Authentication lost after nick change			[ starlite ]
 * Fixed: Problem with shitlist functions			[ starlite ]
 * Fixed: The "/msg <bot> <cmdchar>" bug.

2.6.1.1 --

 * Fixed: Problems with CMD execution on multi-headed mechs.
 * Fixed: Problem with authentications staying in place on
   multi-headed mechs.
 * Fixed: ESAY $channels listing inactive/parted/not-joined	[ found by DMC ]
   channels
 * Fixed: Lockup bug in main loop when dealing with signals.
 * Fixed: Telnet password checking.
 * Fixed: Updated servers in sample.set.
 * Fixed: Bot not rejoining when it's kicked			[ found by DMC ]
 * Fixed: CHAT command not working on Intel hardware.

2.6.1 --

   ## Beta 10 Marker ##
 * Fixed: SEGV when doing "-TOG * toggle".
 * Fixed: New bug from Beta 9 wrecking servers connects.
 * Fixed: Password authentication problem... ouch.

   ## Beta 9 Marker ##
 * Added: NEED-INVITE should now be working.
 * Changed: Output format of HELP, CHANNELS, SERVERLIST
   and BANLIST.
 * Changed: Internal handling of channels. New bugs might
   have been introduced.
 * Fixed: A bundle of optimizations.
 * Fixed: Session data is now also saved on SIGTERM.
 * Fixed: RT now checks if the user giving the command has
   access on the target channel. It's also possible to set
   topics in channels with no +t flag when the bot isnt
   opped. -t has also been fixed in TOPIC command.
 * Fixed: Mech rejoined *all* channels known to it when
   reconnecting to server, this is not correct behaviour
   since it might have been -PART'ed but not -FORGET'ed.
 * Fixed: CSERV didnt print the real server name if one
   was available (well it does now!).
 * Fixed: User turned into ghost users if they parted the
   channel with a message.
 * Added: Help and correct usage line for WHO command.
 * Fixed: WHOM now formats output correctly for linked bots.
 * Fixed: vars.c should now compile even with NEWBIE support
   enabled.

   ## Beta 8 Marker ##
 * Fixed: Crash bug in on_msg() on SunOS.
 * Fixed: Missing reference to mcmd[] in combot.c.

   ## Beta 7 Marker ##
 * Changed: Source distribution tarball is now unpacked into
   a versioned directory, GNU-style.
 * Changed: WHOM output format slightly changed.
 * Fixed: VOICE doing +o if no nick was given. This is a bug
   since channel_massmode() was changed.
 * Changed: Refreshed the mech.help and mech.usage files.
 * Fixed: USERHOST command now works...
 * Added: Command USAGE, shows the usage for a command.
   Usage: USAGE <command>
 * Changed: Format of LAST output has been changed. It's now
   also a per-bot list, not a global as before.
 * Added: Server passwords, ALIAS settings and wingate
   settings are now saved to session file.
 * Changed: SET CMDCHAR => CMDCHAR. With no args given, the
   current commandchar is printed. Only superusers can change
   the commandchar.
   Usage: CMDCHAR [cmdchar]
 * Changed: SET WINGATE => WINGATE
   Usage: WINGATE <host> <port>
 * Changed: SET VIRTUAL => VIRTUAL
   Usage: VIRTUAL <host>
 * Changed: SET LINKPORT => LINK PORT
   Usage: LINK PORT <linkport>
 * Changed: LUSERS output format has been changed.
 * Fixed: Userlist loading in multi-headed bots.
 * Fixed: savesession() return values.
 * Fixed: ALIAS references to on_msg_command[] => mcmd[].
 * Fixed: Missing @SESSIONS@ thingy in config.h.in.
 * Removed: Commands AOP, RAOP and PROT: replaced by USER.

   ## Beta 6 Marker ##
 * Added: The missing LINK stuffs in session support. There
   could still be some more missing though... Also added a
   configure section for it.
 * Changed: With NEWBIE support *disabled* the REPORT cmd
   now prints all the settings in a much more dense format.
 * Fixed: Autolink would try to connect to entities with no
   linkport listed, this has been fixed.

   ## Beta 5 Marker ##
 * Changed: No more hardcoded limit for how many bots can
   run in a process at the same time. Spawn all you want ;P
   All these changes probably introduced a bunch of new bugs.
 * Changed: Replaced an unbelievably ugly kludge in on_msg()
   with some decent code (for "bot cmd ..." style commands).
 * Fixed: Garbage and potential bugs in do_linkcmd().
 * Changed: Lots of optimizations/rewrites all over.
 * Changed: channel_massmode() has been modified to accept
   nicks and several patterns at a time enabling stuff like
   "-deop *snuffit.org *aol.com sicko dumbo".
 * Added: Makefile stub in the root directory which passes
   on all make commands to the src/Makefile.
 * Changed: Lots of code optimizations in on_mode().
 * Fixed: Having +l in ENFM would reset the limit to 69 if
   the limit was removed. Its now reset to the original value.
 * Fixed: Spelling error in configure script.

   ## Beta 4 Marker ##
 * Fixed: Small memory leak in parse_part().
 * Fixed: Typo stuff stopped the bot from undoing bans that
   were set on protected users by people not in the userlist.
 * Fixed: Spy now prints channel for both quits and
   nickchanges. If a person is in several spied-on channels,
   the change will be seen several times.
 * Fixed: Lots of bizarre bug-ish stuff in on_kick().
 * Fixed: Using USER command to set protlevel now works.
 * Changed: CORE information has been enhanced and the info is
   presented in a more dense format. This has made the INFO
   command useless so it has been removed.
 * Fixed: find_shit() was broken so shitlist wouldnt work.
 * Changed: Output of HELP command has been changed.
 * Changed: Output of SERVERLIST command slightly changed.
 * Fixed: Screwed output in CHANNELS, ALLSTATS and CSTATS
   commands when both +l and +k was set. Output of CHANNELS
   command has been slightly changed.

   ## Beta 3 Marker ##
 * Fixed: Problem with %*-style strings in ESAY.
 * Fixed: Problem adding users with long handle names.
 * Fixed: BOTLEVEL (200) users could chat/telnet a bot.
   (BOTLEVEL users *cannot* execute commands tho, they can
   only listen to statmsgs and partyline).
 * New command: USER, should make user sharing work altho
   some parts are still missing. See HELP USER for more details.
   Usage: USER <handle> <flags ...>
 * SIGHUP and SIGUSR1 now affect only the 1st created bot.
   Before it was rather random which performed the action.
 * Fixed SEGV problem in Atoi(). Caused SEGV if reading config
   file w/ servers with no ports.

   ## Beta 2 Marker ##
 * Fixed obscure SEGV in the backgrounding routine (Ugly but
   not a showstopper)
 * Single char error prevented adding of new KICKSAYs. Fixed.
 * Public password commands (VERIFY, PASSWD, SETPASS) are
   ignored to discourage idiots from giving out passwords.
 * Failed autolinks spew 'Link to "blablabla" failed', now only
   manual LINK UPs will give that message.
 * Telnet has been restricted to L10+ users (as with DCC).
 * With LINKING defined, ADDed users werent locked as needed
   thus creating ghost users. This would be a major problem
   when setting up a 2.6.1b1 for the first time... Fixed.
 * Fixed SEGV in cycle_channel().
 * Fixed small FD leakage in parse_error().

   ## Beta 1 Marker ##
 * Added BASIC support for userlist sharing over mechnets.
   Its not quite finished yet.
 * Added SET WINGATE command.
   Usage: SET WINGATE <host> <port>
 * Added missing code for saving the ECHO flag.
 * Link messages from "$" are passed to send_global_statmsg().
   This should have been in 2.6.0 but was forgotten.
 * Verification is required before a bot will reply to
   DCC CHAT requests.
 * Fixed small bug in cfg_nickname(). Trailing spaces or	[ found by Qm-D ]
   other illegal nick could mess things up really big
 * Added WinGate support.
 * Added PASS support for server connect.
 * Made [#channel] option for USERLIST command also list
   global users. Use -C option to list channel-only users.
 * Added command BYE for "clean" DCC/Telnet disconnects.
   Usage: BYE
 * MAJOR struct rewrites. New bugs can have been introduced.

2.6.0 -- May 27th, 1999

2.5.31 -- Version changed to 2.6.0

 * LINK DOWN command completed (`Scooby).
 * Completed $NICK link command for synched bot nicks on
   mech networks.
 * leave_channel() has been moved into do_leave().
 * parse_352() (RPL_WHOREPLY) has been optimized/fixed.
 * do_steal() and do_rsteal() has been rewritten.
 * Channel steals has been optimized and #def'd. The steal
   setting is now part of the channel struct instead of
   having its own list pointer in the bot struct.
 * Revenge kick has been optimized.
 * New config file command: ALIAS, use it to set an alias
   for an already existing command. A section has been added
   to the configure script has been added for this feature.
   Usage: ALIAS <command> <alias>
 * Telnet now works (it seems anyways), a section in the
   configure script has been added for this feature. Linking
   has to be enabled to allow telnet.
 * Single char error in send_statmsg() has been fixed so that
   it sends a timestamp as it should.
 * TRACE command removed. I'd be surprised if anyone even
   knew what this command was used for.
 * TOPKICKS command removed. Too much code for so little.
 * DCCLIST command which was pretty pointless, has been
   removed (Use WHOM instead).
 * Single char error made "Virtual host" always appear in
   the CORE info even if no vhost was set. Fixed.

   ## Beta 10 Marker ##
 * Fixed the no-rejoin-after-disconnection bug.
 * HELP and HELPFULL now lists all commands as a comma
   separated list instead of a fixed width table.
 * CORE will no longer list active channels (use CHANNELS).
 * CORE will now show the currently set vhost, if any.
 * SET can now be used to change virtual host of a bot.
   Usage: SET VIRTUAL <host>
 * Fixed a problem where a mech with an invalid vhost would
   cause heavy system load, it now reverts to INADDR_ANY if
   the vhost cannot be used.
 * Added code for the "HELP on <command>" and "Level needed:"
   in the help-routine which I apparently forgot when
   I remade it.
 * Removed the BK toggle since all the code for the
   beep-kicking has been removed earlier.
 * Added missing help topic for SCREW command.
 * Command SCREWBAN has been renamed to SCREW.
 * Default level for SPAWN has been changed from 90 to 100.
 * Fixed compilation errors in main.c on some UNIX flavours.
 * There was FD leakage in the do_help() routine so I rewrote
   the whole thing. This should greatly help overall stability.
 * Default level for SPYMSG was changed from 80 to 90
   (Why? Cuz RSPYMSG was level 90...).

   ## Beta 9 Marker ##
 * Userlist related coredumps on 64-bit systems has been
   fixed (hopefully anyways).
 * USTATS now checks for matching handles before searching
   after a user by nick.
 * Complete rewrite of the README file.
 * SHITREASON command has been removed: too much code for
   such a small task IMO, use RSHIT+SHIT instead.
 * mech.usage routines has been remade yet again. Some
   commands has been added (DO, JOIN, SET).
 * URL for www.emech.cx added to README file
 * mech.help updates (ADD, SET, TOG).
 * ADD does some extra sanity-checking.
 * Bots can now be added with the ADD command using both
   "BOT" level and "200" ("BOT" is only for backwards
   compatibility, it confuses lamers).
 * HOST command now checks for invalid or global masks
   before adding them...
 * SET can now be used to change commandchar.
   Usage: SET CMDCHAR <char>
 * SET can now be used to change Linkport.
   Usage: SET LINKPORT <1-65535>
 * TOG command now supports arguments for wether a toggle
   should be SET, UNSET or TOGGLED (default).
   Usage: TOG [channel|*] <toggle> [0|1|ON|OFF]
 * Internal handling of CMD command execution have been
   improved somewhat...
 * Command spy (TOG SPY) now wont print random nicks when
   commands are executed across links.
 * Completed the "mech.usage" file. I will try to make
   good use of it now aswell ;)
 * configure script now checks if -pipe switch works and
   uses it (cuts down on disk access).
 * The beep-flooding code has been removed (when was the
   last time you saw a tsunami flood?)
 * The problem with the mech joining and parting when
   its lagged has been fixed (and hopefully nothing new
   has been put in its place).
 * HELP commands with no arguments given now calls HELPFULL	[ guppy ]
   to list all commands
 * HELPFULL now lists commands grouped by level.
 * The 'Beginner help routines' are as follows:
   DIE command in mech configuration file.
   REPORT shows descriptions of what certain switches do.
 * New linking debug code added (debug output to LinkEvents
   file). Debug need to be compiled into the mech to enable
   LinkEvents debugging.
 * Added a Levels section in the online help file (mech.help).
 * The SETPASS now works as its supposed to (it used to allow
   users to set passwords of any other user. Also, you can now
   unset passwords with SETPASS command if you're at level 80
   or higher, just do "SETPASS <handle> none".
 * Bad behaviour on behalf of the configure script could
   cause some big problems (Linux systems) if it was executed
   as root, this has been fixed.

   ## Beta 5 Marker ##
 * New command added: DELSERVER, and updated mech.help and
   mech.usage with help for it.
 * Added a couple of lines to randsignoff.e
 * Internal structure of serverlist has been reworked for
   a more efficient (hopefully anyways) format. Some commands
   has changed functionality because of it (CORE,SERVERLIST).
 * New variable for ESAY: $links. It will show all active mech
   links (if any), prefixing local uplinks with a '*' and
   remote uplinks with a '='.
 * Setting prot to 4 would actually reset it to 0 due to
   a storage limit (only 2 bits were used), this has been
   fixed (it now uses 3 bits, woopie!!)...
 * Linking has been fixed/changed to make it more stable and
   foolproof. Linking with older versions will no longer work,
   so make sure you upgrade...
 * SIGSEGV handler is no longer installed when mech is runnin
   in debug mode (mech -d) so that coredumps will be made
   properly.
 * Removed LOADKICKS and SAVEKICKS commands since they dont
   serv much of a purpose + updated helpfile.
 * Fixed an infinite loop in the USERLIST command
   (only happened if you listed users based on mask).
 * Fixed infinite loops around SIGSEGV debug routine (it now
   cores and exists even if it was compiled with debug).
 * Minor/cosmetic changes of configuration file loader.
 * configure script now asks user about compile time options
   and alters config.h based on the answers.
 * Added a DIE option for configuration file loader to get
   rid of the absolute lamers who copy sample.set to mech.set
   and run it...

2.5.30 -- November 15th, 1998

 * Single char error in command replies over links resulting
   in no replies at all.

2.5.29 -- November 15th, 1998

 * PROT now works for bots aswell (level<200 = access denied)
 * PROT command now accepts level 0 to 4, eliminating the
   need of the RPROT command which has been removed.
 * More mech.usage...
 * SESSIONS define is now off by default (sessions isnt fully
   functioning yet)
 * Due to braindamage some of the new linking routines caused
   bots to stick around while entities was unlinked, causing
   huge link-lists (.whom) after running for a while. This
   should be fixed now... This fix should also make for more
   stable linking of mechs :]

2.5.28 -- November 13, 1998

 FRIDAY THE 13TH!!! -- uh oh...

 * Fixed SESSIONS define in cfgfile.c (error if SESSIONS is
   undefd).
 * When you verify, you should be verified with all linked bots,
   if you have the same password.
 * SCREWBAN now only accepts nicknames (as it should).
 * WALL command now supports undernet NOTICE @#channel stuff,
   add HASONOTICE to your config file to make use of it.

2.5.27 -- October 24th, 1998

 * You can no longer add multiple identical masks to the same
   handle. It will complain (HOST command) or silently ignore
   the mask (internal routines).
 * Fixed the SPY & RSPY commands (RSPY wasnt working at all).
 * Added a new user #define for dynamic command levels (change
   of needed levels with chaccess).
 * Fixed a few entries in the mech.help file.
 * Added a few entries to mech.usage.
 * Updated the sample.set file with some linking stuff.
 * Added a cfg_noop routine to kill warnings for the */NO* type
   tags.
 * Added README file.

2.5.26 -- October 20th, 1998

 * Fixed the coredump from trying to do
   ``LINK UP unknownentity''
 * Fixed some config.h defines stuff (when undef'd it would
   error when compiling).

2.5.25 -- October 19th, 1998

 * Fixed an infinite loop in send_global_statmsg() when you
   had more than 2 bots in the same process and chatted more
   than one bot.
 * DEL command now only accepts handles to delete.
 * VOICE with no nickname given did +o instead of +v, fixed.
 * Added a VERSIONS file...

2.5.24 -- September 10th, 1998

 no previous record.

