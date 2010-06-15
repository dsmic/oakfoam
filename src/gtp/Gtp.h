#ifndef DEF_OAKFOAM_GTP_H
#define DEF_OAKFOAM_GTP_H

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>

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
      
      int getId() { return id; };
      std::string getCommandName() { return commandname; };
      
      unsigned int numArgs();
      std::string getStringArg(int i);
      int getIntArg(int i);
      float getFloatArg(int i);
      Gtp::Color getColorArg(int i);
      Gtp::Vertex getVertexArg(int i);
      
    private:
      int id;
      std::string commandname;
      std::vector<std::string> arguments;
  };
  
  class Output
  {
    public:
      void startResponse(Gtp::Command *cmd, bool success = true);
      void endResponse(bool single = false);
      
      void printString(std::string str);
      void printVertex(Gtp::Vertex vert);
      void printScore(float score);
      void printf(std::string format,...);
      
      void printDebugString(std::string str);
      void printfDebug(std::string format,...);
      void printDebugVertex(Gtp::Vertex vert);
  };
  
  class Engine
  {
    public:
      Engine();
      ~Engine();
      
      typedef void (*CommandFunction)(void *instance, Gtp::Engine*, Gtp::Command*);
      
      class FunctionList
      {
        public:
          FunctionList(std::string cmdname, void *inst, Gtp::Engine::CommandFunction func) { commandname=cmdname; instance=inst; function=func; next=NULL; };
          ~FunctionList() { if (next!=NULL) delete next; };
          
          std::string getCommandName() { return commandname; };
          void *getInstance() { return instance; };
          Gtp::Engine::CommandFunction getFunction() { return function; };
          void setNext(Gtp::Engine::FunctionList *n) { next=n; };
          Gtp::Engine::FunctionList *getNext() { return next; };
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
          
          std::string getCommandName() { return commandname; };
          std::string getValue() { return value; };
          void setNext(Gtp::Engine::ConstantList *n) { next=n; };
          Gtp::Engine::ConstantList *getNext() { return next; };
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
      
      Gtp::Output *getOutput() { return output; };
      
    private:
      Gtp::Engine::FunctionList *functionlist;
      Gtp::Engine::ConstantList *constantlist;
      Gtp::Output *output;
      std::list<std::string> analyzeList;
      
      void parseInput(std::string in, Gtp::Command **cmd);
      void doCommand(Gtp::Command *cmd);
      
      static void cmdListCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
      static void cmdKnownCommand(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
      static void cmdAnalyzeCommands(void *instance, Gtp::Engine* gtpe, Gtp::Command* cmd);
  };
};

#endif
