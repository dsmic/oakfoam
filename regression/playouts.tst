#----------------------------------------------
# playouts.tst
#----------------------------------------------

param move_policy playout
param rand_seed 2 # no effect

#----------------------------------------------


# 2 liberty tactic
loadsgf sgf/playouts/01.sgf
10 reg_genmove b
#? [a4|b4]*

#----------------------------------------------
