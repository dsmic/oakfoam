#!/bin/bash
# Create a decision forest using dt_blank.dat as the template.
# Usage:
#   $ ./dt-init.sh output [size] [types...]
# Arguments:
#   output    Output file for th decision forest
#   size      Number of copies of the base decision forest (default: 1)
#   types     List of the decison tree types to use (default: STONE|NOCOMP)

set -eu
WD="$(dirname "$0")"

if (( $# < 1 )); then
  echo "DT file required" >&2
  exit 1
fi

DTFILE=${1}
DTFORESTSIZE=${2:-1}

DTTYPES="STONE|NOCOMP"
if (( $# > 2 )); then
  DTTYPES=""
  for i in `seq 3 $#`; do
    eval "DTTYPES=\"\$DTTYPES\$$i\" "
  done
fi

DTBASE=`mktemp dt_base.XXXX`
for t in $DTTYPES; do
  cat $WD/dt_blank.dat | sed "s/%TYPE%/$t/g" >> $DTBASE
done

rm -f "$DTFILE" # clear

for i in `seq $DTFORESTSIZE`; do
  cat $DTBASE >> "$DTFILE"
done

rm -f $DTBASE

echo "Initialized \"$DTFILE\" with a decision forest of size $DTFORESTSIZE and types: $DTTYPES." >&2

