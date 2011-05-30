#!/usr/bin/awk -f

BEGIN {
  gamenum=1;
  movesmade=0;
  genmove=0;
  printf("%3d",gamenum);
}

/^clear_board/ {
  if (movesmade) {
    gamenum++;
    movesmade=0;
    printf("\n%3d",gamenum);
  }
}

/^play . / {
  move=$3;
  movesmade++;
  printf(" %s",move);
}

/^genmove/ {
  genmove=1;
}

(genmove) && /^= / {
  genmove=0;
  move=$2;
  movesmade++;
  printf(" %s",move);
}

END {
  if (movesmade) {
    printf("\n");
  }
}
