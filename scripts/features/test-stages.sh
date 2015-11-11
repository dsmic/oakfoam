#!/bin/bash

set -eu
WD="$(dirname "$0")"

SEP=${1:-30}

CMP="`cat`"
MAX=`echo "$CMP" | tail -n1 | awk '{print $1}'`

for i in `seq 1 $SEP $MAX`; do
  let "j=$i+$SEP-1"
  MP=`echo "$CMP" | $WD/test-filter.sh 1 $i $j | head -n1 | awk '{print $2}'`
  LEANDVAR=`echo "$CMP" | $WD/test-filter.sh 0 $i $j`
  echo $i $j $MP $LEANDVAR
done

