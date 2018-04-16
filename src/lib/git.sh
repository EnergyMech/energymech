#!/bin/sh

hash=`git log -n 1 | sed -r 's/^commit (.{7}).*$/\1/g; 1!d'`
branch=`git status | sed -r 's/^On branch (.+)/\1/g; 1!d'`
echo '#define GITHASH " (git:'$branch'-'$hash')"'

