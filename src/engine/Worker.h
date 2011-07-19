#ifndef DEF_OAKFOAM_WORKER_H
#define DEF_OAKFOAM_WORKER_H

#include <string>
#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
//from "Engine.h":
class Engine;
//from "Parameters.h":
class Parameters;
//from "Random.h":
class Random;

namespace Worker
{
  class Thread;
  
  class Settings
  {
    public:
      Worker::Thread *thread;
      Random *rand;
  };
  
  class Thread
  {
    public:
      Thread(int i, Engine *eng);
      ~Thread();
      
      int getID() const { return id; };
      Worker::Settings *getSettings() const { return settings; };
      void setRandomSeed(unsigned long seed);
      
      void start();
      void stop();
      void wait();
      bool isRunning() const { return running; };
    
    private:
      class Functional
      {
        public:
          Functional(Thread *t) : thread(t) {};

          void operator()() { thread->run(); };

        private:
          Thread *const thread;
      };
      
      const int id;
      Engine *const engine;
      Worker::Settings *const settings;
      
      bool alive,running,muststop;
      boost::mutex busymutex;
      boost::mutex::scoped_lock *lock;
      boost::barrier runbarrier;
      boost::thread thisthread;
      
      void run();
  };
  
  class Pool
  {
    public:
      Pool(Parameters *prms);
      ~Pool();
      
      int getSize() const { return size; };
      Worker::Thread *getThreadZero() const { return threads.front(); };
      
      void startAll();
      void stopAll();
      void waitAll();
      void setRandomSeeds(unsigned long seed);
    
    private:
      Parameters *const params; 
      const int size;
      std::list<Worker::Thread*> threads;
  };
};

#endif
