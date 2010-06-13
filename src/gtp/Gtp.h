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
  class Arguments
  {
    public:
      Arguments(std::string cmd, std::list<std::string> args) { command=cmd; arguments=args; };
      
      std::string getCommand() { return command; };
      
    private:
      std::string command;
      std::list<std::string> arguments;
  };
  
  class Engine
  {
    public:
      Engine();
      ~Engine();
      
      typedef void (*CommandFunction)(Gtp::Engine*, Gtp::Arguments*);
      
      class FunctionList
      {
        public:
          FunctionList(std::string cmd, Gtp::Engine::CommandFunction func) { command=cmd; function=func; next=NULL; };
          ~FunctionList() { if (next!=NULL) delete next; };
          
          std::string getCommand() { return command; };
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
          std::string command;
          Gtp::Engine::CommandFunction function;
          Gtp::Engine::FunctionList *next;
      };
      
      class ConstantList
      {
        public:
          ConstantList(std::string cmd, std::string val) { command=cmd; value=val; next=NULL; };
          ~ConstantList() { if (next!=NULL) delete next; };
          
          std::string getCommand() { return command; };
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
          std::string command;
          std::string value;
          Gtp::Engine::ConstantList *next;
      };
      
      void run();
      
      void addConstantCommand(std::string cmd, std::string value);
      void addFunctionCommand(std::string cmd, Gtp::Engine::CommandFunction func);
      
    private:
      Gtp::Engine::FunctionList *functionlist;
      Gtp::Engine::ConstantList *constantlist;
      
      void parseInput(std::string in, int *id, std::string *cmd, Gtp::Arguments **args);
      void doCommand(std::string cmd, Gtp::Arguments *args);
  };
};

#endif
