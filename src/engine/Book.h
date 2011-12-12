#ifndef DEF_OAKFOAM_BOOK_H
#define DEF_OAKFOAM_BOOK_H

#include <string>
#include <list>
#include "Go.h"
//from "Parameters.h":
class Parameters;

/** Opening Book.
 * Management of opening books for various board sizes.
 */
class Book
{
  public:
    /** Create a new opening book.
     * Use the provided paramenters when appropriate.
     */
    Book(Parameters *p);
    ~Book();
    
    /** Add a new variation to an opening book.
     * The board size is @p size.
     * The sequence of move leading up to the new variation is @p movehistory.
     * The new variation move is @p move.
     */
    void add(int size, std::list<Go::Move> *movehistory, Go::Move move);
    /** Remove a variation from an opening book.
     * @see Book::add().
     */
    void remove(int size, std::list<Go::Move> *movehistory, Go::Move move);
    /** Return a list of valid moves according to the opening book.
     * @p movehistory decscibes the current situation.
     * If @p good is true, only "good" variations are returned, otherwise all.
     */
    std::list<Go::Move> getMoves(int size, std::list<Go::Move> *movehistory, bool good=true);
    /** Clear the opening book for a specific size. */
    void clear(int size);
    
    /** Display a piece of an opening book for GoGui analyse commands. */
    std::string show(int size, std::list<Go::Move> *movehistory);
    /** Load the opening book(s) from a file.
     * Use the Fuego opening book format.
     */
    bool loadFile(std::string filename);
    /** Load a line from an opening book file. */
    bool loadLine(std::string line);
    /** Dump the opening books to a file. */
    bool saveFile(std::string filename);
  
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
        void addChild(Book::Tree *child, int size);
        void removeChild(Book::Tree *child);
        void setGood(bool g) { good=g; };
        void setPrimary(bool p) { primary=p; };
        bool isPrimary() { return primary; };
        void setSymmetry(Go::Board::Symmetry s) { sym=s; };
        Go::Board::Symmetry getSymmetry() { return sym; };
      
      private:
        Book::Tree *parent;
        std::list<Book::Tree*> *children;
        Go::Move move;
        bool good,primary;
        Go::Board::Symmetry sym;
        
        void getMovesFromRoot(std::list<Go::Move> *moves);
    };
    class TreeHolder
    {
      public:
      TreeHolder(int sz);
      ~TreeHolder();
      
      int getSize() const { return size; };
      Book::Tree *getTree() const { return tree; };
      void clear();
      
      private:
        const int size;
        Book::Tree *tree;
    };
    
    Parameters *const params;
    std::list<Book::TreeHolder*> trees;
    
    Book::Tree *getTree(int size); //error if const
    
    void addPermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2, bool primary);
    void addSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move, bool primary);
    void removePermutations(int size, std::list<Go::Move> *moves1, std::list<Go::Move> *moves2);
    void removeSingleSeq(int size, std::list<Go::Move> *movehistory, Go::Move move);
    Book::Tree *getTree(int size, std::list<Go::Move> *movehistory);
    void removeIfNoGoodMoves(Book::Tree *starttree);
    void addMoves(int size, std::list<Go::Move> *moves);
    
    std::string outputTree(int size, Book::Tree *tree, std::list<Go::Move> *movehistory);
};
#endif
