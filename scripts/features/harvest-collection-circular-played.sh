#!/bin/bash

TEMPOUTPUT="collection_circ_`date +%F_%T`.tmp"
TEMPCOMBINED="combined_circ_`date +%F_%T`.tmp"

SIZE=$1

echo "" > $TEMPOUTPUT

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME' $SIZE" >&2
 ./harvest-circular-played.sh "$GAME" $SIZE >> ${TEMPOUTPUT}
done

cat ${TEMPOUTPUT} | ./harvest-combine.sh | sort -rn > ${TEMPCOMBINED}
cat $TEMPCOMBINED
./count.pl $TEMPCOMBINED

rm -f $TEMPOUTPUT
#rm -f $TEMPCOMBINED

