#!/bin/bash

TEMPIDS="ids_`date +%F_%T`.tmp"
TEMPLOG="log_`date +%F_%T`.tmp"
OAKFOAM="../oakfoam"
OAKFOAMLOG="../oakfoam --log $TEMPLOG"
PROGRAM="gogui-adapter \"$OAKFOAMLOG\""
MM="./mm"

INITIALPATTERNGAMMAS="shusaku.gamma"
#GAMETOTRAINWITH="/home/frank/Desktop/7dgame.sgf"

if ! test -x ../oakfoam; then
  echo "File ../oakfoam not found" >&2
  exit 1
fi

if ! test -x ./mm; then
  echo "File ./mm not found" >&2
  exit 1
fi

echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nlistfeatureids" | $OAKFOAM | grep -e "^[0-9]* [a-zA-Z0-9]*:[0-9a-fA-FxX]*" > $TEMPIDS
FEATUREIDCOUNT=`cat $TEMPIDS | wc -l`

SEDFORMATMM=`cat $TEMPIDS | sed "s/\([0-9]*\) \(.*\)/s\/\2\/\1\/;/" | tr -d '\n'`

echo "extracting competitions..." >&2
#needs game data below
#MMDATA=`echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_winnerfirst 1\nplay b e5\nplay w d4" | $OAKFOAM 2>&1| grep "^\[features\]:" | sed "s/\[features\]://" | sed "$SEDFORMATMM" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //"`
#echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_winnerfirst 1\nloadsgf $GAMETOTRAINWITH" | gogui-adapter "$OAKFOAMLOG" > /dev/null
i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "$i \t: '$GAME'" >&2
  echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_winnerfirst 1\nloadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
done
MMDATA=`cat $TEMPLOG | grep "^\[features\]:" | sed "s/\[features\]://" | sed "$SEDFORMATMM" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //"`

MMHEADER="! ${FEATUREIDCOUNT}
${FEATUREIDCOUNT}
`cat $TEMPIDS | sed "s/\([0-9]*\) \(.*\)/1 \2/"`
!
"

MMINPUT="${MMHEADER}${MMDATA}"

echo "training..." >&2
MMOUTPUT=`echo "$MMINPUT" | $MM`

echo "done." >&2

echo "$MMOUTPUT" | join $TEMPIDS - | sed "s/ */ /;s/^ //;s/^[0-9]* //"

rm -f $TEMPIDS
rm -f $TEMPLOG


