#!/usr/bin/awk -f

# analyse a log file and print out:
# -- [game]
#   for each clear_board cmd
# [game] [line] [ratio] [delta]
#   for each generated move
#
# usage: ./analyse.awk | awk "\$4<-0.3"

BEGIN {
  IGNORECASE=1;
  lastr=0;
  game=0;
}
/\[genmove\]/{
  if ($2 ~ /^r:/) {
    #print $2;
    match($2,/([0-9.]+)/,m);
    r=m[1];
    delta=(r-lastr);
    print game,NR,r,delta;
    lastr=r;
  }
}
/clear_board/{
  lastr=0;
  game++;
  print "--",game;
}


