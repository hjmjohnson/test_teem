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

#include "air.h"

/*
learned: using these functions correctly to manage even simple
memory usage can be very tricky.  

problem 0: even trying to write airMopPrint, I foolishly thought:
"print the string, then free it".  But the print callback clobbered
the free callback, because of the semantics of airMopAdd().  So,
I had to add _airMopAdd().

problem 1: debugging hest with purify, on case of hitting error after
parsing multiple variable parameter option of strings: so, I allocated
an array of strings (arrays), and registered all the strings with
airMopMem(), and registered the array itself also with airMopMem().
Again, got clobbered.  airSetNull(&((*vP)[0])) clobbered 
airFree(*vP).  So, I gave up on using airMopMem() for the individual
elements, and am using simply airMopAdd(airFree).  The alternative
was to change the airMopAdd()s in airMopMem() to _airMopAdd()s, but
I didn't feel confident that this would be safe ...

-----------  SO: as a result of all that:

airMopAdd() will no longer over-write a callback based on the pointer
It will only over-write the "when" of a (pointer,callback) pair, so that
you can't register multiple copies of a (pointer,callback) pair (regardless
of differences, if any, in "when").  Therefore, there will be AT MOST ONE
instance of a (pointer,callback) pair in a mop.

_airMopAdd() was nixed.

airMopSub() and airMopUnMem were created

*/

#define AIR_MOP_INCR 10

airArray *
airMopNew() {
  
  return airArrayNew(NULL, NULL, sizeof(airMop), AIR_MOP_INCR);
}

void
airMopAdd(airArray *arr, void *ptr, airMopper mop, int when) {
  airMop *mops;
  int i;
  
  if (!arr)
    return;

  mops = arr->data;
  /* first see if this is something we already set a callback for */
  for (i=0; i<=arr->len-1; i++) {
    if (mops[i].ptr == ptr && mops[i].mop == mop) {
      mops[i].when = when;
      /* we're done */
      return;
    }
  }
  /* this is a new ptr */
  i = airArrayIncrLen(arr, 1);
  mops = arr->data;
  mops[i].ptr = ptr;
  mops[i].mop = mop;
  mops[i].when = when;
  return;
}

void
airMopSub(airArray *arr, void *ptr, airMopper mop) {
  airMop *mops;
  int i;
  
  if (!arr)
    return;

  mops = arr->data;
  /* first see if this is something we already set a callback for */
  for (i=0; i<=arr->len-1; i++) {
    if (mops[i].ptr == ptr && mops[i].mop == mop) {
      mops[i].ptr = NULL;
      mops[i].mop = NULL;
      mops[i].when = airMopNever;
      return;
    }
  }
  /* else we've never seen this before, user is a moron */
  return;
}

void
airMopMem(airArray *arr, void *_ptrP, int when) {
  void **ptrP;

  if (!(arr && _ptrP))
    return;

  ptrP = _ptrP;
  airMopAdd(arr, ptrP, (airMopper)airSetNull, when);
  airMopAdd(arr, *ptrP, airFree, when);
  /*
  printf("airMopMem(0x%p): will free() 0x%p\n", 
         (void*)arr, (void*)(*ptrP));
  printf("airMopMem(0x%p): will set 0x%p to NULL\n", 
         (void*)arr, (void*)ptrP);
  */
  return;
}

void
airMopUnMem(airArray *arr, void *_ptrP) {
  void **ptrP;

  if (!(arr && _ptrP))
    return;

  ptrP = _ptrP;
  airMopSub(arr, ptrP, (airMopper)airSetNull);
  airMopSub(arr, *ptrP, airFree);
  return;
}

void *
_airMopPrint(void *_str) {
  char *str;

  str = _str;
  if (str) {
    printf("%s\n", str);
  }
  return NULL;
}

void
airMopPrint(airArray *arr, void *_str, int when) {
  char *copy;

  if (!(arr && _str))
    return;

  copy = airStrdup((char*)_str);
  airMopAdd(arr, copy, airFree, airMopAlways);
  airMopAdd(arr, copy, _airMopPrint, when);
  return;
}

char
_airMopWhenStr[4][128] = {
  " never",
  " error",
  "  okay",
  "always",
};

void
airMopDebug(airArray *arr) {
  airMop *mops;
  int i;

  if (!arr)
    return;

  mops = arr->data;
  printf("airMopDebug: _________________________ mop stack for 0x%p:\n",
	 (void*)arr);
  for (i=arr->len-1; i>=0; i--) {
    printf("% 4d: ", i);
    if (NULL == mops[i].mop && NULL == mops[i].ptr
	&& airMopNever == mops[i].when) {
      printf("no-op\n");
      continue;
    }
    /* else */
    printf("%s: ", _airMopWhenStr[mops[i].when]);
    if (airFree == mops[i].mop) {
      printf("airFree(0x%p)\n", (void*)(mops[i].ptr));
      continue;
    }
    if ((airMopper)airSetNull == mops[i].mop) {
      printf("airSetNull(0x%p)\n", (void*)(mops[i].ptr));
      continue;
    }
    if (_airMopPrint == mops[i].mop) {
      printf("_airMopPrint(\"%s\" == 0x%p)\n",
	     (char*)(mops[i].ptr), (void*)(mops[i].ptr));
      continue;
    }
    if (airFreeP == mops[i].mop) {
      printf("airFreeP(0x%p)\n", (void*)(mops[i].ptr));
      continue;
    }
    if ((airMopper)airFclose == mops[i].mop) {
      printf("airFclose(0x%p)\n", (void*)(mops[i].ptr));
      continue;
    }
    /* else */
    printf("0x%p(0x%p)\n", (void*)(mops[i].mop), (void*)(mops[i].ptr));
  }
  printf("airMopDebug: ^^^^^^^^^^^^^^^^^^^^^^^^^\n");
}

void
airMopDone(airArray *arr, int error) {
  airMop *mops;
  int i;

  /*
  printf("airMopDone(%p): hello, %s\n", (void*)arr, error ? "error" : "okay");
  */
  if (!arr)
    return;

  mops = arr->data;
  for (i=arr->len-1; i>=0; i--) {
    if (mops[i].ptr
	&& (airMopAlways == mops[i].when
	    || (airMopOnError == mops[i].when && error)
	    || (airMopOnOkay == mops[i].when && !error))) {
      mops[i].mop(mops[i].ptr);
    }
  }
  airArrayNuke(arr);
  /*
  printf("airMopDone(%p): done!\n", (void*)arr);
  */
  return;
}

void
airMopError(airArray *arr) {
  
  airMopDone(arr, AIR_TRUE);
}

void
airMopOkay(airArray *arr) {
  
  airMopDone(arr, AIR_FALSE);
}

