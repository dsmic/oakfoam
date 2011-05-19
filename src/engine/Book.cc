#include "Book.h"

#include "Parameters.h"
#include <cstdlib>
#include <sstream>
//#include <iomanip>
//#include <iostream>
//#include <fstream>

Book::Book(Parameters *p)
{
  params=p;
  tree=new Book::Tree();
  
  //this->add(NULL,Go::Move(Go::BLACK,Go::Position::xy2pos(4,4,params->board_size)));
}

Book::~Book()
{
  delete tree;
}

Book::Tree *Book::getTree(std::list<Go::Move> *movehistory)
{
  Book::Tree *ctree=tree;
  
  if (movehistory!=NULL)
  {
    for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
    {
      ctree=ctree->getChild((*iter));
      if (ctree==NULL)
        return NULL;
    }
  }
  
  return ctree;
}

std::string Book::show(std::list<Go::Move> *movehistory)
{
  std::ostringstream ss;
  Book::Tree *ctree=this->getTree(movehistory);
  
  if (ctree==NULL)
    return "TEXT out of book\n";
  
  ss<<"COLOR green";
  for(std::list<Book::Tree*>::iterator iter=ctree->getChildren()->begin();iter!=ctree->getChildren()->end();++iter) 
  {
    if ((*iter)->isGood())
      ss<<" "<<Go::Position::pos2string((*iter)->getMove().getPosition(),params->board_size);
  }
  ss<<"\n";
  ss<<"COLOR blue";
  for(std::list<Book::Tree*>::iterator iter=ctree->getChildren()->begin();iter!=ctree->getChildren()->end();++iter) 
  {
    if (!(*iter)->isGood())
      ss<<" "<<Go::Position::pos2string((*iter)->getMove().getPosition(),params->board_size);
  }
  ss<<"\n";
  ss<<"TEXT G:good B:other\n";
  return ss.str();
}

void Book::add(std::list<Go::Move> *movehistory, Go::Move move)
{
  Book::Tree *ctree=tree;
  
  if (movehistory!=NULL)
  {
    for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
    {
      Book::Tree *newtree=ctree->getChild((*iter));
      if (newtree==NULL)
      {
        newtree=new Book::Tree((*iter),false);
        ctree->addChild(newtree);
      }
      ctree=newtree;
    }
  }
  
  Book::Tree *addtree=ctree->getChild(move);
  if (addtree==NULL)
  {
    addtree=new Book::Tree(move,true);
    ctree->addChild(addtree);
  }
  else
    addtree->setGood(true);
}

Book::Tree::Tree(Go::Move mov, bool gd, Book::Tree *p)
{
  parent=p;
  children=new std::list<Book::Tree*>();
  move=mov;
  good=gd;
}

Book::Tree::~Tree()
{
  for(std::list<Book::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    delete (*iter);
  }
  delete children;
}

Book::Tree *Book::Tree::getChild(Go::Move m)
{
  for(std::list<Book::Tree*>::iterator iter=children->begin();iter!=children->end();++iter) 
  {
    if ((*iter)->getMove()==m)
      return (*iter);
  }
  return NULL;
}

void Book::Tree::addChild(Book::Tree *child)
{
  child->parent=this;
  children->push_back(child);
}

