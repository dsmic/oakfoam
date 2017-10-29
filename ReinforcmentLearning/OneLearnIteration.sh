#!/bin/bash
#1) run gomill to produce the games

cd gomill
echo "start performance test against 4d trained for 1800000 iterations" >>../trainoutput.log
date >>../trainoutput.log

ringmaster gomill-test.ctl reset
ringmaster gomill-test.ctl -j5

head gomill-test.report >>../trainoutput.log
date >>../trainoutput.log

rm gomill.games/*

echo "start gomill to produce games" >>../trainoutput.log
date >>../trainoutput.log

ringmaster gomill.ctl reset
ringmaster gomill.ctl -j5

head gomill.report >>../trainoutput.log
date >>../trainoutput.log

#2) find gomill/gomill.games/* >games.txt
cd ../CNN
find ../gomill/gomill.games/* >games.txt

#3) scripts/CNN$ head -n 10 ../../games.txt | ./CNN-data-collection.sh initial.gamma 13 > test_positions.txt 
echo "setup databases" >>../trainoutput.log
date >>../trainoutput.log

head -n 50 games.txt | ./CNN-data-collection.sh initial.gamma 0.2 > test_positions.txt 

#4) scripts/CNN$ tail -n +11 ../../games.txt | ./CNN-data-collection.sh initial.gamma 13 > train_positions.txt 

tail -n +51 games.txt | ./CNN-data-collection.sh initial.gamma 0.2 > train_positions.txt 

#5) /train-net$ cat ../scripts/CNN/test_positions.txt | python ./generate_sample_data_leveldb.py 

cd ../train-net
rm -r test_leveldb_new
echo "test lines" >>../trainoutput.log
wc -l ../CNN/test_positions.txt >>../trainoutput.log

cat ../CNN/test_positions.txt | python ./generate_sample_data_leveldb.py test_leveldb_new &

#6) mv lenet_train_new lenet_test_new

#mv train_leveldb_new test_leveldb_new

#7) /train-net$ cat ../scripts/CNN/train_positions.txt | python ./generate_sample_data_leveldb.py
rm -r train_leveldb_new
rm -r train_leveldb_new1
rm -r train_leveldb_new2
rm -r train_leveldb_new3

#count lines divided by 4
testvar=$((`wc -l ../CNN/train_positions.txt | grep -o -E '[0-9]+' | head -1 | sed -e 's/^0\+//'` / 4))
echo "lines for training $((testvar*4)) (/4*4)" >>../trainoutput.log

head -n $testvar ../CNN/train_positions.txt | python ./generate_sample_data_leveldb.py train_leveldb_new1 &
tail -n +$(($testvar+1)) ../CNN/train_positions.txt |head -n $testvar  | python ./generate_sample_data_leveldb.py train_leveldb_new2 &
tail -n +$(($testvar*2+1)) ../CNN/train_positions.txt |head -n $testvar | python ./generate_sample_data_leveldb.py train_leveldb_new3 &
tail -n +$(($testvar*3+1))  ../CNN/train_positions.txt | python ./generate_sample_data_leveldb.py train_leveldb_new &

wait
python ./concat_leveldb_multi_at_first.py train_leveldb_new train_leveldb_new1 train_leveldb_new2 train_leveldb_new3

#8) /train-net$ ~/caffe-master/build/tools/caffe train -solver lenet_solver_value_small.prototxt 
echo "caffe train" >>../trainoutput.log
date >>../trainoutput.log

~/caffe-master/build/tools/caffe train -solver lenet_solver_value_small.prototxt --weights ../gomill/selfplay.trained 2>&1 | tee l

tail l >>../trainoutput.log
echo "check for overfitting" >>../trainoutput.log
perl checkOverfitting.pl l|tail >>../trainoutput.log

#9) cp train-net/snapshots_selfplay/_iter_100000.caffemodel gomill/selfplay.trained 

cd ..
cp train-net/snapshots_selfplay/_iter_50000.caffemodel gomill/selfplay.trained 

echo "Iteration finished" >>trainoutput.log
date >>trainoutput.log

