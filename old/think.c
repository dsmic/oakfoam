
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "think.h"

int boardsize;
float komi;

board_node *currentboard;
color_type currentcolor;

void print_board(board_node *board);

color_type other_color(color_type color)
{
  if (color==BLACK)
    return WHITE;
  else
    return BLACK;
}

void spread_group(board_node *board, int x, int y, int group, color_type color)
{
  if (board->data[y*boardsize+x].group!=0)
    return;
  
  if (board->data[y*boardsize+x].color!=color)
    return;
  
  board->data[y*boardsize+x].group=group;
  
  if (x>0 && board->data[y*boardsize+x-1].color==color)
    spread_group(board,x-1,y,group,color);
  if (y>0 && board->data[(y-1)*boardsize+x].color==color)
    spread_group(board,x,y-1,group,color);
  if (x<(boardsize-1) && board->data[y*boardsize+x+1].color==color)
    spread_group(board,x+1,y,group,color);
  if (y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==color)
    spread_group(board,x,y+1,group,color);
}

void update_groups(board_node *board)
{
  int x,y;
  int cg=1;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      board->data[y*boardsize+x].group=0;
    }
  }
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].group==0 && board->data[y*boardsize+x].color!=EMPTY)
      {
        spread_group(board,x,y,cg,board->data[y*boardsize+x].color);
        cg++;
      }
    }
  }
  
  board->numgroups=(cg-1);
}

void add_point(point_node **curptr, int x, int y)
{
  point_node *current;

  if (*curptr==NULL)
  {
    current=malloc(sizeof(point_node));
    *curptr=current;
    current->x=x;
    current->y=y;
    current->next=NULL;
    return;
  }

  current=*curptr;

  if (current->x==x && current->y==y)
    return;
  else
    add_point(&(current->next),x,y);
}

int free_points(point_node *curptr)
{
  int p=1;

  if (curptr==NULL)
    return 0;

  if (curptr->next!=NULL)
    p+=free_points(curptr->next);

  free(curptr);
  return p;
}

int group_liberties(board_node *board, int group)
{
  int x,y;
  point_node *points=NULL;
  
  if (group==0)
    return -1;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].group==group)
      {
        if (x>0 && board->data[y*boardsize+x-1].color==EMPTY)
          add_point(&points,x-1,y);
        if (y>0 && board->data[(y-1)*boardsize+x].color==EMPTY)
          add_point(&points,x,y-1);
        if (x<(boardsize-1) && board->data[y*boardsize+x+1].color==EMPTY)
          add_point(&points,x+1,y);
        if (y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==EMPTY)
          add_point(&points,x,y+1);
      }
    }
  }
  
  return free_points(points);
}

int group_size(board_node *board, int group)
{
  int x,y,s;
  
  if (group==0)
    return -1;
  
  s=0;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].group==group)
        s++;
    }
  }
  
  return s;
}

int get_group(board_node *board, int x, int y)
{
  return board->data[y*boardsize+x].group;
}

int is_solideye(board_node *board, int x, int y)
{
  color_type c;
  int g;
  
  if ((x>0 && board->data[y*boardsize+x-1].color==EMPTY)
        || (y>0 && board->data[(y-1)*boardsize+x].color==EMPTY)
        || (x<(boardsize-1) && board->data[y*boardsize+x+1].color==EMPTY)
        || (y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==EMPTY))
    return 0;
  
  c=EMPTY;
  g=0;
     
  if (x>0)
  {
    if (c==EMPTY)
    {
      c=board->data[y*boardsize+x-1].color;
      g=board->data[y*boardsize+x-1].group;
    }
    if (c!=board->data[y*boardsize+x-1].color || g!=board->data[y*boardsize+x-1].group)
      return 0;
  }
  
  if (y>0)
  {
    if (c==EMPTY)
    {
      c=board->data[(y-1)*boardsize+x].color;
      g=board->data[(y-1)*boardsize+x].group;
    }
    if (c!=board->data[(y-1)*boardsize+x].color || g!=board->data[(y-1)*boardsize+x].group)
      return 0;
  }
  
  if (x<(boardsize-1))
  {
    if (c==EMPTY)
    {
      c=board->data[y*boardsize+x+1].color;
      g=board->data[y*boardsize+x+1].group;
    }
    if (c!=board->data[y*boardsize+x+1].color || g!=board->data[y*boardsize+x+1].group)
      return 0;
  }
  
  if (y<(boardsize-1))
  {
    if (c==EMPTY)
    {
      c=board->data[(y+1)*boardsize+x].color;
      g=board->data[(y+1)*boardsize+x].group;
    }
    if (c!=board->data[(y+1)*boardsize+x].color || g!=board->data[(y+1)*boardsize+x].group)
      return 0;
  }
  
  return 1;
}

int is_eye(board_node *board, int x, int y)
{
  color_type c,oc;
  int other;
  
  if ((x>0 && board->data[y*boardsize+x-1].color==EMPTY)
        || (y>0 && board->data[(y-1)*boardsize+x].color==EMPTY)
        || (x<(boardsize-1) && board->data[y*boardsize+x+1].color==EMPTY)
        || (y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==EMPTY))
    return 0;
  
  c=EMPTY;
     
  if (x>0)
  {
    if (c==EMPTY)
      c=board->data[y*boardsize+x-1].color;
    if (c!=board->data[y*boardsize+x-1].color)
      return 0;
  }
  
  if (y>0)
  {
    if (c==EMPTY)
      c=board->data[(y-1)*boardsize+x].color;
    if (c!=board->data[(y-1)*boardsize+x].color)
      return 0;
  }
  
  if (x<(boardsize-1))
  {
    if (c==EMPTY)
      c=board->data[y*boardsize+x+1].color;
    if (c!=board->data[y*boardsize+x+1].color)
      return 0;
  }
  
  if (y<(boardsize-1))
  {
    if (c==EMPTY)
      c=board->data[(y+1)*boardsize+x].color;
    if (c!=board->data[(y+1)*boardsize+x].color)
      return 0;
  }
  
  if (c==EMPTY)
    return 0;
  
  other=0;
  oc=other_color(c);
  
  if (x>0 && y>0)
  {
    if (oc==board->data[(y-1)*boardsize+x-1].color)
      other++;
  }
  
  if (x>0 && y<(boardsize-1))
  {
    if (oc==board->data[(y+-1)*boardsize+x-1].color)
      other++;
  }
  if (x<(boardsize-1) && y>0)
  {
    if (oc==board->data[(y-1)*boardsize+x+1].color)
      other++;
  }
  if (x<(boardsize-1) && y<(boardsize-1))
  {
    if (oc==board->data[(y+1)*boardsize+x+1].color)
      other++;
  }
  
  if (other>1)
    return 0;
  
  return 1;
}

int is_badmove(board_node *board, color_type color, int x, int y)
//todo: needs to be updated after is_eye() was updated
{
  int g,s;
  color_type c;
  
  if (is_eye(board,x,y))
  {
    if (x>0)
    {
      g=get_group(board,x-1,y);
      c=board->data[y*boardsize+x-1].color;
    }
    else
    {
      g=get_group(board,x+1,y);
      c=board->data[y*boardsize+x+1].color;
    }
    
    if (c==color)
    {
      s=group_size(board,g);
      
      if (s>BAD_MOVE_LARGE_GROUP)
        return 1;
    }
  }
  
  return 0;
}

int is_validmove(board_node *board, color_type color, int x, int y)
{
  int libs;
  
  if (board->data[y*boardsize+x].color!=EMPTY)
    return 0;
  else if ((board->ko_x)==x && (board->ko_y)==y)
    return 0;
  else
  {
    if (!(x>0 && board->data[y*boardsize+x-1].color==EMPTY)
        && !(y>0 && board->data[(y-1)*boardsize+x].color==EMPTY)
        && !(x<(boardsize-1) && board->data[y*boardsize+x+1].color==EMPTY)
        && !(y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==EMPTY))
    {//if no empty intersections surrounding
    
      if (x>0)
      {
        libs=group_liberties(board,get_group(board,x-1,y));
        if (color==board->data[y*boardsize+x-1].color)
        {
          if (libs>1)
            return 1;
        }
        else if (libs==1)
            return 1;
      }
      
      if (y>0)
      {
        libs=group_liberties(board,get_group(board,x,y-1));
        if (color==board->data[(y-1)*boardsize+x].color)
        {
          if (libs>1)
            return 1;
        }
        else if (libs==1)
            return 1;
      }
      
      if (x<(boardsize-1))
      {
        libs=group_liberties(board,get_group(board,x+1,y));
        if (color==board->data[y*boardsize+x+1].color)
        {
          if (libs>1)
            return 1;
        }
        else if (libs==1)
            return 1;
      }
      
      if (y<(boardsize-1))
      {
        libs=group_liberties(board,get_group(board,x,y+1));
        if (color==board->data[(y+1)*boardsize+x].color)
        {
          if (libs>1)
            return 1;
        }
        else if (libs==1)
            return 1;
      }
      
      return 0;
    }
    else
      return 1;
  }
}

int is_anyvalidmove(board_node *board, color_type color)
{
  int x,y;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (is_validmove(board,color,x,y))
        return 1;
    }
  }
  return 0;
}

int is_anygoodmove(board_node *board, color_type color)
{
  int x,y;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (is_validmove(board,color,x,y) && !is_badmove(board,color,x,y))
        return 1;
    }
  }
  return 0;
}

int count_anygoodmove(board_node *board, color_type color)
{
  int x,y;
  int m=0;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (is_validmove(board,color,x,y) && !is_badmove(board,color,x,y))
        m++;
    }
  }
  
  return m;
}

void remove_group(board_node *board, int group)
{
  int x,y;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].group==group)
      {
        board->data[y*boardsize+x].color=EMPTY;
        board->data[y*boardsize+x].group=0;
      }
    }
  }
}

void make_move(board_node *board, color_type color, int x, int y)
{
  int g;
  int ko_x=-1,ko_y=-1;
  int isko=0;
  
  if (color==EMPTY || (x==-1 && y==-1)) //pass
  {
    board->ko_x=-1;
    board->ko_y=-1;
  }
  else
  {
    if (x>0 && color!=board->data[y*boardsize+x-1].color)
    {
      if (group_liberties(board,get_group(board,x-1,y))==1)
      {
        g=get_group(board,x-1,y);
        if (group_size(board,g)==1 && isko==0)
        {
          isko=1;
          ko_x=x-1;
          ko_y=y;
        }
        else
          isko=-1;
        remove_group(board,g);
      }
    }
    
    if (y>0 && color!=board->data[(y-1)*boardsize+x].color)
    {
      if (group_liberties(board,get_group(board,x,y-1))==1)
      {
        g=get_group(board,x,y-1);
        if (group_size(board,g)==1 && isko==0)
        {
          isko=1;
          ko_x=x;
          ko_y=y-1;
        }
        else
          isko=-1;
        remove_group(board,g);
      }
    }
    
    if (x<(boardsize-1) && color!=board->data[y*boardsize+x+1].color)
    {
      if (group_liberties(board,get_group(board,x+1,y))==1)
      {
        g=get_group(board,x+1,y);
        if (group_size(board,g)==1 && isko==0)
        {
          isko=1;
          ko_x=x+1;
          ko_y=y;
        }
        else
          isko=-1;
        remove_group(board,g);
      }
    }
    
    if (y<(boardsize-1) && color!=board->data[(y+1)*boardsize+x].color)
    {
      if (group_liberties(board,get_group(board,x,y+1))==1)
      {
        g=get_group(board,x,y+1);
        if (group_size(board,g)==1 && isko==0)
        {
          isko=1;
          ko_x=x;
          ko_y=y+1;
        }
        else
          isko=-1;
        remove_group(board,g);
      }
    }
    
    board->data[y*boardsize+x].color=color;
    
    update_groups(board);
    
    if (isko==1)
    {
      board->ko_x=ko_x;
      board->ko_y=ko_y;
    }
    else
    {
      board->ko_x=-1;
      board->ko_y=-1;
    }
  }
}

void get_randommove(board_node *board, color_type color, int *mx, int *my)
{
  int x,y;
  
  do
	{
    x=(int)(rand()/((double)RAND_MAX+1)*boardsize);
    y=(int)(rand()/((double)RAND_MAX+1)*boardsize);
  } while (!(is_validmove(board,color,x,y) && !is_badmove(board,color,x,y)));
  
  *mx=x;
  *my=y;
}

float score(board_node *board) //+B, -W
{
  int x,y;
  float s=-komi;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].color==BLACK)
        s++;
      else if (board->data[y*boardsize+x].color==WHITE)
        s--;
      else
      {
        if (x==0)
        {
          if (board->data[y*boardsize+x+1].color==BLACK)
            s++;
          else
            s--;
        }
        else
        {
          if (board->data[y*boardsize+x-1].color==BLACK)
            s++;
          else
            s--;
        }
      }
    }
  }
  
  return s;
}

int canscore(board_node *board)
{
  int i,x,y;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      if (board->data[y*boardsize+x].color==EMPTY)
      {
        if (x>0 && board->data[y*boardsize+x-1].color==EMPTY)
          return 0;
        else if (y>0 && board->data[(y-1)*boardsize+x].color==EMPTY)
          return 0;
        else if (x<(boardsize-1) && board->data[y*boardsize+x+1].color==EMPTY)
          return 0;
        else if (y<(boardsize-1) && board->data[(y+1)*boardsize+x].color==EMPTY)
          return 0;
        
        if (!is_eye(board,x,y))
          return 0;
      }
    }
  }
  
  for (i=1;i<=(board->numgroups);i++)
  {
    if (group_liberties(board,i)==1)
      return 0;
  }
  
  return 1;
}

void play_to_done_random(board_node *board, color_type color)
{
  int lastpass=0;
  int x,y;
  int moves=0;
  
  //printf("play random\n");
  
  while (1)
  {
    if (canscore(board) || !is_anygoodmove(board,color) || moves>RND_PASS_MOVES)
    {
      if (lastpass)
        break;
      lastpass=1;
      make_move(board,EMPTY,-1,-1);
    }
    else
    {
      get_randommove(board,color,&x,&y);
      make_move(board,color,x,y);
      lastpass=0;
    }
    
    color=other_color(color);
    moves++;
    
    if (moves>RND_MAX_MOVES)
    {
      printf("error! rnd moves: %d\n",moves);
      print_board(board);
      exit(1);
    }
  }
}

board_node *copy_board(board_node *board)
{
  board_node *newboard;
  int x,y;
  
  newboard=malloc(sizeof(board_node));
  newboard->data=malloc(sizeof(vertex_type)*boardsize*boardsize);
  newboard->numgroups=0;
  newboard->ko_x=-1;
  newboard->ko_y=-1;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      newboard->data[y*boardsize+x]=board->data[y*boardsize+x];
    }
  }
  
  return newboard;
}

board_node *makeemptyboard()
{
  board_node *newboard;
  int x,y;
  
  newboard=malloc(sizeof(board_node));
  newboard->data=malloc(sizeof(vertex_type)*boardsize*boardsize);
  newboard->numgroups=0;
  newboard->ko_x=-1;
  newboard->ko_y=-1;
  
  for (x=0;x<boardsize;x++)
  {
    for (y=0;y<boardsize;y++)
    {
      newboard->data[y*boardsize+x].color=EMPTY;
      newboard->data[y*boardsize+x].group=0;
    }
  }
  
  return newboard;
}

void freeboard(board_node *board)
{
  free(board->data);
  free(board);
}

void get_move(board_node *cboard, color_type color, int *mx, int *my)
{
  int x,y;
  board_node *board;
  float s;
  int i,sfactor;
  int wins;
  int bestwins,bestx,besty;
  
  if (color==BLACK)
    sfactor=1;
  else
    sfactor=-1;
  
  bestwins=0;
  bestx=-1;
  besty=-1;
  
  for (y=0;y<boardsize;y++)
  {
    for (x=0;x<boardsize;x++)
    {
      if (cboard->data[y*boardsize+x].color==EMPTY)
      {
        if (is_validmove(cboard,color,x,y) && !is_badmove(cboard,color,x,y))
        {
          wins=0;
          
          for (i=0;i<MC_ITERS;i++)
          {
            board=copy_board(cboard);
            make_move(board,color,x,y);
            play_to_done_random(board,other_color(color));
            s=score(board)*sfactor;
            freeboard(board);
            if (s>0)
              wins++;
          }
          //printf(" %02.0f",((float)wins)/MC_ITERS*100);
          if (wins>bestwins)
          {
            bestwins=wins;
            bestx=x;
            besty=y;
          }
        }
      }
    }
  }
  
  *mx=bestx;
  *my=besty;
}

int think_generate_move(color_type color, int *x, int *y)
{
  if (canscore(currentboard) || !is_anygoodmove(currentboard,color))
  {
    (*x)=-1; //pass
    (*y)=-1;
    make_move(currentboard,EMPTY,-1,-1);
  }
  else
  {
    get_move(currentboard,color,x,y);
    make_move(currentboard,color,*x,*y);
  }
  currentcolor=other_color(color);
  
  return 0;
}

void think_make_move_on_board(color_type color, int x, int y)
{
  make_move(currentboard,color,x,y);
  currentcolor=other_color(color);
}

void think_score()
{
  float s;
  board_node *board;
  int rough;
  
  if (canscore(currentboard))
  {
    board=currentboard;
    rough=0;
  }
  else
  {
    board=copy_board(currentboard);
    play_to_done_random(board,currentcolor);
    rough=1;
  }
  
  s=score(board);
  if (s>0)
    printf("B+%.1f",s);
  else if (s<0)
    printf("W+%.1f",-s);
  else
    printf("jigo");
  
  if (rough)
    printf(" # rough estimate");
  
  if (board!=currentboard)
    freeboard(board);
}

void print_board(board_node *board)
{
  int x,y;
  
  printf("\n");
  
  printf("    ");
  for (x=0;x<boardsize;x++)
  {
    if (x>=8)
      printf(" %c",'A'+x+1);
    else
      printf(" %c",'A'+x);
  }
  printf("\n");
  printf(" \n");
  
  for (y=boardsize-1;y>=0;y--)
  {
    printf(" %2d ",y+1);
    for (x=0;x<boardsize;x++)
    {
      switch (board->data[y*boardsize+x].color)
      {
        case BLACK:
          printf(" X");
          break;
        case WHITE:
          printf(" O");
          break;
        default:
          printf(" .");
          break;
      }
    }
    printf("  %d",y+1);
    printf("\n");
  }
  
  printf(" \n");
  printf("    ");
  for (x=0;x<boardsize;x++)
  {
    if (x>=8)
      printf(" %c",'A'+x+1);
    else
      printf(" %c",'A'+x);
  }
  printf("\n");
}

void think_show_board()
{
  print_board(currentboard);
}

void think_show_groups()
{
  int x,y;
  
  printf("\n");
  
  for (y=boardsize-1;y>=0;y--)
  {
    for (x=0;x<boardsize;x++)
    {
      if (currentboard->data[y*boardsize+x].group==0)
        printf(" .");
      else
        printf(" %d",currentboard->data[y*boardsize+x].group);
    }
    printf("\n");
  }
}

void think_show_liberties()
{
  int x,y;
  
  printf("\n");
  
  for (y=boardsize-1;y>=0;y--)
  {
    for (x=0;x<boardsize;x++)
    {
      if (currentboard->data[y*boardsize+x].color==EMPTY)
        printf(" .");
      else
        printf(" %d",group_liberties(currentboard,get_group(currentboard,x,y)));
    }
    printf("\n");
  }
}

void think_show_eval()
{
  int x,y;
  board_node *board;
  float s;
  int i,sfactor;
  int wins;
  
  if (currentcolor==BLACK)
    sfactor=1;
  else
    sfactor=-1;
  
  printf("\n");
  
  for (y=boardsize-1;y>=0;y--)
  {
    for (x=0;x<boardsize;x++)
    {
      if (currentboard->data[y*boardsize+x].color==EMPTY)
      {
        if (is_validmove(currentboard,currentcolor,x,y) && !is_badmove(currentboard,currentcolor,x,y))
        {
          wins=0;
          
          for (i=0;i<MC_ITERS;i++)
          {
            board=copy_board(currentboard);
            make_move(board,currentcolor,x,y);
            play_to_done_random(board,other_color(currentcolor));
            s=score(board)*sfactor;
            freeboard(board);
            if (s>0)
              wins++;
          }
          printf(" %02.0f",((float)wins)/MC_ITERS*100);
        }
        else
          printf(" . ");
      }
      else
      {
        if (currentboard->data[y*boardsize+x].color==BLACK)
          printf(" X ");
        else
          printf(" O ");
      }
    }
    printf("\n");
  }
}

int think_is_move_allowed(color_type color, int x, int y)
{
  return is_validmove(currentboard,color,x,y);
}

void think_init()
{
  int seed;
  
  seed=(int)(time(0));
  srand(seed);
  
  currentboard=makeemptyboard();
  currentcolor=BLACK;
}

void think_stop()
{
  freeboard(currentboard);
}

void think_set_boardsize(int bsize)
{
  boardsize=bsize;
  think_clearboard();
}

int think_get_boardsize()
{
  return boardsize;
}

void think_set_komi(float k)
{
  komi=k;
}

void think_clearboard()
{
  freeboard(currentboard);
  currentboard=makeemptyboard();
  currentcolor=BLACK;
}

