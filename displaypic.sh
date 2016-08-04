#!/bin/sh

PICW=`cat TROOA1.bin | ./picinfo -f "%w"`
PICH=`cat TROOA1.bin | ./picinfo -f "%h"`
cat $1 | ./pictorgba | display -size ${PICW}x${PICH} -depth 8 rgba:-

