#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"
TEMPIDS="ids_`date +%F_%T`.tmp"
TEMPLOG="log_`date +%F_%T`.tmp"
TEMPMM="mm_`date +%F_%T`.tmp"
TEMPGTP="gtp_`date +%F_%T`.tmp"
OAKFOAM="$WD/../../oakfoam --nobook"
OAKFOAMLOG="$WD/../../oakfoam --nobook --log $TEMPLOG"
PROGRAM="gogui-adapter \"$OAKFOAMLOG\""
MM="$WD/mm/mm"

if (( $# < 2 )); then
  echo "At least 2 parameters required" >&2
  exit 1
fi

INITIALPATTERNGAMMAS=${1}
if [ "$2" = "small" ]; then
  SMALLONLY="param features_only_small 1\n"
else
  SMALLONLY=""
fi

DTFILE=${3:--}
# if [ "$DTFILE" != "-" ]; then
#   DTFILE=${OLDPWD}/${DTFILE}
# fi

LADDERS=${4:-0}
TACTICAL=${5:-1}
HISTORY_AGNOSTIC=${6:-0}

if ! test -x $WD/../../oakfoam; then
  echo "File $WD/../oakfoam not found" >&2
  exit 1
fi

if ! test -x $WD/mm/mm; then
  echo "File $WD/mm/mm not found" >&2
  exit 1
fi

echo -e "loadfeaturegammas $INITIALPATTERNGAMMAS\nlistfeatureids" | $OAKFOAM 2>/dev/null | grep -e "^[0-9]* [a-zA-Z0-9]*:[0-9a-fA-FxX]*" > $TEMPIDS
FEATUREIDCOUNT=`cat $TEMPIDS | wc -l`

MMFEATURES=`cat $TEMPIDS | sed "s/[0-9]* \([a-zA-Z0-9]*\):.*/\1/" | uniq -c | sed "s/^ *//"`

if [ "$DTFILE" != "-" ]; then
  LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
  ORIGFEATURES=$FEATUREIDCOUNT
  let "FEATUREIDCOUNT=$FEATUREIDCOUNT+$LEAVES"
  DTFEATURES=`$WD/../decisiontrees/dt-features.sh "$DTFILE" 1 | sed -n '2,$p'`
  MMFEATURES="$MMFEATURES
$DTFEATURES"
fi

MMHEADER="! ${FEATUREIDCOUNT}
`echo "$MMFEATURES" | wc -l`
${MMFEATURES}
!"

echo "$MMHEADER" > $TEMPMM

#here is a probability to introduce (1.0) it says with which probability a move is taken for the data base. set to 1 for original behaviour
echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 0.1\nparam features_output_competitions_mmstyle 1\n${SMALLONLY}" > $TEMPGTP
echo "param features_ladders $LADDERS" >> $TEMPGTP
echo "param features_tactical $TACTICAL" >> $TEMPGTP
echo "param features_history_agnostic $HISTORY_AGNOSTIC" >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

if [ "$DTFILE" != "-" ]; then
  echo "dtload \"$DTFILE\"" >> $TEMPGTP
  echo "param dt_solo_leaf 1" >> $TEMPGTP
  echo "param features_dt_use 1" >> $TEMPGTP
else
  echo "param features_dt_use 0" >> $TEMPGTP
fi

echo "[`date +%F_%T`] extracting game names..." >&2
i=0
cat | while read GAME
do
  let "i=$i+1"
  #echo -e "[`date +%F_%T`] $i \t: '$GAME'" >&2
  #echo -e "loadfeaturegammas ${INITIALPATTERNGAMMAS}\nparam features_output_competitions 1\nparam features_output_competitions_mmstyle 1\n${SMALLONLY}loadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
  #cat $TEMPLOG | grep "^\[features\]:" | sed "s/\[features\]://" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //" >> $TEMPMM
  #rm -f $TEMPLOG
  echo "echo @@ GAME: \"$i '$GAME'\"" >> $TEMPGTP
  echo -e "loadsgf \"$GAME\"" >> $TEMPGTP
done

echo "[`date +%F_%T`] extracting competitions..." >&2

#cat $TEMPGTP | gogui-adapter "$OAKFOAMLOG" > /dev/null
cat "$TEMPGTP" | gogui-adapter "$OAKFOAMLOG" 2>&1 | sed -nu 's/^= @@ //p' >&2
cat $TEMPLOG | grep "^\[features\]:" | sed "s/\[features\]://" | sed "s/^#.*/#/;s/[a-zA-Z0-9]*[*:] //" >> $TEMPMM
rm -f $TEMPLOG

echo "[`date +%F_%T`] training..." >&2
MMOUTPUT="mm_output.tmp"
# MMOUTPUT=`cat $TEMPMM | $MM`
cat $TEMPMM | $MM > $MMOUTPUT
#rm -f mm-with-freq.dat

LINES=`cat "$MMOUTPUT" | wc -l`
if (( $LINES <= 1 )); then
  exit 1
fi

echo "[`date +%F_%T`] saving weights..." >&2

set +e # workaround for random bug
#cat "$MMOUTPUT"
if [ "$DTFILE" != "-" ]; then
  MMOUTPUT_TOP=`cat "$MMOUTPUT" | head -n $ORIGFEATURES`
  LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
  MMOUTPUT_BOT=`cat "$MMOUTPUT" | tail -n $LEAVES`

  echo "dtload \"$DTFILE\"" > $TEMPGTP
  echo "$MMOUTPUT_BOT" | while read IDWEIGHT; do
    ID=`echo "$IDWEIGHT" | awk '{print $1}'`
    WEIGHT=`echo "$IDWEIGHT" | awk '{print $2}'`
    ID=`echo "${ID}-${ORIGFEATURES}" | bc`
    echo "dtset $ID $WEIGHT" >> $TEMPGTP
  done
  echo "dtsave \"$DTFILE\" 1" >> $TEMPGTP
  #cat $TEMPGTP

  cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

  echo "$MMOUTPUT_TOP" > $MMOUTPUT
fi
cat "$MMOUTPUT" | join $TEMPIDS - | sed "s/ */ /;s/^ //;s/^[0-9]* //"
rm -f "$MMOUTPUT"
set -e # end workaround

rm -f $TEMPMM
rm -f $TEMPIDS
rm -f $TEMPLOG
rm -f $TEMPGTP
