#ifndef DEF_OAKFOAM_WORKER_H
#define DEF_OAKFOAM_WORKER_H

#include <string>
#include <list>
#define BOOST_THREAD_USE_LIB
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>
//from "Engine.h":
class Engine;
//from "Parameters.h":
class Parameters;
//from "Random.h":
class Random;

/** Worker thread management. */
namespace Worker
{
  class Thread;
  
  /** Worker-specific settings. */
  class Settings
  {
    public:
      /** The relevant thread. */
      Worker::Thread *thread;
      /** The random number generator for this thread. */
      Random *rand;
  };
  
  /** Worker thread. */
  class Thread
  {
    public:
      /** Create a worker thread. */
      Thread(int i, Engine *eng);
      ~Thread();
      
      /** Get the worker ID. */
      int getID() const { return id; };
      /** Get the worker settings. */
      Worker::Settings *getSettings() const { return settings; };
      /** Set the random seed for this worker. */
      void setRandomSeed(unsigned long seed);
      
      /** Start working. */
      void start();
      /** Stop working. */
      void stop();
      /** Return when done working. */
      void wait();
      /** Determine if this worker is busy. */
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
  
  /** Pool of workers. */
  class Pool
  {
    public:
      /** Create a pool given some parameters. */
      Pool(Parameters *prms);
      ~Pool();
      
      /** Get the number of workers in this pool. */
      int getSize() const { return size; };
      /** Get the first worker. */
      Worker::Thread *getThreadZero() const { return threads.front(); };
      
      /** Start all the workers in this pool. */
      void startAll();
      /** Stop all the workers in this pool. */
      void stopAll();
      /** Wait for all the workers in this pool to finish. */
      void waitAll();
      /** Set the seed for the workers in this pool. */
      void setRandomSeeds(unsigned long seed);
    
    private:
      Parameters *const params; 
      const int size;
      std::list<Worker::Thread*> threads;
  };
};

#endif
