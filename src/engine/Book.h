#ifndef DEF_OAKFOAM_BOOK_H
#define DEF_OAKFOAM_BOOK_H

#include <string>
#include <list>
#include "Go.h"
//from "Parameters.h":
class Parameters;

class Book
{
  public:
    Book(Parameters *p);
    ~Book();
    
    void add(std::list<Go::Move> *movehistory, Go::Move move);
    std::list<Go::Move> getMoves(std::list<Go::Move> *movehistory, bool good=true);
    
    std::string show(std::list<Go::Move> *movehistory);
  
  private:
    class Tree
    {
      public:
        Tree(Go::Move mov=Go::Move(Go::EMPTY,Go::Move::RESIGN), bool gd=true, Book::Tree *p=NULL);
        ~Tree();
        
        Book::Tree *getParent() { return parent; };
        std::list<Book::Tree*> *getChildren() { return children; };
        Go::Move getMove() { return move; };
        Book::Tree *getChild(Go::Move m);
        bool isRoot() { return (parent==NULL); };
        bool isLeaf() { return (children->size()==0); };
        bool isGood() { return good; };
        void addChild(Book::Tree *child);
        void setGood(bool g) { good=g; };
      
      private:
        Book::Tree *parent;
        std::list<Book::Tree*> *children;
        Go::Move move;
        bool good;
    };
    
    Parameters *params;
    Book::Tree *tree;
    
    void addSingleSeq(std::list<Go::Move> *movehistory, Go::Move move);
    void addPermutations(std::list<Go::Move> *moves1, std::list<Go::Move> *moves2);
    Book::Tree *getTree(std::list<Go::Move> *movehistory);
};
#endif
