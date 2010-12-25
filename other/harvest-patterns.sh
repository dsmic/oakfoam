#!/bin/bash

#########################################
# Harvest Patterns from a Game
#########################################
# Usage: ./harvest-patterns.sh GAME.SGF
#########################################

TEMPOUTPUT="patterns_`date +%F_%T`.tmp"
OAKFOAM="../oakfoam"
OAKFOAMLOG="../oakfoam --log $TEMPOUTPUT"
PROGRAM="gogui-adapter \"$OAKFOAM\""
# Use gogui-adapter to emulate loadsgf

if ! test -x ../oakfoam; then
  echo "File ../oakfoam not found" >&2
  exit 1
fi

if (( $# != 1 )); then
  echo "Exactly one GAME.SGF required" >&2
  exit 1
fi

GAME=$1

echo -e "loadsgf $GAME" | gogui-adapter "$OAKFOAMLOG" > /dev/null
MOVES=`cat $TEMPOUTPUT | grep "^play " | wc -l`
rm -f $TEMPOUTPUT

for i in `seq $MOVES`
do
  echo -n -e "  \r" >&2
  echo -n "harvesting from: '$1' at move: $i/$MOVES..." >&2
  
  echo -e "loadsgf $GAME $i\nlistallpatterns" | $PROGRAM | grep "^= 0x" | sed "s/= //;s/ /\\n/g" | grep "^0x" >> $TEMPOUTPUT
done
echo >&2

cat $TEMPOUTPUT | sort | uniq -c | sort -rn

rm -f $TEMPOUTPUT

