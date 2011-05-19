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
  
  //trees.push_back(new Book::TreeHolder(9));
  //trees.push_back(new Book::TreeHolder(13));
  //trees.push_back(new Book::TreeHolder(19));
}

Book::~Book()
{
  for (std::list<Book::TreeHolder*>::iterator iter=trees.begin();iter!=trees.end();++iter)
  {
    delete (*iter);
  }
}

Book::TreeHolder::TreeHolder(int sz)
{
  size=sz;
  tree=new Book::Tree();
}

Book::TreeHolder::~TreeHolder()
{
  delete tree;
}

void Book::TreeHolder::clear()
{
  delete tree;
  tree=new Book::Tree();
}

Book::Tree *Book::getTree(int size)
{
  for (std::list<Book::TreeHolder*>::iterator iter=trees.begin();iter!=trees.end();++iter)
  {
    if ((*iter)->getSize()==size)
      return (*iter)->getTree();
  }
  
  return NULL;
}

void Book::clear(int size)
{
  for (std::list<Book::TreeHolder*>::iterator iter=trees.begin();iter!=trees.end();++iter)
  {
    if ((*iter)->getSize()==size)
    {
      (*iter)->clear();
    }
  }
}

Book::Tree *Book::getTree(int size, std::list<Go::Move> *movehistory)
{
  Book::Tree *ctree=this->getTree(size);
  if (ctree==NULL)
    return NULL;
  
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

std::string Book::show(int size, std::list<Go::Move> *movehistory)
{
  std::ostringstream ss;
  Book::Tree *ctree=this->getTree(size,movehistory);
  
  if (ctree==NULL)
    return "TEXT out of book\n";
  
  ss<<"COLOR green";
  for(std::list<Book::Tree*>::iterator iter=ctree->getChildren()->begin();iter!=ctree->getChildren()->end();++iter) 
  {
    if ((*iter)->isGood())
      ss<<" "<<Go::Position::pos2string((*iter)->getMove().getPosition(),size);
  }
  ss<<"\n";
  ss<<"COLOR blue";
  for(std::list<Book::Tree*>::iterator iter=ctree->getChildren()->begin();iter!=ctree->getChildren()->end();++iter) 
  {
    if (!(*iter)->isGood())
      ss<<" "<<Go::Position::pos2string((*iter)->getMove().getPosition(),size);
  }
  ss<<"\n";
  ss<<"TEXT G:good B:other\n";
  return ss.str();
}

void Book::add(int size, std::list<Go::Move> *movehistory, Go::Move move)
{
  std::list<Go::Move> *moves1=new std::list<Go::Move>();
  std::list<Go::Move> *moves2=new std::list<Go::Move>();
  
  if (this->getTree(size)==NULL)
    trees.push_back(new Book::TreeHolder(size));
  
  for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
  {
    moves2->push_back((*iter));
  }
  moves2->push_back(move);
  
  this->addPermutations(size,moves1,moves2);
  
  delete moves1;
  delete moves2;
}

void Book::addPermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2)
{
  if (moves2->size()==0)
  {
    Go::Move move=moves1->back();
    moves1->pop_back();
    this->addSingleSeq(size,moves1,move);
    moves1->push_back(move);
  }
  else
  {
    Go::Board *board=new Go::Board(size);
    
    for (std::list<Go::Move>::iterator iter=moves1->begin();iter!=moves1->end();++iter)
    {
      board->makeMove((*iter));
    }
    
    Go::Move move=moves2->front();
    moves2->pop_front();
    
    Go::Board::Symmetry sym=board->getSymmetry();
    if (sym==Go::Board::NONE)
    {
      moves1->push_back(move);
      this->addPermutations(size,moves1,moves2);
      moves1->pop_back();
    }
    else
    {
      std::list<Go::Board::SymmetryTransform> transfroms = board->getSymmetryTransformsFromPrimary(sym);
      
      for (std::list<Go::Board::SymmetryTransform>::iterator iter=transfroms.begin();iter!=transfroms.end();++iter)
      {
        Go::Board::SymmetryTransform trans=(*iter);
        Go::Move newmove=Go::Move(move.getColor(),board->doSymmetryTransform(trans,move.getPosition()));
        std::list<Go::Move> *newmoves2=new std::list<Go::Move>();
        
        for (std::list<Go::Move>::iterator iter2=moves2->begin();iter2!=moves2->end();++iter2)
        {
          newmoves2->push_back(Go::Move((*iter2).getColor(),board->doSymmetryTransform(trans,(*iter2).getPosition())));
        }
        
        moves1->push_back(newmove);
        this->addPermutations(size,moves1,newmoves2);
        moves1->pop_back();
        delete newmoves2;
      }
    }
    
    moves2->push_front(move);
    
    delete board;
  }
}

void Book::addSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move)
{
  Book::Tree *ctree=this->getTree(size);
  if (ctree==NULL)
    return;
  
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

void Book::remove(int size, std::list<Go::Move> *movehistory, Go::Move move)
{
  std::list<Go::Move> *moves1=new std::list<Go::Move>();
  std::list<Go::Move> *moves2=new std::list<Go::Move>();
  
  for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
  {
    moves2->push_back((*iter));
  }
  moves2->push_back(move);
  
  this->removePermutations(size,moves1,moves2);
  
  delete moves1;
  delete moves2;
}

void Book::removePermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2)
{
  if (moves2->size()==0)
  {
    Go::Move move=moves1->back();
    moves1->pop_back();
    this->removeSingleSeq(size,moves1,move);
    moves1->push_back(move);
  }
  else
  {
    Go::Board *board=new Go::Board(size);
    
    for (std::list<Go::Move>::iterator iter=moves1->begin();iter!=moves1->end();++iter)
    {
      board->makeMove((*iter));
    }
    
    Go::Move move=moves2->front();
    moves2->pop_front();
    
    Go::Board::Symmetry sym=board->getSymmetry();
    if (sym==Go::Board::NONE)
    {
      moves1->push_back(move);
      this->removePermutations(size,moves1,moves2);
      moves1->pop_back();
    }
    else
    {
      std::list<Go::Board::SymmetryTransform> transfroms = board->getSymmetryTransformsFromPrimary(sym);
      
      for (std::list<Go::Board::SymmetryTransform>::iterator iter=transfroms.begin();iter!=transfroms.end();++iter)
      {
        Go::Board::SymmetryTransform trans=(*iter);
        Go::Move newmove=Go::Move(move.getColor(),board->doSymmetryTransform(trans,move.getPosition()));
        std::list<Go::Move> *newmoves2=new std::list<Go::Move>();
        
        for (std::list<Go::Move>::iterator iter2=moves2->begin();iter2!=moves2->end();++iter2)
        {
          newmoves2->push_back(Go::Move((*iter2).getColor(),board->doSymmetryTransform(trans,(*iter2).getPosition())));
        }
        
        moves1->push_back(newmove);
        this->removePermutations(size,moves1,newmoves2);
        moves1->pop_back();
        delete newmoves2;
      }
    }
    
    moves2->push_front(move);
    
    delete board;
  }
}

void Book::removeSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move)
{
  Book::Tree *ctree=this->getTree(size);
  if (ctree==NULL)
    return;
  
  if (movehistory!=NULL)
  {
    for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
    {
      ctree=ctree->getChild((*iter));
      if (ctree==NULL)
        return;
    }
  }
  
  Book::Tree *removetree=ctree->getChild(move);
  if (removetree!=NULL)
  {
    ctree->removeChild(removetree);
    delete removetree;
    this->removeIfNoGoodMoves(ctree);
  }
}

void Book::removeIfNoGoodMoves(Book::Tree *starttree)
{
  if (starttree->isRoot())
    return;
  
  for(std::list<Book::Tree*>::iterator iter=starttree->getChildren()->begin();iter!=starttree->getChildren()->end();++iter) 
  {
    if ((*iter)->isGood())
    {
      return;
    }
  }
  
  Book::Tree *parent=starttree->getParent();
  parent->removeChild(starttree);
  delete starttree;
  this->removeIfNoGoodMoves(parent);
}

std::list<Go::Move> Book::getMoves(int size, std::list<Go::Move> *movehistory, bool good)
{
  std::list<Go::Move> list = std::list<Go::Move>();
  Book::Tree *ctree=this->getTree(size, movehistory);
  
  if (ctree==NULL)
    return list;
  
  for(std::list<Book::Tree*>::iterator iter=ctree->getChildren()->begin();iter!=ctree->getChildren()->end();++iter) 
  {
    if ((*iter)->isGood() == good)
      list.push_back((*iter)->getMove());
  }
  
  return list;
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

void Book::Tree::removeChild(Book::Tree *child)
{
  children->remove(child);
  child->parent=NULL;
}

