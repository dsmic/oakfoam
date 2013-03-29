#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"

TEMPOUTPUT="collection_`date +%F_%T`.tmp"
TEMPGAME="game_`date +%F_%T`.tmp"

echo "" > $TEMPOUTPUT

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  $WD/harvest-patterns.sh "$GAME" > $TEMPGAME
  
  DATA=`cat ${TEMPOUTPUT}`"\n"`cat ${TEMPGAME}`
  echo -e "$DATA" | $WD/harvest-combine.sh | sort -rn > ${TEMPOUTPUT}
done

cat $TEMPOUTPUT

rm -f $TEMPOUTPUT
rm -f $TEMPGAME

