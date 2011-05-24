#!/usr/bin/awk -f

# analyse a log file and print out:
# -- [game]
#   for each clear_board cmd
# [game] [line] [lastratio] [ratio] [delta]
#   for each generated move
#
# usage: ./analyse.awk | awk '$5<-0.3'

BEGIN {
  IGNORECASE=1;
  lastr=0;
  game=1;
  moveplayed=0;
  print "--",game;
}
/\[genmove\]/{
  if ($2 ~ /^r:/) {
    #print $2;
    match($2,/([0-9.]+)/,m);
    r=m[1];
    delta=(r-lastr);
    printf "%3d %6d %.2f %.2f %+.2f\n",game,NR,lastr,r,delta;
    lastr=r;
    moveplayed=1;
  }
}
/clear_board/{
  if (moveplayed) {
    lastr=0;
    game++;
    moveplayed=0;
    print "--",game;
  }
}


