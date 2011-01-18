#!/bin/bash

TEMPIDS="ids_`date +%F_%T`.tmp"
TEMPLOG="log_`date +%F_%T`.tmp"
TEMPMM="mm_`date +%F_%T`.tmp"
TEMPGTP="gtp_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam"
OAKFOAMLOG="../../oakfoam --log $TEMPLOG"
PROGRAM="gogui-adapter \"$OAKFOAMLOG\""
MM="mm/mm"

if (( $# != 2 )); then
  echo "Exactly 2 parameters required" >&2
  exit 1
fi

INITIALPATTERNGAMMAS=$1
if [ "$2" = "small" ]; then
  SMALLONLY="param features_only_small 1\n"
else
  SMALLONLY=""
fi

if ! test -x ../../oakfoam; then
  echo "File ../oakfoam not found" >&2
  exit 1
fi

if ! test -x mm/mm; then
  echo "File mm/mm not found" >&2
  exit 1
fi

echo -e "loadfeaturegammas $INITIALPATTERNGAMMAS\nlistfeatureids" | $OAKFOAM | grep -e "^[0-9]* [a-zA-Z0-9]*:[0-9a-fA-FxX]*" > $TEMPIDS
FEATUREIDCOUNT=`cat $TEMPIDS | wc -l`

MMFEATURES=`cat $TEMPIDS | sed "s/[0-9]* \([a-zA-Z0-9]*\):.*/\1/" | uniq -c | sed "s/^ *//"`

MMHEADER="! ${FEATUREIDCOUNT}
`echo "$MMFEATURES" | wc -l`
${MMFEATURES}
!"

echo "$MMHEADER" > $TEMPMM

echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_mmstyle 1\n${SMALLONLY}" > $TEMPGTP

echo "[`date +%F_%T`] extracting game names..." >&2
i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "[`date +%F_%T`] $i \t: '$GAME'" >&2
  #echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_mmstyle 1\n${SMALLONLY}loadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
  #cat $TEMPLOG | grep "^\[features\]:" | sed "s/\[features\]://" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //" >> $TEMPMM
  #rm -f $TEMPLOG
  echo -e "loadsgf \"$GAME\"" >> $TEMPGTP
done

echo "[`date +%F_%T`] extracting competitions..." >&2

cat $TEMPGTP | gogui-adapter "$OAKFOAMLOG" > /dev/null
cat $TEMPLOG | grep "^\[features\]:" | sed "s/\[features\]://" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //" >> $TEMPMM
rm -f $TEMPLOG

echo "[`date +%F_%T`] training..." >&2
MMOUTPUT=`cat $TEMPMM | $MM`
rm -f mm-with-freq.dat

echo "[`date +%F_%T`] done." >&2

echo "$MMOUTPUT" | join $TEMPIDS - | sed "s/ */ /;s/^ //;s/^[0-9]* //"

rm -f $TEMPMM
rm -f $TEMPIDS
rm -f $TEMPLOG
rm -f $TEMPGTP


