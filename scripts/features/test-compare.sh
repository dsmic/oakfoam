#!/bin/bash

set -eu
WD="$(dirname "$0")"

TEMPLOG="log_`date +%F_%T`.tmp"
TEMPGTP="gtp_`date +%F_%T`.tmp"
TEMPCMP="cmp_`date +%F_%T`.tmp"
OAKFOAM="$WD/../../oakfoam --nobook"
OAKFOAMLOG="$WD/../../oakfoam --nobook --log $TEMPLOG"
PROGRAM="gogui-adapter \"$OAKFOAMLOG\""

INITIALPATTERNGAMMAS=${1}

DTFILE=${2:--}
# if (( $# > 1 )); then
#   DTFILE=${OLDPWD}/${2}
# fi

LADDERS=${3:-0}
MOVEANDLOG=${4:-0}

if ! test -x $WD/../../oakfoam; then
  echo "File $WD/../../oakfoam not found" >&2
  exit 1
fi

echo "loadfeaturegammas $INITIALPATTERNGAMMAS" >> $TEMPGTP
echo "param features_ordered_comparison 1" >> $TEMPGTP
echo "param features_ordered_comparison_move_num $MOVEANDLOG" >> $TEMPGTP
echo "param features_ordered_comparison_log_evidence $MOVEANDLOG" >> $TEMPGTP
echo "param features_ladders $LADDERS" >> $TEMPGTP
if [ "$DTFILE" != "-" ]; then
  echo "dtload \"$DTFILE\"" >> $TEMPGTP
  echo "param dt_solo_leaf 1" >> $TEMPGTP
  echo "param features_dt_use 1" >> $TEMPGTP
fi
echo 'param undo_enable 0' >> $TEMPGTP # so gogui-adapter doesn't send undo commands

i=0
cat | while read GAME
do
  let "i=$i+1"
  echo "echo @@ GAME: \"$i '$GAME'\"" >> $TEMPGTP
  echo "loadsgf \"$GAME\"" >> $TEMPGTP
done

echo "[`date +%F_%T`] doing comparisons..." >&2
cat "$TEMPGTP" | gogui-adapter "$OAKFOAMLOG" 2>&1 | sed -nu 's/^= @@ //p' >&2
echo "[`date +%F_%T`] done." >&2

cat $TEMPLOG | grep "\[feature_comparison\]:matched" | sed "s/.*: //" >> $TEMPCMP
cat $TEMPCMP | sort -n

rm -f $TEMPLOG
rm -f $TEMPGTP
rm -f $TEMPCMP


