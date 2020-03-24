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

*/

#include <string>
#include "NestedList.h"
#include "ListUtils.h"
#include "StandardTypes.h"
#include "QueryProcessor.h"
#include "drelport.h"

extern NestedList* nl;
extern QueryProcessor* qp;


namespace drel {

ListExpr setDRelPortTM(ListExpr args){
  if(!nl->HasLength(args,1)){
    return listutils::typeError("one element expected");
  }
  if(!CcInt::checkType(nl->First(args))){
    return listutils::typeError("int expected");
  }
  return listutils::basicSymbol<CcBool>();
}


int setDRelPortVM( Word* args, Word& result, int message, 
                   Word& local, Supplier s ) {
  result = qp->ResultStorage(s);
  CcBool* res = (CcBool*) result.addr;
  CcInt* arg = (CcInt*) args[0].addr;
  if(!arg->IsDefined()){
    res->SetDefined(false);
    return 0;
  }
  res->Set(true,setDRelPort(arg->GetValue()));
  return 0;
}

OperatorSpec setDRelPortSpec(
  "int -> bool",
  " setDRelPort(_) ",
  " Changes the port number that is used for internal queries.",
  " query setDRelPort(1239)"
);

Operator setDRelPortOp(
   "setDRelPort",
   setDRelPortSpec.getStr(),
   setDRelPortVM,
   Operator::SimpleSelect,
   setDRelPortTM
);


ListExpr getDRelPortTM(ListExpr args){
  if(!nl->IsEmpty(args)){
    return listutils::typeError("no arguments expected");
  }
  return listutils::basicSymbol<CcInt>();
}


int getDRelPortVM( Word* args, Word& result, int message, 
                   Word& local, Supplier s ) {
  result = qp->ResultStorage(s);
  CcInt* res = (CcInt*) result.addr;
  res->Set(true,getDRelPort());
  return 0;
}

OperatorSpec getDRelPortSpec(
  " -> int",
  " getDRelPort() ",
  " Returns the port number that is used for internal queries.",
  " query getDRelPort()"
);

Operator getDRelPortOp(
   "getDRelPort",
   getDRelPortSpec.getStr(),
   getDRelPortVM,
   Operator::SimpleSelect,
   getDRelPortTM
);






}

