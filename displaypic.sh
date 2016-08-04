#!/bin/sh

PICW=`cat TROOA1.bin | ./picinfo -f "%w"`
PICH=`cat TROOA1.bin | ./picinfo -f "%h"`
#./pictorgba < TROOA1.bin | display -size ${PICW}x${PICH} -depth 8 rgba:-
cat $1 | ./pictorgba | display -size ${PICW}x${PICH} -depth 8 rgba:-

