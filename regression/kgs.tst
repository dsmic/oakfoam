#----------------------------------------------
# kgs.tst
#----------------------------------------------


#----------------------------------------------


loadsgf sgf/kgs-games/game_2013-06-11_05:46:11.sgf 146
reg_genmove b
10 sg_compare_float 0.5 uct_value
#? [-1]*

20 sg_compare_float 0.65 uct_value
#? [-1]

loadsgf sgf/kgs-games/game_2013-06-11_17:28:13.sgf 117
reg_genmove w
30 sg_compare_float 0.4 uct_value
#? [-1]*

40 sg_compare_float 0.6 uct_value
#? [-1]

loadsgf sgf/kgs-games/game_2013-06-11_11:34:29.sgf 212
50 reg_genmove w
#? [J18|H18]*

loadsgf sgf/kgs-games/game_2013-06-11_07:03:18.sgf 207
reg_genmove w
60 sg_compare_float 0.3 uct_value
#? [-1]*

70 sg_compare_float 0.6 uct_value
#? [-1]

loadsgf sgf/kgs-games/game_2013-06-11_06:04:21.sgf 209
80 reg_genmove b
#? [F19]*

loadsgf sgf/kgs-games/game_2013-06-11_04:57:22.sgf 254
reg_genmove b
90 sg_compare_float 0.5 uct_value
#? [-1]*

loadsgf sgf/kgs-games/game_2013-06-10_23:15:53.sgf 230
reg_genmove b
100 sg_compare_float 0.5 uct_value
#? [-1]*

#----------------------------------------------
