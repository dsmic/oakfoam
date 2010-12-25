#!/bin/bash

TEMPOUTPUT="collection_`date +%F_%T`.tmp"

cat | while read GAME
do
  echo "harvesting from '$GAME'..." >&2
  ./harvest-patterns.sh $GAME >> $TEMPOUTPUT
done

cat $TEMPOUTPUT | sort | uniq -c | sort -rn

rm -f $TEMPOUTPUT

