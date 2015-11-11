#!/bin/bash

set -eu
WD="$(dirname "$0")"

MPNOTLE=${1:-1}
MOVE_MIN=${2:-1}
MOVE_MAX=${3:-500} # should be large enough for all 19x19 games

if (( $MPNOTLE != 0 )); then
  echo "[`date +%F_%T`] computing move prediction for moves $MOVE_MIN to $MOVE_MAX" >&2
  MP="$(cat | awk "\$1>=$MOVE_MIN && \$1<=$MOVE_MAX { print \$2 }" | sort -n)"
  TOT=`echo "$MP" | wc -l`
  MAX=`echo "$MP" | tail -n1`
  for i in `seq $MAX`; do
    echo "$MP" | awk "\$1<=$i { c+=1 } END{ printf(\"$i %.3f %d\n\",c/$TOT,$TOT) }"
  done
else
  echo "[`date +%F_%T`] computing mean log-evidence for moves $MOVE_MIN to $MOVE_MAX" >&2
  MP="$(cat | awk "\$1>=$MOVE_MIN && \$1<=$MOVE_MAX { print \$3 }")"
  TOT=`echo "$MP" | wc -l`
  echo "$MP" | awk '{ t+=$1; s+=$1*$1; c+=1 } END{ printf("%.2f %d %.2f\n",t/c,c,s/c-(t/c)*(t/c)) }'
fi

