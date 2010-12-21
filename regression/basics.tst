#----------------------------------------------
# basics.tst
#----------------------------------------------

param playouts_per_move 10000

#----------------------------------------------


# pass if opponent passes and no more points left
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/01.sgf 83
10 reg_genmove w
#? [pass]

# simple capture
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/01.sgf 81
20 reg_genmove w
#? [h2]

#----------------------------------------------
