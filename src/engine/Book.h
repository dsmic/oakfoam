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
    
    void add(int size, std::list<Go::Move> *movehistory, Go::Move move);
    void remove(int size, std::list<Go::Move> *movehistory, Go::Move move);
    std::list<Go::Move> getMoves(int size, std::list<Go::Move> *movehistory, bool good=true);
    void clear(int size);
    
    std::string show(int size, std::list<Go::Move> *movehistory);
    bool loadFile(std::string filename);
    bool loadLine(std::string line);
  
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
        void removeChild(Book::Tree *child);
        void setGood(bool g) { good=g; };
      
      private:
        Book::Tree *parent;
        std::list<Book::Tree*> *children;
        Go::Move move;
        bool good;
    };
    class TreeHolder
    {
      public:
      TreeHolder(int sz);
      ~TreeHolder();
      
      int getSize() { return size; };
      Book::Tree *getTree() { return tree; };
      void clear();
      
      private:
        int size;
        Book::Tree *tree;
    };
    
    Parameters *params;
    std::list<Book::TreeHolder*> trees;
    
    Book::Tree *getTree(int size);
    
    void addPermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2);
    void addSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move);
    void removePermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2);
    void removeSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move);
    Book::Tree *getTree(int size, std::list<Go::Move> *movehistory);
    void removeIfNoGoodMoves(Book::Tree *starttree);
};
#endif
