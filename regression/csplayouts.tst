#----------------------------------------------
# playouts.tst
#----------------------------------------------

param move_policy playout
param rand_seed 2 # no effect

#----------------------------------------------



loadsgf debugging_cs_style.sgf 11
1011002 reg_genmove b
#? [G16]

clear_board
loadsgf debugging_cs_style.sgf 21
1021003 reg_genmove b
#? [B16]

clear_board
loadsgf debugging_cs_style.sgf 26
1026004 reg_genmove w
#? [A15]

#not sure if this really must happen
clear_board
loadsgf debugging_cs_style.sgf 34
1034007 reg_genmove w
#? [E19|F19]*

clear_board
loadsgf debugging_cs_style.sgf 57
1057008 reg_genmove b
#? [J19|H19]

clear_board
loadsgf debugging_cs_style.sgf 81
1081008 reg_genmove b
#? [N3|N2|N1]

clear_board
loadsgf debugging_cs_style.sgf 96
1096012 reg_genmove w
#? [O17]

clear_board
loadsgf debugging_cs_style.sgf 99
1099007 reg_genmove b
#? [N1|N2]

clear_board
loadsgf debugging_cs_style.sgf 113
1113012 reg_genmove b
#? [E7]

clear_board
loadsgf debugging_cs_style.sgf 118
1118010 reg_genmove w
#? [!E7]

clear_board
loadsgf debugging_cs_style.sgf 129
1129011 reg_genmove w
#? [J1]

clear_board
loadsgf debugging_cs_style.sgf 152
1152006 reg_genmove w
#? [Q19]

clear_board
loadsgf debugging_cs_style.sgf 160
1160005 reg_genmove w
#? [!S16]

clear_board
loadsgf debugging_cs_style.sgf 182
1170008 reg_genmove w
#? [T10]

#----------------------------------------------
