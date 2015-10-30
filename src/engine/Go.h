#ifndef DEF_OAKFOAM_GO_H
#define DEF_OAKFOAM_GO_H

#include <string>
#include <forward_list>
#include <unordered_set>
#include <boost/pool/pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/cstdint.hpp>
#include <boost/bimap.hpp>
//#include "fastonebigheader.h"

#include "../gtp/Gtp.h"

//adaptive playouts
#define local_feature_num 13
#define hashto5num 32


#define MARK {fprintf(stderr,"mark %s %d\n",__FILE__,__LINE__);}
//can be set or unordered_set last is faster (this was probably wrong, it is only faster if fetch by value is needed, and only one is needed in erase(group))
#define ourset unordered_set

//from "Features.h":
class Features;
//from "Parameters.h":
class Parameters;
//from "Random.h":
class Random;

#include <set>
#define SYMMETRY_ONLYDEGRAGE false

#define P_N (-size-1)
#define P_S (size+1)
#define P_W (-1)
#define P_E (1)
#define P_NW (P_N+P_W)
#define P_NE (P_N+P_E)
#define P_SW (P_S+P_W)
#define P_SE (P_S+P_E)

//changed ordering to hopefully hit the cache better 2/15/2014
#define foreach_adjacent_debug(__pos ,__adjpos) for (int __adjpos : {__pos+P_E,__pos+P_W,__pos+P_S,__pos+P_N}) 
#define foreach_adjacent(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_N; { __body }; \
  }

#define foreach_onandadj_debug(__pos ,__adjpos) for (int __adjpos : {__pos,__pos+P_E,__pos+P_W,__pos+P_S,__pos+P_N}) 
#define foreach_onandadj(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos; { __body }; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_N; { __body }; \
  }

#define foreach_adjdiag_debug(__pos ,__adjpos) for (int __adjpos : {__pos+P_E,__pos+P_W,__pos+P_SW,__pos+P_S,__pos+P_SE,__pos+P_NW,__pos+P_N,__pos+P_NE}) 
#define foreach_adjdiag(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_E; { __body }; \
    __adjpos = __intpos+P_W; { __body }; \
    __adjpos = __intpos+P_SW; { __body }; \
    __adjpos = __intpos+P_S; { __body }; \
    __adjpos = __intpos+P_SE; { __body }; \
    __adjpos = __intpos+P_NW; { __body }; \
    __adjpos = __intpos+P_N; { __body }; \
    __adjpos = __intpos+P_NE; { __body }; \
  }

#define foreach_diagonal_debug(__pos ,__adjpos) for (int __adjpos : {__pos+P_SW,__pos+P_SE,__pos+P_NW,__pos+P_NE}) 
#define foreach_diagonal(__pos, __adjpos, __body) \
  { \
    int __intpos = __pos; \
    int __adjpos; \
    __adjpos = __intpos+P_SW; { __body }; \
    __adjpos = __intpos+P_SE; { __body }; \
    __adjpos = __intpos+P_NW; { __body }; \
    __adjpos = __intpos+P_NE; { __body }; \
  }

/** Go-related objects. */
namespace Go
{
  /** The memory allocator used for int's. */
  //typedef std::allocator<int> allocator_int;
  //typedef boost::fast_pool_allocator<int> allocator_int;
  class Group;
  /** The memory allocator used for Go::Group pointers. */
  typedef std::allocator<Go::Group*> allocator_groupptr;
  //typedef boost::fast_pool_allocator<Go::Group*> allocator_groupptr;
  /** A list of int's, using the specified memory allocator. */

  // 15.6.2014 fast_pool is much slower!!
  //typedef std::list<int,boost::fast_pool_allocator<int>> list_int;
  typedef std::list<int> list_int;
  typedef std::list<int> list_short; //short would be enough, but slower?!

  /** Go colors. */
  enum Color
  {
    EMPTY,
    BLACK,
    WHITE,
    OFFBOARD
  };
  
  /** Get the other Go color.
   * Black is converted to white and vica versa.
   */
  inline static Go::Color otherColor(Go::Color col)
  {
    switch (col)
    {
      case Go::BLACK:
        return Go::WHITE;
      case Go::WHITE:
        return Go::BLACK;
      default:
        return col;
    }
  };
  
  /** Get a char representation of given color. */
  inline static char colorToChar(Go::Color col)
  {
    switch (col)
    {
      case Go::BLACK:
        return 'B';
      case Go::WHITE:
        return 'W';
      case Go::EMPTY:
        return '.';
      default:
        return '-';
    }
  };
  
  /** Get the manhattan distance between two points. */
  inline static int rectDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    return dx+dy;
  };

  inline static int maxDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    if (dx>dy)
      return dx;
    else
      return dy;
  };

  /** Get the circular distance between two points. */
  inline static int circDist(int x1, int y1, int x2, int y2)
  {
    int dx=abs(x1-x2);
    int dy=abs(y1-y2);
    if (dx>dy)
      return dx+dy+dx;
    else
      return dx+dy+dy;
  };
  inline const char *getColorName(Go::Color col) {if (col==Go::EMPTY) return "empty"; if (col==Go::WHITE) return "white"; if (col==Go::BLACK) return "black"; return "offboard";};
      
  /** Go-related Exceptions.
   * For exceptions such as an illegal move being attempted.
   */
  class Exception
  {
    public:
      /** Create an exception instance with given message. */
      Exception(std::string m = "undefined") : message(m) {}
      /** Get the exception message. */
      std::string msg() {return message;}
    
    private:
      std::string message;
  };
  
  /** Go positions.
   * A position is an intersection on a Go Board, represented with an int.
   * This class is used to manipulate these positions.
   */
  class Position
  {
    public:
      /** Convert x-y coordinates to a position. */
      static inline int xy2pos(int x, int y, int boardsize) { return 1+x+(y+1)*(boardsize+1); };
      /** Convert a postion to an x coordinate. */
      static inline int pos2x(int pos, int boardsize) { return (pos-1)%(boardsize+1); };
      /** Convert a postion to an y coordinate. */
      static inline int pos2y(int pos, int boardsize) { return (pos-1)/(boardsize+1)-1; };
      /** Get the string representation of a position. */
      static std::string pos2string(int pos, int boardsize);
      /** Parse a string representation of a poistion. */
      static int string2pos(std::string str, int boardsize);
      static inline int pos2cnn(int pos, int boardsize) {return (pos-1)%(boardsize+1) * boardsize + (pos-1)/(boardsize+1)-1;};
      static inline int cnn2pos(int cnn, int boardsize) {return xy2pos(cnn/boardsize,cnn%boardsize,boardsize);};
      static inline int pos2grad(int pos, int boardsize) {return pos - boardsize - pos/(boardsize+1) -1;}
  };  
  
  /** Board with a bit for each position. */
  class BitBoard
  {
    public:
      /** Create an instance with given board size. */
      BitBoard(int s);
      ~BitBoard();
      
      /** Get the value of a position. */
      inline bool get(int pos) const { return data[pos]; };
      /** Set the value of a position. */
      inline void set(int pos, bool val=true) { data[pos]=val; };
      /** Clear a position.
       * Set the position to false.
       */
      inline void clear(int pos) { this->set(pos,false); };
      /** Fill the whole board with a given value. */
      inline void fill(bool val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      /** Clear the whole board. */
      inline void clear() { this->fill(false); };

      /** Create a copy of this board. */
      Go::BitBoard *copy() const;
      
    private:
      const int size,sizesq,sizedata;
      bool *const data;
  };

  /** Board with a bit for each position. */
  class IntBoard
  {
    public:
      /** Create an instance with given board size. */
      IntBoard(int s);
      ~IntBoard();
      
      /** Get the value of a position. */
      inline int get(int pos) const { return data[pos]; };
      inline bool getb(int pos) const { return bdata[pos]; };
      /** Set the value of a position. */
      inline void set(int pos, int val=1, bool b=false) { data[pos]=val; bdata[pos]=b;};
      inline void add(int pos, int val=1) { data[pos]=data[pos]+val;};
      /** Clear a position.
       * Set the position to false.
       */
      inline void clear(int pos) { this->set(pos,0); };
      /** Fill the whole board with a given value. */
      inline void fill(int val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      /** Clear the whole board. */
      inline void clear() { this->fill(0); };

      /** Create a copy of this board. */
      Go::IntBoard *copy() const;
      
    private:
      const int size,sizesq,sizedata;
      int *const data;
      bool *const bdata;
  };

  /** Board with a bit for each position. */
  class RespondBoard
  {
    public:
      /** Create an instance with given board size. */
      RespondBoard(int s);
      ~RespondBoard();
      
      /** Get the value of a position. */
      
      void addRespond(int pos, Go::Color col, std::set<int> *respondpositions) {
        if (respondpositions==NULL) return;
        if (lock_full.try_lock()) { //do not waste time, if already in use
          (Go::BLACK==col)?numplayedb[pos]++ :numplayedw[pos]++;
          if (respondpositions->size()>0) {(Go::BLACK==col)?numcaptb[pos]++ :numcaptw[pos]++;}
          for (std::set<int>::iterator itlist=respondpositions->begin();itlist!=respondpositions->end();++itlist) {
            boost::bimap<int,int> *tmp=getresponds(pos,col);
            boost::bimap<int,int>::left_iterator it=tmp->left.find(*itlist);
            int val=0;
            if (it!=tmp->left.end()) {
              val=it->second;
              tmp->left.erase(it);
            }
            tmp->insert({*itlist,val-1});
          }
          lock_full.unlock();
        }
      }; // if respondpos !=pass so >0
      std::list<std::pair<int,int>> getMoves(int pos, Go::Color col, int &nump, int &numcapt) {
        std::list<std::pair<int,int>> returnlist;
        if (lock_full.try_lock()) { //do not waste time, if already in use
          boost::bimap<int,int> *tmp=getresponds(pos,col);
          //fprintf(stderr,"pointer %p col %d %p %p\n",tmp,col,&respondsb[pos],&respondsw[pos]);
          for (boost::bimap <int,int>::right_const_iterator it = tmp->right.begin();it!=tmp->right.end();++it) {
            returnlist.push_back({it->second,it->first});
          }
          nump=(Go::BLACK==col)?numplayedb[pos]:numplayedw[pos];
          numcapt=(Go::BLACK==col)?numcaptb[pos]:numcaptw[pos];
          lock_full.unlock();
        }
        return returnlist;
      }
      void clear() {
        std::list<int> returnlist;
        lock_full.lock();
        for (int i=0;i<sizedata;i++) {
          respondsb[i].clear();
          respondsw[i].clear();
          numplayedb[i]=0;
          numplayedw[i]=0;
        }
        lock_full.unlock();
      }
      void scale(float f) {
        std::list<int> returnlist;
        lock_full.lock();
        for (int i=0;i<sizedata;i++) {
          numplayedb[i]*=f;
          std::list<std::pair<int,int>> returnlist;
          boost::bimap<int,int> *tmp=getresponds(i,Go::BLACK);
          for (boost::bimap <int,int>::right_const_iterator it = tmp->right.begin();it!=tmp->right.end();++it) {
            returnlist.push_back({it->second,it->first});
          }
          tmp->clear();
          for (std::list<std::pair<int,int>>::iterator it=returnlist.begin();it!=returnlist.end();++it) {
            if ((int)(it->second*f)<0) tmp->insert({it->first,(int)(it->second*f)});
          }
        }
        for (int i=0;i<sizedata;i++) {
          numplayedw[i]*=f;
          std::list<std::pair<int,int>> returnlist;
          boost::bimap<int,int> *tmp=getresponds(i,Go::WHITE);
          for (boost::bimap <int,int>::right_const_iterator it = tmp->right.begin();it!=tmp->right.end();++it) {
            returnlist.push_back({it->second,it->first});
          }
          tmp->clear();
          for (std::list<std::pair<int,int>>::iterator it=returnlist.begin();it!=returnlist.end();++it) {
            if ((int)(it->second*f)<0) tmp->insert({it->first,(int)(it->second*f)});
          }
        }
        lock_full.unlock();
      }
    private:
      inline boost::bimap<int,int> *getresponds(int pos, Go::Color col) const { if (col==Go::BLACK) return &respondsb[pos]; else return &respondsw[pos]; }; //not locked
      const int size,sizesq,sizedata;
      boost::bimap<int,int> *const respondsb;
      boost::bimap<int,int> *const respondsw;
      int *const numplayedb;
      int *const numplayedw;
      int *const numcaptb;
      int *const numcaptw;
      boost::mutex lock_full;
  };

  class CorrelationData
  {
    public:
      CorrelationData() {playedsum=0; ownedsum=0; playedXownedsum=0; n=0;};
      //add data, played and owned are +1 for black and -1 for white, 0 if unknown
      void putData(int played, int owned) 
      {
        if (played!=0) {playedsum+=played; ownedsum+=owned; playedXownedsum+=played*owned; n++;}
      };
      float getCorrelation() {if (n==0 || playedsum*playedsum>=n*n || ownedsum*ownedsum>=n*n) 
          return 0; else 
          return ((float)playedXownedsum/n-(float)playedsum/n*ownedsum/n)/
            (sqrt(1.0-(float)playedsum*playedsum/n/n)*sqrt(1.0-(float)ownedsum*ownedsum/n/n)
             );}; 
    private:  
      int playedsum, ownedsum, playedXownedsum, n;
  };
      
  /** Board with an object for each position. */
  template<typename T>
  class ObjectBoard
  {
    public:
      /** Create a board with the given size. */
      ObjectBoard(int s)
        : size(s),
          sizesq(s*s),
          sizedata(1+(s+1)*(s+2)),
          data(new T[sizedata])
      {};
      ~ObjectBoard()
      {
        delete[] data;
      };
      
      /** Get the value of a position. */
      inline T get(int pos) const { return data[pos]; };
      inline T* getp(int pos) const { return &data[pos]; };
      /** Set the value of a position. */
      inline void set(int pos, const T val) { data[pos]=val; };
      /** Fill the whole board with a set value. */
      inline void fill(T val) { for (int i=0;i<sizedata;i++) data[i]=val; };
      inline void copy(ObjectBoard<T> *copyboard) {
        for (int i=0;i<sizedata;i++)
          copyboard->set(i,data[i]);  
      }
      
    private:
      const int size,sizesq,sizedata;
      T *const data;
  };

  template<typename T>
  class ObjectBiBoard
  {
    public:
      /** Create a board with the given size. */
      ObjectBiBoard(int s)
        : size(s),
          sizesq(s*s),
          sizedata(1+(s+1)*(s+2))
        //,          data(new T[sizedata])
      {seed=0;};
      ~ObjectBiBoard()
      {
        //delete[] data;
      };
      
      /** Get the value of a position. */
      inline T get(int pos) const { return data.left.find(pos)->second; };
      inline T* getp(int pos) const { return &data.left.find(pos)->second; };
      /** Set the value of a position. */
      inline void set(int pos, const T val) { data.insert({pos,val+getRandomReal()/1000.0}); };
      /** Fill the whole board with a set value. */
      inline void fill(T val) { for (int i=0;i<sizedata;i++) set(i,val); };
      inline void copy(ObjectBiBoard<T> *copyboard) {
        for (int i=0;i<sizedata;i++)
          copyboard->set(i,get(i));  
      }

    
    private:
      const int size,sizesq,sizedata;
      //T *const data;
      boost::bimap<int,T> data;
      inline unsigned long getRandomInt()
      {
        //Park-Miller "Minimal Standard" PRNG
        
        unsigned long hi, lo;
        
        lo= 16807 * (seed & 0xffff);
        hi= 16807 * (seed >> 16);
        
        lo+= (hi & 0x7fff) << 16;
        lo+= hi >> 15;
        
        if (lo >= 0x7FFFFFFF) lo-=0x7FFFFFFF;

        return (seed=lo);
      };
      inline double getRandomReal()
      {
        return (double)this->getRandomInt() / ((unsigned long)(1) << 31);
      };
      long seed;
    
  };
  
  /** Go move.
   * This represents a move on a Go board.
   */
  class Move
  {
    public:
      /** The type of Go move. */
      enum Type
      {
        NORMAL,
        PASS,
        RESIGN
      };
      
      /** Create an empty move. */
      inline Move() : color(Go::EMPTY), pos(-2), useforlgrf(false) {};
      /** Create a move for given color and position. */
      inline Move(Go::Color col, int p) : color(col), pos(p), useforlgrf(false) {};
      /** Create a move for given color and x-y coordinates. */
      inline Move(Go::Color col, int x, int y, int boardsize) : color(col), pos(x<0?x:Go::Position::xy2pos(x,y,boardsize)) {};
      /** Create a move for given color and type. */
      inline Move(Go::Color col, Go::Move::Type type) : color(col), pos((type==PASS)?-1:-2)
      {
        if (type==NORMAL)
          throw Go::Exception("invalid type");
      };
      /** Get the color of this move. */
      inline Go::Color getColor() const {return color;};
      /** Get the position of this move. */
      inline int getPosition() const {return pos;};

//this does not look ok?!
      /** Get the x-coordinate of this move. */
      inline int getX(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2x(pos,boardsize));};
      /** Get the y-coordinate of this move. */
      inline int getY(int boardsize) const {return (this->isPass()||this->isResign()?pos:Go::Position::pos2y(pos,boardsize));};
      /** Set the position of this move. */
      void setPosition(int p) {pos=p;};
      
      /** Determine if this move is a pass. */
      inline bool isPass() const {return (pos==-1);};
      /** Determine if this move is a resign. */
      inline bool isResign() const {return (pos==-2);};
      /** Determine if this move is a normal move. */
      inline bool isNormal() const {return (pos!=-1) && (pos!=-2);};
      
      /** Get the string representation of this move. */
      std::string toString(int boardsize) const;
      
      /** Determine equality with another move. */
      inline bool operator==(const Go::Move other) const { return (color==other.getColor() && pos==other.getPosition()); };
      /** Determine inequality with another move. */
      inline bool operator!=(const Go::Move other) const { return !(*this == other); };
      inline void set_useforlgrf(bool u) {useforlgrf=u;}
      inline bool is_useforlgrf() {return useforlgrf;}
      
    private:
      Go::Color color;
      int pos;
      bool useforlgrf;
  };
  
  /** Zobrist hash type.
   * 64-bits should be good enough to avoid most false positives.
   */
  typedef boost::uint_fast64_t ZobristHash;
  
  /** Table of Zobrist hashes for each stone. */
  class ZobristTable
  {
    public:
      /** Create a table of given size, with given parameters and seed. */
      ZobristTable(Parameters *prms, int sz, unsigned long seed);
      ~ZobristTable();
      
      /** Get the size of the table. */
      int getSize() const { return size; };
      /** Get the hash for a stone, represented by a color and position. */
      Go::ZobristHash getHash(Go::Color col, int pos) const;
    
    private:
      Parameters *const params;
      const int size,sizedata;
      Random *const rand;
      Go::ZobristHash *const blackhashes,*const whitehashes;
      
      Go::ZobristHash getRandomHash();
  };
  
  /** Binary search tree for storing Zobrist hashes. */
  class ZobristTree
  {
    public:
      ZobristTree();
      ~ZobristTree();
      
      /** Add a hash to the tree. */
      void addHash(Go::ZobristHash hash);
      /** Determine if a hash is present in the tree. */
      bool hasHash(Go::ZobristHash hash) const;
    
    private:
      class Node
      {
        public:
          Node(Go::ZobristHash h);
          ~Node();
          
          Go::ZobristHash getHash() const { return hash; };
          void add(Go::ZobristHash h);
          Go::ZobristTree::Node *find(Go::ZobristHash h) const;
        
        private:
          const Go::ZobristHash hash;
          Go::ZobristTree::Node *left,*right;
      };
      
      Go::ZobristTree::Node *const tree;
  };
  
  /** Map of potential territory. */
  class TerritoryMap
  {
    public:
      /** Create an instance of given board size. */
      TerritoryMap(int sz);
      ~TerritoryMap();

      /** Increment the number of board that have been incorporated into these stats. */
      void incrementBoards() { boards++; };
      /** Add an owner for a specific position. */
      void addPositionOwner(int pos, Go::Color col);
      /** Get the owner for a specific position. */
      float getPositionOwner(int pos) const;
      /** Decay the statistics. */
      void decay(float factor);
      float getPlayouts() {return boards;};
      
    private:
      const int size, sizedata;
      float boards;
      ObjectBoard<float> *blackowns,*whiteowns;
  };

  //this class tries to find the mean value of moves, till pos is played in the playouts
  //if small, the pos is played with a high probability.
  class MoveProbabilityMap
  {
    public:
      /** Create an instance of given board size. */
      MoveProbabilityMap(int sz);
      ~MoveProbabilityMap();

      
      /** Add an owner for a specific position. */
      void setMoveAs(int pos, int move_number);
      void setMoveAsFirst(int pos, int move_number) {if (played->get(pos)==0) {played->set(pos,1); setMoveAs(pos,move_number);}}
      /** Get the owner for a specific position. */
      float getMoveAs(int pos) const;
      /** Decay the statistics. */
      void decay(float factor);
      void resetplayed() {played->fill(0);}
      
    private:
      const int size, sizedata;
      ObjectBoard<long> *move_num,*count;
      ObjectBoard<int> *played;
  };
  
  class Group;
  
  /** Vertex on a Go board. */
  class Vertex
  {
    public:
      /** Color of the vertex. */
      Go::Color color;
      /** Group associated with this vertex. */
      Go::Group *group;
  };
  
  class Board;
  
  /** Group on a Go board.
   * These are disjoint sets.
   */
  class Group
  {
    public:
      /** Create a Group instance for given board, with given key stone position. */
      Group(Go::Board *board, 
            int pos);
      ~Group() {};
      
      /** Get the color of this group. */
      Go::Color getColor() const {return color;};
      /** Get the position of the key stone of this position. */
      inline int getPosition() const {return position;};
     
      /** Set the parent group for this group. */
      void setParent(Go::Group *p) { parent=p; };
      /** Get the root group for this group. */
      inline Go::Group *find() 
      {
        if (parent==NULL)
          return (Go::Group *)this;
        else
          return (parent=parent->find());
          //return parent->find();
      };
      /** Merge two groups.
       * Assumed that both groups are roots.
       */
      void unionWith(Go::Group *othergroup)
      {
        othergroup->setParent(this);
        stonescount+=othergroup->stonescount;
        pseudoliberties+=othergroup->pseudoliberties;
        pseudoends+=othergroup->pseudoends;
        pseudoborderdist+=othergroup->pseudoborderdist;
        //libpossum+=othergroup->libpossum;
        //libpossumsq+=othergroup->libpossumsq;
        all_liberties.insert(othergroup->all_liberties.begin(),othergroup->all_liberties.end());
        if (isSolid() || othergroup->isSolid()) setSolid();
        adjacentgroups.splice(adjacentgroups.end(),*othergroup->getAdjacentGroups());
        //adjacentgroups.insert(adjacentgroups.end(),othergroup->getAdjacentGroups()->begin(),othergroup->getAdjacentGroups()->end());
        //adjacentgroups.insert(othergroup->getAdjacentGroups()->begin(),othergroup->getAdjacentGroups()->end());
        
      };
      /** Determine if this group is a root. */
      inline bool isRoot() const { return (parent==NULL); };
      
      /** Get the number of stones in this group. */
      inline int numOfStones() const { return stonescount; };
      /** Get the number of pseudo liberties for this group. */
      inline int numOfPseudoLiberties() const { return pseudoliberties; };
      inline int numOfRealLiberties() const { return all_liberties.size(); };
      inline int numOfPseudoEnds() const { return pseudoends; };
      inline int numOfPseudoBorderDist() const { return pseudoborderdist; };
      
      /** Add a pseudo liberty to this group. */
      inline void addPseudoLiberty(int pos) { pseudoliberties++; //libpossum+=pos; libpossumsq+=pos*pos; 
        all_liberties.insert(pos);};
      inline void addPseudoEnd() { pseudoends++;};
      inline void addPseudoBorderDist(int dist) { pseudoborderdist+=dist;};
      /** Remove a pseudo liberty from this group. */
      inline void removePseudoLiberty(int pos) { pseudoliberties--; //libpossum-=pos; libpossumsq-=pos*pos; 
        all_liberties.erase(pos);};
      inline void removePseudoEnd() { pseudoends--;};
      /** Determine if this group is in atari. */
      inline bool inAtari() const {return (all_liberties.size()==1);};
      //inline bool inAtari() const {return ((pseudoliberties*libpossumsq)==(libpossum*libpossum) && pseudoliberties>0); };  //pseudoliberties==0 should not happen?!
      //inline bool inAtariNotCompleted() const {return ((pseudoliberties*libpossumsq)==(libpossum*libpossum) && pseudoliberties>0); };  //pseudoliberties==0 should not happen?!
    /*{ 
        if ((all_liberties.size()==1)!=((pseudoliberties*libpossumsq)==(libpossum*libpossum) && pseudoliberties>0))
          fprintf(stderr,"should not happen %d %d %d\n%s",(int)all_liberties.size(),pseudoliberties,stonescount,board->toSting());
        return ((pseudoliberties*libpossumsq)==(libpossum*libpossum) && pseudoliberties>0); };  //pseudoliberties==0 should not happen?!
      */

      /** Get the last remaining postion of a group in atari.
       * If the group is not in atari, -1 is returned.
       */
      inline int getAtariPosition() const { if (this->inAtari()) return *all_liberties.begin(); else return -1; };
      inline bool get2libPositions(int &a, int &b) const { if (this->numRealLibs()==2) {a=*all_liberties.begin(); b=*(++all_liberties.begin()); return true;} else return false; };
      //inline int getAtariPositionNotCompleted() const { if (this->inAtariNotCompleted()) return libpossum/pseudoliberties; else return -1; };
      /** Add the orthogonally adjacent empty pseudo liberties. */
      inline void addTouchingEmpties(Go::Board *);
      /** Determine if given position is one of the last two liberties. */
      bool isOneOfTwoLiberties(const Go::Board *board,int pos) const __attribute__((hot));
      /** Get the other of the last two liberties.
       * If the group doesn't have two liberties, -1 is returned.
       */
      int getOtherOneOfTwoLiberties(const Go::Board *board,int pos) const  __attribute__((hot));
      
      /** Get a list of the adjacent groups to this group. */
      //std::list<int,Go::allocator_int> *getAdjacentGroups() { return &adjacentgroups; };
      list_short *getAdjacentGroups() { return &adjacentgroups; };
      //inline int getLibpossum() {return libpossum;}
      //inline int getLibpossumsq() {return libpossumsq;}
//      inline Go::Board *const getBoard() {return board;}
      int real_libs; //used for very slow real liberty counting
      bool isSolid() {return solid;}
      void setSolid(bool t=true) {solid=t;}
      int numRealLibs() const {return all_liberties.size();}
      int changedAtariAt() {
        if ((changed_atari_pos>=0)!=inAtari()) 
          {
            int atpos=getAtariPosition();
            if (atpos>=0) {changed_atari_pos=atpos; return changed_atari_pos;}
            atpos=changed_atari_pos;
            changed_atari_pos=-1;
            return atpos; //here was atari at the last update (call of changedAtariAt)
          }
        return -1; //no change
      }
    private:
      Go::Board *const board;
      const Go::Color color;
      const int position;
      
      int stonescount;
      Go::Group *parent;
      
      int pseudoliberties;
      int pseudoends;
      int pseudoborderdist;
      //int libpossum;
      //int libpossumsq;

      list_short adjacentgroups;
      bool solid;
      std::ourset<int> all_liberties;
      int changed_atari_pos; //updatePlayoutGammas () uses this to track, if a ataristatus of a group has changed at pos
  };
  
  /** Go board. */
  class Board
  {
    public:
      /** Symmetry types for a board. */
      enum Symmetry
      {
        NONE,
        VERTICAL,
        HORIZONTAL,
        DIAGONAL_DOWN,
        DIAGONAL_UP,
        DIAGONAL_BOTH,
        VERTICAL_HORIZONTAL,
        FULL
      };
      /** Transform for flipping the board through a line of symmetry. */
      struct SymmetryTransform
      {
        bool invertX; /**< Should the x coordinate be inverted? */
        bool invertY; /**< Should the y coordinate be inverted? */
        bool swapXY; /**< Should the x and y coordinates be swapped? */
      };
      
      /** Create a board of given size. */
      Board(int s);
      ~Board();
      
      int getNextPrimePositionMax() {return nextprime;}
      
      /** Create a copy of this board. */
      Go::Board *copy() const;
      /** Copy this board over the given board. */
      void copyOver(Go::Board *copyboard) const;
      /** Get a string representation of this board. */
      std::string toString() const;
      /** Get a SGF representation of this board. */
      std::string toSGFString() const;
      /** Get the raw board data.
       * This should only be used for very special purposes, and usually in a read-only manner.
       */
      const Go::Vertex *boardData() const { return data; };
      /** Get a list of the groups on this board. */
      std::ourset<Go::Group*> *getGroups() { return &groups; };
      
      /** Get the size of this board. */
      int getSize() const { return size; };
      /** Get the position of a simple ko.
       * If there is no current simple ko, return -1.
       */
      int getSimpleKo() const { return simpleko; };
      int getSimpleKoBefore() const { return wassimpleko; };
      /** Determine if there is currently a simple ko. */
      bool isCurrentSimpleKo() const { return (simpleko!=-1); };
      /** Get the number of moves made on this board. */
      int getMovesMade() const { return movesmade; };
      /** Get the number of consecutive passes that have been played before now. */
      int getPassesPlayed() const { return passesplayed; };
      /** Reset the passes that have been played.
       * Used to continue play after two passes.
       */
      void resetPassesPlayed() { passesplayed=0; };
      /** Get the next color to make a move. */
      Go::Color nextToMove() const { return nexttomove; };
      /** Set the next color to make a move. */
      void setNextToMove(Go::Color col) { nexttomove=col; }; //clear ko?
      /** Get the maximum position value for this board size. */
      int getPositionMax() const { return sizedata; };
      /** Get the last move made on this board. */
      Go::Move getLastMove() const { return lastmove; };
      /** Get the second-to-last move made on this board. */
      Go::Move getSecondLastMove() const { return secondlastmove; };
      Go::Move getThirdLastMove() const { return thirdlastmove; };
      Go::Move getForthLastMove() const { return forthlastmove; };
      /** Get the number of prisoners for a given color. */
      int getStoneCapturesOf(Go::Color col) const { return (col==Go::BLACK?blackcaptures:whitecaptures); };
      /** Reset the number of prisoners.
       * Used to make the mercy rule more robust.
       */
      void resetCaptures() { blackcaptures=0; whitecaptures=0; };
      /** Determine if the last move captured a group. */
      bool isLastCapture() { return lastcapture; };
      
      /** Get the color of the given position. */
      inline Go::Color getColor(int pos) const { return data[pos].color; };
      /** Get the group at the given position.
       * It is assumed there is a group at the position.
       */
      inline Go::Group *getGroup(int pos) const { return data[pos].group->find(); };
      /** Determine if the given position is part or a group. */
      inline bool inGroup(int pos) const { return (data[pos].group!=NULL); };
      /** Determine if the given position is actually on the board. */
      inline bool onBoard(int pos) const { return (data[pos].color!=Go::OFFBOARD); };
      
      /** Make a move on this board. */
      void makeMove(Go::Move move,Gtp::Engine* gtpe=NULL);
      /** Determine is the given move is a legal move. */
      inline bool validMove(Go::Move move) const
        {
          Go::BitBoard *validmoves=(move.getColor()==Go::BLACK?blackvalidmoves:whitevalidmoves);
          //return move.isPass() || move.isResign() || validmoves->get(move.getPosition());
          return (move.getPosition()<0) || validmoves->get(move.getPosition()); //faster and more correct, as no neg value allowed for get call!!!
        };
      inline bool validMove(Go::Color col, int pos) const
        {
          Go::BitBoard *validmoves=(col==Go::BLACK?blackvalidmoves:whitevalidmoves);
          //return move.isPass() || move.isResign() || validmoves->get(move.getPosition());
          return (pos<0) || validmoves->get(pos); //faster and more correct, as no neg value allowed for get call!!!
        };
       
      /** Get the number of legal moves currently available. */
      int numOfValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmovecount:whitevalidmovecount); };
      int numOfValidMoves() const {return (blackvalidmovecount+whitevalidmovecount)/2;}
      /** Get a board showing which moves are legal. */
      Go::BitBoard *getValidMoves(Go::Color col) const { return (col==Go::BLACK?blackvalidmoves:whitevalidmoves); };
      
      /** Compute the current score. */
      float score(Parameters* params=NULL, int a_pos=-1);
      /** Determine if the given position is a weak eye for the given color. */
      bool weakEye(Go::Color col, int pos, bool veryweak=0) const;
      /** Determine if the given position has an eye where two weak eyes are shared from two groups. */
      bool twoGroupEye(Go::Color col, int pos) const;
      /** Determine if the given position is surrounded by a single group of the given color. */
      bool strongEye(Go::Color col, int pos) const;
      /** Get the number of empty positions orthogonally adjacent to the given position. */
      int touchingEmpty(int pos) const {int lib=0; foreach_adjacent(pos,p,{if (this->getColor(p)==Go::EMPTY) lib++;});
  
          return lib;
        };
      int touchingColor(int pos,Go::Color col) const {int lib=0; foreach_adjacent(pos,p,{if (this->getColor(p)==col) lib++;});
  
          return lib;
        };
      int diagonalEmpty(int pos) const;
      /** Get the number of empty positions in the eight positions surrounding the given position. */
      int surroundingEmpty(int pos) const;
      int surroundingEmptyPlus(int pos) const;
      /** Get the number of each color in the orthogonally adjacent positions to the given position. */
      void countAdjacentColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      /** Get the number of each color in the diagonally adjacent positions to the given position. */
      void countDiagonalColors(int pos, int &empty, int &black, int &white, int &offboard) const;
      /** Turn symmetry calculations off for this board. */
      void turnSymmetryOff() { symmetryupdated=false;currentsymmetry=NONE; };
      /** Turn symmetry calculations on for this board. */
      void turnSymmetryOn() { symmetryupdated=true;currentsymmetry=this->computeSymmetry(); };
      
      /** Set the features for the board and whether the gamma values should be updated incrementally. */
      void setFeatures(Features *feat, bool inc, bool mchanges=true) { features=feat; incfeatures=inc; markchanges=mchanges; this->refreshFeatureGammas(); };
      void setPlayoutGammaAt(Parameters* params,int p);
      void setPlayoutGammaAround(Parameters* params,int p);
      void updatePlayoutGammas(Parameters* params, Features *feat=NULL);
      
      /** Get the sum ofthe gamma values for this board. */
      float getFeatureTotalGamma() const { return (nexttomove==Go::BLACK?blacktotalgamma:whitetotalgamma); };
      /** Get the gamma value for a position on this board. */
      float getFeatureGamma(int pos) const { return (nexttomove==Go::BLACK?blackgammas:whitegammas)->get(pos); };
      
      /** Determine if the given score is a win for the given color. */
      static bool isWinForColor(Go::Color col, float score);

      /** Compute the symmetry for this board. */
      Go::Board::Symmetry computeSymmetry();
      /** Get the computed symmetry for this board. */
      Go::Board::Symmetry getSymmetry() const { return currentsymmetry; };
      /** Get the string representation for the given symmetry. */
      std::string getSymmetryString(Go::Board::Symmetry sym) const;
      /** Transform a position to a primary position, given a symmetry. */
      int doSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos);
      /** Get the transform used to transform a position to its primary for a given symmetry. */
      Go::Board::SymmetryTransform getSymmetryTransformToPrimary(Go::Board::Symmetry sym, int pos) const;
      /** Get the transform used to transform a position to its primary for a given symmetry. */
      static Go::Board::SymmetryTransform getSymmetryTransformToPrimaryStatic(int size, Go::Board::Symmetry sym, int pos);
      /** Transform a position using the given symmetry transform. */
      int doSymmetryTransform(Go::Board::SymmetryTransform trans, int pos, bool reverse=false);
      /** Transform a position using the given symmetry transform. */
      static int doSymmetryTransformStatic(Go::Board::SymmetryTransform trans, int size, int pos);
      /** Reverse-transform a position using the given symmetry transform. */
      static int doSymmetryTransformStaticReverse(Go::Board::SymmetryTransform trans, int size, int pos);
      /** Get the transform to transform between two given positions. */
      static Go::Board::SymmetryTransform getSymmetryTransformBetweenPositions(int size, int pos1, int pos2);
      /** Get a list of applicable symmetry transforms for a given symmetry. */
      std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimary(Go::Board::Symmetry sym) const;
      /** Get a list of applicable symmetry transforms for a given symmetry. */
      static std::list<Go::Board::SymmetryTransform> getSymmetryTransformsFromPrimaryStatic(Go::Board::Symmetry sym);
      
      /** Determine is the given move is a capture. */
      inline bool isCapture(Go::Move move) const
      {
        Go::Color col=move.getColor();
        int pos=move.getPosition();
        foreach_adjacent(pos,p,{
          if (this->inGroup(p) && col!=this->getColor(p))
          {
            Go::Group *group=this->getGroup(p);
            if (//col!=group->getColor() && 
                group->inAtari())
              return true;
          }
        });
        return false;
      };
      inline bool isCaptureSolid(Go::Move move) const
      {
        Go::Color col=move.getColor();
        int pos=move.getPosition();
        foreach_adjacent(pos,p,{
          if (this->inGroup(p) && col!=this->getColor(p))
          {
            Go::Group *group=this->getGroup(p);
            if (//col!=group->getColor() && 
                group->inAtari() && group->isSolid())
              return true;
          }
        });
        return false;
      };
      
      /** Determine is the given move is an extension. */
      bool isExtension(Go::Move move) const;
      bool isExtension2lib(Go::Move move, bool checkother=true) const;
      bool isApproach(Go::Move move, int other[4]) const;
      /** Determine is the given move is a self-atari. */
      inline bool isSelfAtari(Go::Move move) const {return this->isSelfAtariOfSize(move,0);};
      /** Determine is the given move is a self-atari of a group of a minimum size. */
      inline bool isSelfAtariOfSize(Go::Move move, int minsize=0, bool complex=false) const
{
  Go::Color col=move.getColor();
  int pos=move.getPosition();

  if (this->touchingEmpty(pos)>1 || this->isCapture(move))
    return false;

  int libpos=-1;
  int groupsize=1;
  int groupbent4indicator=this->getPseudoDistanceToBorder(pos);
  Go::Group *groups_used[4];
  int groups_used_num=0;
  int usedneighbours=0;
  int attach_group_pos=-1;
  int capturedstones=0;
  foreach_adjacent(pos,p,{
    if (this->inGroup(p))
    {
      Go::Group *group=this->getGroup(p);
      if (col==group->getColor())
      {
        //if (!(group->inAtari() || group->isOneOfTwoLiberties(this,pos)))
        if (group->numRealLibs()>2)
          return false; // attached group has more than two libs
        attach_group_pos=p;
        usedneighbours++;
        bool found=false;
        for (int i=0;i<groups_used_num;i++)
        {
          if (groups_used[i]==group)
          {
            found=true;
            break;
          }
        }
        if (!found)
        {
          groups_used[groups_used_num]=group;
          groups_used_num++;
          int otherlib=group->getOtherOneOfTwoLiberties(this,pos);
          //fprintf(stderr,"otherlib %d\n",otherlib);
          if (otherlib!=-1)
          {
            if (libpos==-1)
              libpos=otherlib;
            else if (libpos!=otherlib)
              return false; // at least 2 libs
          }
          groupsize+=group->numOfStones();
          groupbent4indicator+=group->numOfPseudoBorderDist();
          //fprintf(stderr,"groupsize now %d %d\n",groupsize,groupbent4indicator);
        }
      }
      else
      {
        //more than on stone is captured, so not self atari
        if (group->inAtari())
          capturedstones+=group->numOfStones();
        if (capturedstones>1)
        {
          //fprintf(stderr,"group catched of size more than 1\n");
          return false;
        }
      }
    }
    else if (this->getColor(p)==Go::EMPTY)
    {
      if (libpos==-1)
        libpos=p;
      else if (libpos!=p)
        return false; // at least 2 libs
    }
  });
  
  if (groupsize>minsize)
    return true;
  else
  {
    if (!complex)
      return false;
    //complex
    if (groupsize<4) return false; //no complex handling necessary
    int pseudoends=4-usedneighbours-usedneighbours;
    for (int i=0;i<groups_used_num;i++)
    {
        pseudoends+=groups_used[i]->numOfPseudoEnds();
        //fprintf(stderr,"-- %d\n",groups_used[i]->numOfPseudoEnds());
    }
    //now we know the pseudoends of the group and can check if it is a good or bad form
    //fprintf(stderr,"complex self atari checked stones %d pseudoends %d bent4indicator %d\n",groupsize,pseudoends,groupbent4indicator);
    if (groupsize==5 && pseudoends!=10)
      return true; //do only play XXX
                   //             XX   form
    ///
    //  
    //  bent 4 handling in the corner not ok     
    // 
    //fprintf(stderr,"debug boardsize must be 9 selfatari 4 or 5 at %s with attached %s\n",move.toString (9).c_str(),Go::Position::pos2string(attach_group_pos,9).c_str());
    if (groupsize==4)
    {
      //here allow play of bent 4 in the corner
      if (groupbent4indicator==4)
        return false; //this is bent 4 in the corner or   
                      // XX
                      // XX in the corner, which is ok too
      if (attach_group_pos>=0) 
      {
        int nattached=0;
        foreach_adjacent(attach_group_pos,p,{
          if (this->getColor(p)==col)
            nattached++;
        });
        if (nattached!=2 && pseudoends!=8)
          return true; //do only play XX   XXX
                       //             XX    X
      }
     }
                                          
    return false;
  }
};
      /** Determine is the given move is an atari. */
      bool isLastLib(int pos, int *groupsize) const;
      bool isAtari(Go::Move move, int *groupsize=NULL) const;
      bool isAtari(Go::Move move, int *groupsize, int other_not) const;
      /** Get the distance from the given position to the board edge. */
      int getDistanceToBorder(int pos) const;
      /** Get the manhattan distance between two positions. */
      int getMaxDistance(int pos1, int pos2) const;
      int getRectDistance(int pos1, int pos2) const;
      inline int getPseudoDistanceToBorder(int pos) const
      {
        int x=Go::Position::pos2x(pos,size);
        int y=Go::Position::pos2y(pos,size);
        int ix=(size-x-1);
        int iy=(size-y-1);
        
        int distx=x;
        int disty=y;
        if (ix<distx)
          distx=ix;
        if (iy<disty)
          disty=iy;
        
        return distx+disty;
      };
      /** Get the circular distance between two positions. */
      int getCircularDistance(int pos1, int pos2) const;
      /** Get the CFG distances from the given position. */
      Go::ObjectBoard<int> *getCFGFrom(int pos, int max=0) const;
      /** Get the center liberty of a group of three.
       * Used for the nakade heuristic.
       * If there is not such position, -1 is returned.
       */
      int getOtherOfEmptyTwoGroup(int pos) const;
      int getThreeEmptyGroupCenterFrom(int pos) const __attribute__((hot));
      int getBent4EmptyGroupCenterFrom(int pos,bool onlycheck=false) const __attribute__((hot));
      int getFourEmptyGroupCenterFrom(int pos) const __attribute__((hot));
      int getFiveEmptyGroupCenterFrom(int pos) const __attribute__((hot));

      bool isThreeEmptyGroupCenterFrom(int pos) const;
      int  getSecondBent4Position(int pos) const;
      bool isBent4EmptyGroupCenterFrom(int pos) const;
      bool isFourEmptyGroupCenterFrom(int pos) const;
      bool isFiveEmptyGroupCenterFrom(int pos) const;
      
      /** Compute the Zobrist hash for this board. */
      Go::ZobristHash getZobristHash(Go::ZobristTable *table) const;
      
      /** Get the owner of given position, determined by the last score computed. */
      Go::Color getScoredOwner(int pos) const;
      /** Update the given territory map. */
      void updateTerritoryMap(Go::TerritoryMap *tmap) const;
      void updateCorrelationMap(Go::ObjectBoard<Go::CorrelationData> *correlationmap, Go::IntBoard *blacklist,Go::IntBoard *whitelist);
      /** Determine if a stone is alive, according to the given territory map and threshold. */
      bool isAlive(Go::TerritoryMap *tmap, float threshold, int pos) const;
      /** Get the territory score using the given territory map and threshold. */
      int territoryScore(Go::TerritoryMap *tmap, float threshold);

      /** Determine if there is a ladder for the group.
       * A ladder is only detected if the group in question is currently in atari.
       */
      bool isLadder(Go::Group *group) const;
      /** Determine if there is a ladder for the group after the move is made. */
      bool isLadderAfter(Go::Group *group, Go::Move move) const;
      /** Determine if the ladder works or is broken in most cases. */
      bool isProbableWorkingLadder(Go::Group *group) const;
      /** Determine if the ladder that exists after the move is made works or is broken in most cases. */
      bool isProbableWorkingLadderAfter(Go::Group *group, Go::Move move) const;
      void updateFeatureGammas(bool both=false);
      inline int getPseudoLiberties(int pos) const { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfPseudoLiberties(); };
      inline int getRealLiberties(int pos) const { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numRealLibs(); };

      void calcSlowLibertyGroups();
      void connectedAtariPos(Go::Move move);//, int CApos[4], int &CAcount);
      int ACcount;
      int ACpos[4];

      void enable_changed_positions(Parameters* params) {if (changed_positions!=NULL) delete changed_positions; updatePlayoutGammas(params); changed_positions =new list_int();}
      void disable_changed_positions() {if (changed_positions!=NULL) delete changed_positions; changed_positions=NULL;}

      Go::ObjectBoard<float> *blackgammas;
      Go::ObjectBoard<float> *whitegammas;
      Go::ObjectBoard<int> *blackpatterns;
      Go::ObjectBoard<int> *whitepatterns;
      Features *getFeatures() {return features;};

      float komi_grouptesting=0;  //only used for group testing!!!!!!!!
      bool hasSolidGroups;
      //check if one of the attachedpos groups is attached to capturepos
      bool groupatached(int capturedpos,list_short &attachedpos) {
        if (attachedpos.size()==0) return false;
        list_short *captatt=getGroup(capturedpos)->getAdjacentGroups();
        std::list<Go::Group*> captachedposgroup;
        for (auto p:attachedpos) {
          captachedposgroup.push_back(getGroup(p));
        }
        for (auto p:*captatt) {
          Go::Group *group=getGroup(p);
          for (auto gg: captachedposgroup) {
            if (gg==group) return true;
          }
        }
        return false;        
      }
      bool groupatached(int capturedpos, Go::Group* captachedposgroup) {
        list_short *captatt=getGroup(capturedpos)->getAdjacentGroups();
        for (auto p:*captatt) {
          Go::Group *group=getGroup(p);
            if (captachedposgroup==group) return true;
        }
        return false;        
      }
    private:
      const int size;
      const int sizesq;
      const int sizedata;
      int nextprime;
      Go::Vertex *const data;
      std::ourset<Go::Group*> groups;
      int movesmade,passesplayed;
      Go::Color nexttomove;
      int simpleko;
      int wassimpleko;
      Go::Move lastmove,secondlastmove,thirdlastmove,forthlastmove;
      bool symmetryupdated;
      Go::Board::Symmetry currentsymmetry;
      int blackvalidmovecount,whitevalidmovecount;
      Go::BitBoard *changes3x3;
      Go::BitBoard *blackvalidmoves,*whitevalidmoves;
      boost::object_pool<Go::Group> pool_group;
      bool markchanges;
      Go::BitBoard *lastchanges;
      Features *features;
      bool incfeatures;
      float blacktotalgamma;
      float whitetotalgamma;
      
      int blackcaptures,whitecaptures;
      bool lastcapture;

      
      bool CSstyle=false;
      list_int *changed_positions=NULL;
      struct ScoreVertex
      {
        bool touched;
        Go::Color color;
      };
      Go::Board::ScoreVertex *lastscoredata;
      
      inline Go::Group *getGroupWithoutFind(int pos) const { return data[pos].group; };
      inline void setColor(int pos, Go::Color col) { data[pos].color=col; if (markchanges) { lastchanges->set(pos); } };
      inline void setGroup(int pos, Go::Group *grp) { data[pos].group=grp; };
      inline int getGroupSize(int pos) const { if (data[pos].group==NULL) return 0; else return data[pos].group->find()->numOfStones(); };
      
      bool touchingAtLeastOneEmpty(int pos) const;
      
      void refreshGroups();
      void spreadGroup(int pos, Go::Group *group);
      int removeGroup(Go::Group *group);
      void spreadRemoveStones(Go::Color col, int pos, list_int *possiblesuicides);
      void mergeGroups(Go::Group *first, Go::Group *second);
      
      bool validMoveCheck(Go::Move move) const;
      bool validMoveCheck(Go::Color col, int pos) const;
      void refreshValidMoves();
      void refreshValidMoves(Go::Color col);
      void addValidMove(Go::Move move);
      void addValidMove(Go::Color col, int pos);
      void removeValidMove(Go::Move move);
      void removeValidMove(Go::Color col, int pos);
      
      bool hasSymmetryVertical() const;
      bool hasSymmetryHorizontal() const;
      bool hasSymmetryDiagonalDown() const;
      bool hasSymmetryDiagonalUp() const;
      void updateSymmetry();
      int doSymmetryTransformPrimitive(Go::Board::Symmetry sym, int pos) const;
      
      void spreadScore(Go::Board::ScoreVertex *scoredata, int pos, Go::Color col);
      bool isProbableWorkingLadder(Go::Group *group, int posA, int movepos=-1) const;
      
      void refreshFeatureGammas();
      void updateFeatureGamma(Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, int pos);
      void updateFeatureGamma(Go::ObjectBoard<int> *cfglastdist, Go::ObjectBoard<int> *cfgsecondlastdist, Go::Color col, int pos);
      int psize=3;
  };
};

#endif
