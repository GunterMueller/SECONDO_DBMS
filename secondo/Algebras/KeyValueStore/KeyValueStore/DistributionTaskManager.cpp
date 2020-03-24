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

#include "DistributionTaskManager.h"

#include "KeyValueStore.h"

#include "boost/thread.hpp"
#include "boost/date_time.hpp"

namespace KVS {

DistributionTaskManager::DistributionTaskManager(KeyValueStore* instance)
    : noRestructure(false),
      restructureProcess(0),
      instance(instance),
      currentDistributionId(-1),
      syncDistribution(false),
      criteria(instance) {
  time(&lastRestructure);
  RESTRUCTURE_INTERVAL = 30;  // in seconds
}

DistributionTaskManager::~DistributionTaskManager() {
  if (restructureProcess) {
    if (restructureProcess->joinable()) {
      restructureProcess->join();
    }
    delete restructureProcess;
  }
}

void DistributionTaskManager::processDistributions(bool* run, unsigned int n) {
  for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
    // process(n) make n scale with nr of tasks, so that many taks will lower n?
    if (tasks[taskIdx]->process(n) > 0) {
      removedTasks.push_back(tasks[taskIdx]);
      removeTask(taskIdx);
      taskIdx--;

      newTasksMutex.lock();
      exitMutex.lock();
      if (tasks.size() == 0 && newTasks.size() == 0) {
        *run = false;
      }
      exitMutex.unlock();
      newTasksMutex.unlock();
    }
  }
}

void DistributionTaskManager::exec(bool* run) {
  // TODO: find out ideal n, writing seems much slower than expected
  int n = 5;

  KOUT << "Starting Distribution Loop..." << endl;

  time(&lastRestructure);

  while (*run) {
    if (newTasks.size() > 0) {
      newTasksMutex.lock();

      for (unsigned int taskIdx = 0; taskIdx < newTasks.size(); ++taskIdx) {
        tasks.push_back(newTasks[taskIdx]);
      }
      newTasks.clear();
      newTasksMutex.unlock();
    }

    processDistributions(run, n);

    if (syncDistribution) {
      syncDistributions(run);
      syncDistribution = false;
    }

    if (restructureProcess != 0) {
      if (restructureProcess->joinable()) {
        restructureProcess->join();
        restructureProcess = 0;
      }
    } else {
      if (removedTasks.size() > 0) {
        for (unsigned int i = 0; i < removedTasks.size(); ++i) {
          delete removedTasks[i];
        }
        removedTasks.clear();
      }

      restructure(run);
    }
  }

  KOUT << "Distribution Loop stopped..." << endl;

  if (restructureProcess) {
    KOUT << "Waiting for restructure process...";
    restructureProcess->join();
    delete restructureProcess;
  }

  for (unsigned int i = 0; i < removedTasks.size(); ++i) {
    delete removedTasks[i];
  }
  removedTasks.clear();

  KOUT << "Transferred " << criteria.distributedBytes() << " bytes / "
       << setprecision(10) << criteria.distributedBytes() / (1024.0 * 1024.0)
       << " MB" << endl;

  if (currentDistributionId > -1) {
    Distribution* currentDist =
        instance->getDistribution(currentDistributionId);
    SaveDebugFile(instance->getRestructureFilePath("finished"), currentDist);
  }
}

bool DistributionTaskManager::addTask(DistributionTask* task) {
  boost::lock_guard<boost::mutex> guard(newTasksMutex);

  if (currentDistributionId == -1) {
    currentDistributionId = task->distParams.distributionId;
    Distribution* currentDist =
        instance->getDistribution(currentDistributionId);
    currentDist->addUpdateListener(this);

  } else if (currentDistributionId != task->distParams.distributionId) {
    return false;
  }

  task->setDistributionCriteria(&criteria);
  newTasks.push_back(task);

  return true;
}

bool DistributionTaskManager::removeTask(unsigned int index) {
  boost::lock_guard<boost::mutex> guard(newTasksMutex);

  if (index < tasks.size()) {
    tasks.erase(tasks.begin() + index);
    if (tasks.size() == 0) {
      Distribution* currentDist =
          instance->getDistribution(currentDistributionId);
      currentDist->removeUpdateListener(this);
      currentDistributionId = -1;
    }
    return true;
  } else {
    return false;
  }
}

void DistributionTaskManager::distributionUpdated() {
  ROUT << "Syncing updates Distribution..." << endl;
  vector<Connection*>& serverList = instance->sm.getConnectionList();

  Distribution* currentDist = instance->getDistribution(currentDistributionId);

  if (currentDist) {
    string distName = instance->getDistributionName(currentDistributionId);
    string distData = currentDist->toBin();

    for (unsigned int serverIdx = 0; serverIdx < serverList.size();
         ++serverIdx) {
      if (!serverList[serverIdx]->restructureInProgress) {
        if (serverList[serverIdx]->kvsConn->sendDistributionUpdate(distName,
                                                                   distData)) {
          serverList[serverIdx]->updateDistributionObject(distName);
        }
      }
    }
  }
}

void DistributionTaskManager::resetRedistribtionProgress() {
  vector<Connection*>& serverList = instance->sm.getConnectionList();

  for (unsigned int serverIdx = 0; serverIdx < serverList.size(); ++serverIdx) {
    serverList[serverIdx]->restructureInProgress = false;
  }
}

void DistributionTaskManager::restructure(bool* run) {
  if (currentDistributionId > -1 && !noRestructure &&
      difftime(time(NULL), lastRestructure) > RESTRUCTURE_INTERVAL) {
    time(&lastRestructure);

    vector<Connection*>& serverList = instance->sm.getConnectionList();

    Distribution* current = instance->getDistribution(currentDistributionId);

    criteria.evaluateCriteria(current, serverList.size());

    if ((criteria.split.size() > 0 &&
         (serverList.size() - current->serverIdOrder.size() > 0)) ||
        criteria.localRestructure.size() > 0) {
      criteria.reset();

      ROUT << DebugTime() << "Starting Restructure." << endl;

      current->syncMutex.lock();

      processDistributions(run, 100);

      vector<Connection*> involvedServers;

      string debugPath = instance->getRestructureFilePath("before");
      SaveDebugFile(debugPath, current);
      ROUT << "Saving current distribution state:" << debugPath << endl;

      // 1.update local distribution => after we continue all future data will
      // directly be transferred to correct sever
      for (unsigned int splitIdx = 0; splitIdx < criteria.split.size();
           ++splitIdx) {
        if (serverList.size() - current->serverIdOrder.size() > 0) {
          ROUT << "Splitting: " << splitIdx << endl;
          int serverId = criteria.split[splitIdx];
          int serverIdx = instance->sm.getConnectionIndex(serverId);

          // TODO: rework this assignment
          int newServerId = current->split(serverId);
          int newServerIdx = instance->sm.getAvailableServer();

          if (newServerIdx >= 0) {
            serverList[newServerIdx]->id = newServerId;
            if (!serverList[serverIdx]->check() ||
                !serverList[serverIdx]->kvsConn->check()) {
              cout << instance->localHost << " : "
                   << instance->localInterfacePort << " : "
                   << instance->localKvsPort << " : "
                   << instance->currentDatabaseName << endl;
              if (!serverList[serverIdx]->initInterface(
                      instance->localHost, instance->localInterfacePort,
                      instance->localKvsPort, instance->currentDatabaseName)) {
                ROUT << "Error: Server could not be initialized.";
                break;
              }
            }

            serverList[serverIdx]->rLock.startRestructuring();
            involvedServers.push_back(serverList[serverIdx]);
          } else {
            break;
          }
        }
      }

      // redistribute
      for (unsigned int restructureIdx = 0;
           restructureIdx < criteria.localRestructure.size();
           ++restructureIdx) {
        int serverId = criteria.localRestructure[restructureIdx].first;
        int n = criteria.localRestructure[restructureIdx].second;

        current->redistribute(serverId, n);

        // we don't know which servers are affected so all are selected
        vector<int>::iterator currentPos =
            find(current->serverIdOrder.begin(), current->serverIdOrder.end(),
                 serverId);

        for (int i = 0; i < n; ++i) {
          int serverIdx = instance->sm.getConnectionIndex(*(currentPos + i));
          serverList[serverIdx]->rLock.startRestructuring();
          involvedServers.push_back(serverList[serverIdx]);
        }

        ROUT << "Redistribution starting with:" << serverId << " +"
             << criteria.localRestructure[restructureIdx].second << endl;
      }

      // 1.1 force finish all running batches?
      for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
        if (!tasks[taskIdx]->finishBatch()) {
          removedTasks.push_back(tasks[taskIdx]);
          removeTask(taskIdx);
          taskIdx--;
        }
      }

      current->syncMutex.unlock();

      debugPath = instance->getRestructureFilePath("after");
      SaveDebugFile(debugPath, current);
      ROUT << "Saving distribution state after redistribution:" << debugPath
           << endl;

      // 2.send updated version to involved servers

      ROUT << "Updating Serverlists on involved Servers" << endl;

      for (unsigned int serverIdx = 0; serverIdx < involvedServers.size();
           ++serverIdx) {
        involvedServers[serverIdx]->exec("query kvsSyncServerList()", true);
      }

      ROUT << "Start Distribution Data State (global-IDs)";

      syncDataState();

      ROUT << "Sending Distribution Updates" << endl;

      string distData = current->toBin();
      // string distName = instance->getDistributionName(currentDistributionId);
      string distName = "restructureUpdateDist";

      for (unsigned int serverIdx = 0; serverIdx < involvedServers.size();
           ++serverIdx) {
        involvedServers[serverIdx]->kvsConn->sendDistributionUpdate(distName,
                                                                    distData);
      }

      ROUT << "Enabling continuous batches until restructure is complete..."
           << endl;
      for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
        tasks[taskIdx]->setContinueCurrentBatch(true);
      }

      // ROUT<<"Starting async restructure phase."<<endl;
      // currently doesnt use threads since connection mutex will be blocked
      // anyway
      // restructureProcess = new
      // boost::thread(&DistributionTaskManager::restructurePhaseTwo, this,
      // involvedServers);

      ROUT << "Starting restructure phase." << endl;
      restructurePhaseTwo(involvedServers);

      ROUT << "Continuing normal operation..." << endl;
    }
  }
}

void DistributionTaskManager::restructurePhaseTwo(
    vector<Connection*> involvedServers) {
  // splitting off - first steps done
  // 3.initiate distribute

  vector<boost::thread> distThreads;
  vector<DistributionParameter> distParams;

  for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
    distParams.push_back(tasks[taskIdx]->distParams);
  }

  ROUT << "Start redistribution threads." << endl;

  for (unsigned int serverIdx = 0; serverIdx < involvedServers.size();
       ++serverIdx) {
    distThreads.push_back(
        boost::thread(&DistributionTaskManager::restructureRedistribute, this,
                      involvedServers[serverIdx], distParams));
  }

  ROUT << "Waiting for redistribution threads..." << endl;

  for (unsigned int distIdx = 0; distIdx < distThreads.size(); ++distIdx) {
    distThreads[distIdx].join();
  }
  distThreads.clear();

  ROUT << "Redistribution threads finished..." << endl;

  ROUT << "Disabling continuous batches ..." << endl;
  for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
    tasks[taskIdx]->setContinueCurrentBatch(false);
  }

  // 4.when distribute is finished we are ready to UPDATE distribution again
  // same as 2. just different distribution

  ROUT << "Signal that restructure is complete." << endl;

  resetRedistribtionProgress();

  ROUT << "Send updated distribution to all servers." << endl;

  distributionUpdated();

  // TODO:

  // splitting off again
  // 6. Collect Garbage in ALL INVOLVED RELATIONS
  // 6.1 try to acquire restructure lock - wait for it.

  // 6.2 delete
  // query RELATIONNAME

  for (unsigned int serverIdx = 0; serverIdx < involvedServers.size();
       ++serverIdx) {
    distThreads.push_back(
        boost::thread(&DistributionTaskManager::restructureCleanData, this,
                      involvedServers[serverIdx], distParams));
  }

  // TODO: Trigger Distribution synchonisation
  for (unsigned int distIdx = 0; distIdx < distThreads.size(); ++distIdx) {
    distThreads[distIdx].join();
  }

  Distribution* current = instance->getDistribution(currentDistributionId);
  if (current->needsSynchronisation()) {
    ROUT << "Distribution needs synchronisation." << endl;
    syncDistribution = true;
  }
  time(&lastRestructure);
}

void DistributionTaskManager::restructureRedistribute(
    Connection* server, vector<DistributionParameter> distParams) {
  Distribution* current = instance->getDistribution(currentDistributionId);

  for (unsigned int distIdx = 0; distIdx < distParams.size(); ++distIdx) {
    // TODO: command
    server->exec(
        "query " + distParams[distIdx].targetRelation +
        " feed kvsFilter['restructureUpdateDist', GeoData, GlobalId, FALSE] " +
        current->serverIdAssignment("ServerId", "restructureUpdateDist", true) +
        " kvsDistribute[ServerId, 'restructureUpdateDist', '" +
        distParams[distIdx].targetRelation + "', '" +
        distParams[distIdx].insertCommand + "', FALSE] count");
  }
}

// TODO: Probably need more than that. (Have class/struct with all important
// information)
void DistributionTaskManager::restructureCleanData(
    Connection* server, vector<DistributionParameter> distParams) {
  if (server->rLock.lock()) {
    for (unsigned int distIdx = 0; distIdx < distParams.size(); ++distIdx) {
      // TODO: Clean

      // query TARGETRELATION feed extendstream[ServerId:
      // qtserverid(bbox(.GeoData), " + distributionName + ")]
      // filter[ServerId<>SERVERID] TARGETRELATION deletesearch DELETECOMMAND
      // server->exec("query " + distParams[distIdx].targetRelation + " feed" )
    }
    server->rLock.unlock();
  } else {
    // TODO: Error
  }
}

void DistributionTaskManager::syncDistributions(bool* run) {
  if (currentDistributionId > -1) {
    ROUT << "Sync process started. (" << DebugTime() << ")" << endl;
    Distribution* current = instance->getDistribution(currentDistributionId);
    string distributionName =
        "Sync" + instance->getDistributionName(currentDistributionId);

    ROUT << "Locking distribution..." << endl;
    // 1. make sure no new entries can be created
    current->syncMutex.lock();

    ROUT << "Process outstanding tuples..." << endl;
    // 2. make sure all data is send
    processDistributions(run, 200);

    ROUT << "Force Batches..." << endl;
    // 2.1 force finish all running batches?
    for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
      if (!tasks[taskIdx]->finishBatch()) {
        removedTasks.push_back(tasks[taskIdx]);

        removeTask(taskIdx);
        taskIdx--;
      }
    }

    ROUT << "Create snapshot" << endl;
    // 3. Prepare Snapshot
    Distribution* distCopy = Distribution::getInstance(current->toBin());
    distCopy->resetWeight();
    string distCopyData = distCopy->toBin();

    string debugPath = instance->getRestructureFilePath("sync_empty");
    SaveDebugFile(debugPath, distCopy);

    debugPath = instance->getRestructureFilePath("sync__before");
    SaveDebugFile(debugPath, current);
    ROUT << "Debug saved snapshot:" << debugPath << endl;

    // 3.1 Prepare to send snapshot
    vector<DistributionParameter> distParams;
    for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
      distParams.push_back(tasks[taskIdx]->distParams);
    }

    ROUT << "Exchange sync data in threads.." << endl;
    vector<boost::thread> distThreads;
    vector<Connection*>& serverList = instance->sm.getConnectionList();
    for (unsigned int serverIdx = 0; serverIdx < serverList.size();
         ++serverIdx) {
      distThreads.push_back(
          boost::thread(&DistributionTaskManager::syncDistributionsThread, this,
                        serverList[serverIdx], distParams, distCopy,
                        distributionName, distCopyData));
    }

    ROUT << "Join threads..." << endl;
    for (unsigned int serverIdx = 0; serverIdx < serverList.size();
         ++serverIdx) {
      distThreads[serverIdx].join();

      ROUT << "Requesting Data.." << endl;
      // 5. Request updated Snapshots
      string tempData;
      if (serverList[serverIdx]->kvsConn->requestDistribution(distributionName,
                                                              tempData)) {
        // 6.Merge Updates into distCopy

        ROUT << "Merge data..." << endl;
        Distribution* tempDist = Distribution::getInstance(tempData);

        debugPath = instance->getRestructureFilePath(
            "sync_" + stringutils::int2str(serverIdx) + "_mid");
        SaveDebugFile(debugPath, tempDist);

        if (tempDist) {
          distCopy->addWeight(tempDist, serverList[serverIdx]->id);

          delete tempDist;
        }
      }
    }

    ROUT << "Update Weights vector..." << endl;
    distCopy->updateWeightVector();

    ROUT << "Update current Distribution..." << endl;
    current->fromBin(distCopy->toBin());
    current->syncMutex.unlock();

    debugPath = instance->getRestructureFilePath("sync_after");
    SaveDebugFile(debugPath, current);
    ROUT << "Debug saved snapshot:" << debugPath << endl;

    delete distCopy;
    ROUT << "Sync finished ...(" << DebugTime() << ")" << endl;
    time(&lastRestructure);
  }
}

void DistributionTaskManager::syncDistributionsThread(
    Connection* server, vector<DistributionParameter> distParams,
    Distribution* dist, string distName, string distData) {
  // 3. Send Snapshot
  server->kvsConn->sendDistributionUpdate(distName, distData);

  // 4. Update Snapshot on Clients
  for (unsigned int distIdx = 0; distIdx < distParams.size(); ++distIdx) {
    ROUT << "Executing:"
         << "query " << distParams[distIdx].targetRelation << " feed "
         << dist->serverIdAssignment("ServerId", distName, false) << "count"
         << endl;
    server->exec("query " + distParams[distIdx].targetRelation + " feed " +
                 dist->serverIdAssignment("ServerId", distName, false) +
                 "count");
  }
}

void DistributionTaskManager::syncDataState() {
  if (currentDistributionId > -1) {
    ROUT << "Data State Sync process started. (" << DebugTime() << ")" << endl;
    Distribution* current = instance->getDistribution(currentDistributionId);
    string distributionName =
        "Sync" + instance->getDistributionName(currentDistributionId);

    current->resetMaxGlobalIds();

    string debugPath = instance->getRestructureFilePath("id_state_sync_before");
    SaveDebugFile(debugPath, current);
    ROUT << "Debug saved snapshot:" << debugPath << endl;

    vector<DistributionParameter> distParams;
    for (unsigned int taskIdx = 0; taskIdx < tasks.size(); ++taskIdx) {
      distParams.push_back(tasks[taskIdx]->distParams);
    }

    string distData = current->toBin();
    Distribution* distCopy = Distribution::getInstance(distData);

    ROUT << "Exchange sync data in threads.." << endl;
    vector<boost::thread> distThreads;
    vector<Connection*>& serverList = instance->sm.getConnectionList();
    for (unsigned int serverIdx = 0; serverIdx < serverList.size();
         ++serverIdx) {
      distThreads.push_back(boost::thread(
          &DistributionTaskManager::syncDataStateThread, this,
          serverList[serverIdx], distParams, distributionName, distData));
    }

    ROUT << "Join threads..." << endl;
    for (unsigned int serverIdx = 0; serverIdx < serverList.size();
         ++serverIdx) {
      distThreads[serverIdx].join();

      ROUT << "Requesting Data.." << endl;
      // 5. Request updated Snapshots
      string tempData;
      if (serverList[serverIdx]->kvsConn->requestDistribution(distributionName,
                                                              tempData)) {
        // 6.Merge Updates into distCopy

        ROUT << "Merge data..." << endl;
        Distribution* tempDist = Distribution::getInstance(tempData);

        debugPath = instance->getRestructureFilePath(
            "id_state_sync_" + stringutils::int2str(serverIdx) + "_mid");
        SaveDebugFile(debugPath, tempDist);

        if (tempDist) {
          current->addMaxGlobalIds(tempDist, serverList[serverIdx]->id);

          delete tempDist;
        }
      }
    }

    debugPath = instance->getRestructureFilePath("id_state_sync_after");
    SaveDebugFile(debugPath, current);
    ROUT << "Debug saved snapshot:" << debugPath << endl;

    delete distCopy;
    ROUT << "Sync finished ...(" << DebugTime() << ")" << endl;
  }
}

void DistributionTaskManager::syncDataStateThread(
    Connection* server, vector<DistributionParameter> distParams,
    string distName, string distData) {
  // 3. Send Snapshot
  server->kvsConn->sendDistributionUpdate(distName, distData);

  // 4. Update Snapshot on Clients
  for (unsigned int distIdx = 0; distIdx < distParams.size(); ++distIdx) {
    server->exec("query " + distParams[distIdx].targetRelation +
                 " feed kvsFilter['" + distName +
                 "', GeoData, GlobalId, TRUE] count");
  }
}
}
