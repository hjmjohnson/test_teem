/*
  Teem: Gordon Kindlmann's research software
  Copyright (C) 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

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

#include "mite.h"
#include "privateMite.h"

char
_miteValStr[][AIR_STRLEN_SMALL] = {
  "(unknown miteVal)",
  "Xw",
  "Xi",
  "Yw",
  "Yi",
  "Zw",
  "Zi",
  "Tw",
  "Ti",
  "V",
  "N",
  "NdotV",
  "NdotL",
  "VrefN",
  "GTdotV",
  "VdefT",
  "VdefTdotV",
  "WdotD"
};

int
_miteValVal[] = {
  miteValUnknown,
  miteValXw,
  miteValXi,
  miteValYw,
  miteValYi,
  miteValZw,
  miteValZi,
  miteValTw,
  miteValTi,
  miteValView,
  miteValNormal,
  miteValNdotV,
  miteValNdotL,
  miteValVrefN,
  miteValGTdotV,
  miteValVdefT,
  miteValVdefTdotV,
  miteValWdotD,
};

char
_miteValStrEqv[][AIR_STRLEN_SMALL] = {
  "xw",
  "xi",
  "yw",
  "yi",
  "zw",
  "zi",
  "tw",
  "ti",
  "view", "v",
  "normal", "n", 
  "ndotv", "vdotn",
  "ndotl", "ldotn",
  "vrefn",
  "gtdotv",
  "vdeft",
  "vdeftdotv",
  "wdotd",
  ""
};

int
_miteValValEqv[] = {
  miteValXw,
  miteValXi,
  miteValYw,
  miteValYi,
  miteValZw,
  miteValZi,
  miteValTw,
  miteValTi,
  miteValView, miteValView, 
  miteValNormal, miteValNormal,
  miteValNdotV, miteValNdotV,
  miteValNdotL, miteValNdotL,
  miteValVrefN,
  miteValGTdotV,
  miteValVdefT,
  miteValVdefTdotV,
  miteValWdotD
};

airEnum
_miteVal = {
  "miteVal",
  MITE_VAL_ITEM_MAX+1,
  _miteValStr, _miteValVal,
  NULL,
  _miteValStrEqv, _miteValValEqv,
  AIR_FALSE
};
airEnum *
miteVal = &_miteVal;

/*
** again, this is not a true gageKind- mainly because these items may
** depend on items in different gageKinds (scalar and vector).  So,
** the prerequisites here are all blank.  Go look in miteQueryAdd()
** to see these items' true prereqs
*/
gageItemEntry
_miteValTable[MITE_VAL_ITEM_MAX+1] = {
  /* enum value        len,deriv,  prereqs,                                       parent item, index*/
  {miteValXw,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValXi,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValYw,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValYi,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValZw,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValZi,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValTw,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValTi,            1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValView,          3,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValNormal,        3,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValNdotV,         1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValNdotL,         1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValVrefN,         3,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValGTdotV,        1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValVdefT,         3,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValVdefTdotV,     1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1},
  {miteValWdotD,         1,  0,  {-1, -1, -1, -1, -1},                                    -1,  -1}
};


gageKind
_miteValGageKind = {
  "mite",
  &_miteVal,
  -1,
  -1,
  MITE_VAL_ITEM_MAX,
  _miteValTable,
  NULL,
  NULL,
  NULL
};

gageKind *
miteValGageKind = &_miteValGageKind;
