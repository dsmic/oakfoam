
#ifndef OAKFOAM_H
#define OAKFOAM_H

typedef enum {
  CMD_PROTO_VERSION,
  CMD_NAME,
  CMD_VERSION,
  
  CMD_KNOWN_COMMAND,
  CMD_LIST_COMMANDS,
  CMD_QUIT,
  
  CMD_BOARDSIZE,
  CMD_CLEAR_BOARD,
  CMD_KOMI,
  
  CMD_PLAY,
  CMD_GENMOVE,
  
  CMD_SHOWBOARD,
  CMD_SHOWGROUPS,
  CMD_SHOWLIBERTIES,
  CMD_SHOWEVAL,
  
  CMD_SCORE,
  
  CMD_ENUM_END
} command_type;

static char *command_names[] = {
  "protocol_version",
  "name",
  "version",
  "known_command",
  "list_commands",
  "quit",
  "boardsize",
  "clear_board",
  "komi",
  "play",
  "genmove",
  "showboard",
  "showgroups",
  "showliberties",
  "showeval",
  "score"
};

#endif
