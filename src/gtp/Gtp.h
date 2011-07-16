#ifndef DEF_OAKFOAM_GTP_H
#define DEF_OAKFOAM_GTP_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/function.hpp>

namespace Gtp
{
  enum Color
  {
    BLACK,
    WHITE,
    INVALID
  };
  
  struct Vertex
  {
    int x;
    int y;
  };
  
  class Command
  {
    public:
      Command(int i, std::string cmdname, std::vector<std::string> args) { id=i; commandname=cmdname; arguments=args; };
      
      int getId() const { return id; };
      std::string getCommandName() const { return commandname; };
      
      unsigned int numArgs() const;
      std::string getStringArg(int i) const;
      int getIntArg(int i) const;
      float getFloatArg(int i) const;
      Gtp::Color getColorArg(int i) const;
      Gtp::Vertex getVertexArg(int i) const;
      
    private:
      int id;
      std::string commandname;
      std::vector<std::string> arguments;
  };
  
  class Output
  {
    public:
      Output() { outputon=true; logfile=NULL; };
      ~Output() { if (logfile!=NULL) { fclose(logfile); }};
      
      bool isOutputOn() const { return outputon; };
      void setOutputOn(bool outon) { outputon=outon; };
      void setLogFile(FILE *lf) { logfile=lf; };
      
      void startResponse(Gtp::Command *cmd, bool success = true);
      void endResponse(bool single = false);
      
      void printString(std::string str);
      void printVertex(Gtp::Vertex vert);
      void printScore(float score);
      void printf(std::string format,...);
      
      void printDebugString(std::string str);
      void printfDebug(std::string format,...);
      void printDebugVertex(Gtp::Vertex vert);
      
      void printfLog(std::string format,...);
    
    private:
      bool outputon;
      FILE *logfile;
  };
  
  class Engine
  {
    public:
      Engine();
      ~Engine();
      
      typedef void (*CommandFunction)(void *instance, Gtp::Engine*, Gtp::Command*);
      typedef void (*PonderFunction)(void *instance);
      
      class FunctionList
      {
        public:
          FunctionList(std::string cmdname, void *inst, Gtp::Engine::CommandFunction func) { commandname=cmdname; instance=inst; function=func; next=NULL; };
          ~FunctionList() { if (next!=NULL) delete next; };
          
          std::string getCommandName() const { return commandname; };
          void *getInstance() const { return instance; };
          Gtp::Engine::CommandFunction getFunction() const { return function; };
          void setNext(Gtp::Engine::FunctionList *n) { next=n; };
          Gtp::Engine::FunctionList *getNext() const { return next; };
          void add(Gtp::Engine::FunctionList *newfunclist)
          {
            if (next==NULL)
              next=newfunclist;
            else
              next->add(newfunclist);
          };
        
        private:
          std::string commandname;
          void *instance;
          Gtp::Engine::CommandFunction function;
          Gtp::Engine::FunctionList *next;
      };
      
      class ConstantList
      {
        public:
          ConstantList(std::string cmdname, std::string val) { commandname=cmdname; value=val; next=NULL; };
          ~ConstantList() { if (next!=NULL) delete next; };
          
          std::string getCommandName() const { return commandname; };
          std::string getValue() const { return value; };
          void setNext(Gtp::Engine::ConstantList *n) { next=n; };
          Gtp::Engine::ConstantList *getNext() const { return next; };
          void add(Gtp::Engine::ConstantList *newconstlist)
          {
            if (next==NULL)
              next=newconstlist;
            else
              next->add(newconstlist);
          };
        
        private:
          std::string commandname;
          std::string value;
          Gtp::Engine::ConstantList *next;
      };
      
      void run();
      
      void addConstantCommand(std::string cmd, std::string value);
      void addFunctionCommand(std::string cmd, void *inst, Gtp::Engine::CommandFunction func);
      void addAnalyzeCommand(std::string cmd, std::string label, std::string type);
      void setInterruptFlag(volatile bool *intr) { interrupt=intr; };
      void setPonderer(Gtp::Engine::PonderFunction f, void *i, volatile bool *s);
      void setWorkerEnabled(bool en) { workerenabled=en; };
      void setPonderEnabled(bool en) { ponderenabled=en; };
      
      bool executeCommand(std::string line);
      void finishLastCommand();
      static void startPonderingWrapper(void *instance) { ((Gtp::Engine*)instance)->startPondering(); };
      void startPondering() { if (ponderenabled && ponderthread!=NULL) ponderthread->ponderStart(); };
      void stopPondering() { if (ponderenabled && ponderthread!=NULL) ponderthread->ponderStop(); };
      
      Gtp::Output *getOutput() const { return output; };
      
    private:
      class WorkerThread
      {
        public:
          WorkerThread(Gtp::Engine *eng);
          ~WorkerThread();
          
          void doCmd(Gtp::Engine::FunctionList *fi, Gtp::Command *c, Gtp::Engine::PonderFunction pf);
          void run();
        
        private:
          class Worker
          {
            public:
              Worker(WorkerThread *wt) { workerthread=wt; };

              void operator()();

            private:
              WorkerThread *workerthread;
          };
          
          Gtp::Engine *engine;
          Gtp::Engine::FunctionList *funcitem;
          Gtp::Command *cmd;
          Gtp::Engine::PonderFunction ponderfunc;
          
          bool running;
          boost::barrier startbarrier;
          boost::mutex::scoped_lock *lock;
          boost::thread thisthread;
      };
      
      class PonderThread
      {
        public:
          PonderThread(Gtp::Engine::PonderFunction f, void *i, volatile bool *s);
          ~PonderThread();
          
          void ponderStart();
          void ponderStop();
          void run();
        
        private:
          class Worker
          {
            public:
              Worker(PonderThread *pt) { ponderthread=pt; };

              void operator()();

            private:
              PonderThread *ponderthread;
          };
          
          Gtp::Engine::PonderFunction func;
          void *instance;
          volatile bool *stop;
          
          bool running;
          boost::barrier startbarrier;
          boost::mutex busymutex;
          boost::mutex::scoped_lock *lock;
          boost::thread thisthread;
      };
      
      Gtp::Engine::FunctionList *functionlist;
      Gtp::Engine::ConstantList *constantlist;
      Gtp::Output *output;
      std::list<std::string> analyzeList;
      bool workerenabled;
      Gtp::Engine::WorkerThread *workerthread;
      boost::mutex workerbusy;
      Gtp::Engine::PonderThread *ponderthread;
      volatile bool *interrupt;
      bool ponderenabled;
      
      void parseInput(std::string in, Gtp::Command **cmd);
      void doCommand(Gtp::Command *cmd);
      
      static void cmdListCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
      static void cmdKnownCommand(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
      static void cmdAnalyzeCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  };
};

#endif
