#!/bin/sh

LUMPNUM=`./lswad ${1} | egrep "\<${2}\>" | sed 's/^ *//g' | cut -f 1 -d ' '`
PICW=`./dumpwad ${1} ${LUMPNUM} | ./picinfo -f "%w"`
PICH=`./dumpwad ${1} ${LUMPNUM} | ./picinfo -f "%h"`
./dumpwad ${1} ${LUMPNUM} | ./pictorgba | convert  -size ${PICW}x${PICH} -depth 8 rgba:- ${2}.tga

