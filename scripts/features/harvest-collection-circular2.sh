#!/bin/bash

set -eu
set -o pipefail
WD="$(dirname "$0")"

#$RANDOM reduces collisions if executed parallel, not 100% sure of cause!
TEMPOUTPUT="patterns_circ_`date +%F_%T`$RANDOM$RANDOM.tmp"
TEMPOUTPUT2="patterns_circ_2_`date +%F_%T`$RANDOM$RANDOM.tmp"
GTPIN="gtpin_`date +%F_%T`$RANDOM$RANDOM.tmp"
OAKFOAM="$WD/../../oakfoam --nobook --log $TEMPOUTPUT"

#echo "test" >$GTPIN

if ! test -x $WD/../../oakfoam; then
  echo "File $WD/../../oakfoam not found" >&2
  exit 1
fi


CAT="cat"
if which pv >/dev/null; then
    CAT="pv -W"
else
    echo "Installing pv supports progress bars" >&2
fi

#echo $CAT >&2

INITGAMMAS=$1
SIZE=$2

#echo $GAME >&2

CMDS="param undo_enable 0\nparam features_circ_list 0.1\nparam features_circ_list_size $SIZE\nloadfeaturegammas \"$INITGAMMAS\"\n"
echo -e $CMDS >$GTPIN

i=0
cat | while read GAME
do
  let "i=$i+1"
#  echo -e "$i \t: '$GAME'" >&2
  echo "echo @@ Size: $SIZE GAME: \"$i '$GAME'\"" >>$GTPIN
  echo -e "loadsgf \"$GAME\"\n" >>$GTPIN
#  $WD/harvest-circular.sh "$GAME" $INITGAMMAS $SIZE >> ${TEMPOUTPUT}
done


# Use gogui-adapter to emulate loadsgf
#cat $GTPIN | gogui-adapter "$OAKFOAM" > /dev/null
cat $GTPIN | gogui-adapter "$OAKFOAM" | sed -nu 's/^= @@ //p' >&2

set +e
HARVESTED=`$CAT $TEMPOUTPUT | grep -e '^[1-9][0-9]*:' | wc -l`
set -e
echo -e "`date +%F_%T`: GREPPING..." >&2
if (( ${HARVESTED:-0} > 0 )); then
  $CAT $TEMPOUTPUT | grep -e '^[1-9][0-9]*:' | sed 's/ /\n/g' | grep '^[1-9][0-9]*:' > $TEMPOUTPUT2
  echo -e "`date +%F_%T`: SORTING..." >&2
  $CAT $TEMPOUTPUT2 | sort | uniq -c | $CAT >$TEMPOUTPUT
  echo -e "`date +%F_%T`: ...SORTING" >&2
  $CAT $TEMPOUTPUT | sort -rn | $CAT
fi

rm -f $TEMPOUTPUT $TEMPOUTPUT2 $GTPIN

