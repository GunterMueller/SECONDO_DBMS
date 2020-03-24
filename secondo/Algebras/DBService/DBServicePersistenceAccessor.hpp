/*

1.1 ~DBServicePersistenceAccessor~

The \textit{DBServicePersistenceAccessor} is the interface towards the
\textit{DBService's} persistant storage in SECONDO relations. It provides
functions to create relations as well as inserting, updating and deleting
tuples.

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
#ifndef ALGEBRAS_DBSERVICE_DBSERVICEPERSISTENCEACCESSOR_HPP_
#define ALGEBRAS_DBSERVICE_DBSERVICEPERSISTENCEACCESSOR_HPP_

#include <map>
#include <queue>
#include <vector>

#include "NestedList.h"

#include "Algebras/DBService/CommandBuilder.hpp"
#include "Algebras/DBService/DBServiceManager.hpp"
#include "Algebras/DBService/LocationInfo.hpp"
#include "Algebras/DBService/RelationInfo.hpp"
#include "Algebras/DBService/DerivateInfo.hpp"

namespace DBService {

/*

1.1.1 Class Definition

*/

class DBServicePersistenceAccessor {

/*

1.1.1.1 \textit{persistLocationInfo}

This function persists the provided \textit{ConnectionID} and the corresponding
\textit{LocationInfo} object into the respective SECONDO relation.

*/
public:
    static bool persistLocationInfo(
            ConnectionID connID, LocationInfo& locationInfo);

/*

1.1.1.1 \textit{persistRelationInfo}

This function persists the provided \textit{RelationInfo} object into the
respective SECONDO relation.

*/
    static bool persistRelationInfo(
            RelationInfo& relationInfo);



/*
1.1.1.1 ~persistDerivateInfo~

This function persists the provide ~DerivateInfo~ object into the
respective SECONDO relation.

*/
   static bool persistDerivateInfo(
              DerivateInfo& derivateInfo);


/*

1.1.1.1 \textit{restoreLocationInfo}

This function restores the persisted \textit{ConnectionIDs} and their
corresponding \textit{LocationInfo} objects from the respective SECONDO
relation.

*/
    static bool restoreLocationInfo(
            std::map<ConnectionID, LocationInfo>& locations);

/*

1.1.1.1 \textit{restoreRelationInfo}

This function restores the persisted \textit{RelationInfo} objects from the
respective SECONDO relation.

*/
    static bool restoreRelationInfo(
            DBServiceRelations& relations);

/*
1.1.1.1 ~restoreDerivateInfo~

*/
    static bool restoreDerivateInfo(
              DBServiceDerivates& derivates);


/*

1.1.1.1 \textit{restoreLocationMapping}

This function restores the mapping of relation to location from the respective
SECONDO relation.

*/
    static bool restoreLocationMapping(
            std::queue<
            std::pair<std::string, std::pair<ConnectionID, bool> > >& mapping);

/*

1.1.1.1 \textit{updateLocationMapping}

This function updates the replication status flag of the mapping of relation to
location in the respective SECONDO relation.

*/
    static bool updateLocationMapping(
            std::string relationID,
            ConnectionID connID,
            bool replicated);

/*

1.1.1.1 \textit{deleteRelationInfo}

This function deletes a persisted \textit{RelationInfo} object and also triggers
the deletion of the corresponding location mapping.

*/
    static bool deleteRelationInfo(RelationInfo& relationInfo);


/*
1.1.1 ~deleteDerivateInfo~

This function deletes a persisted ~DerivateInfo~ object and also triggers 
the deletion of the corresponding location mapping. Note that the 
relation from that the derivateInfo depends on must be available.

*/
   static bool deleteDerivateInfo(DerivateInfo& derivateInfo,
                                  RelationInfo& source);


/*

1.1.1.1 ~persistAllLocations~

This function persists all locations passed as argument at once. All data that
was persisted prior to the call of this function will be lost.

*/
    static bool persistAllLocations(DBServiceLocations dbsLocations);

/*

1.1.1.1 ~persistAllRelations~

This function persists all relations passed as argument at once. All data that
was persisted prior to the call of this function will be lost.

*/
    static bool persistAllReplicas(DBServiceRelations dbsRelations,
                                   DBServiceDerivates dbsDerivates);



/*

1.1.1.1 ~getRecordCount~

Retrieve the number of tuples that the specifies relation contains.

*/
    size_t getRecordCount(
            const std::string& databaseName,
            const std::string& relationName);

/*

1.1.1.1 \textit{deleteLocationMapping}

This function deletes the mapping of relation to location for a specified
relation identifier and a given range of connections.

*/
    static bool deleteLocationMapping(std::string relationID,
            ReplicaLocations::const_iterator nodesBegin,
            ReplicaLocations::const_iterator nodesEnd);
private:

/*

1.1.1.1 \textit{persistLocationMapping}

This function persists the mapping of a relation to its replica locations.

*/
    static bool persistLocationMapping(
            std::string relationID,
            ReplicaLocations::const_iterator nodesBegin,
            ReplicaLocations::const_iterator nodesEnd);

/*

1.1.1.1 \textit{createOrInsert}

This function creates a relation or inserts the specified tuple if the relation
already exists.

*/
    static bool createOrInsert(
            const std::string& relationName,
            const RelationDefinition& rel,
            const std::vector<std::vector<std::string> >& values);

/*

1.1.1.1 ~deleteAndCreate~

This function deletes a relation in case it exists and recreates it using the
given values.

*/
    static bool deleteAndCreate(
            const std::string& relationName,
            const RelationDefinition& rel,
            const std::vector<std::vector<std::string> >& values);


/*

1.1.1.1 \textit{locations}

This member specifies the attributes of the SECONDO relation that is used for
persisting the \textit{DBService} worker node locations.

*/
    static RelationDefinition locations;

/*

1.1.1.1 \textit{relations}

This member specifies the attributes of the SECONDO relation that is used for
persisting \textit{RelationInfo} objects.

*/
    static RelationDefinition relations;

/*

1.1.1.1 \textit{mapping}

This member specifies the attributes of the SECONDO relation that is used for
persisting the mapping of relations to locations.

*/
    static RelationDefinition mapping;

/*
1.1.1.1 ~derivate~

This member specifies the attributes of the SECONDO relation that is 
used for persisting the derivates of a relation.

*/
    static RelationDefinition derivates;

};

} /* namespace DBService */

#endif /* ALGEBRAS_DBSERVICE_DBSERVICEPERSISTENCEACCESSOR_HPP_ */
