#include "Worker.h"

#include <cstdio>
#include <cstdlib>

Worker::Thread::Thread(int i)
  : id(i),
    alive(true),
    running(false),
    muststop(true),
    runbarrier(2),
    thisthread(Worker::Thread::Functional(this))
{
}

Worker::Thread::~Thread()
{
  muststop=true;
  alive=false;
  runbarrier.wait();
  thisthread.join();
}

void Worker::Thread::run()
{
  while (alive)
  {
    runbarrier.wait();
    if (!alive)
      break;
    running=true;
    fprintf(stderr,"run start %d\n",id);
    // todo
    fprintf(stderr,"run done\n");
    running=false;
  }
}

void Worker::Thread::start()
{
  muststop=false;
  runbarrier.wait();
}

void Worker::Thread::stop()
{
  muststop=true;
}

Worker::Pool::Pool(int sz) : size(sz)
{
  for(int i=0;i<size;i++)
  {
    threads.push_back(new Worker::Thread(i));
  }
}

Worker::Pool::~Pool()
{
  for(std::list<Worker::Thread*>::iterator iter=threads.begin();iter!=threads.end();++iter) 
  {
    delete (*iter);
  }
}

void Worker::Pool::startAll()
{
  for(std::list<Worker::Thread*>::iterator iter=threads.begin();iter!=threads.end();++iter) 
  {
    (*iter)->start();
  }
}

void Worker::Pool::stopAll()
{
  for(std::list<Worker::Thread*>::iterator iter=threads.begin();iter!=threads.end();++iter) 
  {
    (*iter)->stop();
  }
}

