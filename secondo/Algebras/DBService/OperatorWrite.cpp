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

#include "Algebras/Distributed2/DArray.h"
#include "Algebras/Distributed2/FileRelations.h"

#include "Algebras/Relation-C++/OperatorConsume.h"

#include "Algebras/DBService/DBServiceClient.hpp"
#include "Algebras/DBService/DebugOutput.hpp"
#include "Algebras/DBService/OperatorWrite.hpp"
#include "Algebras/DBService/ReplicationUtils.hpp"


using namespace std;
using namespace distributed2;

namespace DBService
{

ListExpr OperatorWrite::mapType(ListExpr nestedList)
{
    printFunction("OperatorWrite::mapType");
    print("nestedList", nestedList);

    if (nl->ListLength(nestedList) != 3)
    {
        ErrorReporter::ReportError(
                "expected signature: stream(tuple(...)) x string x bool");
        return nl->TypeError();
    }

    if (!Stream<Tuple>::checkType(nl->First(nl->First(nestedList))))
    {
        ErrorReporter::ReportError(
                "first argument must be: stream(tuple(...))");
        return nl->TypeError();
    }

    if(!CcString::checkType(nl->First(nl->Second(nestedList))))
    {
        ErrorReporter::ReportError(
                "second argument must be: string");
        return nl->TypeError();
    }

    if(!CcBool::checkType(nl->First(nl->Third(nestedList))))
    {
        ErrorReporter::ReportError(
                "third argument must be: bool");
        return nl->TypeError();
    }

    ListExpr consumeTypeMapInput = nl->OneElemList(
            nl->First(nl->First(nestedList)));
    print("consumeTypeMapInput", consumeTypeMapInput);

    ListExpr consumeTypeMapResult = OperatorConsume::ConsumeTypeMap<false>(
            consumeTypeMapInput);

    print("consumeTypeMapResult", consumeTypeMapResult);

    
    return consumeTypeMapResult;
}

int OperatorWrite::mapValue(
        Word* args,
        Word& result,
        int message,
        Word& local,
        Supplier s)
{

    printFunction("OperatorWrite::mapValue");

    ListExpr tupleType = nl->Second(qp->GetType(s));
    print("tupleType", tupleType);

    ListExpr relType = nl->TwoElemList(
            listutils::basicSymbol<Relation>(),
            tupleType);
    print("relType", relType);

    const string databaseName =
            SecondoSystem::GetInstance()->GetDatabaseName();
    string relationName = static_cast<CcString*>(args[1].addr)->GetValue();
    print("relationName", relationName);

    const string fileName =
            ReplicationUtils::getFileName(databaseName, relationName);

    bool async = static_cast<CcBool*>(args[2].addr)->GetValue();

    int consumeResult=0;
    if(async)
    {
        consumeResult =
                OperatorConsume::Consume(
                        args, result, message, local, s);
    }else
    {
        GenericRelation* rel = (GenericRelation*)((qp->ResultStorage(s)).addr);
        if(rel->GetNoTuples() > 0)
        {
            rel->Clear();
        }
        BinRelWriter brw(fileName,relType,0);
        Stream<Tuple> stream(args[0]);
        stream.open();

        Tuple* t;
        while ((t = stream.request()))
        {
            rel->AppendTuple(t);
            brw.writeNextTuple(t);
            t->DeleteIfAllowed();
        }
        stream.close();
        result.setAddr(rel);
    }

    DBServiceClient* dbsc = DBServiceClient::getInstance();

    if(!dbsc){
      print("DBS client could not be startet, check configuration");
    } else {
      if(!dbsc->triggerReplication(
            databaseName,
            relationName,
            relType,
            async))
      {
         print("Replication could not be triggered");
         return 1;
      }
    }

    return consumeResult;
}

} /* namespace DBService */
