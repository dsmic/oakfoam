#! /bin/bash

for i in `seq 1000`;
do
  echo "score" | ./oakfoam > /dev/null
done


