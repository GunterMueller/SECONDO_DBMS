#This file is part of SECONDO.

#Copyright (C) 2004, University in Hagen, Department of Computer Science, 
#Database Systems for New Applications.

#SECONDO is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#(at your option) any later version.

#SECONDO is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.

#You should have received a copy of the GNU General Public License
#along with SECONDO; if not, write to the Free Software
#Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


.PHONY: all
all: NMEA2Secondo.class NMEA2MReal.class NMEA_SpeedExtractor.class MyStringTokenizer.class


NMEA2Secondo.class: NMEA2Secondo.java
	javac NMEA2Secondo.java


NMEA2MReal.class: NMEA2MReal.java
	javac NMEA2MReal.java

NMEA_SpeedExtractor.class: NMEA_SpeedExtractor.java
	javac NMEA_SpeedExtractor.java

MyStringTokenizer.class: MyStringTokenizer.java
	javac MyStringTokenizer.java

.PHONY: clean
clean:
	rm -r *.class

