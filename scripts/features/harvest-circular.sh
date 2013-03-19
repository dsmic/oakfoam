#!/bin/bash

set -eu

TEMPOUTPUT="patterns_circ_`date +%F_%T`.tmp"
TEMPOUTPUT2="patterns_circ_2_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam --nobook --log $TEMPOUTPUT"

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

if (( $# < 1 )); then
  echo "Exactly one GAME.SGF required" >&2
  exit 1
fi

GAME=$1
SIZE=$2

CMDS="param undo_enable 0\nparam features_circ_list 0.1\nparam features_circ_list_size $SIZE\nloadsgf \"$GAME\""
# Use gogui-adapter to emulate loadsgf
echo -e $CMDS | gogui-adapter "$OAKFOAM"

cat $TEMPOUTPUT | grep -e "[1-9][0-9]*:" | sed "s/ /\\n/g" | grep "^[1-9][0-9]*:" >> $TEMPOUTPUT2

cat $TEMPOUTPUT2 | sort | uniq -c | sort -rn

rm -f $TEMPOUTPUT $TEMPOUTPUT2

