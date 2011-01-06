#!/bin/bash

TEMPOUTPUT="collection_`date +%F_%T`.tmp"
TEMPGAME="game_`date +%F_%T`.tmp"

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  ./harvest-patterns.sh "$GAME" > $TEMPGAME
  ./harvest-combine.sh $TEMPOUTPUT $TEMPGAME > $TEMPOUTPUT
done

cat $TEMPOUTPUT | sort -rn

rm -f $TEMPOUTPUT
rm -f $TEMPGAME

