#include "Gtp.h"

Gtp::Engine::Engine()
{
  output=new Gtp::Output();
  functionlist=NULL;
  constantlist=NULL;
  
  this->addConstantCommand("protocol_version","2");
  this->addFunctionCommand("list_commands",&Gtp::Engine::cmdListCommands);
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
  std::list<std::string> tokens;
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
      (*func)(this,cmd);
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

void Gtp::Engine::addFunctionCommand(std::string cmd, Gtp::Engine::CommandFunction func)
{
  if (functionlist==NULL)
    functionlist=new Gtp::Engine::FunctionList(cmd,func);
  else
    functionlist->add(new Gtp::Engine::FunctionList(cmd,func));
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

void Gtp::Engine::cmdListCommands(Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  Gtp::Engine::ConstantList *clist=gtpe->constantlist;
  Gtp::Engine::FunctionList *flist=gtpe->functionlist;
  
  gtpe->getOutput()->startResponse(cmd);
  
  while (clist!=NULL)
  {
    gtpe->getOutput()->printString(clist->getCommandName());
    gtpe->getOutput()->printString("\n");
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    gtpe->getOutput()->printString(flist->getCommandName());
    gtpe->getOutput()->printString("\n");
    flist=flist->getNext();
  }
  
  gtpe->getOutput()->printString("quit");
  gtpe->getOutput()->endResponse();
}

