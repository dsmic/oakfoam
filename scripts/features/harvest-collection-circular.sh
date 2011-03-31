#!/bin/bash

TEMPOUTPUT="collection_circ_`date +%F_%T`.tmp"
TEMPGAME="game_circ_`date +%F_%T`.tmp"

SIZE=$1

echo "" > $TEMPOUTPUT

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  ./harvest-circular.sh "$GAME" $SIZE > $TEMPGAME
  
  DATA=`cat ${TEMPOUTPUT}`"\n"`cat ${TEMPGAME}`
  echo -e "$DATA" | ./harvest-combine.sh | sort -rn > ${TEMPOUTPUT}
done

cat $TEMPOUTPUT

rm -f $TEMPOUTPUT
rm -f $TEMPGAME

