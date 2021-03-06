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

#ifndef BIFF_HAS_BEEN_INCLUDED
#define BIFF_HAS_BEEN_INCLUDED

/* ---- BEGIN non-NrrdIO */

#include <stdio.h>
#include <string.h>

#include <teem/air.h>

/* ---- END non-NrrdIO */


#ifdef __cplusplus
extern "C" {
#endif

#define BIFF_MAXKEYLEN 128  /* maximum allowed key length (not counting 
                               the null termination) */

TEEM_API void biffSet(const char *key, const char *err);
TEEM_API void biffAdd(const char *key, const char *err);
TEEM_API void biffMaybeAdd(const char *key, const char *err, int useBiff);
TEEM_API int biffCheck(const char *key);
TEEM_API void biffDone(const char *key);
TEEM_API void biffMove(const char *destKey, const char *err,
                       const char *srcKey);
TEEM_API char *biffGet(const char *key);
TEEM_API int biffGetStrlen(const char *key);
TEEM_API void biffSetStr(char *str, const char *key);
TEEM_API char *biffGetDone(const char *key);
TEEM_API void biffSetStrDone(char *str, const char *key);

#ifdef __cplusplus
}
#endif

#endif /* BIFF_HAS_BEEN_INCLUDED */
