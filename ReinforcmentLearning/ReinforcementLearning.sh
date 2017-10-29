#!/bin/bash
i="0"
echo "start Training">trainoutput.log
date >>trainoutput.log
while [ $i -lt 100 ]
do
echo "Iteration $i"| tee >>trainoutput.log
bash OneLearnIteration.sh
i=$[$i+1]
done
