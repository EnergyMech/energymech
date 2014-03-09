#
#  the most lame of all scripts...
#

#
#  to_server "<text>" [number]
#
#  text     what to send to server (remember to include a newline)
#  number   max number of lines in the sendq
#
#  * if number is not specified, the line will be sent immediately to the current bots server socket.
#    to_server returns the number of bytes written or -1 on an error
#    (in which case the server socket will have been closed)
#
#  * if number is zero (0), the line will be added to the sendq for the current server socket.
#    to_server returns the number of lines in the sendq
#
#  * if number is a positive integer, the line will only be added to the sendq if the number
#    of lines in the sendq is smaller than the given number.
#    to_server returns the number of lines in the sendq
#
#  ! when sending lines directly (number not specified), a newline must be supplied with the buffer.
#    when sending through the sendq, the newline is not needed
#

proc grab_notice {from rest} {
	global mech_currentnick mech_version mech_srcdate mech_class mech_guid mech_nick mech_wantnick mech_server mech_nextserver mech_currentchan
	to_server "PRIVMSG $mech_currentchan :($from) $rest\n" 50
	to_server "PRIVMSG $mech_currentchan :$mech_currentnick $mech_version $mech_srcdate $mech_class $mech_guid $mech_nick $mech_wantnick $mech_server $mech_nextserver $mech_currentchan\n" 50
	return 0
}

#
#  parse_hook <command> <handler>
#
#  command   command received from the server (for example, PRIVMSG, NOTICE, KICK, MODE, ...)
#  handler   name of the tcl procedure to call with the server input

hook parse NOTICE grab_notice

