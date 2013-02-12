#!/bin/bash

set -eu

TEMPGTP="train_`date +%F_%T`.tmp"
TEMPMM="trainmm_`date +%F_%T`.tmp"
TEMPLOG="trainlog_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam --nobook --log $TEMPLOG"
MM="mm/mm"

if (( $# < 1 )); then
  echo "Exactly one DT required" >&2
  exit 1
fi

DTFILE=$1

DTIGNORESTATS=0
if (( $# > 1 )); then
  DTIGNORESTATS=$2
fi

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

if ! test -x mm/mm; then
  echo "File mm/mm not found" >&2
  exit 1
fi

LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
echo "Training '$DTFILE':" >&2
echo " Leaves: $LEAVES" >&2

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo 'param dt_output_mm 0.1' >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

cat | while read GAME
do
  #echo -e "'$GAME'" >&2
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

echo "Data captured. Training weights..." >&2

echo "! $LEAVES" >> $TEMPMM
echo "$LEAVES" >> $TEMPMM
for i in `seq $LEAVES`; do
  echo "1 feature_$i" >> $TEMPMM
done
echo "!" >> $TEMPMM
cat "$TEMPLOG" | grep '\[dt\]:' | sed 's/\[dt\]://' >> $TEMPMM
#cat $TEMPMM

MMOUTPUT=`cat $TEMPMM | $MM`

echo "Training done. Updating tree..." >&2

echo "dtload \"$DTFILE\"" > $TEMPGTP
echo "$MMOUTPUT" | sed 's/[ \t]\+/ /g; s/^ //' | while read WEIGHT; do
  echo "dtset $WEIGHT" >> $TEMPGTP
done
#echo "dtprint 1" >> $TEMPGTP
echo "dtsave \"$DTFILE\" $DTIGNORESTATS" >> $TEMPGTP
#cat $TEMPGTP

cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

echo "Updated tree." >&2

rm -f $TEMPGTP $TEMPMM $TEMPLOG

