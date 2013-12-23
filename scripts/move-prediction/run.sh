#!/bin/bash

set -eu
set -o pipefail
OLDPWD=`pwd`
cd `dirname "$0"`

function msg_short
{
  echo "${1:-}" | tee -a $LOGFILE | tee -a $STATUSFILE >&2
}

function msg
{
  DATE=`date +%F_%T`
  msg_short "[$DATE] ${1:-}"
  tput sc
}

function init
{ 
  DATE=`date +%F_%T`
  echo -en "[$DATE] ${1:-Working}..." | awk '{printf "%-70s",$0}' | tee -a $LOGFILE | tee -a $STATUSFILE >&2
  echo >> $LOGFILE
  tput sc
}

function check
{ 
  tput rc
  tput el
  if ((${1:-0}==0)); then
    tput setaf 2
    msg_short "[Done]"
    tput sgr0
  else
    tput setaf 1
    msg_short "[Fail]"
    tput sgr0
    trap - ERR
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

trap 'check 1' ERR

# RESDIR="results_`date +%F_%T`"
# mkdir $RESDIR
RESDIR="$(mktemp -d results_$(date +%F)_XXXX)"
cd $RESDIR
LOGFILE="test.log"
STATUSFILE="test.status"
msg "Writing results to '`pwd`'"

init "Loading parameters"
  if (( $# < 1 )); then
    echo "Missing parameter file" >> $LOGFILE
    check 1
  fi

  if ! test -e "${OLDPWD}/${1}"; then
    echo "Parameter file '$1' not found" >> $LOGFILE
    check 1
  fi

  cp "${OLDPWD}/${1}" params.test
  source "${OLDPWD}/${1}"
check $?

INITGAMMAS="init.gamma"
touch $INITGAMMAS

if [ "${PREGEN_GAMMAS:--}" != "-" ]; then
  msg "Using pre-generated gammas: $PREGEN_GAMMAS"
  cp "${OLDPWD}/${PREGEN_GAMMAS}" $INITGAMMAS
  msg "3x3 patterns used:       `cat $INITGAMMAS | sed 's/^\([^:]*\):.*$/\1/' | grep 'pattern3x3' | wc -l`"
  msg "Circular patterns used:  `cat $INITGAMMAS | sed 's/^\([^:]*\):.*$/\1/' | grep 'circpatt' | wc -l`"
else
  if (( ${PATT_3X3:-0} != 0 )); then
    TEMPPATT="patt_3x3.tmp"
    init "Harvesting 3x3 patterns from $(echo "${PATT_3X3_GAMES:-}" | wc -l) games"
    echo "${PATT_3X3_GAMES:-}" > games-3x3.dat
    (echo "${PATT_3X3_GAMES:-}" | ../../features/harvest-collection.sh > $TEMPPATT) 2>&1 | lastline
    if (( $? != 0 )); then
      check 1
    fi
    PATTERNS=`cat $TEMPPATT | awk "BEGIN{m=0} {if (\\$1>=${PATT_3X3_THRESHOLD:-100}) m=NR} END{print m}"`
    cat $TEMPPATT | head -n $PATTERNS | ../../features/train-prepare.sh >> $INITGAMMAS
    check $?
    msg "3x3 patterns harvested: $PATTERNS"
    rm -f $TEMPPATT
  fi

  if (( ${PATT_CIRC:-0} != 0 )); then
    TEMPPATT="patt_circ.tmp"
    init "Harvesting circular patterns from $(echo "${PATT_CIRC_GAMES:-}" | wc -l) games"
    echo "${PATT_CIRC_GAMES:-}" > games-circ.dat
    (echo "${PATT_CIRC_GAMES:-}" | ../../features/harvest-collection-circular-range.sh ${PATT_CIRC_THRESHOLD:-100} ${PATT_CIRC_END:-15} ${PATT_CIRC_START:-3} > $TEMPPATT) 2>&1 | lastline
    check $?
    msg "Circular patterns harvested: `cat $TEMPPATT | wc -l`"
    LINES=`cat "$TEMPPATT" | wc -l`
    if (( $LINES <= 1 )); then
      check 1
    fi
    cat $TEMPPATT >> $INITGAMMAS
    rm -f $TEMPPATT
  fi
fi

if [ "${PREGEN_DT:--}" != "-" ]; then
  DTFILE="dt.dat"
  msg "Using pre-generated decision forest: $PREGEN_DT"
  if [ "${PREGEN_GAMMAS:--}" == "-" ]; then
    msg "WARNING: Did you forget to include gammas?"
  fi
  cp "${OLDPWD}/${PREGEN_DT}" $DTFILE
  msg "Decision forest:"
  msg "  Forest size:   `cat $DTFILE | grep '(DT' | wc -l`"
  msg "  Leaves:        `cat $DTFILE | grep 'WEIGHT' | wc -l`"
else
  if (( ${DT:-0} != 0 )); then
    DTFILE="dt.dat"
    init "Growing decision forest from $(echo "${DT_GAMES:-}" | wc -l) games"
    echo "${DT_GAMES:-}" > games-dt.dat
    rm -f "$DTFILE" # clear
    ../../decisiontrees/dt-init.sh $DTFILE ${DT_FOREST_SIZE:-1} "${DT_TYPES:-STONE|NOCOMP}" &>> $LOGFILE
    export DT_SELECTION_POLICY=${DT_SELECTION_POLICY:-descents}
    export DT_UPDATE_PROB=${DT_UPDATE_PROB:-0.10}
    export DT_SPLIT_AFTER=${DT_SPLIT_AFTER:-1000}
    echo "${DT_GAMES:-}" | ../../decisiontrees/dt-grow.sh $DTFILE 1 2>&1 | lastline
    check $?
    msg "Decision forest:"
    msg "  Types:               ${DT_TYPES:-STONE|NOCOMP}"
    msg "  Duplicates:          ${DT_FOREST_SIZE:-1}"
    msg "  Forest size:         `cat $DTFILE | grep '(DT' | wc -l`"
    msg "  Update probability:  ${DT_UPDATE_PROB}"
    msg "  Selection period:    ${DT_SPLIT_AFTER}"
    msg "  Selection policy:    ${DT_SELECTION_POLICY}"
    msg "  Leaves:              `cat $DTFILE | grep 'WEIGHT' | wc -l`"
  fi
fi

TRAINEDGAMMAS="trained.gamma"

if (( ${TRAIN:-1} != 0 )); then
  init "Training on $(echo "${TRAIN_GAMES:-}" | wc -l) games"
  echo "${TRAIN_GAMES:-}" > games-train.dat
  (echo "${TRAIN_GAMES:-}" | ../../features/train-gammas.sh $INITGAMMAS large ${DTFILE:--} ${NONPATT_LADDERS:-0} ${TACTICAL:-1} ${HISTORY_AGNOSTIC:-0} > $TRAINEDGAMMAS) 2>&1 | lastline
  check $?
else
  cp $INITGAMMAS $TRAINEDGAMMAS
fi

rm -f $INITGAMMAS

init "Testing on $(echo "${TEST_GAMES:-}" | wc -l) games"
  echo "${TEST_GAMES:-}" > games-test.dat
  TESTRESULTS="cmp.txt"
  (echo "${TEST_GAMES:-}" | ../../features/test-compare.sh $TRAINEDGAMMAS ${DTFILE:--} ${NONPATT_LADDERS:-0} 1 > $TESTRESULTS) 2>&1 | lastline
check $?

init "Extracting results"
  (
    cat $TESTRESULTS | awk '{print $2}' | sort -n | uniq -c | awk '{print $2","$1}' > move-prediction.csv # legacy format, used by plot.py
    cat $TESTRESULTS | ../../features/test-filter.sh 1 | sed 's/ /,/g' > results-mp.csv
    cat $TESTRESULTS | ../../features/test-filter.sh 0 | sed 's/ /,/g' > results-le.csv
    cat $TESTRESULTS | ../../features/test-stages.sh ${TEST_STAGE_SIZE:-30} | sed 's/ /,/g' > results-stages.csv
  ) 2>&1 | lastline
check $?

init "Generating reference plots"
  octave -q --eval 'mpin="results-mp.csv";mpout="results-mp.png";stagesin="results-stages.csv";stagesout="results-stages.png";' ../../general/plot.oct > /dev/null
check $?

msg "Results summary:"
msg "  Move prediction accuracy:  `cat results-mp.csv | head -n1 | awk -F',' '{printf("%.1f%%\n",$2*100)}'`"
msg "  Mean log-evidence:         `cat results-le.csv | head -n1 | awk -F',' '{printf("%.2f\n",$1)}'`"

rm -f $TESTRESULTS

