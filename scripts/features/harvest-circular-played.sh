#!/bin/bash

TEMPOUTPUT="patterns_circ_`date +%F_%T`$RANDOM$RANDOM.tmp"
OAKFOAM="../../oakfoam"
OAKFOAMLOG="../../oakfoam --log $TEMPOUTPUT"
PROGRAM="gogui-adapter \"$OAKFOAM\""
# Use gogui-adapter to emulate loadsgf

if ! test -x ../../oakfoam; then
  echo "File ../../oakfoam not found" >&2
  exit 1
fi

if (( $# < 2 )); then
  echo "Exactly one GAME.SGF and one size required" >&2
  exit 1
fi

GAME=$1
SIZE=$2
echo "game: $GAME" >&2

echo -e "loadsgf \"$GAME\"" | gogui-adapter "$OAKFOAMLOG" > /dev/null
MOVES=`cat $TEMPOUTPUT | grep "^play " | wc -l`
ALLMOVES=`cat $TEMPOUTPUT|./moves.pl`
rm -f $TEMPOUTPUT

#echo ${ALLMOVES[0]}
#echo $MOVES
#echo $MOVES
#set $ALLMOVES
read -a item <<<$ALLMOVES
i=0
#echo ${item[$i]}
MOVES2=$(($MOVES * 2))
#echo $MOVES2
for i in `seq $MOVES2`
do
    MOVE[$i]=${item[$(($i-1))]}
#    echo "EEEEE $i ${MOVE[$i]}"
    ((i++))
done

CMDS=""
for i in `seq $MOVES`
do
  TMP="${MOVE[$i*2]} ${MOVE[$i*2-1]}"
#  echo "ttt $i $TMP"
  ii=$(($i-1))
  CMDS="${CMDS}\nloadsgf \"$GAME\" $ii\nlistcircpatternsatsize ${MOVE[$i*2]} $SIZE ${MOVE[$i*2-1]}"
done
#echo $CMDS

echo -e $CMDS | $PROGRAM | grep -e "[1-9][0-9]*:" | sed "s/= //;s/ /\\n/g" | grep "^[1-9][0-9]*:" >> $TEMPOUTPUT
#cat $TEMPOUTPUT
cat $TEMPOUTPUT | sort | uniq -c | sort -rn

rm -f $TEMPOUTPUT

