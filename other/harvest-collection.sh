#!/bin/bash

TEMPOUTPUT="collection_`date +%F_%T`.tmp"

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  ./harvest-patterns.sh "$GAME" >> $TEMPOUTPUT
done

#cat $TEMPOUTPUT | sort | uniq -c | sort -rn
cat $TEMPOUTPUT

rm -f $TEMPOUTPUT

