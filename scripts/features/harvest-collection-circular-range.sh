#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"

TEMPGAMES="harvest_range_games_`date +%F_%T`.tmp"
TEMPGAMMAS="harvest_range_gammas_`date +%F_%T`.tmp"
TEMPPATT="harvest_range_patt_`date +%F_%T`.tmp"

THRESHOLD=${1:-100}
END=${2:-15}
START=${3:-3}

touch $TEMPGAMES
cat | while read GAME
do
  echo $GAME >> $TEMPGAMES
done

touch $TEMPGAMMAS

for i in `seq $END -1 $START`; do
  echo -e "\n\n=== Harvesting size: $i" >&2
  (cat $TEMPGAMES | $WD/harvest-collection-circular2.sh $TEMPGAMMAS $i > $TEMPPATT)
# 2>&1 | sed -u "s/^/Size $i: /" >&2

  LINES=`cat "$TEMPPATT" | wc -l`
  if (( $LINES <= 1 )); then
    exit 1
  fi

  PATTERNS=`cat $TEMPPATT | awk "BEGIN{m=0} {if (\\$1>=${THRESHOLD}) m=NR} END{print m}"`
  echo "Preparing $PATTERNS gammas for size $i..." >&2
  set +e # disable error handling for the next command, workaround
  cat $TEMPPATT | head -n $PATTERNS | $WD/train-prepare-circular.sh >> $TEMPGAMMAS
  set -e
done

cat $TEMPGAMMAS

rm -f $TEMPGAMES $TEMPGAMMAS $TEMPPATT

