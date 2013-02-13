#!/bin/bash

set -eu

TEMPGTP="grow_`date +%F_%T`.tmp"
TEMPLOG="growlog_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam --nobook --log $TEMPLOG"

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

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo 'param dt_update_prob 0.10' >> $TEMPGTP
echo 'param dt_split_after 1000' >> $TEMPGTP
echo 'param dt_split_threshold 0.40' >> $TEMPGTP
echo 'param dt_range_divide 10' >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

cat | while read GAME
do
  #echo -e "'$GAME'" >&2
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

echo "dtsave \"$DTFILE\" $DTIGNORESTATS" >> $TEMPGTP

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

LINES=`cat "$DTFILE" | wc -l`
LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
SIZE=`ls -lh "$DTFILE" | awk '{print $5}'`

echo "Done growing '$DTFILE':" >&2
echo " Size:   $SIZE" >&2
echo " Lines:  $LINES" >&2
echo " Leaves: $LEAVES" >&2

rm -f $TEMPGTP $TEMPLOG

