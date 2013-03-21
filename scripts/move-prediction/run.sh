#!/bin/bash

set -u
OLDPWD=`pwd`
cd `dirname "$0"`

function msg_short
{
  echo "${1:-}" | tee -a $LOGFILE >&2
}

function msg
{
  DATE=`date +%F_%T`
  msg_short "[$DATE] ${1:-}"
}

function init
{ 
  DATE=`date +%F_%T`
  echo -en "[$DATE] ${1:-Working}..." | awk '{printf "%-70s",$0}' | tee -a $LOGFILE >&2
  echo >> $LOGFILE
}

function check
{ 
  if ((${1:-0}==0)); then
    tput setaf 2
    msg_short "[Done]"
    tput sgr0
  else
    tput setaf 1
    msg_short "[Fail]"
    tput sgr0
    exit 1
  fi
}

function lastline
{
  tput sc
  while read LINE; do
    tput rc
    tput sc
    tput el
    echo "$LINE" >> $LOGFILE
    echo -n "$LINE" >&2
  done
  tput rc
  tput el
}

RESDIR="results_`date +%F_%T`"
mkdir $RESDIR
cd $RESDIR
LOGFILE="test.log"
msg "Writting results to '`pwd`'"

init "Loading parameters"
  if (( $# < 1 )); then
    echo "Missing parameter file" >> $LOGFILE
    check 1
  fi

  if ! test -e ${OLDPWD}/${1}; then
    echo "Parameter file '$1' not found" >> $LOGFILE
    check 1
  fi

  cp ${OLDPWD}/${1} params.test
  source ${OLDPWD}/${1}
check $?

INITGAMMAS="init.gamma"
touch $INITGAMMAS

if (( ${PATT_3X3:-0} != 0 )); then
  TEMPPATT="patt_3x3.tmp"
  init "Harvesting 3x3 patterns from $(echo "${PATT_3X3_GAMES:-}" | wc -l) games"
  (echo "${PATT_3X3_GAMES:-}" | ../../features/harvest-collection.sh > $TEMPPATT) 2>&1 | lastline
  if (( $? != 0 )); then
    check 1
  fi
  PATTERNS=`cat $TEMPPATT | awk "BEGIN{m=0} {if (\\$1>=${PATT_3X3_THRESHOLD:-100}) m=NR} END{print m}"`
  cat $TEMPPATT | head -n $PATTERNS | ../../features/train-prepare-circular.sh >> $INITGAMMAS
  check $?
  msg "3x3 patterns harvested: $PATTERNS"
  rm -f $TEMPPATT
fi

if (( ${PATT_CIRC:-0} != 0 )); then
  TEMPPATT="patt_circ.tmp"
  init "Harvesting circular patterns from $(echo "${PATT_3X3_GAMES:-}" | wc -l) games"
  (echo "${PATT_CIRC_GAMES:-}" | ../../features/harvest-collection-circular-range.sh ${PATT_CIRC_THRESHOLD:-100} ${PATT_CIRC_END:-15} ${PATT_CIRC_START:-3} > $TEMPPATT) 2>&1 | lastline
  check $?
  msg "Circular patterns harvested: `cat $TEMPPATT | wc -l`"
  cat $TEMPPATT >> $INITGAMMAS
  rm -f $TEMPPATT
fi

if (( ${DT:-0} != 0 )); then
  DTFILE="dt.dat"
  init "Growing decision forest from $(echo "${DT_GAMES:-}" | wc -l) games"
  rm -f "$DTFILE" # clear
  ../../decisiontrees/dt-init.sh $DTFILE ${DT_FOREST_SIZE:-1} &>> $LOGFILE
  echo "${DT_GAMES:-}" | ../../decisiontrees/dt-grow.sh $DTFILE 1 2>&1 | lastline
  check $?
  msg "Decision forest:"
  msg "  Forest size:   ${DT_FOREST_SIZE:-1}"
  msg "  Leaves:        `cat $DTFILE | grep 'WEIGHT' | wc -l`"
fi

TRAINEDGAMMAS="trained.gamma"

init "Training on $(echo "${TRAIN_GAMES:-}" | wc -l) games"
(echo "${TRAIN_GAMES:-}" | ../../features/train-gammas.sh $INITGAMMAS large ${DTFILE:-} > $TRAINEDGAMMAS) 2>&1 | lastline
check $?

rm -f $INITGAMMAS

# plot weights?

init "Testing on $(echo "${TEST_GAMES:-}" | wc -l) games"
(echo "${TEST_GAMES:-}" | ../../features/test-compare.sh $TRAINEDGAMMAS ${DTFILE:-} | sort -n | uniq -c > cmp.txt) 2>&1 | lastline
check $?

init "Generating reference plots"
octave -q ../../general/plot.oct > /dev/null
check $?

mv plot.png move-prediction.png
cat cmp.txt | awk '{print $2","$1}' > move-prediction.csv
if (( $? == 0 )); then
  rm cmp.txt
fi

