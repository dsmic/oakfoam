/*
 * oakfoam
 * go engine
 *
 * author: francois van niekerk
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "oakfoam.h"
#include "think.h"

int quit;

void printmove(color_type color, int x, int y)
{
  char c;
  
  if (color==EMPTY)
  {
    printf("PASS");
  }
  else
  {
    c=x+'A';
    if (x>=8)
      c++;
    printf("%c%d",c,y+1);
  }
}

int playmove(color_type color, int x, int y)
{
  
  if (!(x==-1 && y==-1))
  {
    if (!think_is_move_allowed(color,x,y))
      return 1;
    
    think_make_move_on_board(color,x,y);
  }
  else
  {
    think_make_move_on_board(EMPTY,0,0);
  }
  return 0;
}

void genmove(color_type color)
{
  int x,y,ret;
  
  ret=think_generate_move(color,&x,&y);
  
  if (ret==0 && !(x==-1 && y==-1))
  {
    printmove(color,x,y);
  }
  else if (x==-1 && y==-1)
  {
    printf("PASS");
  }
  else
  {
    printf("RESIGN");
  }
}

int parsevertex(char *move, int *x, int *y)
{
  char c;
  
  if (strcmp(move,"pass")==0)
  {
    *x=-1;
    *y=-1;
    return 0;
  }
  else if (strcmp(move,"resign")==0)
  {
    *x=-1;
    *y=-1;
    return 0;
  }
  else
  {
    if (strlen(move)!=2) //can only parse up to 9x9
      return 1;
    
    c=move[0];
    *x=c-'a';
    if (*x>=9)
      (*x)--;
    
    c=move[1];
    *y=c-'1';
    
    if ((*x)>=0 && (*x)<think_get_boardsize() && (*y)>=0 && (*y)<think_get_boardsize())
      return 0;
    else
      return 1;
  }
}

char *next_tok()
{
  return strtok(NULL," \t\r\n");
}

void out_success(int id)
{
  if (id<0)
    printf("= ");
  else
    printf("=%d ",id);
}

void out_error(int id)
{
  if (id<0)
    printf("? ");
  else
    printf("?%d ",id);
}

void out_end()
{
  printf("\n\n");
}

void evalcommand(char *cmdstr)
{
  char *tok;
  int i,x,y;
  color_type c;
  float f;
  int id;
  char *p;
  command_type cmd;
  
  //remove invalid characters
  
  for (i=0;i<strlen(cmdstr);i++)
    cmdstr[i]=tolower(cmdstr[i]);
  
  if ((p=strchr(cmdstr, '#'))!=NULL)
    *p=0;
  
  tok=strtok(cmdstr," \t\r\n");
  if (tok==NULL)
    return;
  
  if (sscanf(tok,"%d",&id)==1)
    tok=next_tok();
  else
    id = -1;
  
  cmd=-1;
  for (i=0;i<CMD_ENUM_END;i++)
  {
    if (strcmp(tok,command_names[i])==0)
    {
      cmd=i;
      break;
    }
  }
  
  switch (cmd)
  {
    case CMD_PROTO_VERSION:
      out_success(id);
      printf("2");
      out_end();
      break;
    case CMD_NAME:
      out_success(id);
      printf("Oakfoam");
      out_end();
      break;
    case CMD_VERSION:
      out_success(id);
      printf("0.01");
      out_end();
      break;
    case CMD_KNOWN_COMMAND:
      tok=next_tok();
      if (tok==NULL)
      {
        out_success(id);
        printf("false");
        out_end();
      }
      else
      {
        int found=0;
        for (i=0;i<CMD_ENUM_END;i++)
        {
          if (strcmp(tok,command_names[i])==0)
          {
            out_success(id);
            printf("true");
            out_end();
            found=1;
            break;
          }
        }
        if (!found)
          out_success(id);
          printf("false");
          out_end();
      }
      break;
    case CMD_LIST_COMMANDS:
      out_success(id);
      for (i=0;i<CMD_ENUM_END;i++)
      {
        printf("%s\n",command_names[i]);
      }
      printf("\n");
      break;
    case CMD_QUIT:
      quit=1;
      out_success(id);
      out_end();
      break;
    case CMD_BOARDSIZE:
      tok=next_tok();
      if (tok!=NULL && sscanf(tok,"%d",&i)==1)
      {
        if (i==9)
        {
          think_set_boardsize(i);
          out_success(id);
          out_end();
        }
        else
        {
          out_error(id);
          printf("unacceptable size");
          out_end();
        }
      }
      else
      {
        out_error(id);
        printf("syntax error");
        out_end();
      }
      
      break;
    case CMD_CLEAR_BOARD:
      think_clearboard();
      out_success(id);
      out_end();
      break;
    case CMD_KOMI:
      tok=next_tok();
      if (tok!=NULL && sscanf(tok,"%f",&f)==1)
      {
        think_set_komi(f);
        out_success(id);
        out_end();
      }
      else
      {
        out_error(id);
        printf("syntax error");
        out_end();
      }
      break;
    case CMD_PLAY:
      tok=next_tok();
      if (tok!=NULL)
      {
        switch (tok[0])
        {
          case 'b':
            c=BLACK;
            break;
          case 'w':
            c=WHITE;
            break;
          default:
            c=EMPTY;
            break;
        }
        
        tok=next_tok();
        if (tok!=NULL)
        {
          i=parsevertex(tok,&x,&y);
          if (c!=EMPTY && i==0)
          {
            i=playmove(c,x,y);
            if (i==0)
            {
              out_success(id);
              out_end();
            }
            else
            {
              out_error(id);
              printf("illegal move");
              out_end();
            }
          }
          else
          {
            out_error(id);
            printf("illegal move");
            out_end();
          }
        }
        else
        {
          out_error(id);
          printf("syntax error");
          out_end();
        }
      }
      else
      {
        out_error(id);
        printf("syntax error");
        out_end();
      }
      break;
    case CMD_GENMOVE:
      tok=next_tok();
      if (tok!=NULL)
      {
        switch (tok[0])
        {
          case 'b':
            c=BLACK;
            break;
          case 'w':
            c=WHITE;
            break;
          default:
            c=EMPTY;
            break;
        }
        if (c!=EMPTY)
        {
          out_success(id);
          genmove(c);
          out_end();
        }
        else
        {
          out_error(id);
          printf("invalid color");
          out_end();
        }
      }
      else
      {
        out_error(id);
        printf("syntax error");
        out_end();
      }
      break;
    case CMD_SHOWBOARD:
      out_success(id);
      think_show_board();
      printf("\n");
      break;
    case CMD_SHOWGROUPS:
      out_success(id);
      think_show_groups();
      printf("\n");
      break;
    case CMD_SHOWLIBERTIES:
      out_success(id);
      think_show_liberties();
      printf("\n");
      break;
    case CMD_SHOWEVAL:
      out_success(id);
      think_show_eval();
      printf("\n");
      break;
    case CMD_SCORE:
      out_success(id);
      think_score();
      out_end();
      break;
    default:
      out_error(id);
      printf("unknown command: %s",tok);
      out_end();
      break;
  }
  
  fflush(stdout);
}

void usage()
{
  printf("usage: mango [OPTIONS]\n\n");
  printf("options:\n");
  printf("  -h            display this help\n");
}

int main (int argc, char *argv[])
{
	int i;
  char *buff;

  buff=malloc(1024);
  quit=0;
  
  think_init();
  think_set_boardsize(9);
  think_set_komi(6.5);
  
  for (i=1;i<argc;i++)
  {
    if (strcmp(argv[i],"-h")==0)
    {
      usage();
      exit(0);
    }
  }
  
  while (!quit)
  {
    buff[0]='\0';
    if (!fgets(buff,1024,stdin))
      break;
    if (strlen(buff)==0)
      continue;
    
    evalcommand(buff);
  }
  
  think_stop();
  free(buff);
  return 0;
}
