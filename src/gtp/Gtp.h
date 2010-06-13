#ifndef DEF_OAKFOAM_GTP_H
#define DEF_OAKFOAM_GTP_H

#include <iostream>
#include <cstdio>
#include <string>
#include <sstream>
#include <list>
#include <algorithm>

namespace Gtp
{ 
  class Command
  {
    public:
      Command(int i, std::string cmdname, std::list<std::string> args) { id=i; commandname=cmdname; arguments=args; };
      
      int getId() { return id; };
      std::string getCommandName() { return commandname; };
      
    private:
      int id;
      std::string commandname;
      std::list<std::string> arguments;
  };
  
  class Output
  {
    public:
      void startResponse(Gtp::Command *cmd, bool success = true);
      void endResponse();
      
      void printString(std::string str);
  };
  
  class Engine
  {
    public:
      Engine();
      ~Engine();
      
      typedef void (*CommandFunction)(Gtp::Engine*, Gtp::Command*);
      
      class FunctionList
      {
        public:
          FunctionList(std::string cmdname, Gtp::Engine::CommandFunction func) { commandname=cmdname; function=func; next=NULL; };
          ~FunctionList() { if (next!=NULL) delete next; };
          
          std::string getCommandName() { return commandname; };
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
      void addFunctionCommand(std::string cmd, Gtp::Engine::CommandFunction func);
      
      Gtp::Output *getOutput() { return output; };
      
    private:
      Gtp::Engine::FunctionList *functionlist;
      Gtp::Engine::ConstantList *constantlist;
      Gtp::Output *output;
      
      void parseInput(std::string in, Gtp::Command **cmd);
      void doCommand(Gtp::Command *cmd);
  };
};

#endif
