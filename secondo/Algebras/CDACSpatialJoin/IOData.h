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


1 IOData class

Holds all current input data (i.e. TBlocks or RectangleBlocks) and provides
methods to a) extract the SortEdges and RectangleInfos from it, and b) fill
the output tuple block with result tuples.

*/

#pragma once

#include "InputStream.h" // -> "Algebras/CRel/TBlock.h"
#include "RectangleInfo.h" // -> ... -> <memory>, <vector>
#include "SortEdge.h"
#include "JoinEdge.h"


namespace cdacspatialjoin {

typedef CRelAlgebra::TBlock* TBlockPtr;

class IOData {
   // static constexpr and functions

public:
   /* returns the "set" information stored in the given address */
   inline static SET getSet(const SetRowBlock_t address) {
      return ((address & SET_MASK) == 0) ? SET::A : SET::B;
   }

   inline static SetRowBlock_t removeSet(const SetRowBlock_t address) {
      return (address ^ SET_MASK);
   }

   static std::string getSetName(SET set);

private:
   /* returns the number of bits needed to store a block indices for the given
    * block count (which is then the number of bits that the row information
    * is left shifted inside a SetRowBlock_t address) */
   static unsigned getRowShift(size_t blockCount);

   /* returns a bit mask for the row information inside a SetRowBlock_t address
    * (which is left-shifted by the given number of bits) */
   static SetRowBlock_t getRowMask(size_t rowShift);

   /* returns a bit mask for the block information inside a SetRowBlock_t
    * address with the riven rowShift */
   static SetRowBlock_t getBlockMask(size_t rowShift);

   // -----------------------------------------------------
   // data from the input streams, given to the constructor

   /* the desired output type (count, tuple stream or TBlock stream) */
   const OutputType outputType;

   /* the output tuple type (if the desired outputType is a tuple stream) */
   TupleType* outputTupleType;

   /* the TupleBlocks (TBlocks) received from the two input streams
    * (in case full tuples are required) */
   std::vector<CRelAlgebra::TBlock*>* tBlocks[SET_COUNT];

   /* the tuples received from the two input streams
    * (in case the output should be a tuple stream, too) */
   std::vector<Tuple*>* tuples[SET_COUNT];

   /* the RectangleBlocks received from the two input streams
    * (in case rectangles are required only) */
   std::vector<RectangleBlock*>* rBlocks[SET_COUNT];

   /* the position of the join attribute in the tuple (for each stream) */
   const unsigned attrIndices[SET_COUNT];

   /* the number of columns (attributes) in the tuples (for each stream) */
   const uint64_t columnCounts[SET_COUNT];

   /* the spatial dimension (2 or 3) of the GeoData (for each stream) */
   const unsigned dims[SET_COUNT];

   /* the number of spatial dimensions common to both streams */
   const unsigned minDim;

   // -----------------------------------------------------
   // variables for computing SetRowBlock_t values (i.e. the "address" of
   // a rectangle in the input blocks). Note that the input sets A and B have
   // independent values for rowShift (and consequently for rowMask and
   // blockMask)

   /* the number of bits by which the row information is left-shifted
    * inside a SetRowBlock_t. */
   const unsigned rowShift[SET_COUNT];

   /* the bit mask for the row information inside a SetRowBlock_t */
   const SetRowBlock_t rowMask[SET_COUNT];

   /* the bit mask for the block information in the lower bits of a
    * SetRowBlock_t value. */
   const SetRowBlock_t blockMask[SET_COUNT];

   /* the number of bytes used for the TBlocks / RBlocks of the respective
    * input stream */
   const size_t usedMemory[SET_COUNT];

   // -----------------------------------------------------
   // variables for the appendToOutput() function

   /* the maximum size of the output buffer (i.e. the output TBlock or
    * the outTuples vector) in bytes */
   const uint64_t outBufferSize;

   /* the additional memory required for each output tuple (if an output tuple
    * stream is required). Since the input tuples and their attributes already
    * exist in memory when an output tuple is concatenated, the output tuple
    * will usually only get pointers to these attributes, so outTupleAddSize
    * does not count the attribute sizes */
   const uint64_t outTupleAddSize;

   /* if the result stream is a tuple stream, outBufferTupleCountMax is the
    * maximum number of output tuples that can be temporarily stored in
    * the outTuples vector, before the vector must to be flushed to the
    * output stream */
   const uint64_t outBufferTupleCountMax;

   /* an array of attributes of the result tuple type; this is used to
    * compile the next output tuple and pass it to the outTBlock */
   CRelAlgebra::AttrArrayEntry* const newTuple;

   /* the last source rectangle from set A that was used for output */
   SetRowBlock_t lastAddressA;

   /* the last source rectangle from set B that was used for output */
   SetRowBlock_t lastAddressB;

   /* the output TBlock (used only if a TBlock stream is the desired output) */
   CRelAlgebra::TBlock* outTBlock = nullptr;

   /* the output tuples (used only if a tuple stream is the desired output) */
   std::vector<Tuple*>* outTuples = nullptr;

   /* the total number of output tuples returned by this JoinState. The next
    * chunk of these tuples is temporarily stored either in outTBlock or in
    * outTuples. */
   uint64_t outTupleCount = 0;

#ifdef CDAC_SPATIAL_JOIN_METRICS
   /* the MemSize in bytes of all output tuples returned by a nextTBlock call */
   uint64_t outTuplesMemSize = 0;
#endif

public:
   /* creates an IOData instance from the given InputStreams */
   IOData(OutputType outputType_, TupleType* outputTupleType_,
          InputStream* inputA, InputStream* inputB, uint64_t outBufferSize_,
          uint64_t outTupleAddSize_, uint64_t outBufferTupleCountMax_);

   ~IOData();

#ifdef CDAC_SPATIAL_JOIN_DETAILED_REPORT_TO_CONSOLE
   std::string toString(const JoinEdge& joinEdge) const;
#endif

   /* calculates the bounding box of the given set; if addToEdges == true,
    * the edges of the rectangles in this set are added to sortEdges, and
    * information on these rectangles to rectangleInfos. otherBbox is only
    * required for addToEdges == true and used as a filter (since rectangles
    * that are completely outside the other set's bounding box need not be
    * considered any further) */
   Rectangle<3> calculateBboxAndEdges(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

private:
   /* specializes calculateBboxAndEdges() for 2-dimensional input data in
    * TBlocks (nevertheless, a 3-dimensional bounding box is returned to
    * provide for cases of mixed 2-/3-dimensional input data) */
   Rectangle<3> calculateBboxAndEdges2DFromTBlocks(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

   /* specializes calculateBboxAndEdges() for 3-dimensional input data in
    * TBlocks */
   Rectangle<3> calculateBboxAndEdges3DFromTBlocks(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

   /* specializes calculateBboxAndEdges() for 2-dimensional input data in
    * Tuples (nevertheless, a 3-dimensional bounding box is returned to
    * provide for cases of mixed 2-/3-dimensional input data) */
   Rectangle<3> calculateBboxAndEdges2DFromTuples(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

   /* specializes calculateBboxAndEdges() for 3-dimensional input data in
    * Tuples */
   Rectangle<3> calculateBboxAndEdges3DFromTuples(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

   /* specializes calculateBboxAndEdges() for input data in RectangleBlocks
    * (both 2- and 3-dimensional) */
   Rectangle<3> calculateBboxAndEdgesCount(SET set, bool addToEdges,
           const Rectangle<3>& otherBbox,
           std::vector<SortEdge>* sortEdges,
           std::vector<RectangleInfo>* rectangleInfos) const;

   /* returns the SetRowBlock_t value that contains the given "address"
    * (set, row, block) in one value */
   inline SetRowBlock_t getAddress(const SET set, const RowIndex_t row,
                                   const BlockIndex_t block) const {
      return ((set == SET::A) ? 0 : SET_MASK)
             | static_cast<SetRowBlock_t>(row << rowShift[set])
             | block;
      // SetRowBlock_t address = ...;
      // assert (set == getSet(address));
      // assert (row == getRowIndex(set, address));
      // assert (block == getBlockIndex(set, address));
      // return address;
   }

   /* returns the row index stored in the given address (the set value is
    * also contained in the address, but must be provided for efficiency) */
   inline RowIndex_t getRowIndex(const SET set, const SetRowBlock_t address)
   const {
      return (address & rowMask[set]) >> rowShift[set];
   }

   /* returns the block index stored in the given address (the set value is
    * also contained in the address, but must be provided for efficiency) */
   inline BlockIndex_t getBlockIndex(const SET set, const SetRowBlock_t address)
   const {
      return static_cast<BlockIndex_t>(address & blockMask[set]);
   }

public:
   /* sets the output tuple block and tuple vector to the given value,
    * Depending on the desired outputType, either outTBlock is used (for an
    * output stream of TBlocks), or outTuples (for an output stream of tuples),
    * or none (for CDACSpatialJoinCount). If not used, outTBlock and outTuples
    * may be nullptr. */
   inline void setOutput(CRelAlgebra::TBlock* outTBlock_,
           std::vector<Tuple*>* outTuples_) {
      outTBlock = outTBlock_;
      outTuples = outTuples_;
#ifdef CDAC_SPATIAL_JOIN_METRICS
      outTuplesMemSize = 0;
#endif
   }

   /* returns the number of tuples that were added to the output tuple block
    * (or, in "countOnly" mode, the number of intersections that were
    * counted) */
   inline uint64_t getOutTupleCount() const { return outTupleCount; }

   /* returns the number of bytes that were additionally reserved from memory
    * for the output tuples created by this IOData instance. In case of an
    * output tuple stream, this differs from getOutputMemSize() since the
    * output tuples point to the same Attribute instances as the input tuples */
   size_t getOutputAddSize(uint64_t tuplesAdded) const;

#ifdef CDAC_SPATIAL_JOIN_METRICS
   /* returns the total memory size of the tuples that were added to the
    * output by this IOData instance, including the memory used for Attribute
    * instances that are referred to from both the input and the output tuples
    * */
   size_t getOutputMemSize() const;
#endif

   /* adds the given number to the output tuple count which is used for the
    * CDACSpatialJoinCount operator */
   void addToOutTupleCount(uint64_t count);

   /* expects entryS and entryT to be representing two rectangles in a self
    * join that intersect in the x and y dimensions; checks whether
    * the rectangles intersect in the z dimension, too (if applicable), then
    * appends a new tuple to the outTBlock or increases the result counter */
   bool selfJoinAppendToOutput(const JoinEdge& entryS, const JoinEdge& entryT);

   /* expects entryS and entryT to be representing two rectangles from
    * different sets that intersect in the x and y dimensions; checks whether
    * the rectangles intersect in the z dimension, too (if applicable), then
    * appends a new tuple to the outTBlock or increases the result counter */
   bool appendToOutput(const JoinEdge& entryS, const JoinEdge& entryT,
           bool overrideSet = false);

#ifdef CDAC_SPATIAL_JOIN_METRICS
   /* returns the number of bytes currently used by this IOData instance,
    * including the memory used for the tBlocks / rBlocks of the InputStreams */
   size_t getUsedMemory() const;
#endif
};

} // end of namespace cdacspatialjoin