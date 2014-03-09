# -*- coding: utf-8 -*-
#
#  Example python script
#

import emech # you want this!

def myCallback(strFrom, strRest): # requires two arguments
    print( "from = " + strFrom )
    print( "rest = " + strRest )
    parts = strRest.split(' ')
    print( "to = " + parts[0] )
    print( "text = " + parts[1][1:] )
    print( "currentnick = " + emech.currentnick() )
    print( "botnick = " + emech.botnick() )
    print( "wantnick = " + emech.wantnick() )
    print( "userhost = " + emech.userhost() )
    #print( "server = " + str(emech.server()) )
    print( "nextserver = " + str(emech.nextserver()) )
    print( "currentchan = " + emech.currentchan() )
    print( "userlevel = " + str(emech.userlevel(strFrom)) )
    emech.to_server("PRIVMSG #emech :Hello world !\n")
    emech.debug("Debugging myself!")
    #emech.sendfile('sample.py', strFrom)
    return emech.OK # you must return this or emech.ERROR

def myTimerCallback(strFrom, strRest):
    print( "Timer called back!" )
    print( strFrom )
    print( strRest )

# create a parse hook:
emech.hook(emech.HOOK_PARSE, "PRIVMSG", myCallback)
# create a timer hook:
emech.hook(emech.HOOK_TIMER, "30 0 0 0", myTimerCallback) # in 30 sec
