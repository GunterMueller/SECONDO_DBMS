/*
//paragraph [1] Title: [{\Large \bf] [}]
//[ue] [\"{u}]
//[us] [\_]
//[_] [\_]


[1] The DBServiceAlgebra:\newline{Fault-Tolerant Query Execution in SECONDO}

\tableofcontents
\newpage

1 Core Components

1.1 \textit{DBServiceAlgebra}

The \textit{DBServiceAlgebra} registers the \textit{DBService} functionality
with the \textit{AlgebraManager}.

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
#ifndef ALGEBRAS_DBSERVICE_DBSERVICEALGEBRA_HPP_
#define ALGEBRAS_DBSERVICE_DBSERVICEALGEBRA_HPP_

#include "Algebra.h"
#include "NList.h"
#include "QueryProcessor.h"

extern NestedList* nl;
extern QueryProcessor* qp;

namespace DBService
{

/*

1.1.1 Class Definition

*/

class DBServiceAlgebra: public Algebra
{
public:

/*

1.1.1.1 Constructor

In the constructor, the implemented operators are registered at the
~DBServiceAlgebra~ so that they can be recognized by the ~QueryProcessor~.

*/
    DBServiceAlgebra();
};

} /* namespace DBService */

/*

1.1.1 Algebra Initialization

This function initializes the \textit{DBServiceAlgebra} so that it can expose
its functionality to the SECONDO system.

*/

extern "C" Algebra*
InitializeDBServiceAlgebra(NestedList* nlRef, QueryProcessor* qpRef)
{
    return new DBService::DBServiceAlgebra;
}

#endif /* ALGEBRAS_DBSERVICE_DBSERVICEALGEBRA_HPP_ */
