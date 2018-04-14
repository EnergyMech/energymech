#!/bin/bash
# Simple wrapper that alows energymech to connect to SSL and/or ipv6 servers
# Run it before running energymech

# Listen port. In the energymech config put 'server localhost 6003'
LPORT=6003

# IRC server/port to connect to. This can be a hostname, ipv4, or ipv6 address
# Examples. Pick one.

# Freenode, with SSL and valid/signed server certificate
# Will prefer ipv6 if available
IRCSERVER=OPENSSL:irc.freenode.net:6697,pf=ip6

# Freenode, prefer ipv4
# IRCSERVER=OPENSSL:irc.freenode.net:6697,pf=ip4

# ipv4 server with self-signed SSL certificate
# Use this noverify option if you're having trouble with SSL..
# IRCSERVER=OPENSSL:69.132.46.1:6697,noverify

# Raw ipv6 address, no SSL
# IRCSERVER=TCP6:[2a01:128:79d:1::8]:6667


SOCAT=`which socat`
if [ -z "$SOCAT" ]; then
    echo socat not found, please install it
    echo On Debian: sudo apt-get install socat
    echo On Redhat: sudo yum install socat
    echo Or you can build it from source and add it to your \$PATH
    exit 1
fi

PIDFILE=socat.pid
kill `cat $PIDFILE 2>/dev/null` >/dev/null 2>&1
${SOCAT} TCP4-LISTEN:${LPORT},reuseaddr,fork,bind=127.0.0.1 ${IRCSERVER} >socat.log 2>&1 </dev/null &
echo $! >$PIDFILE

echo running. Check socat.log if you are having problems
