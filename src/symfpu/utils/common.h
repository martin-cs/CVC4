/*
** Copyright (C) 2016 Martin Brain
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
** common.h
**
** Martin Brain
** martin.brain@cs.ox.ac.uk
** 05/08/14
**
** Commonly used utility functions.
**
*/

#include <assert.h>
#include <stdint.h>

#include "symfpu/utils/properties.h"

#ifndef SYMFPU_COMMON
#define SYMFPU_COMMON

namespace symfpu {
  uint64_t previousPowerOfTwo (uint64_t x);

  // The number of bits required to represet a number
  //  == the position of the leading 0 + 1
  //  == ceil(log_2(value + 1))
  template <class T>
  T bitsToRepresent (const T value) {
    T i = 0; 
    //unsigned T working = *((unsigned T)&value);   // Implementation defined for signed types
    T working = value;

    while (working != 0) {
      ++i;
      working >>= 1;
    }

    return i;
  }

  template <class T>
  T positionOfLeadingOne (const T value) {
    //PRECONDITION(value != 0);
    assert(value != 0);

    T i = -1;
    //unsigned T working = *((unsigned T)&value);   // Implementation defined for signed types
    T working = value;

    while (working != 0) {
      ++i;
      working >>= 1;
    }

    return i;
  }
}

#endif
