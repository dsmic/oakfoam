#include "Book.h"

#include "Parameters.h"
#include <cstdlib>
#include <sstream>
//#include <iomanip>
//#include <iostream>
#include <fstream>

Book::Book(Parameters *p) : params(p)
{
}

Book::~Book()
{
  for (std::list<Book::TreeHolder*>::iterator iter=trees.begin();iter!=trees.end();++iter)
  {
    delete (*iter);
  }
}

Book::TreeHolder::TreeHolder(int sz) : size(sz)
{
  tree=new Book::Tree();
  tree->setSymmetry(Go::Board::FULL);
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

void Book::addMoves(int size, std::list<Go::Move> *moves)
{
  Book::Tree *tree=this->getTree(size);
  if (tree==NULL)
    return;
  
  for (std::list<Go::Move>::iterator iter=moves->begin();iter!=moves->end();++iter)
  {
    Book::Tree *newtree=tree->getChild((*iter));
    if (newtree==NULL)
    {
      newtree=new Book::Tree((*iter),false);
      tree->addChild(newtree,size);
    }
    tree=newtree;
  }
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
  
  this->addPermutations(size,moves1,moves2,true);
  
  delete moves1;
  delete moves2;
}

void Book::addPermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2, bool primary)
{
  if (moves2->size()==0)
  {
    Go::Move move=moves1->back();
    moves1->pop_back();
    this->addSingleSeq(size,moves1,move,primary);
    moves1->push_back(move);
  }
  else
  {
    //Go::Board *board=new Go::Board(size);
    
    //for (std::list<Go::Move>::iterator iter=moves1->begin();iter!=moves1->end();++iter)
    //{
    //  board->makeMove((*iter));
    //}
    
    Go::Move move=moves2->front();
    moves2->pop_front();
    
    //Go::Board::Symmetry sym=board->getSymmetry();
    this->addMoves(size,moves1);
    Book::Tree *tree=this->getTree(size,moves1);
    Go::Board::Symmetry sym=tree->getSymmetry();
    //Go::Board *board=new Go::Board(size);
    //fprintf(stderr,"sym: %s %s\n",board->getSymmetryString(sym).c_str(),move.toString(size).c_str());
    if (sym==Go::Board::NONE)
    {
      moves1->push_back(move);
      this->addPermutations(size,moves1,moves2,primary);
      moves1->pop_back();
    }
    else
    {
      //Go::Board::SymmetryTransform primtrans=board->getSymmetryTransformToPrimary(sym,move.getPosition());
      Go::Board::SymmetryTransform primtrans=Go::Board::getSymmetryTransformToPrimaryStatic(size,sym,move.getPosition());
      //int primarypos=board->doSymmetryTransform(primtrans,move.getPosition());
      int primarypos=Go::Board::doSymmetryTransformStatic(primtrans,size,move.getPosition());
      //std::list<Go::Board::SymmetryTransform> transfroms = board->getSymmetryTransformsFromPrimary(sym);
      std::list<Go::Board::SymmetryTransform> transfroms = Go::Board::getSymmetryTransformsFromPrimaryStatic(sym);
      
      for (std::list<Go::Board::SymmetryTransform>::iterator iter=transfroms.begin();iter!=transfroms.end();++iter)
      {
        Go::Board::SymmetryTransform trans=(*iter);
        //int newpos=board->doSymmetryTransform(trans,move.getPosition());
        int newpos=Go::Board::doSymmetryTransformStatic(trans,size,move.getPosition());
        Go::Move newmove=Go::Move(move.getColor(),newpos);
        std::list<Go::Move> *newmoves2=new std::list<Go::Move>();
        
        for (std::list<Go::Move>::iterator iter2=moves2->begin();iter2!=moves2->end();++iter2)
        {
          //int npos=board->doSymmetryTransform(trans,(*iter2).getPosition());
          int npos=Go::Board::doSymmetryTransformStatic(trans,size,(*iter2).getPosition());
          newmoves2->push_back(Go::Move((*iter2).getColor(),npos));
        }
        
        moves1->push_back(newmove);
        this->addPermutations(size,moves1,newmoves2,primary&&(newpos==primarypos));
        moves1->pop_back();
        delete newmoves2;
      }
    }
    
    moves2->push_front(move);
    
    //delete board;
  }
}

void Book::addSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move, bool primary)
{
  Book::Tree *ctree=this->getTree(size);
  if (ctree==NULL)
    return;
  
  //Go::Board *board=new Go::Board(size);
  
  if (movehistory!=NULL)
  {
    for (std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter)
    {
      Book::Tree *newtree=ctree->getChild((*iter));
      if (newtree==NULL)
      {
        newtree=new Book::Tree((*iter),false);
        //newtree->setSymmetry(board->getSymmetry());
        ctree->addChild(newtree,size);
      }
      newtree->setPrimary(primary);
      ctree=newtree;
      //board->makeMove((*iter));
    }
  }
  
  Book::Tree *addtree=ctree->getChild(move);
  if (addtree==NULL)
  {
    addtree=new Book::Tree(move,true);
    //addtree->setSymmetry(board->getSymmetry());
    ctree->addChild(addtree,size);
  }
  else
    addtree->setGood(true);
  addtree->setPrimary(primary);
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
  primary=false;
  sym=Go::Board::NONE;
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

void Book::Tree::addChild(Book::Tree *child, int size)
{
  child->parent=this;
  children->push_back(child);
  
  std::list<Go::Move> *moves=new std::list<Go::Move>();
  this->getMovesFromRoot(moves);
  
  Go::Board *board=new Go::Board(size);
    
  for (std::list<Go::Move>::iterator iter=moves->begin();iter!=moves->end();++iter)
  {
    board->makeMove((*iter));
  }
  board->makeMove(child->getMove());
  
  child->setSymmetry(board->getSymmetry());
  //fprintf(stderr,"ac: %s %s\n",board->getSymmetryString(board->getSymmetry()).c_str(),child->getMove().toString(size).c_str());
  
  delete board;
  delete moves;
}

void Book::Tree::removeChild(Book::Tree *child)
{
  children->remove(child);
  child->parent=NULL;
}

void Book::Tree::getMovesFromRoot(std::list<Go::Move> *moves)
{
  if (this->isRoot())
    return;
  else
  {
    parent->getMovesFromRoot(moves);
    moves->push_back(move);
  }
}

bool Book::loadFile(std::string filename)
{
  std::ifstream fin(filename.c_str());
  
  if (!fin)
    return false;
  
  std::string line;
  while (std::getline(fin,line))
  {
    if (!this->loadLine(line))
    {
      fin.close();
      return false;
    }
  }
  
  fin.close();
  
  return true;
}

bool Book::loadLine(std::string line)
{
  std::string sizestring,part;
  int size;
  std::list<Go::Move> movehistory=std::list<Go::Move>();
  bool founddivider=false;
  Go::Color col=Go::BLACK;

  int ro;
  while ((ro=line.find('\r'))>=0)
  {
    line.erase(ro,1);
  }

  std::transform(line.begin(),line.end(),line.begin(),::tolower);
  
  std::istringstream issline(line);
  if (!getline(issline,sizestring,' '))
    return false;
  std::istringstream isssize(sizestring);
  if (!(isssize >> size))
    return false;
  
  //fprintf(stderr,"s: %d\n",size);
  
  while (getline(issline,part,' '))
  {
    if (part=="|")
      founddivider=true;
    else if (founddivider)
    {
      Go::Move move=Go::Move(col,Go::Position::string2pos(part,size));
      //fprintf(stderr,"m: %s %s\n",part.c_str(),move.toString(size).c_str());
      this->add(size,&movehistory,move);
    }
    else
    {
      Go::Move move=Go::Move(col,Go::Position::string2pos(part,size));
      movehistory.push_back(move);
      //fprintf(stderr,"h: %s %s\n",part.c_str(),move.toString(size).c_str());
      col=Go::otherColor(col);
    }
  }
  
  return true;
}

bool Book::saveFile(std::string filename)
{
  std::ofstream fout(filename.c_str());
  std::list<Go::Move> *movehistory=new std::list<Go::Move>();
  
  if (!fout)
  {
    delete movehistory;
    return false;
  }
  
  //fprintf(stderr,"out\n");
  
  for (std::list<Book::TreeHolder*>::iterator iter=trees.begin();iter!=trees.end();++iter)
  {
    int size=(*iter)->getSize();
    Book::Tree *tree=(*iter)->getTree();
    
    //fprintf(stderr,"sz: %d\n",size);
    
    fout<<this->outputTree(size,tree,movehistory);
  }
  
  fout.close();
  delete movehistory;
  return true;
}

std::string Book::outputTree(int size, Book::Tree *tree, std::list<Go::Move> *movehistory)
{
  std::ostringstream ss,ssgood;
  bool foundgood=false;
  
  for(std::list<Book::Tree*>::iterator iter=tree->getChildren()->begin();iter!=tree->getChildren()->end();++iter) 
  {
    if ((*iter)->isPrimary() && (*iter)->isGood())
    {
      foundgood=true;
      ssgood<<" "<<Go::Position::pos2string((*iter)->getMove().getPosition(),size);
    }
  }
  
  if (foundgood)
  {
    ss<<size;
    for(std::list<Go::Move>::iterator iter=movehistory->begin();iter!=movehistory->end();++iter) 
    {
      ss<<" "<<Go::Position::pos2string((*iter).getPosition(),size);
    }
    ss<<" |"<<ssgood.str()<<"\n";
  }
  
  for(std::list<Book::Tree*>::iterator iter=tree->getChildren()->begin();iter!=tree->getChildren()->end();++iter) 
  {
    if ((*iter)->isPrimary())
    {
      movehistory->push_back((*iter)->getMove());
      ss<<this->outputTree(size,(*iter),movehistory);
      movehistory->pop_back();
    }
  }
  
  //fprintf(stderr,"s: %s\n",ss.str().c_str());
  
  return ss.str();
}

