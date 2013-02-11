#!/bin/bash

set -eu

TEMPGTP="train_`date +%F_%T`.tmp"
TEMPMM="trainmm_`date +%F_%T`.tmp"
TEMPLOG="trainlog_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam --nobook --log $TEMPLOG"
MM="mm/mm"

if (( $# != 1 )); then
  echo "Exactly one DT required" >&2
  exit 1
fi

DTFILE=$1

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

if ! test -x mm/mm; then
  echo "File mm/mm not found" >&2
  exit 1
fi

LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
echo "Starting training '$DTFILE':" >&2
echo " Leaves: $LEAVES" >&2

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo 'param dt_output_mm 0.1' >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

cat | while read GAME
do
  #echo -e "'$GAME'" >&2
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

#echo "dtsave \"$DTFILE\"" >> $TEMPGTP

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

echo "! $LEAVES" >> $TEMPMM
echo "$LEAVES" >> $TEMPMM
for i in `seq $LEAVES`; do
  echo "1 feature_$i" >> $TEMPMM
done
echo "!" >> $TEMPMM
cat "$TEMPLOG" | grep '\[dt\]:' | sed 's/\[dt\]://' >> $TEMPMM

#cat $TEMPMM

cat $TEMPMM | $MM

rm -f $TEMPGTP $TEMPMM $TEMPLOG

