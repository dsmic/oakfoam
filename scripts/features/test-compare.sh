#!/bin/bash

TEMPLOG="log_`date +%F_%T`.tmp"
TEMPCMP="cmp_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam"
OAKFOAMLOG="../../oakfoam --log $TEMPLOG"
PROGRAM="gogui-adapter \"$OAKFOAMLOG\""

INITIALPATTERNGAMMAS=$1

DTFILE="-"
if (( $# > 2 )); then
  DTFILE=$2
fi

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

echo "[`date +%F_%T`] doing comparisons..." >&2
i=0
cat | while read GAME
do
  let "i=$i+1"
  echo -e "[`date +%F_%T`] $i \t: '$GAME'" >&2
  if [ "$DTFILE" != "-" ]; then
    echo -e "loadfeaturegammas $INITIALPATTERNGAMMAS\nparam features_ordered_comparison 1\ndtload \"$DTFILE\"\nparam dt_solo_leaf 1\nparam features_dt_use 1\nloadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
  else
    echo -e "loadfeaturegammas $INITIALPATTERNGAMMAS\nparam features_ordered_comparison 1\nloadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
  fi
  cat $TEMPLOG | grep "\[feature_comparison\]:matched" | sed "s/.*: //" >> $TEMPCMP
  rm -f $TEMPLOG
done
echo "[`date +%F_%T`] done." >&2

cat $TEMPCMP | sort -n

rm -f $TEMPLOG
rm -f $TEMPCMP


