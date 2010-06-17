#!/bin/bash

BLACK="../oakfoam -c oakfoam.rc"
#WHITE="gnugo --mode gtp"
#WHITE="/data/go/software/brown-1.0/brown"
WHITE="/data/go/software/amigogtp-1.6/amigogtp/amigogtp"
#WHITE="fuego"

REFEREE="fuego"
GAMES=10
TIME="5m"

TWOGTP="gogui-twogtp -black \"$BLACK\" -white \"$WHITE\" -referee \"$REFEREE\" -games $GAMES -size 9 -alternate -sgffile trials"
gogui -size 9 -program "$TWOGTP" -computer-both -auto -time $TIME

gogui-twogtp -analyze trials.dat

