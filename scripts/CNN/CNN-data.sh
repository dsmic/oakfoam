#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"

#$RANDOM reduces collisions if executed parallel, not 100% sure of cause!
TEMPOUTPUT="patterns_circ_`date +%F_%T`$RANDOM$RANDOM.tmp"
TEMPOUTPUT2="patterns_circ_2_`date +%F_%T`$RANDOM$RANDOM.tmp"
OAKFOAM="$WD/../../oakfoam --nobook --log $TEMPOUTPUT"

if ! test -x $WD/../../oakfoam; then
  echo "File $WD/../../oakfoam not found" >&2
  exit 1
fi

if (( $# < 1 )); then
  echo "Exactly one GAME.SGF required" >&2
  exit 1
fi

GAME=$1
INITGAMMAS=$2
SIZE=$3

echo $GAME >&2

CMDS="param undo_enable 0\nparam CNN_data 1.0\nloadfeaturegammas \"$INITGAMMAS\"\nloadsgf \"$GAME\"\ndonplayouts 1000\nshowterritory"
# Use gogui-adapter to emulate loadsgf
echo -e $CMDS | gogui-adapter "$OAKFOAM" > /dev/null

TERRITORY=$(cat $TEMPOUTPUT | grep -A20 "showterritory" |tail -n 19|tr "\\n" " "|tr " " ","| sed 's/,,/,/g' | sed 's/,$/\n/')
set +e
HARVESTED=`cat $TEMPOUTPUT | grep -E '^[0-9]+,' | wc -l`
set -e

tn=0
if (( ${HARVESTED:-0} > 0 )); then
  cat $TEMPOUTPUT | grep -E '^[0-9]+,' | while read line; do echo "$line,$tn,$TERRITORY"; let "tn+=1"; done > $TEMPOUTPUT2

  cat $TEMPOUTPUT2
fi

#rm -f $TEMPOUTPUT $TEMPOUTPUT2

