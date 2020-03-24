/*
---- 
This file is part of SECONDO.

Copyright (C) 2004, University in Hagen, Department of Computer Science, 
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----

//paragraph [1] title: [{\Large \bf ]   [}]



[1] Cost Functions for the adaptive Join Operators

March 2006 M. Spiekermann 

This file contains some data structures for maintaining cost functions. These
cost functions will be used after a probe join for choosing the best evaluation
method for a join.

Cost estimation formulas for SECONDO's join operators are hard to develop for
the following reasons:

  * Base relations and itermediate result relations are stored in berkeley-db 
    record files which are itself organized as B-trees using a record ID as 
    key. The records are of variable length and may be spread over more than
    one page. In fact, we don't want to take into account the berkeley-db 
    internals. Moreover, for attributes with FLOBs, the FLOB data is stored 
    in a separate record file. 

  * Caching is done at several levels: (i) the operating system caches
    file-data, (ii) berkeley-db uses its own page buffer, (iii) operator
    implementations use tuple buffers.

  * Secondo queries (whose data is less than the computers memory) are highly
    CPU bound. In join predicates for standard attributes we need comparisons
    or to compute hash values but before we can do those operations a pointer
    to the function must be determined since we use an object oriented 
    approach with polymorphism, e.g. the use of virtual functions, which 
    are known as performance problem, refer to \cite{Veld00}.

Hence we try to estimate only the asymtotic behaviour of join
costs based on page accesses and the number of computations which need to
be done for the input tuples. If the computer's memory is small compared to
the input data, the cache will be unimportant and a join method using less
page accesses should always be better than one using more.

If the intermediate materializations during a join fit into the computer's
memory many caching effects will happen and the CPU costs must be taken into
account. 

Currently, we will only consider equi-joins with standard attributes,
hence costs for comparing their values or for computing and comparing i
hash values (probing hash-buckets). Basically, a cost formula for a join
between relations $A$ and $B$ will look like

\[
  cost(cA, cB, tsA, tsB, sel) = W_1 \cdot read(pagesA, pagesB) 
      + W_2 \cdot write(pagesA, pagesB) + W_3 \cdot cpu(cA, cB, sel)
\]

where $cA, cB$ are the number of input tuples with average sizes 
$tsA$ and $tsB$.
Depending on the page size $pagesA=pages(cA, tsA)$ and 
$pagesB=pages(cB, tsB)$ will be the
number of pages which are needed for input $A$ and input $B$. 
The join selectivity 
$jsel$ is used to estimate the number of computations. 
The factors $W_i$  are used
for weighting the three terms. The fraction between buffer memory 
and the relation sizes and the complexity of computations (complex 
join predicates) should be determine the weighting. 


1 Preliminaries

1.1 Includes and global declarations

*/


#ifndef SEC_COST_FUNCTION_H
#define SEC_COST_FUNCTION_H

#include <algorithm>
#include <math.h>

#include "WinUnix.h"

#include "Counter.h"
#include "TraceMacros.h"

extern QueryProcessor* qp;

/*
The parameters which influence cost functions are kept together in a
simple structure called ~CostParams~.
   
*/

struct CostParams {

  public:
    int cardA;
    int cardB;
    int tsA;    // avg. tuple size for input A 
    int tsB;    // avg. tuple size for input B
    int pagesA;
    int pagesB;
    int sizeA;
    int sizeB;
    float sel; // join selectivity: card(Result) / cardA * cardB
    
   CostParams() : 
     cardA(0), 
     cardB(0), 
     tsA(0), 
     tsB(0),
     pagesA(0),
     pagesB(0),
     sizeA(0),
     sizeB(0), 
     sel(0.0)
   {
   }
   
   CostParams( int inA, int in_tsA, 
               int inB, int in_tsB, float inSel ) :
    cardA( std::max(inA, 1) ),
    cardB( std::max(inB, 1) ),
    tsA( std::max(in_tsA, 1) ),
    tsB( std::max(in_tsB, 1) ),
    sel(inSel)
   {
     computeSizes();
   }

   void computeSizes()
   {
     sizeA = cardA*tsA;
     sizeB = cardB*tsB;
     
     static int pageSize = WinUnix::getPageSize(); 
     pagesA = sizeA / pageSize;
     pagesB = sizeB / pageSize;
   } 
    
   void swap()
   {
     CostParams h = *this;
     cardA = h.cardB;
     cardB = h.cardA;
     tsA = h.tsB;
     tsB = h.tsA;
     computeSizes();
   } 
   
#define VAR(a) #a << "=" << a
   std::ostream& print(std::ostream& os) const
   {
     os << "CostParams(" 
        << VAR(cardA) << ", " 
        << VAR(cardB) << ", " 
        << VAR(tsA) << ", " 
        << VAR(tsB) << ", " 
        << VAR(sizeA) << ", " 
        << VAR(sizeB) << ", " 
        << VAR(pagesA) << ", " 
        << VAR(pagesB) << ", " 
        << VAR(sel) << " )";
     return os; 
   } 
   
}; 

std::ostream& operator<<(std::ostream&, const CostParams&);

struct CostResult {

  int read;
  int write;
  int cpu;
  static float wr() { return 0.5; }
  static float ws() { return 0.5; }
  static float wc() { return 0.0005; }
  float value;

  CostResult() : read(0), write(0), cpu(0), value(0.0) {}
  ~CostResult() {}

  void weightResult(float scale=1.0) 
  { 
    value = scale * wr()*read + ws()*write + wc()*cpu; 
  }
   
  std::ostream& print(std::ostream& os) const
  {
    os << "(" << VAR(read) << ", " 
                   << VAR(write) << ", " 
                   << VAR(cpu) << ", " 
                   << VAR(value);
    return os;
  } 

}; 

std::ostream& operator<<(std::ostream& os, const CostResult&);

/*
The base class ~CostFunction~. A cost function must inherit this class
and implement the ~read~,  ~write~ and ~cpu~ function.
   
*/

class CostFunction {

  public:  
    const std::string name;
    const int index;
    int maxMem;
    float scale;

  CostFunction( const std::string& inName, int inIndex, float inScale=1.0) :
    name(inName),
    index(inIndex),
    scale(inScale)
  {
    maxMem = qp->FixedMemory();
  }    
  virtual ~CostFunction() {} 
    
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu) = 0;
  
  CostResult cost(const CostParams& p) 
  { 
    CostResult r;
    costs(p, r.read, r.write, r.cpu);
    r.weightResult(scale);
    const std::string prefix="PSA::Cost1_"+name;
    Counter::getRef(prefix+"_read") = r.read;
    Counter::getRef(prefix+"_write") = r.write;
    Counter::getRef(prefix+"_cpu") = r.cpu;
    return r; 
  } 
  
/*
The function below computes the costs if the inputs were
interchanged.

*/  
  CostResult cost2(const CostParams& p) 
  { 
    CostParams s = p;
    CostResult r;
    s.swap();
    costs(s, r.read, r.write, r.cpu);
    r.weightResult(scale);
    const std::string prefix="PSA::Cost2_"+name;
    Counter::getRef(prefix+"_read") = r.read;
    Counter::getRef(prefix+"_write") = r.write;
    Counter::getRef(prefix+"_cpu") = r.cpu;
    return r;
  } 

  
  const std::string& getName() { return name; } 
  
  double log2(double x) { return std::max(log(x)/log(2.0), 0.0); }
  
};


class HashJoinCost : private CostFunction 
{
  public:
  HashJoinCost(int index = 0) : CostFunction("hashjoin", index, 0.00154)
  {}
  virtual ~HashJoinCost() {}
  
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu)
  {

    TRACE("*** START HashJoinCost ***")
    // Memory resources of tuple buffers. A hash table is built for
    // input B.
    int bufferB = 3*maxMem/4; 
    
    // average number of tuples in buckets
    const int buckets=9997;
    int bufferedTuplesB = std::min(bufferB / p.tsB, p.cardB);

    // we assume that 5% of the buckets will not be hit. Moreover, we assume
    // that the buckets are not equally filled. Hence we reduce the average
    // length by 50%. 

    float avgHashChainB = std::max(1.0, (0.5 * bufferedTuplesB) 
                                         / (0.95 * buckets));
    
    // If B does not fit A will be flushed to disk and read
    // multiple times while B is scanned only once. The 
    // number of scans for A are a important factor.
    int f = (p.sizeB / bufferB) + 1;
    
    // Disk I/O is proportional to f 
    if (f > 1) {
      write += p.pagesA;
      // before writing all tuples are used for probing
      read  += (f - 1) * p.pagesA; 
    }
    
    // hash operations:
    cpu += (f * p.cardA) + p.cardB;

    // expected compare operations while probing buckets
    //cpu += (int)ceil((f * p.cardA * 1.0) * p.cardB * p.sel * avgHashChainB);
    cpu += (int)ceil(p.cardA * (p.cardB * p.sel));
    SHOW(cpu)		    
    cpu += (int)ceil(f * p.cardA * avgHashChainB);
    SHOW(cpu)		    

    SHOW(p.sizeB)
    SHOW(bufferB)
    SHOW(f)
    SHOW(bufferedTuplesB)
    SHOW(avgHashChainB) 
    TRACE("*** END HashJoinCost ***")
  } 
  
}; 

class SortCost : private CostFunction 
{
  public:
  SortCost(int index = 0) : CostFunction("sort", index)
  {}
  virtual ~SortCost() {}

/*
For inserting n tuples into a heap we will have the costs
\[
sum_k=1^n k \cdot log2(k) \approx n \cdot log2(n/2) 
\]

If the tuples do not fit into memory 

*/
 
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu)
  {
    // usage of tuple buffers
    int buffer = maxMem;
    int maxHeapSize = buffer / p.tsA;
    bool inMemory = (p.sizeA <= buffer);

    double dcpu = cpu;
    
    TRACE("*** START SortCost ***")
    SHOW(maxHeapSize)
    SHOW(inMemory)
    if (inMemory)
    {
      read += 0;
      write += 0;
      dcpu += 1.1 * p.cardA * log2(p.cardA); 
    }  
    else
    {
      assert(p.cardA > maxHeapSize);
     
      dcpu += p.cardA * log2(0.5*maxHeapSize); // inserting tuples into the heap
      
      // now the sort algorithm creates sorted runs stored on disk.
      // The average length and the total number of runs is estimated 
      // as follows
      int avgLen = 2 * maxHeapSize;
      int diskParts = p.cardA / avgLen;
      int restTuples = p.cardA - maxHeapSize;
      assert(restTuples >= 0); 
      
      int restPages = (int)ceil((restTuples * 1.0 * p.pagesA) / p.cardA);
      write += restPages;
      
      SHOW(diskParts)
      SHOW(restPages) 
      // some of the tuples are written directly to disk
      // and some are inserted and or removed from the heap
      dcpu += 0.5 * restTuples * log2(maxHeapSize); 

      // finally, the disk partitions and two memory partitions
      // are read from disk (or removed from the heap) and sorted by
      // inserting and removing by using a small heap.
      read += restPages; 
      dcpu += maxHeapSize * log2(0.5*maxHeapSize); // removing from heap
      dcpu += p.cardA * log2(diskParts + 2);     // sorting
    } 
    cpu = static_cast<int>( ceil(dcpu) );

    SHOW(read)
    SHOW(write)
    SHOW(dcpu)
    SHOW(cpu)

    TRACE("*** END SortCost ***")
  }  
}; 

/*
 
The sort-merge-join operator creates outputs which are grouped by the join
attribute.  Materialization of intermediate results is needed when both inputs
have more tuples with the same value for the join attribute as could be
buffered. But this is hard to predict since we know nothing about the
distribution of attribute values for the input tuples.

The only information which is helpful is the join selectivity $sel$.
 Together with
the input cardinalities the average group size of results may be estimated by
\[
  avgGrpSize = \frac{sel \cdot A \cdot B}{\max(A, B)} 
\]
If this value is bigger than the buffer size we suggest that in 50% of 
the result
group computation an extra materialization is needed.

*/

class SortMergeJoinCost : private CostFunction 
{
  public:
  SortCost sort;
  SortMergeJoinCost(int index = 0) : 
    CostFunction("sortmergejoin", index, 0.0028)
  {}
  virtual ~SortMergeJoinCost() {}
  
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu)
  {
    TRACE("*** START SortMergeJoinCost ***")
    // sort costs for A
    TRACE("Costs for A")
    int cpuA = 0;
    int readA = 0;
    int writeA = 0;
    sort.costs(p, read, write, cpuA);
    
    // sort costs for B
    TRACE("Costs for B")
    CostParams paramsB(p.cardB, p.tsB, p.cardA, p.tsA, p.sel);
    
    int cpuB = 0;
    int readB = 0;
    int writeB = 0;
    sort.costs(paramsB, read, write, cpuB);
    
    cpu += cpuA + cpuB;
    read += readA + readB;
    write += writeA + writeB;
    SHOW(read)
    SHOW(write) 
    SHOW(cpu) 
     
    // the merge step will need comparisons and maybe
    // some extra materializations of result groups 
    cpu += p.cardA + p.cardB;
   
    int resultCard = (int) ceil((p.sel * p.cardA) * p.cardB);
    if (resultCard == 0)
      return;
    
    
    // we assume uniform distribution of join attribute values, hence
    // every group of joining tuples will have the following average
    // charcteristics
    int avgGrpTuples = std::max( resultCard / std::max(p.cardA, p.cardB), 1);
    int groups = resultCard / avgGrpTuples;
    
    int avgTupSize = (p.tsA + p.tsB) / 2;
    int avgGrpBytes = avgGrpTuples * avgTupSize;
    int avgGrpPages = p.pagesA * p.pagesB / groups;
    
    // Next we derive a factor for groups which need to be merged on disk.
    // If avgGrpBytes > maxMem 50% of the groups are assumed to need
    // extra materialization.
    double grpsOnDisk = std::max( 0.5 * (((2.0 * avgGrpBytes) / maxMem) - 1), 
                                  0.0);

    SHOW(avgGrpTuples)
    SHOW(groups)
    SHOW(avgGrpPages) 
    SHOW(grpsOnDisk)
    
    write += (int) ceil(grpsOnDisk * avgGrpPages);
    read += (int) ceil(grpsOnDisk * avgGrpTuples * avgGrpPages);
    
    TRACE("*** END SortMergeJoinCost ***")
   }  
    
}; 

/*
When a join is computed by a pipeline of the ~product~ and ~filter~ operator
the right input is read into memory and flushed to disk if necessary. Hence the
factor $z = min(sizeB/maxMem,1) \in {0,1}$ is important for the number of 
read and write operations.
The costs are given by

\[
  read = pagesA + pagesB + z * (cA - 1 )* pagesB \\
  write = z * pagesB \\
  cpu = cA * cB
\]

Note: The performance of the operator product may be improved if as many
 tuples of input $A$ 
which fit on a single page are read into memory. Currently, it may happen
 that pages of $A$ need
to be fetched multiple times when $B$ is bigger than the page buffer.

*/

class ProductFilterCost : private CostFunction 
{
  public:
  ProductFilterCost(int index = 0) : CostFunction("productfilter", index)
  {}
  
  virtual ~ProductFilterCost() {}
  
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu)
  {
     int z = p.sizeB <= maxMem ? 0 : 1;
     SHOW(z)
     read = p.pagesA + p.pagesB + z*(p.cardA-1)*p.pagesB;
     write = z * p.pagesB;
     cpu = p.cardA * p.cardB;
  }
};

/*
The index loopjoin will run ~cardA~ times an exactmatch query retrieving
 matching tuples.
In order to retrieve the tuples it gets them from a B-tree which stores pairs of
attribute values and tuple ID. These tuple IDs are used to get the tuples 
from its
berkeley-db file by random access. A factor $m = sel * cardB$ defines how
many matches are returned. In the worst case every matching tuple needs a page
access to load the page where it is stored. The buffer hit ratio is defined as
$hr =  min(bufferPages / pagesB, 1)$.

Hence we have
\[
  read(Btree) = cardA * (avgKeySize + tupIdSize) / pageSize \\
  read(Rel) = cardA * min(m, pagesB) + (1 - hr) * m 
\]  

*/

class IndexLoopJoinCost : private CostFunction 
{
  public:
  IndexLoopJoinCost(int index = 0) : CostFunction("indexloopjoin", index, 0.01)
  {}
  
  virtual ~IndexLoopJoinCost() {}
  
  virtual void costs(const CostParams& p, int& read, int& write, int& cpu)
  {
     TRACE("*** START IndexLoopJoinCost ***")
     static const int pageSize = WinUnix::getPageSize();
     static const int bufferPages = 4 * 1024 * 1024 / pageSize;
     
     const double avgMatches = p.sel * p.cardB;
     const double hitRatio = std::min( (bufferPages * 1.0 / p.pagesB), 1.0 );
     SHOW(avgMatches)
     SHOW(hitRatio) 
     
     const int readBtree = p.cardA * 20 / pageSize;
     const int readMatches1 = std::min( (int)ceil(avgMatches), p.pagesB );
     const int readMatches2 = 
                  (int)ceil( p.cardA * (1.0 - hitRatio) * avgMatches );
     SHOW(readBtree)
     SHOW(readMatches1) 
     SHOW(readMatches2) 

     read = readBtree + readMatches1 + readMatches2; 
     write = 0;
     cpu = (int)ceil(p.cardA * avgMatches * 10);
     
     TRACE("*** END IndexLoopJoinCost ***")
  }
  
};

struct CostInfo 
{
  CostFunction* cf;
  CostResult costs;
  bool reversed;
  
  CostInfo( CostFunction* incf, 
            CostResult inCosts, bool isReversed = false ) :
    cf(incf),
    costs(inCosts), 
    reversed(isReversed)
  {
  }

  bool operator<(const CostInfo& rhs) const { 
    return costs.value < rhs.costs.value; 
  }  
  
  std::ostream& print(std::ostream& os) const
  { 
    os << cf->name << "[ " 
       << VAR(costs) << ", " 
       << VAR(reversed) << " ]";
    return os;
  } 
 
}; 

std::ostream& operator<<(std::ostream&, const CostInfo&);


class CostFunctions {

  typedef std::vector<CostFunction*> CostFunVec;
 
  private:
    CostFunVec cfv;
    
  public:  
  CostFunctions() {}
  ~CostFunctions()
  {
    CostFunVec::const_iterator it;
    for (it = cfv.begin(); it != cfv.end(); it++ )
    {
      delete *it;
    } 
  } 

  bool append(const std::string& name, int index)
  {
    if (name == "hj") {
      cfv.push_back( (CostFunction*) new HashJoinCost(index) );
      return true;
    } 
    
    if (name == "smj") {
      cfv.push_back( (CostFunction*) new SortMergeJoinCost(index) );
      return true;
    } 

    if (name == "pf") {
      cfv.push_back( (CostFunction*) new ProductFilterCost(index) );
      return true;
    } 
    
    if (name == "ilj") {
      cfv.push_back( (CostFunction*) new IndexLoopJoinCost(index) );
      return true;
    }
    
    /*
    if (name == "syj") {
      cfv.push_back( (CostFunction*) new SymmjoinCost(index) );
      return true;
    }*/
    
    return false;
  } 
  
  const CostInfo findBest(const CostParams& p, bool withRolesReversed = false) 
  {
    
    typedef std::vector<CostInfo> CostInfoVec; 
   
    CostInfoVec civ;
    CostFunVec::const_iterator it;
    int i = 0;
    for (it = cfv.begin(); it != cfv.end(); it++ )
    {
      CostFunction* cfp = *it;
      TRACE((*it)->index)
      CostInfo values( cfp, cfp->cost(p) );
      civ.push_back( values );
      cout << values << endl;
      if (withRolesReversed)
      {
        CostInfo values( cfp, cfp->cost2(p), true );
        civ.push_back( values );
        cout << values << endl;
      } 
      i++;
    } 
    stable_sort(civ.begin(), civ.end());
    return civ[0];
  }  

}; 

#endif

/*
\begin{thebibliography}{ABCD99}

\bibitem[Veld00]{Veld00} 
Todd Veldhuizen, Techniques for Scientific C++,
Indiana University Computer Science Technical Report
 \verb!#!542, 2000. URL: \verb+http://osl.iu.edu/[~]tveldhui+.

\end{thebibliography}

*/

