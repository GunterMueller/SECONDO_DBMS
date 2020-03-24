//This file is part of SECONDO.

//Copyright (C) 2014, University in Hagen, Department of Computer Science,
//Database Systems for New Applications.

//SECONDO is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.

//SECONDO is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with SECONDO; if not, write to the Free Software
//Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

package stlib.interfaces.intime;

import stlib.interfaces.GeneralTypeIF;
import stlib.interfaces.time.TimeInstantIF;

/**
 * Interface that should be provided by objects of type Intime
 * 
 * Stores a value of type {@code <E>} and an 'TimeInstantIF' object
 * 
 * @param <E>
 *           - type of the value of a 'IntimeIF' object
 * 
 * @author Markus Fuessel
 */
public interface IntimeIF<E extends GeneralTypeIF> extends GeneralTypeIF, Comparable<IntimeIF<E>> {

   /**
    * Get the 'TimeInstant'
    * 
    * @return the instant
    */
   TimeInstantIF getInstant();

   /**
    * Get the value of type {@code <E>}
    * 
    * @return the value
    */
   E getValue();

}