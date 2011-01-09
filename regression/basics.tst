#----------------------------------------------
# basics.tst
#----------------------------------------------

param playouts_per_move 1000

#----------------------------------------------


# pass if opponent passes (assume Tromp-Taylor rules)
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/01.sgf 83
10 reg_genmove w
#? [pass]

# simple capture
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/01.sgf 81
20 reg_genmove w
#? [h2]

# easy problem
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/02.sgf
30 reg_genmove b
#? [b1]

# progressive widening doesn't cause a bad pass or resign
komi 7.5 # komi not sent by gogui-adapter!
loadsgf sgf/basics/03.sgf 42
param uct_progressive_widening_enabled 1
donplayouts 1000
play b pass
40 reg_genmove w
#? [!pass|resign]

#----------------------------------------------
