#!/bin/bash

set -eu
WD="$(dirname "$0")"

if (( $# < 1 )); then
  echo "DT file required" >&2
  exit 1
fi
DTFILE=${1}
DTFOREST=${2:-1}

rm -f "$DTFILE" # clear

for i in `seq $DTFOREST`; do
  cat $WD/dt_blank.dat >> "$DTFILE"
done

echo "Initialized \"$DTFILE\" with a decision tree forest of size $DTFOREST." >&2

