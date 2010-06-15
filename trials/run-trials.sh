#!/bin/bash

BLACK="../oakfoam"
#WHITE="gnugo --mode gtp"
#WHITE="/data/go/software/brown-1.0/brown"
WHITE="/data/go/software/amigogtp-1.6/amigogtp/amigogtp"
REFEREE="fuego"
TWOGTP="gogui-twogtp -black \"$BLACK\" -white \"$WHITE\" -referee \"$REFEREE\" -games 10 -size 9 -alternate -sgffile trials"
gogui -size 9 -program "$TWOGTP" -computer-both -auto

gogui-twogtp -analyze trials.dat

