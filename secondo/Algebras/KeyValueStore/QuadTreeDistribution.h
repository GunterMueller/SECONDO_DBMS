/*
----
This file is part of SECONDO.

Copyright (C) 2015,
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

*/

#ifndef QUADTREEDISTRIBUTION_H_
#define QUADTREEDISTRIBUTION_H_

#include "Distribution.h"
#include <functional>
#include <set>

namespace KVS {

class QuadNode {
 public:
  QuadNode();
  QuadNode(double x, double y, double width, double height);
  QuadNode(QuadNode* parent, double x, double y, double width, double height);
  QuadNode(QuadNode* node);
  QuadNode(const QuadNode& node);

  ~QuadNode();

  bool isLeaf();

  bool isOverlapping(double* mbb);
  bool isOverlappingDebug(double* mbb);

  bool isInside(double* mbb);

  QuadNode* get(const int& i);

  void init(QuadNode* prototype);

  void split();
  int level();
  int levels();
  QuadNode* root();

  QuadNode* parent;
  QuadNode* children[4];

  double x, y;
  double width, height;

  int serverId;
  int weight;

  unsigned int maxGlobalId;

 private:
  void levels(QuadNode* node, const int& level, int* maxLevel);
};

void propagateUp(QuadNode* node, std::function<void(QuadNode*)> f);

void propagateDown(QuadNode* node, std::function<void(QuadNode*)> f);

void propagateDownB(QuadNode* node, std::function<bool(QuadNode*)> f);

void propagateDownPost(QuadNode* node, std::function<void(QuadNode*)> f);

bool findDescendant(QuadNode* node, std::function<bool(QuadNode*)> f);

bool touching(QuadNode* a, QuadNode* b);

class QuadTreeDistribution : public Distribution {
 public:
  QuadTreeDistribution();
  QuadTreeDistribution(int initialWidth, int initialHeight, int nrServers);
  QuadTreeDistribution(const QuadTreeDistribution& rhs);
  ~QuadTreeDistribution();

  QuadTreeDistribution* Clone();

  void init(QuadTreeDistribution* prototype);

  void expand(double* mbb);
  void insert(QuadNode* node, double* mbb, std::set<int>* results);
  void insertDebug(QuadNode* node, double* mbb, std::set<int>* results);

  void retrieveIds(QuadNode* node, double* mbb, std::set<int>* results);
  void retrieveIdsDebug(QuadNode* node, double* mbb, std::set<int>* results);

  void redistributeCluster();
  void leafesInClusterOrder(QuadNode* node, std::function<bool(QuadNode*)> f);

  int split(int serverId);
  void redistribute(int serverId,
                    int n);  // n: number of servers after serverId (referencing
  // serverIdMapping order)

  int neighbourId(int id);
  void changeServerId(int oldid, int newid);
  int pointId(double x, double y);

  void resetWeight();
  void updateWeightVector();
  void addWeight(Distribution* dist, const int& id);

  void resetMaxGlobalIds();
  void addMaxGlobalIds(Distribution* dist, const int& id);

  bool filter(int nrcoords, double* coords, const unsigned int& globalId,
              bool update);

  void add(int value, std::set<int>* resultIds);
  void add(int nrcoords, double* coords, std::set<int>* resultIds);
  void addDebug(int nrcoords, double* coords, std::set<int>* resultIds);

  void request(int value, std::set<int>* resultIds);
  void request(int nrcoords, double* coords, std::set<int>* resultIds);
  void requestDebug(int nrcoords, double* coords, std::set<int>* resultIds);

  std::string serverIdAssignment(std::string attributeName,
                                 std::string distributionName,
                                 bool requestOnly);

  void createAreaObjectCountList(std::list<std::pair<double*, int> >* areaList);

  std::string toBin();
  bool fromBin(const std::string& data);

  QuadNode* root;
  int initialWidth, initialHeight;

 private:
  std::string quadnodeToBin(QuadNode* node);
  QuadNode* quadnodeFromBin(std::stringstream& data, QuadNode* parent);

  void addWeightNode(QuadNode* base, QuadNode* add, const int& id);

  void addGlobalIdNode(QuadNode* base, QuadNode* add, const int& id);
  void filterUpdate(QuadNode* node, double* mbb, const unsigned int& globalId);
  void filterCheck(QuadNode* node, double* mbb, const unsigned int& globalId,
                   bool& result);

  int nextServerId();
  void fixNodeServerIds();
  void fixNodeWeights();
  void consolidateLayers();
  void leafesInClusterOrderR(QuadNode* node, const int& currentIdx,
                             const int& nextIdx, QuadNode** lastVisited,
                             std::function<bool(QuadNode*)> f);
};
}

#endif /* QUADTREEDISTRIBUTION_H_ */
