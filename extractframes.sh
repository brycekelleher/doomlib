# extract all frames for a set of pictures
./lswad ${1} | egrep "\<${2}" | sed 's/^ *//g' | cut -f 2 -d ' ' | xargs -n 1 ./picconvert.sh ${1}

