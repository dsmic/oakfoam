#ifndef DEF_OAKFOAM_GTP_H
#define DEF_OAKFOAM_GTP_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>
#include <list>
#define BOOST_THREAD_USE_LIB
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/function.hpp>

/** GTP Interface.
 * Contains all the necessarly components to setup a GTP
 * session.
 */
namespace Gtp
{
  /**
   * GTP Color
   */
  enum Color
  {
    BLACK,
    WHITE,
    INVALID
  };
  
  /**
   * GTP Vertex
   */
  struct Vertex
  {
    int x; /**< The x coordinate. */
    int y; /**< The y coordinate. */
  };
  
  /** GTP Command.
   * Contains a breakdown of a GTP command.
   */
  class Command
  {
    public:
      /** Create a Command object.
       * @param i         ID of the command.
       * @param cmdname   Name of the command.
       * @param args      List of arguments.
       */
      Command(int i, std::string cmdname, std::vector<std::string> args) { id=i; commandname=cmdname; arguments=args; };
      
      /** The ID for this command. */
      int getId() const { return id; };
      /** The name of this command. */
      std::string getCommandName() const { return commandname; };
      
      /** The number of command arguments. */
      unsigned int numArgs() const;
      /** Return the @p i'th argument, converted to a std::string. */
      std::string getStringArg(int i) const;
      /** Return the @p i'th argument, converted to an int. */
      int getIntArg(int i) const;
      /** Return the @p i'th argument, converted to a float. */
      float getFloatArg(int i) const;
      /** Return the @p i'th argument, converted to a Gtp::Color. */
      Gtp::Color getColorArg(int i) const;
      /** Return the @p i'th argument, converted to a Gtp::Vertex. */
      Gtp::Vertex getVertexArg(int i) const;
      
    private:
      int id;
      std::string commandname;
      std::vector<std::string> arguments;
  };
  
  /** GTP Output.
   * Provides a method to respond to a command.
   */
  class Output
  {
    public:
      Output() { outputon=true; logfile=NULL; sout=serr=NULL; };
      ~Output() { if (logfile!=NULL) { fclose(logfile); }};
      
      /** Will anything be written to stdout? */
      bool isOutputOn() const { return outputon; };
      /** Set whether out is written to stdout. */
      void setOutputOn(bool outon) { outputon=outon; };
      /** Set the log file. */
      void setLogFile(FILE *lf) { logfile=lf; };
      /** Set redirect string(s).
       * All stdout is also appended to @p so and all stderr to @p se.
       * Set either to NULL to disable.
       */
      void setRedirectStrings(std::string *so, std::string *se) { sout=so; serr=se; }
      
      /** Begin a response to a command. */
      void startResponse(Gtp::Command *cmd, bool success = true);
      /** Finish a response to a command. */
      void endResponse(bool single = false);
      
      /** Output a std::string. */
      void printString(std::string str);
      /** Output a Gtp::Vertex. */
      void printVertex(Gtp::Vertex vert);
      /** Output a formatted score. */
      void printScore(float score);
      /** Output a formatted std::string. */
      void printf(std::string format,...);
      
      /** Output a debug std::string. */
      void printDebugString(std::string str);
      /** Output a formatted debug std::string. */
      void printfDebug(std::string format,...);
      /** Output a debug vertex. */
      void printDebugVertex(Gtp::Vertex vert);
      
      /** Output a formatted std::string to the log file only. */
      void printfLog(std::string format,...);
    
    private:
      bool outputon;
      FILE *logfile;
      std::string *sout, *serr;
  };
  
  /** GTP Engine.
   * Engine for managing a GTP conenction.
   */
  class Engine
  {
    public:
      Engine();
      ~Engine();
      
      /** A function to be called when a specific GTP command is received. */
      typedef void (*CommandFunction)(void *instance, Gtp::Engine*, Gtp::Command*);
      /** Function called when pondering can happen. */
      typedef void (*PonderFunction)(void *instance);
      
      /** A list of CommandFunction's.
       * Should be replaced with std::list<CommandFunction>.
       */
      class FunctionList
      {
        public:
          /** Create a function command with given name, instance, and function. */
          FunctionList(std::string cmdname, void *inst, Gtp::Engine::CommandFunction func) { commandname=cmdname; instance=inst; function=func; next=NULL; };
          ~FunctionList() { if (next!=NULL) delete next; };
          
          /** Get the command name. */
          std::string getCommandName() const { return commandname; };
          /** Get the instance variable. */
          void *getInstance() const { return instance; };
          /** Get the function. */
          Gtp::Engine::CommandFunction getFunction() const { return function; };

          /** Set the next function command in the list. */
          void setNext(Gtp::Engine::FunctionList *n) { next=n; };
          /** Get the next function command in the list. */
          Gtp::Engine::FunctionList *getNext() const { return next; };
          /** Add a function command to the list. */
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
      
      /** A list of GTP constants.
       * Should be replace with std::list<std::string>.
       */
      class ConstantList
      {
        public:
          /** Create a constant command with given name and value. */
          ConstantList(std::string cmdname, std::string val) { commandname=cmdname; value=val; next=NULL; };
          ~ConstantList() { if (next!=NULL) delete next; };
          
          /** Get the command name. */
          std::string getCommandName() const { return commandname; };
          /** Get the constant command value. */
          std::string getValue() const { return value; };

          /** Set the next command in the list. */
          void setNext(Gtp::Engine::ConstantList *n) { next=n; };
          /** Get the next command in the list. */
          Gtp::Engine::ConstantList *getNext() const { return next; };
          /** Add a command to the list. */
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
      
      /** Manage a GTP connection. */
      void run();
      
      /** Add a new command that always returns the same value. */
      void addConstantCommand(std::string cmd, std::string value);
      /** Add a new command that is represented by a function. */
      void addFunctionCommand(std::string cmd, void *inst, Gtp::Engine::CommandFunction func);
      /** Add a command to the list of GoGui analyse commands. */
      void addAnalyzeCommand(std::string cmd, std::string label, std::string type);
      /** Set a pointer a flag for interrupting. */
      void setInterruptFlag(volatile bool *intr) { interrupt=intr; };
      /** Set the pondering function. */
      void setPonderer(Gtp::Engine::PonderFunction f, void *i, volatile bool *s);
      /** Set whether workers are used for commands (to allow interrupting). */
      void setWorkerEnabled(bool en) { workerenabled=en; };
      /** Set whether pondering occurs. */
      void setPonderEnabled(bool en) { ponderenabled=en; };
      
      /** Interprete a line as a GTP command and execute it. */
      bool executeCommand(std::string line);
      /** Make sure the last command is finished. */
      void finishLastCommand();
      /** Wrapper for pondering. */
      static void startPonderingWrapper(void *instance) { ((Gtp::Engine*)instance)->startPondering(); };
      /** Start pondering. */
      void startPondering() { if (ponderenabled && ponderthread!=NULL) ponderthread->ponderStart(); };
      /** Stop pondering. */
      void stopPondering() { if (ponderenabled && ponderthread!=NULL) ponderthread->ponderStop(); };
      
      /** Return a pointer to a Gtp::Output object. */
      Gtp::Output *getOutput() const { return output; };
      
    private:
      /** Worker thread container. */
      class WorkerThread
      {
        public:
          WorkerThread(Gtp::Engine *eng);
          ~WorkerThread();
          
          void doCmd(Gtp::Engine::FunctionList *fi, Gtp::Command *c, Gtp::Engine::PonderFunction pf);
          void run();
        
        private:
          /** Worker thread functional. */
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
      
      /** Ponder thread container. */
      class PonderThread
      {
        public:
          PonderThread(Gtp::Engine::PonderFunction f, void *i, volatile bool *s);
          ~PonderThread();
          
          void ponderStart();
          void ponderStop();
          void run();
        
        private:
          /** Ponder thread functional. */
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
