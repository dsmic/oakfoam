#!/bin/sed -unf

/^genmove/{
  N
  N
  /\n= .*/!{N}
  
  s/^genmove \([a-zA-Z]\)\n.*\n= \(.*\)/play \1 \2/
  s/^play . resign//
}

/^boardsize/p
/^clear_board/p
/^komi/p
/^play/p

