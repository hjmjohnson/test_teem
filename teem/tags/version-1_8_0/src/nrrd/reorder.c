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

#include "nrrd.h"
#include "privateNrrd.h"

#include <teem32bit.h>

/*
******** nrrdAxesInsert
**
** like reshape, but preserves axis information on old axes, and
** this is only for adding a "stub" axis with length 1.  All other
** axis attributes are initialized as usual.
*/
int
nrrdAxesInsert(Nrrd *nout, const Nrrd *nin, int ax) {
  char me[]="nrrdAxesInsert", func[]="axinsert", err[AIR_STRLEN_MED];
  int d;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(0, ax, nin->dim)) {
    sprintf(err, "%s: given axis (%d) outside valid range [0, %d]",
            me, ax, nin->dim);
    biffAdd(NRRD, err); return 1;
  }
  if (NRRD_DIM_MAX == nin->dim) {
    sprintf(err, "%s: given nrrd already at NRRD_DIM_MAX (%d)",
            me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->dim = 1 + nin->dim;
  for (d=nin->dim-1; d>=ax; d--) {
    _nrrdAxisInfoCopy(&(nout->axis[d+1]), &(nin->axis[d]),
                      NRRD_AXIS_INFO_NONE);
  }
  /* the ONLY thing we can say about the new axis is its size */
  _nrrdAxisInfoInit(&(nout->axis[ax]));
  if (!nrrdStateKindNoop) {
    /* except maybe the kind */
    nout->axis[ax].kind = nrrdKindStub;
  }
  nout->axis[ax].size = 1;
  if (nrrdContentSet(nout, func, nin, "%d", ax)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* all basic info has already been copied by nrrdCopy() above */
  return 0;
}

/* ---- BEGIN non-NrrdIO */

/*
******** nrrdInvertPerm()
**
** given an array (p) which represents a permutation of n elements,
** compute the inverse permutation ip.  The value of this function
** is not its core functionality, but all the error checking it
** provides.
*/
int
nrrdInvertPerm(int *invp, const int *p, int n) {
  char me[]="nrrdInvertPerm", err[AIR_STRLEN_MED];
  int problem, i;

  if (!(invp && p && n > 0)) {
    sprintf(err, "%s: got NULL pointer or non-positive n (%d)", me, n);
    biffAdd(NRRD, err); return 1;
  }
  
  /* use the given array "invp" as a temp buffer for validity checking */
  memset(invp, 0, n*sizeof(int));
  for (i=0; i<n; i++) {
    if (!(AIR_IN_CL(0, p[i], n-1))) {
      sprintf(err, "%s: permutation element #%d == %d out of bounds [0,%d]",
              me, i, p[i], n-1);
      biffAdd(NRRD, err); return 1;
    }
    invp[p[i]]++;
  }
  problem = AIR_FALSE;
  for (i=0; i<n; i++) {
    if (1 != invp[i]) {
      sprintf(err, "%s: element #%d mapped to %d times (should be once)",
              me, i, invp[i]);
      biffAdd(NRRD, err); problem = AIR_TRUE;
    }
  }
  if (problem) {
    return 1;
  }

  /* the skinny */
  for (i=0; i<n; i++) 
    invp[p[i]] = i;

  return 0;
}


/*
******** nrrdAxesPermute
**
** changes the scanline ordering of the data in a nrrd
** 
** The basic means by which data is moved around is with memcpy().
** The goal is to call memcpy() as few times as possible, on memory 
** segments as large as possible.  Currently, this is done by 
** detecting how many of the low-index axes are left untouched by 
** the permutation- this constitutes a "scanline" which can be
** copied around as a unit.  For permuting the y and z axes of a
** matrix-x-y-z order matrix volume, this optimization produced a
** factor of 5 speed up (exhaustive multi-platform tests, of course).
**
** The axes[] array determines the permutation of the axes.
** axis[i] = j means: axis i in the output will be the input's axis j
** (axis[i] answers: "what do I put here", from the standpoint of the output,
** not "where do I put this", from the standpoint of the input)
*/
int
nrrdAxesPermute(Nrrd *nout, const Nrrd *nin, const int *axes) {
  char me[]="nrrdAxesPermute", func[]="permute", err[AIR_STRLEN_MED],
    buff1[NRRD_DIM_MAX*30], buff2[AIR_STRLEN_SMALL];
  size_t idxOut, idxIn,      /* indices for input and output scanlines */
    lineSize,                /* size of block of memory which can be
                                moved contiguously from input to output,
                                thought of as a "scanline" */
    numLines;                /* how many "scanlines" there are to permute */
  char *dataIn, *dataOut;
  int 
    szIn[NRRD_DIM_MAX], *lszIn,
    szOut[NRRD_DIM_MAX], *lszOut,
    cIn[NRRD_DIM_MAX],
    cOut[NRRD_DIM_MAX],
    ip[NRRD_DIM_MAX+1],      /* inverse of permutation in "axes" */
    laxes[NRRD_DIM_MAX+1],   /* copy of axes[], but shifted down by lowPax
                                elements, to remove i such that i == axes[i] */
    lowPax,                  /* lowest axis which is "p"ermutated */
    d,                       /* running index along dimensions */
    dim,                     /* copy of nin->dim */
    ldim;                    /* dim - lowPax */
  airArray *mop;

  mop = airMopNew();
  if (!(nin && nout && axes)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  /* we don't actually need ip[], computing it is for error checking */
  if (nrrdInvertPerm(ip, axes, nin->dim)) {
    sprintf(err, "%s: couldn't compute axis permutation inverse", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  
  dim = nin->dim;
  for (d=0; d<dim && axes[d] == d; d++)
    ;
  lowPax = d;

  /* allocate output by initial copy */
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: trouble copying input", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;      
    }
    dataIn = (char*)nin->data;
  } else {
    dataIn = (char*)calloc(nrrdElementNumber(nin), nrrdElementSize(nin));
    if (!dataIn) {
      sprintf(err, "%s: couldn't create local copy of data", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    airMopAdd(mop, dataIn, airFree, airMopAlways);
    memcpy(dataIn, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  }
  if (lowPax < dim) {
    /* if lowPax == dim, then we were given the identity permutation, so
       there's nothing to do other than the copy already done.  Otherwise,
       here we are (actually, lowPax < dim-1) */
    nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, szIn);
    if (nrrdAxisInfoCopy(nout, nin, axes, NRRD_AXIS_INFO_NONE)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    nrrdAxisInfoGet_nva(nout, nrrdAxisInfoSize, szOut);
    /* the skinny */
    lineSize = 1;
    for (d=0; d<lowPax; d++) {
      lineSize *= szIn[d];
    }
    numLines = nrrdElementNumber(nin)/lineSize;
    lineSize *= nrrdElementSize(nin);
    lszIn = szIn + lowPax;
    lszOut = szOut + lowPax;
    ldim = dim - lowPax;
    memset(laxes, 0, NRRD_DIM_MAX*sizeof(int));
    for (d=0; d<ldim; d++) {
      laxes[d] = axes[d+lowPax]-lowPax;
    }
    dataOut = nout->data;
    memset(cIn, 0, NRRD_DIM_MAX*sizeof(int));
    memset(cOut, 0, NRRD_DIM_MAX*sizeof(int));
    for (idxOut=0; idxOut<numLines; idxOut++) {
      /* in our representation of the coordinates of the start of the
         scanlines that we're copying, we are not even storing all the
         zeros in the coordinates prior to lowPax, and when we go to
         a linear index for the memcpy(), we multiply by lineSize */
      for (d=0; d<ldim; d++) {
        cIn[laxes[d]] = cOut[d];
      }
      NRRD_INDEX_GEN(idxIn, cIn, lszIn, ldim);
      memcpy(dataOut + idxOut*lineSize, dataIn + idxIn*lineSize, lineSize);
      NRRD_COORD_INCR(cOut, lszOut, ldim, 0);
    }
    /* set content */
    strcpy(buff1, "");
    for (d=0; d<dim; d++) {
      sprintf(buff2, "%s%d", (d ? "," : ""), axes[d]);
      strcat(buff1, buff2);
    }
    if (nrrdContentSet(nout, func, nin, "%s", buff1)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    if (nout != nin) {
      if (nrrdBasicInfoCopy(nout, nin,
                            NRRD_BASIC_INFO_DATA_BIT
                            | NRRD_BASIC_INFO_TYPE_BIT
                            | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                            | NRRD_BASIC_INFO_DIMENSION_BIT
                            | NRRD_BASIC_INFO_CONTENT_BIT
                            | NRRD_BASIC_INFO_COMMENTS_BIT
                            | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
        sprintf(err, "%s:", me);
        biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
  }
  airMopOkay(mop); 
  return 0;
}

/*
******** nrrdAxesSwap()
**
** for when you just want to switch the order of two axes, without
** going through the trouble of creating the permutation array 
** needed to call nrrdAxesPermute()
*/
int
nrrdAxesSwap(Nrrd *nout, const Nrrd *nin, int ax1, int ax2) {
  char me[]="nrrdAxesSwap", func[]="swap", err[AIR_STRLEN_MED];
  int i, axes[NRRD_DIM_MAX];

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_IN_CL(0, ax1, nin->dim-1) 
        && AIR_IN_CL(0, ax2, nin->dim-1))) {
    sprintf(err, "%s: ax1 (%d) or ax2 (%d) out of bounds [0,%d]", 
            me, ax1, ax2, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }

  for (i=0; i<nin->dim; i++) {
    axes[i] = i;
  }
  axes[ax2] = ax1;
  axes[ax1] = ax2;
  if (nrrdAxesPermute(nout, nin, axes)
      || nrrdContentSet(nout, func, nin, "%d,%d", ax1, ax2)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* basic info already copied by nrrdAxesPermute */
  return 0;
}

/*
******** nrrdShuffle
**
** rearranges hyperslices of a nrrd along a given axis according to
** given permutation.  This could be used to on a 4D array,
** representing a 3D volume of vectors, to re-order the vector 
** components.
**
** the given permutation array must allocated for at least as long as
** the input nrrd along the chosen axis.  perm[j] = i means that the
** value at position j in the _new_ array should come from position i
** in the _old_array.  The standpoint is from the new, looking at
** where to find the values amid the old array (perm answers "what do
** I put here", not "where do I put this").  This allows multiple
** positions in the new array to copy from the same old position, and
** insures that there is an source for all positions along the new
** array.
*/
int
nrrdShuffle(Nrrd *nout, const Nrrd *nin, int axis, const int *perm) {
  char me[]="nrrdShuffle", func[]="shuffle", err[AIR_STRLEN_MED],
    buff1[NRRD_DIM_MAX*30], buff2[AIR_STRLEN_SMALL];
  int size[NRRD_DIM_MAX], *lsize,
    d, ldim, len,
    cIn[NRRD_DIM_MAX+1],
    cOut[NRRD_DIM_MAX+1];
  size_t idxIn, idxOut, lineSize, numLines;
  unsigned char *dataIn, *dataOut;

  if (!(nin && nout && perm)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: nout==nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(0, axis, nin->dim-1)) {
    sprintf(err, "%s: axis %d outside valid range [0,%d]", 
            me, axis, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  len = nin->axis[axis].size;
  for (d=0; d<len; d++) {
    if (!AIR_IN_CL(0, perm[d], len-1)) {
      sprintf(err, "%s: perm[%d] (%d) outside valid range [0,%d]", me,
              d, perm[d], len-1);
      biffAdd(NRRD, err); return 1;
    }
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  /* set information in new volume */
  nout->blockSize = nin->blockSize;
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, nin->type, nin->dim, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_NONE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* the min and max along the shuffled axis are now meaningless */
  nout->axis[axis].min = nout->axis[axis].max = AIR_NAN;
  nout->axis[axis].kind = _nrrdKindAltered(nin->axis[axis].kind);
  /* the skinny */
  lineSize = 1;
  for (d=0; d<axis; d++) {
    lineSize *= nin->axis[d].size;
  }
  numLines = nrrdElementNumber(nin)/lineSize;
  lineSize *= nrrdElementSize(nin);
  lsize = size + axis;
  ldim = nin->dim - axis;
  dataIn = nin->data;
  dataOut = nout->data;
  memset(cIn, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  memset(cOut, 0, (NRRD_DIM_MAX+1)*sizeof(int));
  for (idxOut=0; idxOut<numLines; idxOut++) {
    memcpy(cIn, cOut, ldim*sizeof(int));
    cIn[0] = perm[cOut[0]];
    NRRD_INDEX_GEN(idxIn, cIn, lsize, ldim);
    NRRD_INDEX_GEN(idxOut, cOut, lsize, ldim);
    memcpy(dataOut + idxOut*lineSize, dataIn + idxIn*lineSize, lineSize);
    NRRD_COORD_INCR(cOut, lsize, ldim, 0);
  }
  /* content */
  strcpy(buff1, "");
  for (d=0; d<nin->dim; d++) {
    sprintf(buff2, "%s%d", (d ? "," : ""), perm[d]);
    strcat(buff1, buff2);
  }
  if (nrrdContentSet(nout, func, nin, "%s", buff1)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBasicInfoCopy(nout, nin,
                        NRRD_BASIC_INFO_DATA_BIT
                        | NRRD_BASIC_INFO_TYPE_BIT
                        | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                        | NRRD_BASIC_INFO_DIMENSION_BIT
                        | NRRD_BASIC_INFO_CONTENT_BIT
                        | NRRD_BASIC_INFO_COMMENTS_BIT
                        | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }

  return 0;
}

/*
******** nrrdFlip()
**
** reverse the order of slices along the given axis.
** Actually, just a wrapper around nrrdShuffle() (with the minor addition
** of setting nout->axis[axis].min and .max)
*/
int
nrrdFlip(Nrrd *nout, const Nrrd *nin, int axis) {
  char me[]="nrrdFlip", func[]="flip", err[AIR_STRLEN_MED];
  int i, *perm;
  airArray *mop;

  mop = airMopNew();
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (!(AIR_IN_CL(0, axis, nin->dim-1))) {
    sprintf(err, "%s: given axis (%d) is outside valid range ([0,%d])", 
            me, axis, nin->dim-1);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (!(perm = calloc(nin->axis[axis].size, sizeof(int)))) {
    sprintf(err, "%s: couldn't alloc permutation array", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, perm, airFree, airMopAlways);
  for (i=0; i<nin->axis[axis].size; i++) {
    perm[i] = nin->axis[axis].size-1-i;
  }
  /* nrrdBasicInfoCopy called by nrrdShuffle() */
  if (nrrdShuffle(nout, nin, axis, perm)
      || nrrdContentSet(nout, func, nin, "%d", axis)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  _nrrdAxisInfoCopy(&(nout->axis[axis]), &(nin->axis[axis]),
                    NRRD_AXIS_INFO_SIZE_BIT
                    | NRRD_AXIS_INFO_KIND_BIT);
  /* HEY: (Tue Jan 18 00:28:26 EST 2005) there's a basic question to
     be answered here: do we want to keep the "location" of the
     samples fixed, while changing their ordering, or do want to flip
     the location of the samples?  In the former, the position
     information has to be flipped to cancel the flipping of the the
     sample order, so that samples maintain location.  In the latter,
     the position information is copied verbatim from the original.  */
  nout->axis[axis].min = nin->axis[axis].max;
  nout->axis[axis].max = nin->axis[axis].min;
  /* HEY: Fri Jan 14 02:53:30 EST 2005: isn't spacing supposed to be
     the step from one sample to the next?  So its a signed quantity.
     If min and max can be flipped (so min > max), then spacing can
     be negative, right?  */
  nout->axis[axis].spacing = -nin->axis[axis].spacing;
  /* HEY: Fri Jan 14 02:53:30 EST 2005: but not thickness */
  nout->axis[axis].thickness = nin->axis[axis].thickness;
  airMopOkay(mop); 
  return 0;
}

/*
**
** NOTE: this seems to destroy all space/orientation info.  What
** should be done? 
*/
int
nrrdJoin(Nrrd *nout, const Nrrd *const *nin, int numNin,
         int axis, int incrDim) {
  char me[]="nrrdJoin", err[AIR_STRLEN_MED];
  int mindim, maxdim, diffdim, outdim, map[NRRD_DIM_MAX], size[NRRD_DIM_MAX],
    i, d, outlen, permute[NRRD_DIM_MAX], ipermute[NRRD_DIM_MAX],
    axmap[NRRD_DIM_MAX];
  size_t outnum, chunksize;
  char *dataPerm;
  Nrrd *ntmpperm,    /* axis-permuted version of output */
    **ninperm;
  airArray *mop;

  /* error checking */
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(numNin >= 1)) {
    sprintf(err, "%s: numNin (%d) must be >= 1", me, numNin);
    biffAdd(NRRD, err); return 1;
  }
  for (i=0; i<numNin; i++) {
    if (!(nin[i])) {
      sprintf(err, "%s: input nrrd #%d NULL", me, i);
      biffAdd(NRRD, err); return 1;
    }
    if (nout==nin[i]) {
      sprintf(err, "%s: nout==nin[%d] disallowed", me, i);
      biffAdd(NRRD, err); return 1;
    }
  }

  mop = airMopNew();
  ninperm = calloc(numNin, sizeof(Nrrd *));
  if (!(ninperm)) {
    sprintf(err, "%s: couldn't calloc() temp nrrd array", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, ninperm, airFree, airMopAlways);

  maxdim = mindim = nin[0]->dim;
  for (i=0; i<numNin; i++) {
    mindim = AIR_MIN(mindim, nin[i]->dim);
    maxdim = AIR_MAX(maxdim, nin[i]->dim);
  }
  diffdim = maxdim - mindim;
  if (diffdim > 1) {
    sprintf(err, "%s: will only reshape up one dimension (not %d)",
            me, diffdim);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (axis > maxdim) {
    sprintf(err, "%s: can't join along axis %d with highest input dim = %d",
            me, axis, maxdim);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  
  /* figure out dimension of output (outdim) */
  if (diffdim) {
    /* case A: (example) 2D slices and 3D slabs are being joined
       together to make a bigger 3D volume */
    outdim = maxdim;
  } else {
    /* diffdim == 0 */
    if (axis == maxdim) {
      /* case B: this is like the old "stitch": a bunch of equal-sized
         slices of dimension N are being stacked together to make an
         N+1 dimensional volume, which is essentially just the result of
         concatenating the memory of individual inputs */
      outdim = maxdim + 1;
    } else {
      /* case C: axis < maxdim; maxdim == mindim */
      /* case C1 (!incrDim): a bunch of N-D slabs are being joined
         together to make a bigger N-D volume.  The axis along which
         they are being joined could be any of existing axes (from 0
         to maxdim-1) */
      /* case C2 (incrDim): this is also a "stitch", but the new axis
         created by the stitching is inserted into the existing
         axes. (ex: stitch 3 PGMs (R, G, B) together into a PPM (with
         color on axis zero) */
      outdim = maxdim + !!incrDim;
    }
  }
  if (outdim > NRRD_DIM_MAX) {
    sprintf(err, "%s: output dimension (%d) exceeds NRRD_DIM_MAX (%d)",
            me, outdim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); airMopError(mop); return 1;    
  }
  
  /* do tacit reshaping, and possibly permuting, as needed */
  for (i=0; i<outdim; i++) {
    permute[i] = (i < axis
                  ? i 
                  : (i < outdim-1
                     ? i + 1
                     : axis));
    /* fprintf(stderr, "!%s: 1st permute[%d] = %d\n", me, i, permute[i]); */
  }
  for (i=0; i<numNin; i++) {
    ninperm[i] = nrrdNew();
    diffdim = outdim - nin[i]->dim;
    /* fprintf(stderr, "!%s: i = %d ---> diffdim = %d\n", me, i, diffdim); */
    if (diffdim) {
      /* we do a tacit reshaping, which actually includes
         a tacit permuting, so we don't have to call permute
         on the parts that don't actually need it */
      /* NB: we register nrrdNix, not nrrdNuke */
      /* fprintf(stderr, "!%s: %d: tacit reshape/permute\n", me, i); */
      airMopAdd(mop, ninperm[i], (airMopper)nrrdNix, airMopAlways);
      nrrdAxisInfoGet_nva(nin[i], nrrdAxisInfoSize, size);
      for (d=nin[i]->dim-1; d>=mindim+1; d--) {
        size[d] = size[d-1];
      }
      size[mindim] = 1;
      /* this may be done needlessly often */
      for (d=0; d<=nin[i]->dim; d++) {
        if (d < mindim) {
          axmap[d] = d;
        } else if (d > mindim) {
          axmap[d] = d-1;
        } else {
          axmap[d] = -1;
        }
      }
      /* we don't have to actually call nrrdReshape(): we just nrrdWrap()
         the input data with the reshaped size array */
      if (nrrdWrap_nva(ninperm[i], nin[i]->data, nin[i]->type,
                   nin[i]->dim+1, size)) {
        sprintf(err, "%s: trouble creating intermediate version of nrrd %d",
                me, i);
        biffAdd(NRRD, err); airMopError(mop); return 1;    
      }
      nrrdAxisInfoCopy(ninperm[i], nin[i], axmap, 
                       (NRRD_AXIS_INFO_SIZE_BIT
                        /* HEY: this is being nixed because I can't think
                           of a sane way of keeping it consistent */
                        | NRRD_AXIS_INFO_SPACEDIRECTION_BIT));
    } else {
      /* on this part, we permute (no need for a reshape) */
      airMopAdd(mop, ninperm[i], (airMopper)nrrdNuke, airMopAlways);
      if (nrrdAxesPermute(ninperm[i], nin[i], permute)) {
        sprintf(err, "%s: trouble permuting input part %d", me, i);
        biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
  }

  /* make sure all parts are compatible in type and shape,
     determine length of final output along axis (outlen) */
  outlen = 0;
  for (i=0; i<numNin; i++) {
    if (ninperm[i]->type != ninperm[0]->type) {
      sprintf(err, "%s: type (%s) of part %d unlike first's (%s)",
              me, airEnumStr(nrrdType, ninperm[i]->type),
              i, airEnumStr(nrrdType, ninperm[0]->type));
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    if (nrrdTypeBlock == ninperm[0]->type) {
      if (ninperm[i]->blockSize != ninperm[0]->blockSize) {
        sprintf(err, "%s: blockSize (%d) of part %d unlike first's (%d)",
                me, ninperm[i]->blockSize, i, ninperm[0]->blockSize);
        biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
    if (!nrrdElementSize(ninperm[i])) {
      sprintf(err, "%s: got wacky elements size (%d) for part %d",
              me, nrrdElementSize(ninperm[i]), i);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
    
    /* fprintf(stderr, "!%s: part %03d shape: ", me, i); */
    for (d=0; d<=outdim-2; d++) {
      /* fprintf(stderr, "%03d ", ninperm[i]->axis[d].size); */
      if (ninperm[i]->axis[d].size != ninperm[0]->axis[d].size) {
        sprintf(err, "%s: axis %d size (%d) of part %d unlike first's (%d)",
                me, d, ninperm[i]->axis[d].size, i, ninperm[0]->axis[d].size);
        biffAdd(NRRD, err); airMopError(mop); return 1;
      }
    }
    /* fprintf(stderr, "%03d\n", ninperm[i]->axis[outdim-1].size); */
    outlen += ninperm[i]->axis[outdim-1].size;
  }
  /* fprintf(stderr, "!%s: outlen = %d\n", me, outlen); */

  /* allocate temporary nrrd and concat input into it */
  outnum = 1;
  for (d=0; d<=outdim-2; d++) {
    size[d] = ninperm[0]->axis[d].size;
    outnum *= size[d];
  }
  size[outdim-1] = outlen;
  outnum *= size[outdim-1];
  if (nrrdMaybeAlloc_nva(ntmpperm = nrrdNew(), ninperm[0]->type,
                         outdim, size)) {
    sprintf(err, "%s: trouble allocating permutation nrrd", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  airMopAdd(mop, ntmpperm, (airMopper)nrrdNuke, airMopAlways);
  dataPerm = ntmpperm->data;
  for (i=0; i<numNin; i++) {
    /* here is where the actual joining happens */
    chunksize = nrrdElementNumber(ninperm[i])*nrrdElementSize(ninperm[i]);
    memcpy(dataPerm, ninperm[i]->data, chunksize);
    dataPerm += chunksize;
  }
  
  /* copy other axis-specific fields from nin[0] to ntmpperm */
  for (d=0; d<=outdim-2; d++) {
    map[d] = d;
  }
  map[outdim-1] = -1;
  nrrdAxisInfoCopy(ntmpperm, ninperm[0], map, 
                   (NRRD_AXIS_INFO_NONE
                    /* HEY: this is being nixed because I can't think
                       of a sane way of keeping it consistent */
                    | NRRD_AXIS_INFO_SPACEDIRECTION_BIT));
  ntmpperm->axis[outdim-1].size = outlen;

  /* do the permutation required to get output in right order */
  nrrdInvertPerm(ipermute, permute, outdim);
  if (nrrdAxesPermute(nout, ntmpperm, ipermute)) {
    sprintf(err, "%s: error permuting temporary nrrd", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  /* basic info is either already set or invalidated by joining */

  /* HEY: set content on output! */

  airMopOkay(mop); 
  return 0;
}

/*
******** nrrdAxesSplit
**
** like reshape, but only for splitting one axis into a fast and slow part.
*/
int
nrrdAxesSplit(Nrrd *nout, const Nrrd *nin,
              int ax, int sizeFast, int sizeSlow) {
  char me[]="nrrdAxesSplit", func[]="axsplit", err[AIR_STRLEN_MED];
  int d;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(0, ax, nin->dim)) {
    sprintf(err, "%s: given axis (%d) outside valid range [0, %d]",
            me, ax, nin->dim);
    biffAdd(NRRD, err); return 1;
  }
  if (NRRD_DIM_MAX == nin->dim) {
    sprintf(err, "%s: given nrrd already at NRRD_DIM_MAX (%d)",
            me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  if (!(sizeFast*sizeSlow == nin->axis[ax].size)) {
    sprintf(err, "%s: # samples along axis %d (%d) != product of "
            "fast and slow sizes (%d * %d = %d)",
            me, ax, nin->axis[ax].size,
            sizeFast, sizeSlow, sizeFast*sizeSlow);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->dim = 1 + nin->dim;
  for (d=nin->dim-1; d>=ax+1; d--) {
    _nrrdAxisInfoCopy(&(nout->axis[d+1]), &(nin->axis[d]),
                      NRRD_AXIS_INFO_NONE);
  }
  /* the ONLY thing we can say about the new axes are their sizes */
  _nrrdAxisInfoInit(&(nout->axis[ax]));
  _nrrdAxisInfoInit(&(nout->axis[ax+1]));
  nout->axis[ax].size = sizeFast;
  nout->axis[ax+1].size = sizeSlow;
  if (nrrdContentSet(nout, func, nin, "%d,%d,%d", ax, sizeFast, sizeSlow)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* all basic information already copied by nrrdCopy */
  return 0;
}

/*
******** nrrdAxesDelete
**
** like reshape, but preserves axis information on old axes, and
** this is only for removing a "stub" axis with length 1.
*/
int
nrrdAxesDelete(Nrrd *nout, const Nrrd *nin, int ax) {
  char me[]="nrrdAxesDelete", func[]="axdelete", err[AIR_STRLEN_MED];
  int d;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(0, ax, nin->dim-1)) {
    sprintf(err, "%s: given axis (%d) outside valid range [0, %d]",
            me, ax, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: given nrrd already at lowest dimension (1)", me);
    biffAdd(NRRD, err); return 1;
  }
  if (1 != nin->axis[ax].size) {
    sprintf(err, "%s: size along axis %d is %d, not 1",
            me, ax, nin->axis[ax].size);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->dim = nin->dim - 1;
  for (d=ax; d<=nin->dim-2; d++) {
    _nrrdAxisInfoCopy(&(nout->axis[d]), &(nin->axis[d+1]),
                      NRRD_AXIS_INFO_NONE);
  }
  if (nrrdContentSet(nout, func, nin, "%d", ax)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* all basic information already copied by nrrdCopy */
  return 0;
}

/*
******** nrrdAxesMerge
**
** like reshape, but preserves axis information on old axes
** merges axis ax and ax+1 into one 
*/
int
nrrdAxesMerge(Nrrd *nout, const Nrrd *nin, int ax) {
  char me[]="nrrdAxesMerge", func[]="axmerge", err[AIR_STRLEN_MED];
  int d, sizeFast, sizeSlow;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!AIR_IN_CL(0, ax, nin->dim-2)) {
    sprintf(err, "%s: given axis (%d) outside valid range [0, %d]",
            me, ax, nin->dim-2);
    biffAdd(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: given nrrd already at lowest dimension (1)", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  sizeFast = nin->axis[ax].size;
  sizeSlow = nin->axis[ax+1].size;
  nout->dim = nin->dim - 1;
  for (d=ax+1; d<nout->dim; d++) {
    _nrrdAxisInfoCopy(&(nout->axis[d]), &(nin->axis[d+1]),
                      NRRD_AXIS_INFO_NONE);
  }
  /* the ONLY thing we can say about the new axis is its size */
  _nrrdAxisInfoInit(&(nout->axis[ax]));
  nout->axis[ax].size = sizeFast*sizeSlow;
  if (nrrdContentSet(nout, func, nin, "%d", ax)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  /* all basic information already copied by nrrdCopy */
  return 0;
}


/*
******** nrrdReshape_nva()
**
*/
int
nrrdReshape_nva(Nrrd *nout, const Nrrd *nin, int dim, const int *size) {
  char me[]="nrrdReshape_nva", func[]="reshape", err[AIR_STRLEN_MED],
    buff1[NRRD_DIM_MAX*30], buff2[AIR_STRLEN_SMALL];
  size_t numOut;
  int d;
  
  if (!(nout && nin && size)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_IN_CL(1, dim, NRRD_DIM_MAX))) {
    sprintf(err, "%s: given dimension (%d) outside valid range [1,%d]",
            me, dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  if (_nrrdSizeCheck(dim, size, AIR_TRUE)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  numOut = 1;
  for (d=0; d<dim; d++) {
    numOut *= size[d];
  }
  if (numOut != nrrdElementNumber(nin)) {
    sprintf(err, "%s: new sizes product (" _AIR_SIZE_T_FMT ") "
            "!= # elements (" _AIR_SIZE_T_FMT ")",
            me, numOut, nrrdElementNumber(nin));
    biffAdd(NRRD, err); return 1;
  }

  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nout->dim = dim;
  for (d=0; d<dim; d++) {
    /* the ONLY thing we can say about the axes is the size */
    _nrrdAxisInfoInit(&(nout->axis[d]));
    nout->axis[d].size = size[d];
  }

  strcpy(buff1, "");
  for (d=0; d<dim; d++) {
    sprintf(buff2, "%s%d", (d ? "x" : ""), size[d]);
    strcat(buff1, buff2);
  }
  /* basic info copied by _nrrdCopy, but probably more than we 
     want- perhaps space dimension and origin should be nixed? */
  if (nrrdContentSet(nout, func, nin, "%s", buff1)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdReshape()
**
** var-args version of nrrdReshape_nva()
*/
int
nrrdReshape(Nrrd *nout, const Nrrd *nin, int dim, ...) {
  char me[]="nrrdReshape", err[AIR_STRLEN_MED];
  int d, size[NRRD_DIM_MAX];
  va_list ap;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!(AIR_IN_CL(1, dim, NRRD_DIM_MAX))) {
    sprintf(err, "%s: given dimension (%d) outside valid range [1,%d]",
            me, dim, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  va_start(ap, dim);
  for (d=0; d<dim; d++) {
    size[d] = va_arg(ap, int);
  }
  va_end(ap);
  /* basic info copied (indirectly) by nrrdReshape_nva() */
  if (nrrdReshape_nva(nout, nin, dim, size)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

/*
******** nrrdBlock()
**
** collapse the first axis (axis 0) of the nrrd into a block, making
** an output nrrd of type nrrdTypeBlock.  The input type can be block.
** All information for other axes is shifted down one axis.
*/
int
nrrdBlock(Nrrd *nout, const Nrrd *nin) {
  char me[]="nrrdBlock", func[]="block", err[AIR_STRLEN_MED];
  int d, numEl, map[NRRD_DIM_MAX], size[NRRD_DIM_MAX];

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: due to laziness, nout==nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (1 == nin->dim) {
    sprintf(err, "%s: can't blockify 1-D nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nin)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  numEl = nin->axis[0].size;;
  nout->blockSize = numEl*nrrdElementSize(nin);
  /*
  fprintf(stderr, "!%s: nout->blockSize = %d * %d = %d\n", me,
          numEl, nrrdElementSize(nin), nout->blockSize);
  */
  for (d=0; d<=nin->dim-2; d++) {
    map[d] = d+1;
    size[d] = nin->axis[map[d]].size;
  }

  /* nout->blockSize set above */
  if (nrrdMaybeAlloc_nva(nout, nrrdTypeBlock, nin->dim-1, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  if (nrrdAxisInfoCopy(nout, nin, map, NRRD_AXIS_INFO_NONE)) {
    sprintf(err, "%s: failed to copy axes", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdContentSet(nout, func, nin, "")) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBasicInfoCopy(nout, nin,
                        NRRD_BASIC_INFO_DATA_BIT
                        | NRRD_BASIC_INFO_TYPE_BIT
                        | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                        | NRRD_BASIC_INFO_DIMENSION_BIT
                        | NRRD_BASIC_INFO_CONTENT_BIT
                        | NRRD_BASIC_INFO_COMMENTS_BIT
                        | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdUnblock()
**
** takes a nrrdTypeBlock nrrd and breaks the blocks into elements of 
** type "type", and shifts other axis information up by one axis
*/
int
nrrdUnblock(Nrrd *nout, const Nrrd *nin, int type) {
  char me[]="nrrdUnblock", func[]="unblock", err[AIR_STRLEN_MED];
  int size[NRRD_DIM_MAX], d, map[NRRD_DIM_MAX], outElSz;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout == nin) {
    sprintf(err, "%s: due to laziness, nout==nin disallowed", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock != nin->type) {
    sprintf(err, "%s: need input nrrd type %s", me,
            airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  if (NRRD_DIM_MAX == nin->dim) {
    sprintf(err, "%s: input nrrd already at dimension limit (%d)",
            me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdType, type)) {
    sprintf(err, "%s: invalid requested type %d", me, type);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == type && (!(0 < nout->blockSize))) {
    sprintf(err, "%s: for %s type, need nout->blockSize set", me,
            airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!(nrrdElementSize(nin))) {
    sprintf(err, "%s: nin or nout reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }

  nout->type = type;
  outElSz = nrrdElementSize(nout);
  if (nin->blockSize % outElSz) {
    sprintf(err, "%s: input blockSize (%d) not multiple of output "
            "element size (%d)",
            me, nin->blockSize, outElSz);
    biffAdd(NRRD, err); return 1;
  }
  for (d=0; d<=nin->dim; d++) {
    map[d] = !d ?  -1 : d-1;
    size[d] = !d ? nin->blockSize / outElSz : nin->axis[map[d]].size;
  }
  /* if nout->blockSize is needed, we've checked that its set */
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim+1, size)) {
    sprintf(err, "%s: failed to allocate output", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nout->data, nin->data, nrrdElementNumber(nin)*nrrdElementSize(nin));
  if (nrrdAxisInfoCopy(nout, nin, map, NRRD_AXIS_INFO_NONE)) {
    sprintf(err, "%s: failed to copy axes", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdContentSet(nout, func, nin, "")) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdBasicInfoCopy(nout, nin,
                        NRRD_BASIC_INFO_DATA_BIT
                        | NRRD_BASIC_INFO_TYPE_BIT
                        | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                        | NRRD_BASIC_INFO_DIMENSION_BIT
                        | NRRD_BASIC_INFO_CONTENT_BIT
                        | NRRD_BASIC_INFO_COMMENTS_BIT
                        | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/* for nrrdTile...

will require that # slices be <= number of images: won't crop for you,
but will happy pad with black.  This will be handled in another
function.  Probably unu tile.

*/

/*
******** nrrdTile2D()
**
** Splits axis axSplit into two pieces of size sizeFast and sizeSlow.
** The data from the fast partition is juxtaposed following ax0, the
** slow after ax1.  nrrdAxesMerge is then called to join ax0 and ax1
** with their respective newly permuted data.  There should be one
** fewer dimensions in the output nrrd than in the input nrrd.
*/
int
nrrdTile2D(Nrrd *nout, const Nrrd *nin, int ax0, int ax1,
           int axSplit, int sizeFast, int sizeSlow) {
  char me[]="nrrdTile2D", err[AIR_STRLEN_MED];
  int E,                     /* error flag */
    insAxis[2*NRRD_DIM_MAX], /* array for inserting the two axes resulting
                                from the initial split amongst the other
                                axes: inserted axes go in odd slots, 
                                other axes go in even slots */
    map[NRRD_DIM_MAX],       /* axis map for axis permute */
    mapIdx,                  /* index for filling map[] */
    merge[2],                /* two axes to be merged post-permute */
    mergeIdx,                /* index for filling merge[] */
    ii;                      /* loop variable */

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }

  /* at least for now, axSplit, ax0, and ax1 need to be distinct */
  if (!( axSplit != ax0 
         && axSplit != ax1 
         && ax0 != ax1 )) {
    sprintf(err, "%s: axSplit, ax0, ax1 (%d,%d,%d) must be distinct",
            me, axSplit, ax0, ax1);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_CL(0, ax0, nin->dim-1)
         && AIR_IN_CL(0, ax1, nin->dim-1)
         && AIR_IN_CL(0, axSplit, nin->dim-1) )) {
    sprintf(err, "%s: axSplit, ax0, ax1 (%d,%d,%d) must be in range [0,%d]",
            me, axSplit, ax0, ax1, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  
  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* increment ax0 and ax1 if they're above axSplit, since the
     initial axis split will bump up the corresponding axes */
  ax0 += (axSplit < ax0);
  ax1 += (axSplit < ax1);
  /* initialize insAxis to all invalid (blank) values */
  for (ii=0; ii<2*(nout->dim+1); ii++) {
    insAxis[ii] = -1;
  }
  /* run through post-split axes, inserting axSplit and axSplit+1
     into the slots after ax0 and ax1 respectively, otherwise
     set the identity map */
  for (ii=0; ii<(nout->dim+1); ii++) {
    if (axSplit == ii) {
      insAxis[2*ax0 + 1] = axSplit;
    } else if (axSplit+1 == ii) {
      insAxis[2*ax1 + 1] = axSplit+1;
    } else {
      insAxis[2*ii + 0] = ii;
    }
  }
  /* settle the values from insAxis[] into map[] by removing the -1's */
  mergeIdx = mapIdx = 0;
  for (ii=0; ii<2*(nout->dim+1); ii++) {
    if (insAxis[ii] != -1) {
      if (1 == ii % 2) {
        /* its an odd entry in insAxis[], so the previous axis is to be
           merged.  Using mapIdx-1 is legit because we disallow
           axSplit == ax{0,1} */
        merge[mergeIdx++] = mapIdx-1;
      }
      map[mapIdx++] = insAxis[ii];
    }
  }

  E = AIR_FALSE;
  if (!E) E |= nrrdAxesSplit(nout, nout, axSplit, sizeFast, sizeSlow);
  if (!E) E |= nrrdAxesPermute(nout, nout, map);
  if (!E) E |= nrrdAxesMerge(nout, nout, merge[1]);    
  if (!E) E |= nrrdAxesMerge(nout, nout, merge[0]);
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  /* HEY: set content */
  if (nrrdBasicInfoCopy(nout, nin,
                        NRRD_BASIC_INFO_DATA_BIT
                        | NRRD_BASIC_INFO_TYPE_BIT
                        | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                        | NRRD_BASIC_INFO_DIMENSION_BIT
                        | NRRD_BASIC_INFO_CONTENT_BIT
                        | NRRD_BASIC_INFO_COMMENTS_BIT
                        | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdUntile2D()
**
** This will split ax0 into nin->axis[ax0].size/sizeFast and sizeFast
** sizes.  ax1 will then be split into nin->axis[ax1].size/sizeSlow
** and sizeSlow sizes.  The axes corresponding to sizeFast and
** sizeSlow will be permuted and merged such that
** nout->axis[axMerge].size == sizeFast*sizeSlow.
**
** The thing to be careful of is that axMerge identifies an axis
** in the array set *after* the two axis splits, not before.  This
** is in contrast to the axSplit (and ax0 and ax1) argument of nrrdTile2D
** which identifies axes in the original nrrd.
*/
int nrrdUntile2D(Nrrd *nout, const Nrrd *nin, int ax0, int ax1,
                 int axMerge, int sizeFast, int sizeSlow) {
  char me[]="nrrdUntile2D", err[AIR_STRLEN_MED];
  int E, map[NRRD_DIM_MAX], ii, mapIdx;
  
  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (ax0 == ax1) {
    sprintf(err, "%s: ax0 (%d) and ax1 (%d) must be distinct",
            me, ax0, ax1);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_CL(0, ax0, nin->dim-1)
         && AIR_IN_CL(0, ax1, nin->dim-1) )) {
    sprintf(err, "%s: ax0, ax1 (%d,%d) must be in range [0,%d]",
            me, ax0, ax1, nin->dim-1);
    biffAdd(NRRD, err); return 1;
  }
  if (!( AIR_IN_CL(0, axMerge, nin->dim) )) {
    sprintf(err, "%s: axMerge (%d) must be in range [0,%d]",
            me, axMerge, nin->dim);
    biffAdd(NRRD, err); return 1;
  }
  if (nin->axis[ax0].size != sizeFast*(nin->axis[ax0].size/sizeFast)) {
    sprintf(err, "%s: sizeFast (%d) doesn't divide into axis %d size (%d)",
            me, sizeFast, ax0, nin->axis[ax0].size);
    biffAdd(NRRD, err); return 1;
  }
  if (nin->axis[ax1].size != sizeSlow*(nin->axis[ax1].size/sizeSlow)) {
    sprintf(err, "%s: sizeSlow (%d) doesn't divide into axis %d size (%d)",
            me, sizeSlow, ax1, nin->axis[ax1].size);
    biffAdd(NRRD, err); return 1;
  }

  if (nout != nin) {
    if (_nrrdCopy(nout, nin, (NRRD_BASIC_INFO_COMMENTS_BIT
                              | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT))) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* Split the larger (slower) axis first. */
  E = AIR_FALSE;
  if (ax0 < ax1) {
    if (!E) E |= nrrdAxesSplit(nout, nout, ax1,
                               nin->axis[ax1].size/sizeSlow, sizeSlow);
    if (!E) E |= nrrdAxesSplit(nout, nout, ax0, 
                               nin->axis[ax0].size/sizeFast, sizeFast);
    /* Increment the larger value as it will get shifted by the lower
       split. */
    ax1++;
  } else {
    if (!E) E |= nrrdAxesSplit(nout, nout, ax0,
                               nin->axis[ax0].size/sizeFast, sizeFast);
    if (!E) E |= nrrdAxesSplit(nout, nout, ax1, 
                               nin->axis[ax1].size/sizeSlow, sizeSlow);
    ax0++;
  }
  if (E) {
    sprintf(err, "%s: trouble with initial splitting", me);
    biffAdd(NRRD, err); return 1;
  }

  /* Determine the axis permutation map */
  mapIdx = 0;
  for (ii=0; ii<nout->dim; ii++) {
    if (mapIdx == axMerge) {
      /* Insert the slow parts of the axes that have been split */
      map[mapIdx++] = ax0+1;
      map[mapIdx++] = ax1+1;
    }
    if (ii == ax0+1 || ii == ax1+1) {
      /* These are handled by the logic above */
    } else {
      /* Otherwise use the identity map */
      map[mapIdx++] = ii;
    }
  }

  /*
  fprintf(stderr, "%s: map =", me);
  for (ii=0; ii<nout->dim; ii++) {
    fprintf(stderr, " %d", map[ii]);
  }
  fprintf(stderr, "; axMerge = %d\n", axMerge);
  */

  E = AIR_FALSE;
  if (!E) E |= nrrdAxesPermute(nout, nout, map);
  if (!E) E |= nrrdAxesMerge(nout, nout, axMerge);    
  if (E) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  if (nrrdBasicInfoCopy(nout, nin,
                        NRRD_BASIC_INFO_DATA_BIT
                        | NRRD_BASIC_INFO_TYPE_BIT
                        | NRRD_BASIC_INFO_BLOCKSIZE_BIT
                        | NRRD_BASIC_INFO_DIMENSION_BIT
                        | NRRD_BASIC_INFO_CONTENT_BIT
                        | NRRD_BASIC_INFO_COMMENTS_BIT
                        | NRRD_BASIC_INFO_KEYVALUEPAIRS_BIT)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}
                        
/* ---- END non-NrrdIO */
