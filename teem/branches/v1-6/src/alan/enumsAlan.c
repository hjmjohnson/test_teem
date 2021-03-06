/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "alan.h"

char
_alanStopStr[ALAN_STOP_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_stop)",
  "not",
  "iter",
  "nonexist",
  "converged",
  "diverged"
};

char
_alanStopDesc[ALAN_STOP_MAX+1][AIR_STRLEN_MED] = {
  "unknown_stop",
  "there is no reason to stop",
  "hit the maximum number of iterations",
  "got non-existant values",
  "simulation converged",
  "simulation hit divergent instability"
};

airEnum
_alanStop = {
  "stop",
  ALAN_STOP_MAX,
  _alanStopStr,  NULL,
  _alanStopDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
alanStop = &_alanStop;
