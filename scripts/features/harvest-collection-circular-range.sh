#!/bin/bash

set -eu
OLDPWD=`pwd`
cd `dirname "$0"`

TEMPGAMES="harvest_range_games_`date +%F_%T`.tmp"
TEMPGAMMAS="harvest_range_gammas_`date +%F_%T`.tmp"
TEMPPATT="harvest_range_patt_`date +%F_%T`.tmp"

THRESHOLD=$1
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
  (cat $TEMPGAMES | ./harvest-collection-circular.sh $TEMPGAMMAS $i > $TEMPPATT) 2>&1 | sed -u "s/^/Size $i: /" >&2
  PATTERNS=`cat $TEMPPATT | awk "BEGIN{m=0} {if (\\$1>=${1:-100}) m=NR} END{print m}"`
  cat $TEMPPATT | head -n $PATTERNS | ./train-prepare-circular.sh >> $TEMPGAMMAS
done

cat $TEMPGAMMAS

rm -f $TEMPGAMES $TEMPGAMMAS $TEMPPATT

