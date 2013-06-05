#/bin/bash
#simple use it like this: ls *.sgf|./wr-graph.sh
while read GAME
do
./wr-graph.pl $GAME
done
