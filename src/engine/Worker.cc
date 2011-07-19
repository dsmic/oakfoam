#include "Worker.h"

#include <cstdio>
#include <cstdlib>
#include "Engine.h"
#include "Parameters.h"
#include "Random.h"

Worker::Thread::Thread(int i, Engine *eng)
  : id(i),
    engine(eng),
    settings(new Worker::Settings()),
    alive(true),
    running(false),
    muststop(true),
    runbarrier(2),
    thisthread(Worker::Thread::Functional(this))
{
  settings->thread=this;
  settings->rand=new Random(0,id);
}

Worker::Thread::~Thread()
{
  this->wait();
  muststop=true;
  alive=false;
  runbarrier.wait();
  thisthread.join();
  delete settings->rand;
  delete settings;
}

void Worker::Thread::setRandomSeed(unsigned long seed)
{
  delete settings->rand;
  settings->rand=new Random(seed,id);
}

void Worker::Thread::run()
{
  while (true)
  {
    runbarrier.wait();
    if (!alive)
      break;
    running=true;
    //fprintf(stderr,"run start %d %lu\n",id,settings->rand->getSeed());
    engine->doThreadWork(settings);
    //fprintf(stderr,"run done\n");
    running=false;
    delete lock;
  }
}

void Worker::Thread::start()
{
  lock=new boost::mutex::scoped_lock(busymutex);
  muststop=false;
  runbarrier.wait();
}

void Worker::Thread::stop()
{
  muststop=true;
}

void Worker::Thread::wait()
{
  boost::mutex::scoped_lock lock(busymutex);
}

Worker::Pool::Pool(Parameters *prms)
  : params(prms),
    size(params->thread_count)
{
  for(int i=0;i<size;i++)
  {
    threads.push_back(new Worker::Thread(i,params->engine));
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
  this->waitAll();
}

void Worker::Pool::waitAll()
{
  for(std::list<Worker::Thread*>::iterator iter=threads.begin();iter!=threads.end();++iter) 
  {
    (*iter)->wait();
  }
}

void Worker::Pool::setRandomSeeds(unsigned long seed)
{
  for(std::list<Worker::Thread*>::iterator iter=threads.begin();iter!=threads.end();++iter) 
  {
    (*iter)->setRandomSeed(seed);
  }
}

