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

DTSOLOLEAF=1
if (( $# > 1 )); then
  DTSOLOLEAF=$2
fi

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo "param dt_solo_leaf $DTSOLOLEAF" >> $TEMPGTP
echo 'param dt_ordered_comparison 1' >> $TEMPGTP
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo "echo @@ GAME: \"$i '$GAME'\"" >> $TEMPGTP
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" 2>&1 | sed -n 's/^= @@ //p' >&2

cat "$TEMPLOG" | grep 'matched at' | sed 's/.*matched at: //'

rm -f $TEMPGTP $TEMPLOG


