#!/bin/bash

set -eu
WD="$(dirname "$0")"

TEMPOUTPUT="collection_circ_`date +%F_%T`.tmp"
TEMPCOMBINED="combined_circ_`date +%F_%T`.tmp"

INITGAMMAS=$1
SIZE=$2

echo "" > $TEMPOUTPUT

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  $WD/harvest-circular.sh "$GAME" $INITGAMMAS $SIZE >> ${TEMPOUTPUT}
done

cat ${TEMPOUTPUT} | $WD/harvest-combine.sh | sort -rn > ${TEMPCOMBINED}
cat $TEMPCOMBINED

rm -f $TEMPOUTPUT
rm -f $TEMPCOMBINED

