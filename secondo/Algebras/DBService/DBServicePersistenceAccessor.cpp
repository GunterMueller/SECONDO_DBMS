/*

1.1.1 Class Implementation

----
This file is part of SECONDO.

Copyright (C) 2017,
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
#include <sstream>
#include <utility>

#include "SecondoException.h"
#include "StringUtils.h"

#include "Algebras/DBService/DBServicePersistenceAccessor.hpp"
#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/SecondoUtilsLocal.hpp"

using namespace std;

namespace DBService {


bool DBServicePersistenceAccessor::deleteAndCreate(
        const string& relationName,
        const RelationDefinition& rel,
        const vector<vector <string> >& values)
{
    printFunction("DBServicePersistenceAccessor::createOrInsert");
    string databaseName("dbservice");
    print(relationName);

    SecondoUtilsLocal::adjustDatabase(databaseName);

    if(SecondoSystem::GetCatalog()->IsObjectName(relationName))
    {
        SecondoSystem::GetCatalog()->DeleteObject(relationName);
    }

    if(values.empty())
    {
        print("Nothing to persist");
        return true;
    }
    return createOrInsert(relationName, rel, values);
}

bool DBServicePersistenceAccessor::createOrInsert(
        const string& relationName,
        const RelationDefinition& rel,
        const vector<vector<string> >& values)
{
    printFunction("DBServicePersistenceAccessor::createOrInsert");
    string databaseName("dbservice");
    print(relationName);

    SecondoUtilsLocal::adjustDatabase(databaseName);

    bool resultOk = false;
    string errorMessage;

    if(!SecondoSystem::GetCatalog()->IsObjectName(relationName))
    {
        print("relation does not exist: ", relationName);
//        SecondoSystem::CommitTransaction(true);
//        SecondoSystem::BeginTransaction();
        resultOk = SecondoUtilsLocal::createRelation(
                CommandBuilder::buildCreateCommand(
                        relationName,
                        rel,
                        {values}),
                errorMessage);
        if(resultOk)
        {
//            SecondoSystem::CommitTransaction(true);
            print("created relation: ", relationName);
        }else
        {
//            SecondoSystem::AbortTransaction(true);
            print("failed to create relation: ", relationName);
        }
        return resultOk;
    }
    print("relation exists, trying insert command");

    if(values.size() != 1)
    {
        print("values has wrong format for insert, aborting");
        return false;
    }
//    SecondoSystem::BeginTransaction();
    resultOk = SecondoUtilsLocal::executeQuery2(
            CommandBuilder::buildInsertCommand(
                    relationName,
                    rel,
                    values[0]));

    if(resultOk)
    {
//        SecondoSystem::CommitTransaction(true);
        print("insert successful");
    }else
    {
//        SecondoSystem::AbortTransaction(true);
        print("insert failed");
    }
    return resultOk;
}

bool DBServicePersistenceAccessor::persistLocationInfo(
        ConnectionID connID, LocationInfo& locationInfo)
{
    printFunction("DBServicePersistenceAccessor::persistLocationInfo");

    string relationName("locations_DBSP");

    vector<string> value =
    {
        stringutils::int2str(connID),
        locationInfo.getHost(),
        locationInfo.getPort(),
        locationInfo.getConfig(),
        locationInfo.getDisk(),
        locationInfo.getCommPort(),
        locationInfo.getTransferPort(),
    };

    return createOrInsert(relationName, locations, {value});
}

bool DBServicePersistenceAccessor::persistRelationInfo(
        RelationInfo& relationInfo)
{
    printFunction("DBServicePersistenceAccessor::persistRelationInfo");

    string relationName("relations_DBSP");

    vector<string> value =
    {
        relationInfo.toString(),
        relationInfo.getDatabaseName(),
        relationInfo.getRelationName(),
        relationInfo.getOriginalLocation().getHost(),
        relationInfo.getOriginalLocation().getPort(),
        relationInfo.getOriginalLocation().getDisk()
    };

    bool resultOk =
            createOrInsert(relationName, relations, {value});
    if(resultOk)
    {
        print("RelationInfo persisted");
        resultOk = persistLocationMapping(
                   relationInfo.toString(),
                   relationInfo.nodesBegin(),
                   relationInfo.nodesEnd());
    }else
    {
        print("Could not persist RelationInfo. Skipping mapping.");
    }
    return resultOk;
}

bool DBServicePersistenceAccessor::persistLocationMapping(
        string relationID,
        ReplicaLocations::const_iterator nodesBegin,
        ReplicaLocations::const_iterator nodesEnd)
{
    printFunction("DBServicePersistenceAccessor::persistLocationMapping");

    bool resultOk = true;
    for(ReplicaLocations::const_iterator it = nodesBegin;
            it != nodesEnd; it++)
    {
        string relationName("mapping_DBSP");

        vector<string> value =
        {
            relationID ,
            stringutils::int2str(it->first) ,
            it->second ? string("TRUE") : string("FALSE") ,
        };

        resultOk = resultOk &&
                createOrInsert(
                        relationName, mapping, {value});
        if(!resultOk)
        {
            print("failed to persist location mapping");
        }
    }
    if(resultOk)
    {
        print("location mapping persisted successfully");
    }
    return resultOk;
}


bool DBServicePersistenceAccessor::persistDerivateInfo(
        DerivateInfo& derivateInfo)
{
    printFunction(__PRETTY_FUNCTION__);

    string relationName("derivate_DBSP");

    vector<string> value =
    {
       derivateInfo.getName(),
       derivateInfo.getSource(),
       derivateInfo.getFun()
    };

    bool resultOk =
            createOrInsert(relationName, derivates, {value});
    if(resultOk)
    {
        print("RelationInfo persisted");
        resultOk = persistLocationMapping(
                   derivateInfo.toString(),
                   derivateInfo.nodesBegin(),
                   derivateInfo.nodesEnd());
    }else
    {
        print("Could not persist DerivateInfo. Skipping mapping.");
    }
    return resultOk;
}


bool DBServicePersistenceAccessor::restoreLocationInfo(
        map<ConnectionID, LocationInfo>& locations)
{
    printFunction("DBServicePersistenceAccessor::restoreLocationInfo");
    bool resultOk = true;
    if(SecondoSystem::GetCatalog()->IsObjectName(string("locations_DBSP")))
    {
        string query("query locations_DBSP");
        string errorMessage;
        ListExpr resultList;
        resultOk = SecondoUtilsLocal::executeQueryCommand(
                query, resultList, errorMessage);
        if(resultOk)
        {
            print("resultList", resultList);
            ListExpr resultData = nl->Second(resultList);
            print("resultData", resultData);

            int resultCount = nl->ListLength(resultData);
            print(resultCount);

            for(int i = 0; i < resultCount; i++)
            {
                if(!nl->IsEmpty(resultData))
                {
                    print("resultData", resultData);
                    ListExpr currentRow = nl->First(resultData);
                    ConnectionID conn(nl->IntValue(nl->First(currentRow)));
                    string host(nl->StringValue(nl->Second(currentRow)));
                    string port(nl->StringValue(nl->Third(currentRow)));
                    string config(nl->Text2String(nl->Fourth(currentRow)));
                    string disk(nl->Text2String(nl->Fifth(currentRow)));
                    string commPort(nl->StringValue(nl->Sixth(currentRow)));
                    string transferPort(
                            nl->StringValue(nl->Seventh(currentRow)));

                    LocationInfo location(
                            host, port, config, disk, commPort, transferPort);
                    print(location);
                    locations.insert(
                            pair<ConnectionID, LocationInfo>(conn, location));
                    resultData = nl->Rest(resultData);
                }
            }
        }else
        {
            print(errorMessage);
        }
    }else
    {
        print("locations_DBSP not found -> nothing to restore");
    }
    return resultOk;
}

bool DBServicePersistenceAccessor::restoreRelationInfo(
        map<string, RelationInfo>& relations)
{
    printFunction("DBServicePersistenceAccessor::restoreRelationInfo");

    bool resultOk = true;
    if(SecondoSystem::GetCatalog()->IsObjectName(string("relations_DBSP")))
    {
        string query("query relations_DBSP");
        string errorMessage;
        ListExpr resultList;
        resultOk = SecondoUtilsLocal::executeQueryCommand(
                query, resultList, errorMessage);
        if(resultOk)
        {
            print("resultList", resultList);
            ListExpr resultData = nl->Second(resultList);
            print("resultData", resultData);

            int resultCount = nl->ListLength(resultData);
            print(resultCount);

            for(int i = 0; i < resultCount; i++)
            {
                if(!nl->IsEmpty(resultData))
                {
                    print("resultData", resultData);
                    ListExpr currentRow = nl->First(resultData);
                    string relID(nl->StringValue(nl->First(currentRow)));
                    string dbName(nl->StringValue(nl->Second(currentRow)));
                    string relName(nl->StringValue(nl->Third(currentRow)));
                    string host(nl->StringValue(nl->Fourth(currentRow)));
                    string port(nl->StringValue(nl->Fifth(currentRow)));
                    string disk(nl->Text2String(nl->Sixth(currentRow)));
                    RelationInfo relationInfo(
                            dbName, relName, host, port, disk);
                    print(relationInfo);
                    relations.insert(
                            pair<string, RelationInfo>(relID, relationInfo));
                    resultData = nl->Rest(resultData);
                }
            }
        }else
        {
            print(errorMessage);
        }
    }else
    {
        print("relations_DBSP not found -> nothing to restore");
    }
    return resultOk;
}


bool DBServicePersistenceAccessor::restoreDerivateInfo(
        DBServiceDerivates& derivates)
{
    printFunction(__PRETTY_FUNCTION__);

    bool resultOk = true;
    string relName ("derivates_DBSP");
    if(SecondoSystem::GetCatalog()->IsObjectName(relName))
    {
        string query("query " + relName);
        string errorMessage;
        ListExpr resultList;
        resultOk = SecondoUtilsLocal::executeQueryCommand(
                query, resultList, errorMessage);
        if(resultOk)
        {
            print("resultList", resultList);
            ListExpr resultData = nl->Second(resultList);
            print("resultData", resultData);

            int resultCount = nl->ListLength(resultData);
            print(resultCount);

            for(int i = 0; i < resultCount; i++)
            {
                if(!nl->IsEmpty(resultData))
                {
                    print("resultData", resultData);
                    ListExpr currentRow = nl->First(resultData);
                    string objectName(nl->StringValue(nl->First(currentRow)));
                    string source(nl->StringValue(nl->Second(currentRow)));
                    string fun(nl->TextValue(nl->Third(currentRow)));
                    DerivateInfo derivateInfo(objectName, source, fun);
                    print(derivateInfo);
                    string did = derivateInfo.toString();
                    derivates.insert(
                            pair<string, DerivateInfo>(did, derivateInfo));
                    resultData = nl->Rest(resultData);
                }
            }
        }else
        {
            print(errorMessage);
        }
    }else
    {
        print(relName + " not found -> nothing to restore");
    }
    return resultOk;
}



bool DBServicePersistenceAccessor::restoreLocationMapping(
        queue<pair<string, pair<ConnectionID, bool> > >& mapping)
{
    printFunction("DBServicePersistenceAccessor::restoreLocationMapping");
    bool resultOk = true;
    if(SecondoSystem::GetCatalog()->IsObjectName(string("mapping_DBSP")))
    {
        string query("query mapping_DBSP");
        string errorMessage;
        ListExpr resultList;
        resultOk = SecondoUtilsLocal::executeQueryCommand(
                query, resultList, errorMessage);
        if(resultOk)
        {
            print("resultList", resultList);
            ListExpr resultData = nl->Second(resultList);
            print("resultData", resultData);

            int resultCount = nl->ListLength(resultData);
            print(resultCount);

            for(int i = 0; i < resultCount; i++)
            {
                if(!nl->IsEmpty(resultData))
                {
                    print("resultData", resultData);
                    ListExpr currentRow = nl->First(resultData);
                    string objectID(nl->StringValue(nl->First(currentRow)));
                    int conn = nl->IntValue(nl->Second(currentRow));
                    bool replicated = nl->BoolValue(nl->Third(currentRow));
                    print("ObjectID: ", objectID);
                    print("ConnectionID: ", conn);
                    print("Replicated: ", replicated);
                    mapping.push(
                            pair<string, pair<ConnectionID, bool> >(
                                    objectID, make_pair(conn, replicated)));
                    resultData = nl->Rest(resultData);
                }
            }
        }else
        {
            print(errorMessage);
        }
    }else
    {
        print("mapping_DBSP not found -> nothing to restore");
    }
    return resultOk;
}

bool DBServicePersistenceAccessor::updateLocationMapping(
        string objectID,
        ConnectionID connID,
        bool replicated)
{
    printFunction("DBServicePersistenceAccessor::updateLocationMapping");

    SecondoUtilsLocal::adjustDatabase(string("dbservice"));

    FilterConditions filterConditions =
    {
        { {AttributeType::STRING, string("ObjectID")}, objectID },
        { {AttributeType::INT, string("ConnectionID")},
                stringutils::int2str(connID) }
    };
    AttributeInfoWithValue valueToUpdate =
    {{AttributeType::BOOL, string("Replicated")},
            (replicated ? string("TRUE") : string("FALSE")) };

    return SecondoUtilsLocal::executeQuery2(
            CommandBuilder::buildUpdateCommand(
                    string("mapping_DBSP"),
                    filterConditions,
                    valueToUpdate));
}

bool DBServicePersistenceAccessor::deleteRelationInfo(
        RelationInfo& relationInfo)
{
    printFunction("DBServicePersistenceAccessor::deleteRelationInfo");
    string relationName("relations_DBSP");

  //    SecondoUtilsLocal::adjustDatabase(relationInfo.getDatabaseName());

    string relationID = relationInfo.toString();
    FilterConditions filterConditions =
    {
        { { AttributeType::STRING, string("ObjectID")}, relationID },
    };

    bool resultOk = SecondoUtilsLocal::executeQuery2(
            CommandBuilder::buildDeleteCommand(
                    relationName,
                    filterConditions));

    if(!resultOk)
    {
        print("Could not delete relation metadata");
        print("ObjectID: ", relationID);
    }

    return resultOk && deleteLocationMapping(
            relationID,
            relationInfo.nodesBegin(),
            relationInfo.nodesEnd());
}

bool DBServicePersistenceAccessor::deleteDerivateInfo(
        DerivateInfo& derivateInfo,
        RelationInfo& source)
{
    printFunction("DBServicePersistenceAccessor::deleteRelationInfo");

    if(derivateInfo.getSource() != source.toString()){
        print("relationInfo is not the source of the derivate.");
        print(derivateInfo);
        print(source);
        return false;
    }

    string relationName("derivate_DBSP");
    SecondoUtilsLocal::adjustDatabase(source.getDatabaseName());

    string objectName = derivateInfo.getName();
    string sourceName = derivateInfo.getSource();
    FilterConditions filterConditions =
    {
        { { AttributeType::STRING, string("ObjectName")}, objectName },
        { { AttributeType::STRING, string("Source")}, sourceName }

    };

    bool resultOk = SecondoUtilsLocal::executeQuery2(
            CommandBuilder::buildDeleteCommand(
                    relationName,
                    filterConditions));

    if(!resultOk)
    {
        print("Could not delete  metadata");
        print("ObjectName: ", objectName);
    }
    string objectId = derivateInfo.toString();

    return resultOk && deleteLocationMapping(
            objectId , 
            derivateInfo.nodesBegin(),
            derivateInfo.nodesEnd());
}



bool DBServicePersistenceAccessor::deleteLocationMapping(
        string objectID,
        ReplicaLocations::const_iterator nodesBegin,
        ReplicaLocations::const_iterator nodesEnd)
{
    printFunction("DBServicePersistenceAccessor::deleteLocationMapping");
    string relationName("mapping_DBSP");
    bool resultOk = true;
    for(ReplicaLocations::const_iterator it = nodesBegin;
            it != nodesEnd; it++)
    {
        FilterConditions filterConditions =
        {
            { {AttributeType::STRING, string("ObjectID") }, objectID },
            { {AttributeType::INT, string("ConnectionID") },
                    stringutils::int2str(it->first) }
        };

        resultOk = resultOk && SecondoUtilsLocal::executeQuery2(
                CommandBuilder::buildDeleteCommand(
                        relationName,
                        filterConditions));
        if(!resultOk)
        {
            print("Could not delete mapping");
            print("ObjectID: ", objectID);
            print("ConnectionID: ", it->first);
        }
    }
    return resultOk;
}

bool DBServicePersistenceAccessor::persistAllLocations(
        DBServiceLocations dbsLocations)
{
    printFunction("DBServicePersistenceAccessor::persistAllLocations");
    vector<vector<string> > values;
    if(!dbsLocations.empty())
    {
        for(const auto& location : dbsLocations)
        {
            const LocationInfo& locationInfo = location.second.first;
            vector<string> value = {
                    stringutils::int2str(location.first),
                    locationInfo.getHost(),
                    locationInfo.getPort(),
                    locationInfo.getConfig(),
                    locationInfo.getDisk(),
                    locationInfo.getCommPort(),
                    locationInfo.getTransferPort(),
            };
            values.push_back(value);
        }
    }
    return deleteAndCreate(
            string("locations_DBSP"),
            locations,
            values);
}

bool DBServicePersistenceAccessor::persistAllReplicas(
        DBServiceRelations dbsRelations,
        DBServiceDerivates dbsDerivates)
{
    printFunction("DBServicePersistenceAccessor::persistAllRelations");
    vector<vector<string> > relationValues;
    vector<vector<string> > mappingValues;
    if(!dbsRelations.empty())
    {
        for(const auto& relation : dbsRelations)
        {
            const RelationInfo& relationInfo = relation.second;
            vector<string> value = {
                relation.first,
                relationInfo.getDatabaseName(),
                relationInfo.getRelationName(),
                relationInfo.getOriginalLocation().getHost(),
                relationInfo.getOriginalLocation().getPort(),
                relationInfo.getOriginalLocation().getDisk()
            };
            relationValues.push_back(value);
            for(ReplicaLocations::const_iterator it =
                    relationInfo.nodesBegin();
                    it != relationInfo.nodesEnd(); it++)
            {
                vector<string> mapping = {
                        relation.first,
                        stringutils::int2str(it->first),
                        it->second ? string("TRUE") : string("FALSE")
                };
                mappingValues.push_back(mapping);
            }
        }
    }

    vector<vector<string> > derivateValues;

    if(!dbsDerivates.empty())
    {
        for(const auto& derivate : dbsDerivates)
        {
            const DerivateInfo& derivateInfo = derivate.second;
            vector<string> value = {
                derivateInfo.getName(),
                derivateInfo.getSource(),
                derivateInfo.getFun()
            };
            derivateValues.push_back(value);
            string id = derivateInfo.toString();
            for(ReplicaLocations::const_iterator it =
                    derivateInfo.nodesBegin();
                    it != derivateInfo.nodesEnd(); it++)
            {
                vector<string> mapping = {
                        id,
                        stringutils::int2str(it->first),
                        it->second ? string("TRUE") : string("FALSE")
                };
                mappingValues.push_back(mapping);
            }
        }
    }


    return     deleteAndCreate(
                 string("relations_DBSP"),
                 relations,
                 relationValues)
           && deleteAndCreate(
                 string("derivates_DBSP"),
                 derivates,
                 derivateValues)
           && deleteAndCreate(
                    string("mapping_DBSP"),
                    mapping,
                    mappingValues);
}


size_t getRecordCount(const string& databaseName, const string& relationName)
{
    printFunction("DBServicePersistenceAccessor::getRecordCount");
    print(databaseName);
    print(relationName);

   // seems to be wrong, waiting for answer
   // SecondoUtilsLocal::adjustDatabase(databaseName);
    return 0;
}

RelationDefinition DBServicePersistenceAccessor::locations =
{
     { AttributeType::INT, "ConnectionID" },
     { AttributeType::STRING, "Host" },
     { AttributeType::STRING, "Port" },
     { AttributeType::TEXT, "Config" },
     { AttributeType::TEXT, "Disk" },
     { AttributeType::STRING, "CommPort" },
     { AttributeType::STRING, "TransferPort" }
};

RelationDefinition DBServicePersistenceAccessor::relations =
{
    { AttributeType::STRING, "RelationID" },
    { AttributeType::STRING, "DatabaseName" },
    { AttributeType::STRING, "RelationName" },
    { AttributeType::STRING, "Host" },
    { AttributeType::STRING, "Port" },
    { AttributeType::TEXT, "Disk" }
};

RelationDefinition DBServicePersistenceAccessor::mapping =
{
    { AttributeType::STRING, "ObjectID" },
    { AttributeType::INT, "ConnectionID" },
    { AttributeType::BOOL, "Replicated" }
};


RelationDefinition DBServicePersistenceAccessor::derivates =
{
    { AttributeType::STRING, "ObjectName" },
    { AttributeType::STRING, "DependsOn" },
    { AttributeType::TEXT, "FunDef" }
};

} /* namespace DBService */

