#include "Gtp.h"

Engine engine;
int quit;

Go::Color gocol(color_type color)
{
  if (color==BLACK)
    return Go::BLACK;
  else if (color==WHITE)
    return Go::WHITE;
  else
    return Go::EMPTY;
}

void printmove(color_type color, int x, int y)
{
  char c;
  
  if (color==EMPTY)
    printf("PASS");
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
    //if (!think_is_move_allowed(color,x,y))
    if (!engine.isMoveAllowed(Go::Move(gocol(color),x,y)))
      return 1;
    
    //think_make_move_on_board(color,x,y);
    engine.makeMove(Go::Move(gocol(color),x,y));
  }
  else
  {
    //think_make_move_on_board(EMPTY,0,0);
    engine.makeMove(Go::Move(gocol(color),Go::Move::PASS));
  }
  return 0;
}

void genmove(color_type color)
{
  int x,y,ret;
  Go::Move *move;
  
  //ret=think_generate_move(color,&x,&y);
  //ret=1;
  //x=-1;
  //y=-1;
  engine.generateMove(gocol(color),&move);
  
  /*if (ret==0 && !(x==-1 && y==-1))
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
  }*/
  
  if (move->isPass())
    printf("PASS");
  else if (move->isResign())
    printf("RESIGN");
  else
    printmove(color,move->getX(),move->getY());
  
  delete move;
}

bool validmove(color_type color, int x, int y)
{
  
  if (!(x==-1 && y==-1))
    return engine.isMoveAllowed(Go::Move(gocol(color),x,y));
  else
    return engine.isMoveAllowed(Go::Move(gocol(color),Go::Move::PASS));
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
    
    if ((*x)>=0 && (*x)<engine.getBoardSize() && (*y)>=0 && (*y)<engine.getBoardSize())
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
  
  for (i=0;(unsigned int)i<strlen(cmdstr);i++)
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
  
  cmd=CMD_ENUM_END;
  for (i=0;i<CMD_ENUM_END;i++)
  {
    if (strcmp(tok,command_names[i])==0)
    {
      cmd=(command_type)i;
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
      printf(PACKAGE_NAME);
      out_end();
      break;
    case CMD_VERSION:
      out_success(id);
      printf(PACKAGE_VERSION);
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
          //think_set_boardsize(i);
          engine.setBoardSize(i);
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
      //think_clearboard();
      engine.clearBoard();
      out_success(id);
      out_end();
      break;
    case CMD_KOMI:
      tok=next_tok();
      if (tok!=NULL && sscanf(tok,"%f",&f)==1)
      {
        //think_set_komi(f);
        engine.setKomi(f);
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
    case CMD_VALIDMOVE:
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
            //i=playmove(c,x,y);
            if (validmove(c,x,y))
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
    case CMD_SHOWBOARD:
      out_success(id);
      //think_show_board();
      printf("\n");
      {
        int size=engine.getBoardSize();
        for (y=size-1;y>=0;y--)
        {
          for (x=0;x<size;x++)
          {
            Go::Color col=engine.getCurrentBoard()->boardData()[y*size+x].color;
            if (col==Go::BLACK)
              printf("X ");
            else if (col==Go::WHITE)
              printf("O ");
            else
              printf(". ");
          }
          printf("\n");
        }
      }
      printf("\n");
      break;
    case CMD_SHOWGROUPS:
      out_success(id);
      //think_show_groups();
      printf("\n");
      {
        int size=engine.getBoardSize();
        for (y=size-1;y>=0;y--)
        {
          for (x=0;x<size;x++)
          {
            int group=engine.getCurrentBoard()->boardData()[y*size+x].group;
            if (group!=-1)
              printf("%2d",group);
            else
              printf(". ");
          }
          printf("\n");
        }
      }
      printf("\n");
      break;
    case CMD_SHOWLIBERTIES:
      out_success(id);
      //think_show_liberties();
      printf("\n");
      {
        int size=engine.getBoardSize();
        for (y=size-1;y>=0;y--)
        {
          for (x=0;x<size;x++)
          {
            int liberties=engine.getCurrentBoard()->boardData()[y*size+x].liberties;
            if (liberties!=-1)
              printf("%2d",liberties);
            else
              printf(". ");
          }
          printf("\n");
        }
      }
      printf("\n");
      break;
    case CMD_SHOWEVAL:
      out_success(id);
      //think_show_eval();
      printf("\n");
      break;
    case CMD_SCORE:
      out_success(id);
      //think_score();
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

void Gtp::run()
{
  int i;
  char *buff;

  //buff=malloc(1024);
  buff=new char[1024];
  quit=0;
  
  //think_init();
  engine.init();
  //think_set_boardsize(9);
  engine.setBoardSize(9);
  //think_set_komi(5.5);
  engine.setKomi(5.5);
  
  while (!quit)
  {
    buff[0]='\0';
    if (!fgets(buff,1024,stdin))
      break;
    if (strlen(buff)==0)
      continue;
    
    evalcommand(buff);
  }
  
  //think_stop();
  //free(buff);
}

void Gtp::setEngine(Engine eng)
{
  engine=eng;
}


