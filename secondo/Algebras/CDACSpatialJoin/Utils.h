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


1 Utils class

*/

#pragma once

#include <string>
#include <ctime>

#include "Base.h"

namespace cdacspatialjoin {
   /* Returns a formatted string representation of the given num, inserting
   thousands separators. */
   std::string formatInt(long num);

   /* returns a string representation of the given duration; the unit "ms" is
    * included in the string */
   std::string formatMillis(clock_t duration);

   /* returns the separator char used for paths on the current operating
    * system */
   inline char getPathSeparator();

   /* returns the string combined from the given directory (path1) and the
    * given subdirectory or file (path2) */
   std::string pathCombine(const std::string& path1, const std::string& path2);

   /* returns true if a directory with the given path exists in the file
    * system */
   bool directoryExists(const std::string& dir);

}
