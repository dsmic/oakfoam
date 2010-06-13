#include "Gtp.h"

Gtp::Engine::Engine()
{
  output=new Gtp::Output();
  functionlist=NULL;
  constantlist=NULL;
  
  this->addConstantCommand("protocol_version","2");
  this->addFunctionCommand("list_commands",this,&Gtp::Engine::cmdListCommands);
  this->addFunctionCommand("known_command",this,&Gtp::Engine::cmdKnownCommand);
}

Gtp::Engine::~Engine()
{
  delete output;
  if (functionlist!=NULL)
    delete functionlist;
  if (constantlist!=NULL)
    delete constantlist;
}

void Gtp::Engine::run()
{
  int id;
  std::string buff,cmdname;
  Gtp::Command *cmd;
  bool running=true;
  
  while (running)
  {
    if (!std::getline(std::cin,buff))
      break;
    
    //TODO: remove invalid chars
    
    this->parseInput(std::string(buff),&cmd);
    id=cmd->getId();
    cmdname=cmd->getCommandName();
    
    if (cmdname=="quit")
    {
      this->getOutput()->startResponse(cmd);
      this->getOutput()->endResponse();
      running=false;
    }
    else
      doCommand(cmd);
    
    std::cout.flush();
    
    delete cmd;
  }
}

void Gtp::Engine::parseInput(std::string in, Gtp::Command **cmd)
{
  std::string token;
  std::istringstream iss(in);
  std::vector<std::string> tokens;
  int id;
  std::string cmdname;
  
  id=-1; //TODO: check for and get an id
  
  if (!getline(iss, cmdname, ' '))
  {
    *cmd=NULL;
    return;
  }
  
  std::transform(cmdname.begin(),cmdname.end(),cmdname.begin(),::tolower);
  
  while (getline(iss,token,' '))
    tokens.push_back(token);
  
  *cmd=new Gtp::Command(id,cmdname,tokens);
}

void Gtp::Engine::doCommand(Gtp::Command *cmd)
{
  Gtp::Engine::ConstantList *clist=constantlist;
  Gtp::Engine::FunctionList *flist=functionlist;
  
  while (clist!=NULL)
  {
    if (clist->getCommandName()==cmd->getCommandName())
    {
      this->getOutput()->startResponse(cmd);
      this->getOutput()->printString(clist->getValue());
      this->getOutput()->endResponse();
      return;
    }
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    if (flist->getCommandName()==cmd->getCommandName())
    {
      Gtp::Engine::CommandFunction func;
      func=flist->getFunction();
      (*func)(flist->getInstance(),this,cmd);
      return;
    }
    flist=flist->getNext();
  }
  
  this->getOutput()->startResponse(cmd,false);
  this->getOutput()->printString("unknown command");
  this->getOutput()->endResponse();
}

void Gtp::Engine::addConstantCommand(std::string cmd, std::string value)
{
  if (constantlist==NULL)
    constantlist=new Gtp::Engine::ConstantList(cmd,value);
  else
    constantlist->add(new Gtp::Engine::ConstantList(cmd,value));
}

void Gtp::Engine::addFunctionCommand(std::string cmd, void *inst, Gtp::Engine::CommandFunction func)
{
  if (functionlist==NULL)
    functionlist=new Gtp::Engine::FunctionList(cmd,inst,func);
  else
    functionlist->add(new Gtp::Engine::FunctionList(cmd,inst,func));
}

void Gtp::Engine::cmdListCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Gtp::Engine::ConstantList *clist=gtpe->constantlist;
  Gtp::Engine::FunctionList *flist=gtpe->functionlist;
  
  gtpe->getOutput()->startResponse(cmd);
  
  while (clist!=NULL)
  {
    gtpe->getOutput()->printString(clist->getCommandName()+"\n");
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    gtpe->getOutput()->printString(flist->getCommandName()+"\n");
    flist=flist->getNext();
  }
  
  gtpe->getOutput()->printString("quit");
  gtpe->getOutput()->endResponse();
}

void Gtp::Engine::cmdKnownCommand(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Gtp::Engine::ConstantList *clist=gtpe->constantlist;
  Gtp::Engine::FunctionList *flist=gtpe->functionlist;
  std::string possiblecmd;
  
  possiblecmd=cmd->getStringArg(0);
  std::transform(possiblecmd.begin(),possiblecmd.end(),possiblecmd.begin(),::tolower);
  
  if (possiblecmd=="quit")
  {
    gtpe->getOutput()->startResponse(cmd);
    gtpe->getOutput()->printString("true");
    gtpe->getOutput()->endResponse();
    return;
  }
  
  while (clist!=NULL)
  {
    if (clist->getCommandName()==possiblecmd)
    {
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->printString("true");
      gtpe->getOutput()->endResponse();
      return;
    }
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    if (flist->getCommandName()==possiblecmd)
    {
      gtpe->getOutput()->startResponse(cmd);
      gtpe->getOutput()->printString("true");
      gtpe->getOutput()->endResponse();
      return;
    }
    flist=flist->getNext();
  }
  
  gtpe->getOutput()->startResponse(cmd);
  gtpe->getOutput()->printString("false");
  gtpe->getOutput()->endResponse();
}

void Gtp::Output::startResponse(Gtp::Command *cmd, bool success)
{
  std::cout << (success ? "=" : "?");
  if (cmd->getId()>=0)
    std::cout << cmd->getId();
  std::cout << " ";
}

void Gtp::Output::endResponse(bool single)
{
  std::cout << std::endl;
  if (!single)
    std::cout << std::endl;
}

void Gtp::Output::printString(std::string str)
{
  std::cout << str;
}

unsigned int Gtp::Command::numArgs()
{
  return arguments.size();
}

std::string Gtp::Command::getStringArg(int i)
{
  if (i<0 || i>=((int)arguments.size()))
    return "";
  else
    return arguments.at(i);
}

int Gtp::Command::getIntArg(int i)
{
  std::string arg=this->getStringArg(i);
  std::istringstream iss(arg);
  int ret;
  
  if (iss >> ret)
    return ret;
  else
    return 0;
}

float Gtp::Command::getFloatArg(int i)
{
  std::string arg=this->getStringArg(i);
  std::istringstream iss(arg);
  float ret;
  
  if (iss >> ret)
    return ret;
  else
    return 0;
}

Gtp::Color Gtp::Command::getColorArg(int i)
{
  std::string arg=this->getStringArg(i);
  std::transform(arg.begin(),arg.end(),arg.begin(),::tolower);
  
  if (arg=="b" || arg=="black")
    return Gtp::BLACK;
  else if (arg=="w" || arg=="white")
    return Gtp::WHITE;
  else
    return Gtp::INVALID;
}

Gtp::Vertex Gtp::Command::getVertexArg(int i)
{
  std::string arg=this->getStringArg(i);
  std::transform(arg.begin(),arg.end(),arg.begin(),::tolower);
  Gtp::Vertex vert;
  Gtp::Vertex invalidvertex = {-1,-1};
  
  if (arg.length()<2)
    return invalidvertex;
  
  char xletter=arg.at(0);
  vert.x=((int)xletter)-'a';
  if (vert.x>=8)
    vert.x--;
  
  std::istringstream iss(arg.substr(1));
  if (iss >> vert.y)
  {
    vert.y--;
    return vert;
  }
  else
    return invalidvertex;
}

void Gtp::Output::printVertex(Gtp::Vertex vert)
{
  char xletter='A'+vert.x;
  if (vert.x>=8)
    xletter++;
  std::cout << xletter << (vert.y+1); //TODO:: not correct
}


