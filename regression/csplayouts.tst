#----------------------------------------------
# playouts.tst
#----------------------------------------------

param move_policy playout
param rand_seed 2 # no effect

#----------------------------------------------



loadsgf debugging_cs_style.sgf 11
10 reg_genmove b
#? [G16]*
11 clear_board

loadsgf debugging_cs_style.sgf 21
20 reg_genmove b
#? [B16]*
21 clear_board

loadsgf debugging_cs_style.sgf 26
30 reg_genmove w
#? [!A15]*
31 clear_board

loadsgf debugging_cs_style.sgf 28
40 reg_genmove w
#? [!A15]*
41 clear_board

loadsgf debugging_cs_style.sgf 34
50 reg_genmove w
#? [!A15]*
51 clear_board

loadsgf debugging_cs_style.sgf 57
60 reg_genmove b
#? [J19|H19]*
61 clear_board

loadsgf debugging_cs_style.sgf 81
70 reg_genmove b
#? [N3|N2|N1]*
71 clear_board

loadsgf debugging_cs_style.sgf 96
80 reg_genmove w
#? [O17]*



#----------------------------------------------
