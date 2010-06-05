
#ifndef THINK_H
#define THINK_H

#define BAD_MOVE_LARGE_GROUP 6

#define RND_MAX_MOVES 200
#define RND_PASS_MOVES RND_MAX_MOVES-50

#define MC_ITERS 300

typedef enum {
  EMPTY,
  BLACK,
  WHITE
} color_type;

typedef struct {
  color_type color;
  int group;
} vertex_type;

struct point_node_struct {
  int x;
  int y;
  struct point_node_struct *next;
};
typedef struct point_node_struct point_node;

struct board_node_struct {
  vertex_type *data;
  int numgroups;
  int ko_x;
  int ko_y;
};
typedef struct board_node_struct board_node;

struct move_node_struct {
  int x;
  int y;
  float weight;
  int flag;
  struct move_node_struct *next;
};
typedef struct move_node_struct move_node;


void think_init();
void think_stop();

void think_clearboard();

void think_set_boardsize(int bsize);
void think_set_komi(float k);

int think_get_boardsize();

int think_generate_move(color_type color, int *x, int *y);
void think_make_move_on_board(color_type color, int x, int y);
int think_is_move_allowed(color_type color, int x, int y);

void think_score();

void think_show_board();
void think_show_groups();
void think_show_liberties();
void think_show_eval();


#endif
