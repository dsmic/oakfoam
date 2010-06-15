#!/bin/bash

BLACK="../oakfoam"
#WHITE="gnugo --mode gtp"
#WHITE="/data/go/software/brown-1.0/brown"
WHITE="/data/go/software/amigogtp-1.6/amigogtp/amigogtp"
REFEREE="fuego"
GAMES=10
COMMAND="gogui-twogtp-black param playouts_per_move 300"

TWOGTP="gogui-twogtp -black \"$BLACK\" -white \"$WHITE\" -referee \"$REFEREE\" -games $GAMES -size 9 -alternate -sgffile trials"
gogui -size 9 -program "$TWOGTP" -computer-both -auto -command "$COMMAND"

gogui-twogtp -analyze trials.dat

