#!/bin/bash

set -eu

if (( $# < 1 )); then
  echo "DT file required" >&2
  exit 1
fi
DTFILE=$1

DTFOREST=1
if (( $# > 1 )); then
  DTFOREST=$2
fi

rm -f "$DTFILE" # clear

for i in `seq $DTFOREST`; do
  cat dt_blank.dat >> "$DTFILE"
done

