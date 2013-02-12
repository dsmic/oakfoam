#!/bin/bash

set -eu

TEMPGTP="cmp_`date +%F_%T`.tmp"
TEMPLOG="cmplog_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam --nobook --log $TEMPLOG"

if (( $# < 1 )); then
  echo "Exactly one DT required" >&2
  exit 1
fi

DTFILE=$1

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo 'param dt_ordered_comparison 1' >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

cat | while read GAME
do
  #echo -e "'$GAME'" >&2
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" &> /dev/null

cat "$TEMPLOG" | grep 'matched at' | sed 's/.*matched at: //'

rm -f $TEMPGTP $TEMPLOG


