#!/bin/bash

TEMPOUTPUT="patterns_`date +%F_%T`.tmp"
OAKFOAM="../../oakfoam"
OAKFOAMLOG="../../oakfoam --log $TEMPOUTPUT"
PROGRAM="gogui-adapter \"$OAKFOAM\""
# Use gogui-adapter to emulate loadsgf

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

if (( $# != 1 )); then
  echo "Exactly one GAME.SGF required" >&2
  exit 1
fi

GAME=$1

echo -e "loadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
MOVES=`cat $TEMPOUTPUT | grep "^play " | wc -l`
rm -f $TEMPOUTPUT

CMDS=""
for i in `seq $MOVES`
do
  CMDS="${CMDS}\nloadsgf \"$GAME\" $i\nlistallpatterns"
done

echo -e $CMDS | $PROGRAM | grep "^= 0x" | sed "s/= //;s/ /\\n/g" | grep "^0x" >> $TEMPOUTPUT

cat $TEMPOUTPUT | sort | uniq -c | sort -rn

rm -f $TEMPOUTPUT

