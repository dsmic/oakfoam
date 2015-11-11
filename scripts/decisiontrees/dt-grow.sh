#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"

TEMPGTP="grow_`date +%F_%T`.tmp"
TEMPLOG="growlog_`date +%F_%T`.tmp"
OAKFOAM="$WD/../../oakfoam --nobook --log $TEMPLOG"

if (( $# < 1 )); then
  echo "Exactly one DT required" >&2
  exit 1
fi

DTFILE=${1}
DTIGNORESTATS=${2:-1}

if ! test -x $WD/../../oakfoam; then
  echo "File $WD/../../oakfoam not found" >&2
  exit 1
fi

echo "dtload \"$DTFILE\"" >> $TEMPGTP
echo "param dt_selection_policy ${DT_SELECTION_POLICY:-descents}" >> $TEMPGTP
echo "param dt_update_prob ${DT_UPDATE_PROB:-0.10}" >> $TEMPGTP
echo "param dt_split_after ${DT_SPLIT_AFTER:-1000}" >> $TEMPGTP
echo "param undo_enable 0" >> $TEMPGTP # so gogui-adapter doesn't send undo commands

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo "echo @@ GAME: \"$i '$GAME'\"" >> $TEMPGTP
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

echo "echo @@ Saving..." >> $TEMPGTP
echo "dtsave \"$DTFILE\" $DTIGNORESTATS" >> $TEMPGTP

# Use gogui-adapter to emulate loadsgf
cat "$TEMPGTP" | gogui-adapter "$OAKFOAM" 2>&1 | sed -nu 's/^= @@ //p' >&2

LINES=`cat "$DTFILE" | wc -l`
LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`
SIZE=`ls -lh "$DTFILE" | awk '{print $5}'`

echo "Done growing '$DTFILE':" >&2
echo " Size:   $SIZE" >&2
echo " Lines:  $LINES" >&2
echo " Leaves: $LEAVES" >&2

rm -f $TEMPGTP $TEMPLOG

