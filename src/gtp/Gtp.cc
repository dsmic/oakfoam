#include "Gtp.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#ifdef HAVE_MPI
  #include <mpi.h>
#endif

Gtp::Engine::Engine()
{
  output=new Gtp::Output();
  functionlist=NULL;
  constantlist=NULL;
  
  this->addConstantCommand("protocol_version","2");
  this->addFunctionCommand("list_commands",this,&Gtp::Engine::cmdListCommands);
  this->addFunctionCommand("known_command",this,&Gtp::Engine::cmdKnownCommand);
  
  this->addFunctionCommand("gogui-analyze_commands",this,&Gtp::Engine::cmdAnalyzeCommands);
  this->addConstantCommand("gogui-interrupt","");
  
  workerenabled=true;
  workerthread=new WorkerThread(this);
  interrupt=NULL;
  ponderthread=NULL;
  ponderenabled=false;
}

Gtp::Engine::~Engine()
{
  this->finishLastCommand();
  if (workerthread!=NULL)
    delete workerthread;
  if (ponderthread!=NULL)
  {
    ponderthread->ponderStop();
    delete ponderthread;
  }
  delete output;
  if (functionlist!=NULL)
    delete functionlist;
  if (constantlist!=NULL)
    delete constantlist;
}

void Gtp::Engine::run()
{
  std::string buff;
  
  while (true)
  {
    if (!std::getline(std::cin,buff))
      break;
    
    if (!this->executeCommand(buff))
      break;
  }
  
  this->finishLastCommand();
}

bool Gtp::Engine::executeCommand(std::string line)
{
  std::string cmdname;
  Gtp::Command *cmd;
  bool running=true;
  
  this->parseInput(line,&cmd);
  if (cmd!=NULL)
  {
    cmdname=cmd->getCommandName();
    
    if (cmdname=="quit")
    {
      this->finishLastCommand();
      this->getOutput()->startResponse(cmd);
      this->getOutput()->endResponse();
      running=false;
      delete cmd;
    }
    else
      doCommand(cmd);
  }
  
  return running;
}

void Gtp::Engine::parseInput(std::string in, Gtp::Command **cmd)
{
  std::string token;
  std::vector<std::string> tokens;
  int id;
  std::string cmdname;
  
  this->getOutput()->printfLog("%s\n",in.c_str());
  
  std::string inproper="";
  for (int i=0;i<(int)in.length();i++)
  {
    if (in.at(i)=='#')
    {
      if (interrupt!=NULL && (in.rfind("interrupt")!=((unsigned)-1)))
      {
        *interrupt=true;
        //fprintf(stderr,"interrupt!!!\n");
      }
      break;
    }
    else if (in.at(i)=='\t')
      inproper+=' ';
    else if (in.at(i)=='\n' || ((int)in.at(i)>=32 && (int)in.at(i)<127))
      inproper+=in.at(i);
  }
  std::istringstream iss(inproper);
  
  if (inproper.length()==0 || !getline(iss, cmdname, ' ') || cmdname.length()==0)
  {
    *cmd=NULL;
    return;
  }
  
  std::istringstream issid(cmdname);
  if (issid >> id)
  {
    if (!getline(iss, cmdname, ' ') || cmdname.length()==0)
    {
      *cmd=NULL;
      return;
    }
  }
  else
    id=-1;
  
  std::transform(cmdname.begin(),cmdname.end(),cmdname.begin(),::tolower);
  
  std::string args;
  if (getline(iss,args))
  {
    bool instring=false;
    std::string arg="";
    char prevc='\0';
    for (int i=0;i<(int)args.length();i++)
    {
      char c=args.at(i);
      if (c==' ' && !instring)
      {
        tokens.push_back(arg);
        arg="";
      }
      else if (c=='"' && instring)
      {
        if (prevc!='\\')
          instring=false;
        else
          arg[arg.length()-1]='"';
      }
      else if (c=='"' && (prevc==' ' || prevc=='\0'))
        instring=true;
      else if (c=='\n' || ((int)c>=32 && (int)c<127))
        arg+=c;
      prevc=c;
    }
    if (arg.length()>0)
      tokens.push_back(arg);
  }
  
  *cmd=new Gtp::Command(id,cmdname,tokens);
}

void Gtp::Engine::doCommand(Gtp::Command *cmd)
{
  Gtp::Engine::ConstantList *clist=constantlist;
  Gtp::Engine::FunctionList *flist=functionlist;
  
  this->finishLastCommand();
  
  while (clist!=NULL)
  {
    if (clist->getCommandName()==cmd->getCommandName())
    {
      this->getOutput()->startResponse(cmd);
      this->getOutput()->printString(clist->getValue());
      this->getOutput()->endResponse();
      delete cmd;
      return;
    }
    clist=clist->getNext();
  }
  
  while (flist!=NULL)
  {
    if (flist->getCommandName()==cmd->getCommandName())
    {
      if (workerenabled)
        workerthread->doCmd(flist,cmd,&Gtp::Engine::startPonderingWrapper);
      else
      {
        Gtp::Engine::CommandFunction func=flist->getFunction();
        (*func)(flist->getInstance(),this,cmd);
        delete cmd;
        this->startPondering();
      }
      return;
    }
    flist=flist->getNext();
  }
  
  this->getOutput()->startResponse(cmd,false);
  this->getOutput()->printString("unknown command");
  this->getOutput()->endResponse();
  delete cmd;
}

void Gtp::Engine::finishLastCommand()
{
  boost::mutex::scoped_lock lock(workerbusy);
  this->stopPondering();
}

void Gtp::Engine::setPonderer(Gtp::Engine::PonderFunction f, void *i, volatile bool *s)
{
  if (ponderthread!=NULL)
  {
    ponderthread->ponderStop();
    delete ponderthread;
    ponderthread=NULL;
  }
  if (f!=NULL)
    ponderthread=new PonderThread(f,i,s);
}

Gtp::Engine::WorkerThread::WorkerThread(Gtp::Engine *eng)
  : engine(eng),
    running(true),
    startbarrier(2),
    thisthread(Gtp::Engine::WorkerThread::Worker(this))
{
}

Gtp::Engine::WorkerThread::~WorkerThread()
{
  running=false;
  startbarrier.wait();
  thisthread.join();
}

void Gtp::Engine::WorkerThread::Worker::operator()()
{
  workerthread->run();
}

void Gtp::Engine::WorkerThread::run()
{
  while (running)
  {
    startbarrier.wait();
    if (!running)
      break;
    //fprintf(stderr,"job start %p %p %p\n",engine,funcitem,cmd);
    Gtp::Engine::CommandFunction func=funcitem->getFunction();
    (*func)(funcitem->getInstance(),engine,cmd);
    delete cmd;
    //fprintf(stderr,"job done\n");
    (*ponderfunc)(engine);
    delete lock;
  }
}

void Gtp::Engine::WorkerThread::doCmd(Gtp::Engine::FunctionList *fi, Gtp::Command *c, Gtp::Engine::PonderFunction pf)
{
  lock=new boost::mutex::scoped_lock(engine->workerbusy);
  funcitem=fi;
  cmd=c;
  ponderfunc=pf;
  startbarrier.wait();
}

Gtp::Engine::PonderThread::PonderThread(Gtp::Engine::PonderFunction f, void *i, volatile bool *s)
  : func(f),
    instance(i),
    stop(s),
    running(true),
    startbarrier(2),
    thisthread(Gtp::Engine::PonderThread::Worker(this))
{
}

Gtp::Engine::PonderThread::~PonderThread()
{
  this->ponderStop();
  running=false;
  startbarrier.wait();
  thisthread.join();
}

void Gtp::Engine::PonderThread::Worker::operator()()
{
  ponderthread->run();
}

void Gtp::Engine::PonderThread::run()
{
  while (true)
  {
    startbarrier.wait();
    if (!running)
      break;
    //fprintf(stderr,"pondering start\n");
    (*func)(instance);
    //fprintf(stderr,"pondering done\n");
    delete lock;
  }
}

void Gtp::Engine::PonderThread::ponderStart()
{
  lock=new boost::mutex::scoped_lock(busymutex);
  *stop=false;
  startbarrier.wait();
}

void Gtp::Engine::PonderThread::ponderStop()
{
  //fprintf(stderr,"pondering stop\n");
  *stop=true;
  boost::mutex::scoped_lock lock(busymutex);
  //fprintf(stderr,"pondering stopped\n");
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
  if (!outputon)
    return;
  
  fflush(stdout);
  fflush(stderr);
  
  this->printf((success ? "=" : "?"));
  if (cmd->getId()>=0)
    this->printf("%d",cmd->getId());
  this->printf(" ");
}

void Gtp::Output::endResponse(bool single)
{
  if (!outputon)
    return;
  
  this->printf("\n");
  if (!single)
    this->printf("\n");
  
  fflush(stdout);
  fflush(stderr);
}

void Gtp::Output::printString(std::string str)
{
  this->printf(str);
}

unsigned int Gtp::Command::numArgs() const
{
  return arguments.size();
}

std::string Gtp::Command::getStringArg(int i) const
{
  if (i<0 || i>=((int)arguments.size()))
    return "";
  else
    return arguments.at(i);
}

int Gtp::Command::getIntArg(int i) const
{
  std::string arg=this->getStringArg(i);
  std::istringstream iss(arg);
  int ret;
  
  if (iss >> ret)
    return ret;
  else
    return 0;
}

float Gtp::Command::getFloatArg(int i) const
{
  std::string arg=this->getStringArg(i);
  std::istringstream iss(arg);
  float ret;
  
  if (iss >> ret)
    return ret;
  else
    return 0;
}

Gtp::Color Gtp::Command::getColorArg(int i) const
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

Gtp::Vertex Gtp::Command::getVertexArg(int i) const
{
  std::string arg=this->getStringArg(i);
  std::transform(arg.begin(),arg.end(),arg.begin(),::tolower);
  Gtp::Vertex vert;
  Gtp::Vertex invalidvertex = {-3,-3};
  
  if (arg.length()<2)
    return invalidvertex;
  
  if (arg=="pass")
  {
    vert.x=-1;
    vert.y=-1;
    return vert;
  }
  else if (arg=="resign")
  {
    vert.x=-2;
    vert.y=-2;
    return vert;
  }
  
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
  if (vert.x==-1 && vert.y==-1)
    this->printf("pass");
  else if (vert.x==-2 && vert.y==-2)
    this->printf("resign");
  else
  {
    char xletter='a'+vert.x;
    if (vert.x>=8)
      xletter++;
    this->printf("%c%d",xletter,(vert.y+1));
  }
}

void Gtp::Output::printScore(float score)
{
  if (score==0)
    this->printf("0");
  else if (score>0)
    this->printf("B+%.1f",score);
  else
    this->printf("W+%.1f",-score);
}

void Gtp::Output::printDebugString(std::string str)
{
  this->printfDebug(str);
}

void Gtp::Output::printf(std::string format,...)
{
  if (!outputon)
    return;
  
  std::va_list ap;
  va_start (ap, format);
  vfprintf (stdout, format.c_str(), ap);
  va_end (ap);
  
  va_start (ap, format);
  if (logfile!=NULL)
    vfprintf (logfile, format.c_str(), ap);
  va_end (ap);
  
  if (logfile!=NULL)
    fflush(logfile);

  if (sout!=NULL)
  {
    va_start (ap, format);
    char buffer[4096];
    vsprintf(buffer, format.c_str(), ap);
    *sout+=buffer;
    va_end (ap);
  }
}

void Gtp::Output::printfDebug(std::string format,...)
{
  if (!outputon)
    return;
  
  std::va_list ap;
  va_start (ap, format);
  vfprintf (stderr, format.c_str(), ap);
  va_end (ap);
  
  va_start (ap, format);
  if (logfile!=NULL)
    vfprintf (logfile, format.c_str(), ap);
  va_end (ap);
  
  if (logfile!=NULL)
    fflush(logfile);

  if (serr!=NULL)
  {
    va_start (ap, format);
    char buffer[4096];
    vsprintf(buffer, format.c_str(), ap);
    *serr+=buffer;
    va_end (ap);
  }
}

void Gtp::Output::printfLog(std::string format,...)
{
  if (!outputon)
    return;
  
  std::va_list ap;
  va_start (ap, format);
  if (logfile!=NULL)
    vfprintf (logfile, format.c_str(), ap);
  va_end (ap);
  
  if (logfile!=NULL)
    fflush(logfile);
}

void Gtp::Output::printDebugVertex(Gtp::Vertex vert)
{ 
  if (vert.x==-1 && vert.y==-1)
    this->printfDebug("pass");
  else if (vert.x==-2 && vert.y==-2)
    this->printfDebug("resign");
  else
  {
    char xletter='a'+vert.x;
    if (vert.x>=8)
      xletter++;
    this->printfDebug("%c%d",xletter,(vert.y+1));
  }
}

void Gtp::Engine::addAnalyzeCommand(std::string cmd, std::string label, std::string type)
{
  std::transform(cmd.begin(),cmd.end(),cmd.begin(),::tolower);
  std::transform(type.begin(),type.end(),type.begin(),::tolower);
  std::string entry=type+"/"+label+"/"+cmd;
  analyzeList.push_back(entry);
}

void Gtp::Engine::cmdAnalyzeCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd)
{
  gtpe->getOutput()->startResponse(cmd);
  
  for(std::list<std::string>::iterator iter=gtpe->analyzeList.begin();iter!=gtpe->analyzeList.end();++iter)
  {
    gtpe->getOutput()->printString((*iter)+"\n");
  }
  
  gtpe->getOutput()->endResponse(true);
}


