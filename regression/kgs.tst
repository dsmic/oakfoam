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

loadsgf sgf/kgs-games/game_2013-06-11_11:34:29.sgf 213
reg_genmove b
50 sg_compare_float 0.5 uct_value
#? [1]*

loadsgf sgf/kgs-games/game_2013-06-11_07:03:18.sgf 207
reg_genmove w
60 sg_compare_float 0.3 uct_value
#? [-1]*

70 sg_compare_float 0.6 uct_value
#? [-1]

loadsgf sgf/kgs-games/game_2013-06-11_06:04:21.sgf 209
80 reg_genmove b
#? [F19]

loadsgf sgf/kgs-games/game_2013-06-11_04:57:22.sgf 254
reg_genmove b
90 sg_compare_float 0.5 uct_value
#? [-1]

loadsgf sgf/kgs-games/game_2013-06-10_23:15:53.sgf 230
reg_genmove b
100 sg_compare_float 0.5 uct_value
#? [-1]*

loadsgf sgf/kgs-games/game_2013-07-07_21:42:22.sgf 213
reg_genmove b
110 sg_compare_float 0.5 uct_value
#? [-1]*

loadsgf sgf/kgs-games/game_2013-07-15_20:35:35.sgf 192
reg_genmove w
120 sg_compare_float 0.7 uct_value
#? [-1]*

130 reg_ownerat 0.2 R3
#? [1]*

140 reg_ownerat 0.5 A8
#? [1]


#----------------------------------------------
