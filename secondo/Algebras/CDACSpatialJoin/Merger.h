/*
----
This file is part of SECONDO.

Copyright (C) 2019,
Faculty of Mathematics and Computer Science,
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


//[<] [\ensuremath{<}]
//[>] [\ensuremath{>}]

\setcounter{tocdepth}{2}
\tableofcontents


1 includes

*/

#pragma once

#include <ostream>

#include "MergedArea.h" // -> ... -> <memory>
#include "MergerStats.h"
#include "IOData.h"

namespace cdacspatialjoin {

/*
3 Merger class

This class merges two adjacent MergedArea instances into a new MergedArea.
The process is interrupted whenever the output TBlock is full and can be
resumed by calling merge() again.

To keep terminology clear,

  * A / B always refers to the two rectangle sets A / B to be joined;

  * 1 / 2 always refers to the two areas 1 / 2 to be merged
    (area 1 being the left neighbor of area 2);

  * left / right (or L / R) always refers to the left / right edge of a
    single rectangle;

  * S / T is used for two different JoinEdge vectors in reportPairsSub():

    * regarding sets, either (S=A and T=B) or (S=B and T=A);

    * regarding areas, either (S=1 and T=2) or (S=2 and T=1);

    * regarding edges, either (S=left and T=right) or (S=right and T=left);

*/
class Merger {
   /* a rough sequence of tasks to be performed by the Merger */
   enum TASK { initialize, report, postProcess, done };

   // -----------------------------------------------------
   // data passed to the constructor

   /* the first area to be merged (left neighbor of area2) */
   const MergedAreaPtr area1;

   /* the second area to be merged (right neighbor of area1) */
   const MergedAreaPtr area2;

   /* true if this is the last merge operation called by a JoinState instance;
    * the post-processing can then be omitted */
   const bool isLastMerge;

   /* ioData holds all current input data and provides a function to add a
    * result tuple to the output tuple block */
   IOData* const ioData;

   /* the result area to be calculated by the Merger */
   MergedAreaPtr result;

   // -----------------------------------------------------
   // temporary vectors used for finding intersections and calculating
   // the result MergedArea

   /* the left edges of set A in area1 which span the complete x extent of
    * area2 (i.e. the corresponding right edge is outside area2 to the right) */
   JoinEdgeVec leftASpan;

   /* the right edges of set A in area2 which span the complete x extent of
    * area1 (i.e. the corresponding left edge is outside area1 to the left) */
   JoinEdgeVec rightASpan;

   /* the rectangles of set A which have a left edge in area1 and a right edge
    * in area2 */
   JoinEdgeVec leftRightA;


   /* the left edges of set B in area1 which span the complete x extent of
    * area2 (i.e. the corresponding right edge is outside area2 to the right) */
   JoinEdgeVec leftBSpan;

   /* the right edges of set B in area2 which span the complete x extent of
    * area1 (i.e. the corresponding left edge is outside area1 to the left) */
   JoinEdgeVec rightBSpan;

   /* the rectangles of set B which have a left edge in area1 and a right edge
    * in area2 */
   JoinEdgeVec leftRightB;

   // -----------------------------------------------------
   // control variables used to interrupt the process whenever the outTBlock
   // is full and to resume it when merge() is called

   /* the task currently performed */
   TASK currentTask;

   /* the type of reportPairs() call currently performed in the outer loop */
   unsigned reportType;

   /* the subtype of reportPairs() call currently performed in the inner
    * loop */
   unsigned reportSubType;

   /* the current start index of vector edgesS */
   size_t indexSBegin;

   /* the current index of vector edgesS */
   size_t indexS;

   /* the current start index of vector edgesT */
   size_t indexTBegin;

   /* the current index of vector T (starting from indexTBegin, indexT is being
    * incremented until edgeT.yMin exceeds edgeS.yMax ) */
   size_t indexT;

public:
   /* constructor expects two adjacent areas which shall be merged to a single
    * area. Note that the input areas must fulfil the invariants listed in the
    * MergedArea.h comment */
   Merger(MergedAreaPtr area1_, MergedAreaPtr area2_,
           bool isLastMerge_, IOData* ioData_);

   /* destructor */
   ~Merger();

   /* starts or continues merging the areas given in the constructor;
    * returns false if the outTBlock is full and merge needs to be resumed
    * later (by calling this function again with a new outTBlock),
    * or true if merge was completed and the result can be obtained by
    * calling getResult() */
   bool merge();

   /* returns the resulting MergedArea. Must only be called after merge() was
    * completed (i.e. merge() returned true) */
   inline MergedAreaPtr getResult() const {
      // assert (currentTask == TASK::done);
      return result;
   }

private:
   /* reports rectangle intersections between
    * a) an edge in the "span" vector (from one area and set), and
    * b) an edge in either the "left" or the "complete" vector (both from the
    * other area and set).
    * Returns true if completed, or false if the output TBlock is full */
   bool reportPairs(const JoinEdgeVec& span,
                    const JoinEdgeVec& left,
                    const JoinEdgeVec& complete);

   /* reports rectangle intersections between
    * a) an edge in the "edgesS" vector (from one area and set), and
    * b) an edge in the "edgesT" vector (from the other area and set).
    * returns true if completed, or false if the output TBlock is full */
   bool reportPairsSub(const JoinEdgeVec& edgesS,
                       const JoinEdgeVec& edgesT);

   /* specialized version of reportPairsSub() for edgesS containing only 1
    * edge, but edgesT containing multiple edges */
   bool reportPairsSub1(const JoinEdge& edgeS,
                       const JoinEdgeVec& edgesT, size_t sizeT);

   /* specialized version of reportPairsSub() for both edgesS and edgesT
    * containing only 1 edge */
   inline bool reportPairsSub11(const JoinEdge& edgeS,
                         const JoinEdge& edgeT);
   // this function is only called from one point in reportPairsSub(),
   // requires just 82 bytes assembly code, but is used very often, so it
   // deserves to be "inline"

public:
   /* returns the total number of JoinEdge instances stored in this Merger,
    * optionally including the input MergedAreas and the result MergedArea */
   size_t getJoinEdgeCount(bool includeAreas) const;

   /* returns the number of bytes currently used by this Merger, including
    * the input MergedAreas and the result MergedArea */
   size_t getUsedMemory() const;

};

} // end of namespace cdacspatialjoin