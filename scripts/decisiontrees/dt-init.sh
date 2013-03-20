#!/bin/bash

set -eu
OLDPWD=`pwd`
cd `dirname "$0"`

if (( $# < 1 )); then
  echo "DT file required" >&2
  exit 1
fi
DTFILE=${OLDPWD}/${1}
DTFOREST=${2:-1}

rm -f "$DTFILE" # clear

for i in `seq $DTFOREST`; do
  cat dt_blank.dat >> "$DTFILE"
done

echo "Initialized \"$DTFILE\" with a decision tree forest of size $DTFOREST." >&2

