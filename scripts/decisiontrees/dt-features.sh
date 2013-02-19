#!/bin/bash

set -eu

DTFILE=$1

DTSOLOLEAF=1
if (( $# > 1 )); then
  DTSOLOLEAF=$2
fi

LEAVES=`cat "$DTFILE" | grep 'WEIGHT' | wc -l`

if (( $DTSOLOLEAF != 0 )); then
  TREES=`cat "$DTFILE" | grep '(DT' | wc -l`
  echo "$TREES"
  for i in `seq $TREES`; do
    LN1=`cat "$DTFILE" | grep -n '(DT' | sed -n "${i}p" | sed 's/:.*//'`
    if (( $i == $TREES )); then
      LN2='$'
    else
      let "j=$i+1"
      LN2=`cat "$DTFILE" | grep -n '(DT' | sed -n "${j}p" | sed 's/:.*//'`
    fi

    L=`cat "$DTFILE" | sed -n "${LN1},${LN2}p" | grep 'WEIGHT' | wc -l`

    #echo "$TREES $i $LN1 $LN2 $L"
    echo "$L dt_$i"
  done
else
  echo "$LEAVES"
  for i in `seq $LEAVES`; do
    echo "1 dt_leaf_$i"
  done
fi

