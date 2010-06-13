#include "Gtp.h"

Gtp::Engine::Engine()
{
  functionlist=NULL;
  constantlist=NULL;
}

Gtp::Engine::~Engine()
{
  if (functionlist!=NULL)
    delete functionlist;
  if (constantlist!=NULL)
    delete constantlist;
}

void Gtp::Engine::run()
{
}

void Gtp::Engine::parseInput(std::string in, int *id, std::string *cmd, Gtp::Arguments **args)
{
  std::string token;
  std::istringstream iss(in);
  std::list<std::string> tokens;
  
  *id=-1; //TODO: check for and get an id
  
  if (!getline(iss, *cmd, ' '))
  {
    cmd=NULL;
    *args=NULL;
    return;
  }
  
  while (getline(iss,token,' '))
    tokens.push_back(token);
  
  *args=new Gtp::Arguments(*cmd,tokens);
}

void Gtp::Engine::doCommand(std::string cmd, Gtp::Arguments *args)
{
  Gtp::Engine::ConstantList *clist=constantlist;
  Gtp::Engine::FunctionList *flist=functionlist;
  
  while (clist!=NULL)
  {
    if (clist->getCommand()==cmd)
    {
      printf("= %s\n",clist->getValue().c_str()); //TODO: replace printf
      return;
    }
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    if (flist->getCommand()==cmd)
    {
      Gtp::Engine::CommandFunction func;
      func=flist->getFunction();
      (*func)(this,args);
      return;
    }
    flist=flist->getNext();
  }
  
  printf("? unknown command\n"); //TODO: replace printf
}

void Gtp::Engine::addConstantCommand(std::string cmd, std::string value)
{
  if (constantlist==NULL)
    constantlist=new Gtp::Engine::ConstantList(cmd,value);
  else
    constantlist->add(new Gtp::Engine::ConstantList(cmd,value));
}

void Gtp::Engine::addFunctionCommand(std::string cmd, Gtp::Engine::CommandFunction func)
{
  if (functionlist==NULL)
    functionlist=new Gtp::Engine::FunctionList(cmd,func);
  else
    functionlist->add(new Gtp::Engine::FunctionList(cmd,func));
}

