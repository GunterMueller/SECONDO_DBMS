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

1.1 Aconsume2

The operator "aconsume2"[2] ~consumes~ a stream of tuples -- maybe containing
further attribute relations -- and builds an attribute relation of type
"arel2"[2] from it. To distinguish it from "aconsume"[2] of NestedRelation
algebra it is postfixed.

*/

#ifndef ALGEBRAS_NESTEDRELATION2_OPERATORS_CONSUME_H_
#define ALGEBRAS_NESTEDRELATION2_OPERATORS_CONSUME_H_

#include "Stream.h"

namespace nr2a {

class Aconsume
{
  public:
    struct Info : OperatorInfo
    {

        Info()
        {
          name = "aconsume2";
          signature =  "stream(tuple(X)) -> "
              + ARel::BasicType() + "(tuple(X))";
          syntax = "_ aconsume2";
          meaning = "Collects tuples from a stream.";
          example = "query Documents feed aconsume2";
        }
    };
    virtual ~Aconsume();

    static ListExpr MapType(ListExpr args);
    static ValueMapping functions[];
    static int SelectFunction(ListExpr args);
    static int AconsumeValue(Word* args, Word& result, int message,
        Word& local, Supplier s);

    static CreateCostEstimation costEstimators[];

  protected:

  private:
    Aconsume();
    // Declared, but not defined => Linker error on usage

};

} /* namespace nr2a*/

#endif /* ALGEBRAS_NESTEDRELATION2_OPERATORS_CONSUME_H_*/
