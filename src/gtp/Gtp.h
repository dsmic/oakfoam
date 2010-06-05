#ifndef DEF_OAKFOAM_GTP_H
#define DEF_OAKFOAM_GTP_H

#include <config.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstring>

#include "../engine/Engine.h"

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
  
  CMD_VALIDMOVE,
  
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
  "validmove",
  "showboard",
  "showgroups",
  "showliberties",
  "showeval",
  "score"
};

typedef enum {
  EMPTY,
  BLACK,
  WHITE
} color_type;

class Gtp
{
  public:
    void run();
    void setEngine(Engine eng);
};

#endif
