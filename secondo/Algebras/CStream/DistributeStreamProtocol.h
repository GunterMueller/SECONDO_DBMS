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

//paragraph [1] Title: [{\Large \bf \begin{center}] [\end{center}}]
//characters [1] Type: [] []
//characters [2] Type: [] []
//[ae] [\"{a}]
//[oe] [\"{o}]
//[ue] [\"{u}]
//[ss] [{\ss}]
//[Ae] [\"{A}]
//[Oe] [\"{O}]
//[Ue] [\"{U}]
//[x] [$\times $]
//[->] [$\rightarrow $]
//[toc] [\tableofcontents]

[1] Implementation of datatype DistributeStreamProtocol and operators.

[toc]

1 DistributeStreamProtocol class implementation
see DistributeStreamProtocol.cpp for details.

*/
#ifndef _DISTRIBUTE_STREAM_PROTOCOL_H_
#define _DISTRIBUTE_STREAM_PROTOCOL_H_

#include <string>
#include "ListUtils.h"

namespace cstream {

class DistributeStreamProtocol {
public:
    static std::string requestStream();
    static std::string confirmStream();
    static std::string requestSupportedTypes();
    static std::string sendSupportedTypes(bool binary);
    static std::string confirmStreamOK();
    static std::string tupleMessage();
    static bool requestStream(std::string request, std::string& tupledesc,
            ListExpr& funList, bool& format);
    static std::string confirmStream(std::string confirmMsg);
    static std::string tupleMessage(std::string tupleMsg);
    static std::string streamDone();
};

} /* namespace cstream */

#endif /* _DISTRIBUTE_STREAM_PROTOCOL_H_ */
