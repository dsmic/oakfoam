#ifndef DEF_OAKFOAM_WORKER_H
#define DEF_OAKFOAM_WORKER_H

#include <string>
#include <list>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/barrier.hpp>

namespace Worker
{
  class Thread
  {
    public:
      Thread(int i);
      ~Thread();
      
      void start();
      void stop();
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
      bool alive,running,muststop;
      boost::barrier runbarrier;
      boost::thread thisthread;
      
      void run();
  };
  
  class Pool
  {
    public:
      Pool(int sz);
      ~Pool();
      
      int getSize() const { return size; };
      void startAll();
      void stopAll();
    
    private:
      const int size;
      std::list<Worker::Thread*> threads;
  };
};

#endif
