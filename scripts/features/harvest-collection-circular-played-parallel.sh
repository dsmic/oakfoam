#!/bin/bash

TEMPOUTPUT="collection_circ_`date +%F_%T`.tmp"
TEMPCOMBINED="combined_circ_`date +%F_%T`.tmp"

SIZE=$1


i=0
cat | parallel ./harvest-circular-played.sh {} $SIZE >$TEMPOUTPUT

#while read GAME
#do
#  let "i=$i+1"
#  echo -e "$i \t: '$GAME' $SIZE" >&2
# ./harvest-circular-played.sh "$GAME" $SIZE >> ${TEMPOUTPUT}
#done

#mawk was unusable slow and default on Debian
cat ${TEMPOUTPUT} | ./harvest-combine-gawk.sh | sort -rn > ${TEMPCOMBINED}
cat $TEMPCOMBINED
#./count.pl $TEMPCOMBINED

#rm -f $TEMPOUTPUT
#rm -f $TEMPCOMBINED

