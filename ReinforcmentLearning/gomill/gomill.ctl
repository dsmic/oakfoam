competition_type = 'playoff'

record_games = True
stderr_to_log = True

oakfoam_gtp_commands=[
]

players = {
  # Oakfoam
  'oakfoam-fix'   : Player("../../oakfoam -c nicego-cnn.gtp",
                      discard_stderr=True,
                      gtp_aliases={
                        'gomill-describe_engine' : 'describeengine',
                        'gomill-explain_last_move' : 'explainlastmove'
                      },
                      startup_gtp_commands=oakfoam_gtp_commands),
                      
  # Oakfoam
  'oakfoam'   : Player("../../oakfoam -c nicego-cnn.gtp",
                      discard_stderr=True,
                      gtp_aliases={
                        'gomill-describe_engine' : 'describeengine',
                        'gomill-explain_last_move' : 'explainlastmove'
                      },
                      startup_gtp_commands=oakfoam_gtp_commands)
}

board_size = 19
komi = 7.5

matchups = [
  Matchup('oakfoam', 'oakfoam-fix',
    alternating=True,
    number_of_games=1050,
    move_limit=800,
    scorer="internal"),
]

