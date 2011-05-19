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

void Book::addSingleSeq(std::list<Go::Move> *movehistory, Go::Move move)
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

void Book::add(std::list<Go::Move> *movehistory, Go::Move move)
{
  std::list<Go::Move> *moves1=new std::list<Go::Move>();
  std::list<Go::Move> *moves2=new std::list<Go::Move>();
  
  for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
  {
    moves2->push_back((*iter));
  }
  moves2->push_back(move);
  
  this->addPermutations(moves1,moves2);
  
  delete moves1;
  delete moves2;
}

void Book::addPermutations(std::list<Go::Move> *moves1, std::list<Go::Move> *moves2)
{
  if (moves2->size()==0)
  {
    Go::Move move=moves1->back();
    moves1->pop_back();
    this->addSingleSeq(moves1,move);
    moves1->push_back(move);
  }
  else
  {
    Go::Board *board=new Go::Board(params->board_size);
    
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
      this->addPermutations(moves1,moves2);
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
        this->addPermutations(moves1,newmoves2);
        moves1->pop_back();
        delete newmoves2;
      }
    }
    
    moves2->push_front(move);
    
    delete board;
  }
}

std::list<Go::Move> Book::getMoves(std::list<Go::Move> *movehistory, bool good)
{
  std::list<Go::Move> list = std::list<Go::Move>();
  Book::Tree *ctree=this->getTree(movehistory);
  
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

