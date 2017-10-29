1) run gomill to produce the games

2) find gomill/gomill.games/* >games.txt


3) scripts/CNN$ head -n 100 ../../games.txt | ./CNN-data-collection.sh initial.gamma 13 > test_positions.txt 


4) scripts/CNN$ tail -n -100 ../../games.txt | ./CNN-data-collection.sh initial.gamma 13 > train_positions.txt 

5) /train-net$ cat ../scripts/CNN/test_positions.txt | python ./generate_sample_data_leveldb.py 

6) mv lenet_train_new lenet_test_new

7) /train-net$ cat ../scripts/CNN/train_positions.txt | python ./generate_sample_data_leveldb.py

8) /train-net$ ~/caffe-master/build/tools/caffe train -solver lenet_solver_value_small.prototxt --weights ../gomill/selfplay.trained 

9) cp train-net/snapshots_selfplay/_iter_100000.caffemodel gomill/selfplay.trained 




Testing with 1800000 iterations trained

I1027 20:11:54.712779 32424 solver.cpp:458] Snapshotting to binary proto file snapshots_selfplay/_iter_1800000.caffemodel
I1027 20:11:54.785917 32424 sgd_solver.cpp:273] Snapshotting solver state to binary proto file snapshots_selfplay/_iter_1800000.solverstate
I1027 20:11:54.791834 32424 solver.cpp:339] Iteration 1800000, Testing net (#0)
I1027 20:12:03.121134 32424 solver.cpp:407]     Test net output #0: accloss1 = 1.99886 (* 5 = 9.9943 loss)
I1027 20:12:03.121184 32424 solver.cpp:407]     Test net output #1: accloss2 = 317.247
I1027 20:12:03.121192 32424 solver.cpp:407]     Test net output #2: accloss3 = 284.938
I1027 20:12:03.121199 32424 solver.cpp:407]     Test net output #3: accloss4 = 0.125921 (* 50 = 6.29606 loss)
I1027 20:12:03.121204 32424 solver.cpp:407]     Test net output #4: accloss5 = 0.0638965
I1027 20:12:03.121210 32424 solver.cpp:407]     Test net output #5: accloss6 = 0.196584 (* 50 = 9.82922 loss)
I1027 20:12:03.121215 32424 solver.cpp:407]     Test net output #6: accuracy = 0.46275

