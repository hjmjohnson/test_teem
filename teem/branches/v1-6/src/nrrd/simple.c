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

#include "nrrd.h"
#include "privateNrrd.h"

#include <teem32bit.h>
#include <limits.h>

const char *
nrrdBiffKey = "nrrd";

/*
******** nrrdCNPP
**
** As far as I can tell, a function like this is unfortunately
** necessary to pass a <Nrrd**> to a function expecting a 
** <const Nrrd**> (a "CNPP").  This function just creates a new
** array of the right type and copies all the pointers into it.  
*/
const Nrrd **
nrrdCNPP(Nrrd **nin, int N) {
  const Nrrd **ret;
  int i;

  ret = (const Nrrd **)calloc(N, sizeof(Nrrd *));
  for (i=0; i<N; i++) {
    ret[i] = nin[i];
  }
  return ret;
}

/*
******** nrrdPeripheralInit
**
** resets peripheral information
*/
int
nrrdPeripheralInit (Nrrd *nrrd) {

  if (!nrrd) 
    return 1;

  /* nrrd->min = nrrd->max = AIR_NAN; */
  nrrd->oldMin = nrrd->oldMax = AIR_NAN;
  /* nrrd->hasNonExist = nrrdNonExistUnknown; */
  return 0;
}

/*
******** nrrdPeripheralCopy
**
** copies peripheral information
*/
int
nrrdPeripheralCopy (Nrrd *nout, const Nrrd *nin) {

  if (!( nout && nin ))
    return 1;

  /* HEY: who copies the content? */
  /*
  nout->min = nin->min;
  nout->max = nin->max;
  */
  nout->oldMin = nin->oldMin;
  nout->oldMax = nin->oldMax;
  /* nout->hasNonExist = nin->hasNonExist; */
  return 0;
}

/*
** _nrrdContentGet
**
** ALLOCATES a string for the content of a given nrrd
** panics and exits if allocation failed
*/
char *
_nrrdContentGet(const Nrrd *nin) {
  char me[]="_nrrdContentGet";
  char *ret;
  
  ret = ((nin && nin->content) ? 
	 airStrdup(nin->content) : 
	 airStrdup(nrrdStateUnknownContent));
  if (!ret) {
    fprintf(stderr, "%s: PANIC: content strdup failed!\n", me);
    exit(1);
  }
  return ret;
}

int
_nrrdContentSet_nva (Nrrd *nout, const char *func,
		     char *content, const char *format, va_list arg) {
  char me[]="_nrrdContentSet_nva", err[AIR_STRLEN_MED],
    *buff;

  buff = malloc(128*AIR_STRLEN_HUGE);
  if (!buff) {
    sprintf(err, "%s: couln't alloc buffer!", me);
    biffAdd(NRRD, err); return 1;
  }
  AIR_FREE(nout->content);

  /* we are currently praying that this won't overflow the "buff" array */
  /* HEY: replace with vsnprintf or whatever when its available */
  vsprintf(buff, format, arg);

  nout->content = calloc(strlen("(,)")
			 + airStrlen(func)
			 + 1                      /* '(' */
			 + airStrlen(content)
			 + 1                      /* ',' */
			 + airStrlen(buff)
			 + 1                      /* ')' */
			 + 1, sizeof(char));      /* '\0' */
  if (!nout->content) {
    sprintf(err, "%s: couln't alloc output content!", me);
    biffAdd(NRRD, err); AIR_FREE(buff); return 1;
  }
  sprintf(nout->content, "%s(%s%s%s)", func, content,
	  airStrlen(buff) ? "," : "", buff);
  AIR_FREE(buff);
  return 0;
}

int
_nrrdContentSet (Nrrd *nout, const char *func,
		 char *content, const char *format, ...) {
  char me[]="_nrrdContentSet", err[AIR_STRLEN_MED];
  va_list ap;
  
  va_start(ap, format);
  if (_nrrdContentSet_nva(nout, func, content, format, ap)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(content); return 1;
  }
  va_end(ap);

  /* free(content);  */
  return 0;
}

/*
******** nrrdContentSet
**
** Kind of like sprintf, but for the content string of the nrrd.
**
** Whether or not we write a new content for an old nrrd ("nin") with
** NULL content is decided here, according to
** nrrdStateAlwaysSetContent.
**
** Does the string allocation and some attempts at error detection.
** Does allow nout==nin, which requires some care.
*/
int
nrrdContentSet (Nrrd *nout, const char *func,
		const Nrrd *nin, const char *format, ...) {
  char me[]="nrrdContentSet", err[AIR_STRLEN_MED];
  va_list ap;
  char *content;
  
  if (!(nout && func && nin && format)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdStateDisableContent) {
    /* we kill content always */
    AIR_FREE(nout->content);
    return 0;
  }
  if (!nin->content && !nrrdStateAlwaysSetContent) {
    /* there's no input content, and we're not supposed to invent any
       content, so after freeing nout's content we're done */
    AIR_FREE(nout->content);
    return 0;
  }
  /* we copy the input nrrd content first, before blowing away the
     output content, in case nout == nin */
  content = _nrrdContentGet(nin);
  va_start(ap, format);
  if (_nrrdContentSet_nva(nout, func, content, format, ap)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); va_end(ap); free(content); return 1;
  }
  va_end(ap);
  free(content); 

  return 0;
}

/*
******** nrrdDescribe
** 
** writes verbose description of nrrd to given file
*/
void
nrrdDescribe (FILE *file, const Nrrd *nrrd) {
  int i;

  if (file && nrrd) {
    fprintf(file, "Nrrd at 0x%p:\n", (void*)nrrd);
    fprintf(file, "Data at 0x%p is " _AIR_SIZE_T_FMT
	    " elements of type %s.\n",
	    nrrd->data, nrrdElementNumber(nrrd), 
	    airEnumStr(nrrdType, nrrd->type));
    if (nrrdTypeBlock == nrrd->type) 
      fprintf(file, "The blocks have size %d\n", nrrd->blockSize);
    if (airStrlen(nrrd->content))
      fprintf(file, "Content = \"%s\"\n", nrrd->content);
    fprintf(file, "%d-dimensional array, with axes:\n", nrrd->dim);
    for (i=0; i<nrrd->dim; i++) {
      if (airStrlen(nrrd->axis[i].label))
	fprintf(file, "%d: (\"%s\") ", i, nrrd->axis[i].label);
      else
	fprintf(file, "%d: ", i);
      fprintf(file, "%s-centered, size=%d, ",
	      airEnumStr(nrrdCenter, nrrd->axis[i].center),
	      nrrd->axis[i].size);
      airSinglePrintf(file, NULL, "spacing=%lg, \n", nrrd->axis[i].spacing);
      airSinglePrintf(file, NULL, "    axis(Min,Max) = (%lg,",
		       nrrd->axis[i].min);
      airSinglePrintf(file, NULL, "%lg)\n", nrrd->axis[i].max);
    }
    /*
    airSinglePrintf(file, NULL, "The min, max values are %lg",
		     nrrd->min);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->max);
    */
    airSinglePrintf(file, NULL, "The old min, old max values are %lg",
		     nrrd->oldMin);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->oldMax);
    /* fprintf(file, "hasNonExist = %d\n", nrrd->hasNonExist); */
    if (nrrd->cmtArr->len) {
      fprintf(file, "Comments:\n");
      for (i=0; i<nrrd->cmtArr->len; i++) {
	fprintf(file, "%s\n", nrrd->cmt[i]);
      }
    }
    fprintf(file, "\n");
  }
}

/*
******** nrrdCheck()
**
** does some consistency checks for things that can go wrong in a nrrd
*/
int
nrrdCheck (const Nrrd *nrrd) {
  char me[] = "nrrdCheck", err[AIR_STRLEN_MED];
  int size[NRRD_DIM_MAX], i, ret;
  double val[NRRD_DIM_MAX];

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(nrrd->data)) {
    sprintf(err, "%s: nrrd has NULL data pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdType, nrrd->type)) {
    sprintf(err, "%s: type (%d) of array is invalid", me, nrrd->type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nrrd->type && (!(0 < nrrd->blockSize)) ) {
    sprintf(err, "%s: nrrd type is %s but nrrd->blockSize (%d) invalid", me,
	    airEnumStr(nrrdType, nrrdTypeBlock),
	    nrrd->blockSize);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %d is outside valid range [1,%d]",
	    me, nrrd->dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSize, size);
  if (_nrrdSizeCheck(nrrd->dim, size, AIR_TRUE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  /* these checks basically cut/paste from validity checks in 
     _nrrdReadNrrdParse_* */
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSpacing, val);
  for (i=0; i<=nrrd->dim-1; i++) {
    if (!( !airIsInf_d(val[i]) && (airIsNaN(val[i]) || (0 != val[i])) )) {
      sprintf(err, "%s: spacing %d (%g) invalid", me, i, val[i]);
      biffAdd(NRRD, err); return 1;
    }
  }
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoMin, val);
  for (i=0; i<=nrrd->dim-1; i++) {
    if ((ret=airIsInf_d(val[i]))) {
      sprintf(err, "%s: axis min %d %sinf invalid", me, i, 1==ret ? "+" : "-");
      biffAdd(NRRD, err); return 1;
    }
  }
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoMax, val);
  for (i=0; i<=nrrd->dim-1; i++) {
    if ((ret=airIsInf_d(val[i]))) {
      sprintf(err, "%s: axis ax %d %sinf invalid", me, i, 1==ret ? "+" : "-");
      biffAdd(NRRD, err); return 1;
    }
  }
  if ((ret=airIsInf_d(nrrd->oldMin))) {
    sprintf(err, "%s: old min %sinf invalid", me, 1==ret ? "+" : "-");
    biffAdd(NRRD, err); return 1;
  }
  if ((ret=airIsInf_d(nrrd->oldMax))) {
    sprintf(err, "%s: old max %sinf invalid", me, 1==ret ? "+" : "-");
    biffAdd(NRRD, err); return 1;
  }
  /*
  if ((ret=airIsInf_d(nrrd->min))) {
    sprintf(err, "%s: min %sinf invalid", me, 1==ret ? "+" : "-");
    biffAdd(NRRD, err); return 1;
  }
  if ((ret=airIsInf_d(nrrd->max))) {
    sprintf(err, "%s: max %sinf invalid", me, 1==ret ? "+" : "-");
    biffAdd(NRRD, err); return 1;
  }
  */
  return 0;
}

/*
******** nrrdSameSize()
**
** returns 1 iff given two nrrds have same dimension and axes sizes.
** This does NOT look at the type of the elements.
*/
int
nrrdSameSize (const Nrrd *n1, const Nrrd *n2, int useBiff) {
  char me[]="nrrdSameSize", err[AIR_STRLEN_MED];
  int i;

  if (!(n1 && n2)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  if (n1->dim != n2->dim) {
    sprintf(err, "%s: n1->dim (%d) != n2->dim (%d)", me, n1->dim, n2->dim);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  for (i=0; i<n1->dim; i++) {
    if (n1->axis[i].size != n2->axis[i].size) {
      sprintf(err, "%s: n1->axis[%d].size (%d) != n2->axis[%d].size (%d)", 
	      me, i, n1->axis[i].size, i, n2->axis[i].size);
      biffMaybeAdd(NRRD, err, useBiff); 
      return 0;
    }
  }
  return 1;
}

/*
******** nrrdElementSize()
**
** So just how many bytes long is one element in this nrrd?  This is
** needed (over the simple nrrdTypeSize[] array) because some nrrds
** may be of "block" type, and because it does bounds checking on
** nrrd->type.  Returns 0 if given a bogus nrrd->type, or if the block
** size isn't greater than zero (in which case it sets nrrd->blockSize
** to 0, just out of spite).  This function never returns a negative
** value; using (!nrrdElementSize(nrrd)) is a sufficient check for
** invalidity.
**
** Besides learning how many bytes long one element is, this function
** is useful as a way of detecting an invalid blocksize on a block nrrd.
*/
int
nrrdElementSize (const Nrrd *nrrd) {

  if (!( nrrd && !airEnumValCheck(nrrdType, nrrd->type) )) {
    return 0;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  /* else its block type */
  if (nrrd->blockSize > 0) {
    return nrrd->blockSize;
  }
  /* else we got an invalid block size */
  /* nrrd->blockSize = 0; */
  return 0;
}

/*
******** nrrdElementNumber()
**
** takes the place of old "nrrd->num": the number of elements in the
** nrrd, which is just the product of the axis sizes.  A return of 0
** means there's a problem.  Negative numbers are never returned.
**
** does NOT use biff
*/
size_t
nrrdElementNumber (const Nrrd *nrrd) {
  size_t num;
  int d, size[NRRD_DIM_MAX];

  if (!nrrd) {
    return 0;
  }
  /* else */
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSize, size);
  if (_nrrdSizeCheck(nrrd->dim, size, AIR_FALSE)) {
    /* the nrrd's size information is invalid, can't proceed */
    return 0;
  }
  num = 1;
  for (d=0; d<nrrd->dim; d++) {
    /* negative numbers and overflow were caught by _nrrdSizeCheck() */
    num *= size[d];
  }
  return num;
}

/*
******** nrrdHasNonExistSet()
**
** This function will always (assuming type is valid) set the value of
** nrrd->hasNonExist to either nrrdNonExistTrue or nrrdNonExistFalse,
** and it will return that value.  For lack of a more sophisticated
** policy, blocks are currently always considered to be existant
** values (because nrrdTypeIsIntegral[nrrdTypeBlock] is currently true).
** This function will ALWAYS determine the correct answer and set the
** value of nrrd->hasNonExist: it ignores the value of
** nrrd->hasNonExist on the input nrrd.  Exception: if nrrd is null or
** type is bogus, no action is taken and nrrdNonExistUnknown is
** returned.
**
** Because this will return either nrrdNonExistTrue or nrrdNonExistFalse,
** and because the C boolean value of these are true and false (respectively),
** it is possible (and encouraged) to use the return of this function
** as the expression of a conditional:
**
**   if (nrrdHasNonExistSet(nrrd)) {
**     ... handle existance of non-existant values ...
**   }
*/
/*
int
nrrdHasNonExistSet (Nrrd *nrrd) {
  size_t I, N;
  float val;

  if (!( nrrd && !airEnumValCheck(nrrdType, nrrd->type) ))
    return nrrdNonExistUnknown;

  if (nrrdTypeIsIntegral[nrrd->type]) {
    nrrd->hasNonExist = nrrdNonExistFalse;
  } else {
    nrrd->hasNonExist = nrrdNonExistFalse;
    N = nrrdElementNumber(nrrd);
    for (I=0; I<N; I++) {
      val = nrrdFLookup[nrrd->type](nrrd->data, I);
      if (!AIR_EXISTS(val)) {
	nrrd->hasNonExist = nrrdNonExistTrue;
	break;
      }
    }
  }
  return nrrd->hasNonExist;
}
*/

int
_nrrdCheckEnums (void) {
  char me[]="_nrrdCheckEnums", err[AIR_STRLEN_MED],
    which[AIR_STRLEN_SMALL];

  if (nrrdFormatTypeLast-1 != NRRD_FORMAT_TYPE_MAX) {
    strcpy(which, "nrrdFormat"); goto err;
  }
  if (nrrdBoundaryLast-1 != NRRD_BOUNDARY_MAX) {
    strcpy(which, "nrrdBoundary"); goto err;
  }
  if (nrrdTypeLast-1 != NRRD_TYPE_MAX) {
    strcpy(which, "nrrdType"); goto err;
  }
  if (nrrdEncodingTypeLast-1 != NRRD_ENCODING_TYPE_MAX) {
    strcpy(which, "nrrdEncodingType"); goto err;
  }
  if (nrrdMeasureLast-1 != NRRD_MEASURE_MAX) {
    strcpy(which, "nrrdMeasure"); goto err;
  }
  if (nrrdCenterLast-1 != NRRD_CENTER_MAX) {
    strcpy(which, "nrrdCenter"); goto err;
  }
  if (nrrdAxisInfoLast-1 != NRRD_AXIS_INFO_MAX) {
    strcpy(which, "nrrdAxisInfo"); goto err;
  }
  /* can't really check on endian enum */
  if (nrrdField_last-1 != NRRD_FIELD_MAX) {
    strcpy(which, "nrrdField"); goto err;
  }
  if (nrrdHasNonExistLast-1 != NRRD_HAS_NON_EXIST_MAX) {
    strcpy(which, "nrrdHasNonExist"); goto err;
  }
  if (nrrdUnaryOpLast-1 != NRRD_UNARY_OP_MAX) {
    strcpy(which, "nrrdUnaryOp"); goto err;
  }
  if (nrrdBinaryOpLast-1 != NRRD_BINARY_OP_MAX) {
    strcpy(which, "nrrdBinaryOp"); goto err;
  }
  if (nrrdTernaryOpLast-1 != NRRD_TERNARY_OP_MAX) {
    strcpy(which, "nrrdTernaryOp"); goto err;
  }
  
  /* no errors so far */
  return 0;

 err:
  sprintf(err, "%s: Last vs. MAX incompatibility for %s enum", me, which);
  biffAdd(NRRD, err); return 1;
}

/*
******** nrrdSanity()
**
** makes sure that all the basic assumptions of nrrd hold for
** the architecture/etc which we're currently running on.  
** 
** returns 1 if all is okay, 0 if there is a problem
*/
int
nrrdSanity (void) {
  char me[]="nrrdSanity", err[AIR_STRLEN_MED];
  int aret, type, maxsize;
  airLLong tmpLLI;
  airULLong tmpULLI;
  static int _nrrdSanity = 0;

  if (_nrrdSanity) {
    /* we've been through this once before and things looked okay ... */
    /* Is this thread-safe?  I think so.  If we assume that any two
       threads are going to compute the same value, isn't it the case
       that, at worse, both of them will go through all the tests and
       then set _nrrdSanity to the same thing? */
    return 1;
  }
  
  aret = airSanity();
  if (aret != airInsane_not) {
    sprintf(err, "%s: airSanity() failed: %s", me, airInsaneErr(aret));
    biffAdd(NRRD, err); return 0;
  }

  if (!nrrdDefWriteEncoding) {
    sprintf(err, "%s: nrrdDefWriteEncoding is NULL", me);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdBoundary, nrrdDefRsmpBoundary)) {
    sprintf(err, "%s: nrrdDefRsmpBoundary (%d) not in valid range [%d,%d]",
	    me, nrrdDefRsmpBoundary,
	    nrrdBoundaryUnknown+1, nrrdBoundaryLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdCenter, nrrdDefCenter)) {
    sprintf(err, "%s: nrrdDefCenter (%d) not in valid range [%d,%d]",
	    me, nrrdDefCenter,
	    nrrdCenterUnknown+1, nrrdCenterLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (!( nrrdTypeDefault == nrrdDefRsmpType
	 || !airEnumValCheck(nrrdType, nrrdDefRsmpType) )) {
    sprintf(err, "%s: nrrdDefRsmpType (%d) not in valid range [%d,%d]",
	    me, nrrdDefRsmpType,
	    nrrdTypeUnknown, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdType, nrrdStateMeasureType)) {
    sprintf(err, "%s: nrrdStateMeasureType (%d) not in valid range [%d,%d]",
	    me, nrrdStateMeasureType,
	    nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdType, nrrdStateMeasureHistoType)) {
    sprintf(err,
	    "%s: nrrdStateMeasureHistoType (%d) not in valid range [%d,%d]",
	    me, nrrdStateMeasureType,
	    nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }

  if (!( nrrdTypeSize[nrrdTypeChar] == sizeof(char)
	 && nrrdTypeSize[nrrdTypeUChar] == sizeof(unsigned char)
	 && nrrdTypeSize[nrrdTypeShort] == sizeof(short)
	 && nrrdTypeSize[nrrdTypeUShort] == sizeof(unsigned short)
	 && nrrdTypeSize[nrrdTypeInt] == sizeof(int)
	 && nrrdTypeSize[nrrdTypeUInt] == sizeof(unsigned int)
	 && nrrdTypeSize[nrrdTypeLLong] == sizeof(airLLong)
	 && nrrdTypeSize[nrrdTypeULLong] == sizeof(airULLong)
	 && nrrdTypeSize[nrrdTypeFloat] == sizeof(float)
	 && nrrdTypeSize[nrrdTypeDouble] == sizeof(double) )) {
    sprintf(err, "%s: sizeof() for nrrd types has problem: "
	    "expected (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d) "
	    "but got (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)", me,
	    nrrdTypeSize[nrrdTypeChar],
	    nrrdTypeSize[nrrdTypeUChar],
	    nrrdTypeSize[nrrdTypeShort],
	    nrrdTypeSize[nrrdTypeUShort],
	    nrrdTypeSize[nrrdTypeInt],
	    nrrdTypeSize[nrrdTypeUInt],
	    nrrdTypeSize[nrrdTypeLLong],
	    nrrdTypeSize[nrrdTypeULLong],
	    nrrdTypeSize[nrrdTypeFloat],
	    nrrdTypeSize[nrrdTypeDouble],
	    (int)sizeof(char),
	    (int)sizeof(unsigned char),
	    (int)sizeof(short),
	    (int)sizeof(unsigned short),
	    (int)sizeof(int),
	    (int)sizeof(unsigned int),
	    (int)sizeof(airLLong),
	    (int)sizeof(airULLong),
	    (int)sizeof(float),
	    (int)sizeof(double));
    biffAdd(NRRD, err); return 0;
  }

  /* check on NRRD_TYPE_SIZE_MAX */
  maxsize = 0;
  for (type=nrrdTypeUnknown+1; type<=nrrdTypeLast-2; type++) {
    maxsize = AIR_MAX(maxsize, nrrdTypeSize[type]);
  }
  if (maxsize != NRRD_TYPE_SIZE_MAX) {
    sprintf(err, "%s: actual max type size is %d != %d == NRRD_TYPE_SIZE_MAX",
	    me, maxsize, NRRD_TYPE_SIZE_MAX);
    biffAdd(NRRD, err); return 0;
  }

  /* check on NRRD_TYPE_BIGGEST */
  if (maxsize != sizeof(NRRD_TYPE_BIGGEST)) {
    sprintf(err, "%s: actual max type size is %d != "
	    "%d == sizeof(NRRD_TYPE_BIGGEST)",
	    me, maxsize, (int)sizeof(NRRD_TYPE_BIGGEST));
    biffAdd(NRRD, err); return 0;
  }
  
  /* nrrd-defined type min/max values */
  tmpLLI = NRRD_LLONG_MAX;
  if (tmpLLI != NRRD_LLONG_MAX) {
    sprintf(err, "%s: long long int can't hold NRRD_LLONG_MAX (" AIR_ULLONG_FMT ")", me,
	    NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpLLI += 1;
  if (NRRD_LLONG_MIN != tmpLLI) {
    sprintf(err, "%s: long long int min (" AIR_LLONG_FMT ") or max (" AIR_LLONG_FMT ") incorrect", me,
	    NRRD_LLONG_MIN, NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI = NRRD_ULLONG_MAX;
  if (tmpULLI != NRRD_ULLONG_MAX) {
    sprintf(err, 
	    "%s: unsigned long long int can't hold NRRD_ULLONG_MAX (" AIR_ULLONG_FMT ")",
	    me, NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI += 1;
  if (tmpULLI != 0) {
    sprintf(err, "%s: unsigned long long int max (" AIR_ULLONG_FMT ") incorrect", me,
	    NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }

  if (_nrrdCheckEnums()) {
    sprintf(err, "%s: problem with enum definition", me);
    biffAdd(NRRD, err); return 0;
  }
  
  if (!( NRRD_DIM_MAX >= 3 )) {
    sprintf(err, "%s: NRRD_DIM_MAX == %d seems awfully small, doesn't it?",
	    me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 0;
  }

  if (!nrrdTypeIsIntegral[nrrdTypeBlock]) {
    sprintf(err, "%s: nrrdTypeInteger[nrrdTypeBlock] is not true, things "
	    "could get wacky", me);
    biffAdd(NRRD, err); return 0;
  }

  /* HEY: any other assumptions built into teem? */

  _nrrdSanity = 1;
  return 1;
}
