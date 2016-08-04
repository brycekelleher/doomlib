#!/bin/sh

LUMPNUM=`./lswad doom1.wad | egrep "\<${1}\>" | sed 's/^ //g' | cut -f 1 -d ' '`
PICW=`./dumpwad doom1.wad ${LUMPNUM} | ./picinfo -f "%w"`
PICH=`./dumpwad doom1.wad ${LUMPNUM} | ./picinfo -f "%h"`
./dumpwad doom1.wad ${LUMPNUM} | ./pictorgba | display -size ${PICW}x${PICH} -depth 8 rgba:-

