/*
** ninspect.c

  Copyright (C) 2003 University of Utah

  This software,  is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

**
** This demonstrates some nrrd functions and API.  Given a 3-d scalar
** volume, produces an image composed of three colorized axis-aligned
** projections, in which the R, G, B channels are the sum, variance,
** and max projections. The sum and variance projections are histogram
** equalized (up to some amount).  Projections along the X, Y, and Z are 
** joined together into one image, kind of like the set of orthographic
** projections you might see in a technical drawing. This demonstrates the 
** basic usage of:
**
** nrrdNew, nrrdAlloc, nrrdNuke
** nrrdLoad, nrrdSave
** nrrdProject
** nrrdHistoEq
** nrrdQuantize
** nrrdJoin
** nrrdAxesSwap
** nrrdInset
**
** as well as biffGetDone, biffAdd, biffMove.
**
** This file is heavily commented in an effort to make up for otherwise
** sparse documentation in using the nrrd API.  One thing to keep in
** mind is that all the airMop* functions are my poor-man's memory
** management routines, which you won't need if you wrap nrrd in smart
** pointers or something like that.  All you need to know is that the
** third argument to airMopAdd() is the function that must be called to
** "clean up" the pointer which is passed as the second argument to
*** airMopAdd().
**
** How to compile:

   cc -o ninspect ninspect.c -Iteem/include -I/<other include dirs> \
     -Lteem/${TEEM_ARCH}/lib -L/<other lib dirs> \
     -lteem [-lpng -lbz2 -lz]

** where "teem" is the top-level directory of the teem distribution,
** and TEEM_ARCH is environment variable for your architecture. Only
** some of "-lpng -lbz2 -lz" may be needed for your teem build, depending
** on what optional libraries you compiled teem with (the pre-compiled
** binary releases use all these libraries).
*/

#include <stdio.h>
#include <teem/nrrd.h>

char *ninspect_me;

int
ninspect_proj(Nrrd *nout, Nrrd *nin, int axis, float amount) {
  char me[]="ninspect_proj", err[AIR_STRLEN_MED];
  airArray *mop;
  Nrrd *ntmpA, *ntmpB, *nrgb[3];
  int bins, smart;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ninspect_me, err);
    return 1;
  }
  if (!( AIR_IN_CL(0, axis, 2) )) {
    sprintf(err, "%s: given axis %d outside valid range [0,1,2]", me, axis);
    biffAdd(ninspect_me, err);
    return 1;
  }

  /* allocate a bunch of nrrds to use as basically temp variables */
  mop = airMopNew();
  airMopAdd(mop, ntmpA = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmpB = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nrgb[0] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nrgb[1] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nrgb[2] = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  /* these arguments to nrrdHistoEq will control its behavior */
  bins = 3000;  /* equalization will use a histogram with this many bins */
  smart = 1;    /* one of the bins will be ignored in histogram integration,
		   presumably representing the background */

  /* the following idiom is one way of handling the fact that any 
     non-trivial nrrd call can fail, and if it does, then any subsequent
     nrrd calls should be avoided (to be perfectly safe), so that you can
     get the error message from biff.  Because of the left-to-right ordering
     ensured for logical expressions, this will all be called in sequence
     until one of them has a non-zero return.  If he had exception handling,
     we'd put all the nrrd calls in one "try" block.  */
  if (nrrdProject(ntmpA, nin, axis, nrrdMeasureSum, nrrdTypeDefault)
      || nrrdHistoEq(ntmpB, ntmpA, NULL, bins, smart, amount)
      || nrrdQuantize(nrgb[0], ntmpB, NULL, 8)
      || nrrdProject(ntmpA, nin, axis, nrrdMeasureVariance, nrrdTypeDefault)
      || nrrdHistoEq(ntmpB, ntmpA, NULL, bins, smart, amount)
      || nrrdQuantize(nrgb[1], ntmpB, NULL, 8)
      || nrrdProject(ntmpA, nin, axis, nrrdMeasureMax, nrrdTypeDefault)
      || nrrdQuantize(nrgb[2], ntmpA, NULL, 8)
      || nrrdJoin(nout, (const Nrrd**)nrgb, 3, 0, AIR_TRUE)) {
    sprintf(err, "%s: trouble with nrrd operations", me);
    biffMove(ninspect_me, err, NRRD);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

int
ninspect_doit(Nrrd *nout, Nrrd *nin, float amount) {
  char me[]="ninspect_doit", err[AIR_STRLEN_MED];
  Nrrd *nproj[3], *ntmp;
  airArray *mop;
  int axis, sx, sy, sz, min[3], E, margin;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(ninspect_me, err);
    return 1;
  }
  if (!(3 == nin->dim)) {
    sprintf(err, "%s: given nrrd has dimension %d, not 3\n", me, nin->dim);
    biffAdd(ninspect_me, err);
    return 1;
  }
  sx = nin->axis[0].size;
  sy = nin->axis[1].size;
  sz = nin->axis[2].size;
  
  mop = airMopNew();
  airMopAdd(mop, nproj[0]=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nproj[1]=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nproj[2]=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, ntmp=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);

  /* how much space to put between and around the projections */
  margin = 6;

  /* allocate output as 8-bit color image.  We know output type is
     nrrdTypeUChar because ninspect_proj finishes each projection
     with nrrdQuantize to 8-bits */
  if (nrrdAlloc(nout, nrrdTypeUChar, 3,
		3, sx + 3*margin + sz, sy + 3*margin + sz)) {
    sprintf(err, "%s: couldn't allocate output", me);
    biffMove(ninspect_me, err, NRRD);
    airMopError(mop); return 1;
  }

  /* do projections for each axis, with some progress indication to sterr */
  for (axis=0; axis<=2; axis++) {
    fprintf(stderr, "%s: doing axis %d projections ... ", me, axis);
    fflush(stderr);
    if (ninspect_proj(nproj[axis], nin, axis, amount)) {
      fprintf(stderr, "ERROR\n");
      sprintf(err, "%s: trouble doing projections for axis %d", me, axis);
      biffAdd(ninspect_me, err);
      airMopError(mop); return 1;
    }
    fprintf(stderr, "done\n");
  }

  /* this is another idiom (the first one is in ninspect_proj) for
     stringing together nrrd invocations in a way that will start
     falling through as soon as there as been one error */
  min[0] = 0; 
  E = 0;
  if (!E) { min[1] = margin; min[2] = margin; }
  if (!E) E |= nrrdInset(nout, nout, nproj[2], min);
  if (!E) { min[1] = margin; min[2] = 2*margin + sy; }
  if (!E) E |= nrrdInset(nout, nout, nproj[1], min);
  if (!E) E |= nrrdAxesSwap(ntmp, nproj[0], 1, 2);
  if (!E) { min[1] = 2*margin + sx; min[2] = margin; }
  if (!E) E |= nrrdInset(nout, nout, ntmp, min);
  if (E) {
    sprintf(err, "%s: couldn't composite output", me);
    biffMove(ninspect_me, err, NRRD);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}

void
ninspect_usage() {

  fprintf(stderr, "\nusage: %s <input volume> <output image>\n\n",
	  ninspect_me);
  fprintf(stderr, "<input volume>: must be a 3-D array in NRRD or "
	  "NRRD-compatible format.\n");
  fprintf(stderr, "<output image>: will be saved out in whatever format " 
	  "is implied by the\n");
  fprintf(stderr, "   extension (if recognized), or else in NRRD format\n");
}

int
main(int argc, char *argv[]) {
  char *inS, *outS, *err;
  airArray *mop;
  Nrrd *nin, *nout;

  ninspect_me = argv[0];
  if (3 != argc) {
    ninspect_usage();
    return 1;
  }
  inS = argv[1];
  outS = argv[2];

  mop = airMopNew();
  nin = nrrdNew();
  nout = nrrdNew();
  airMopAdd(mop, nin, (airMopper)nrrdNuke, airMopAlways);
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /* setting this means that the "content" field of the nrrds created
     as part of this processing will never be set by nrrd functions.
     Normally the content can be used a parse-able record of how a given
     nrrd was created, but in teem1.6 or lower, nrrdJoin forgets to 
     set content, so there's no point in doing any content stuff ... */
  nrrdStateDisableContent = AIR_TRUE;

  if (nrrdLoad(nin, inS, NULL)) {
    /* a non-zero return from nrrdLoad (and most every other nrrd
       function returning int) means there was trouble --> an error
       message has been stored with biff under the NRRD key.
       biffGetDone(NRRD) returns that error message (as well as
       freeing up some internal biff resources).  To be completely
       proper, we have to free this pointer, for which we use the mop */
    err=biffGetDone(NRRD);
    airMopAdd(mop, err, airFree, airMopAlways);
    fprintf(stderr, "%s: trouble loading input nrrd \"%s\":\n%s",
	    ninspect_me, inS, err);
    airMopError(mop); return 1;
  }

  /* I thought about making the amount of histogram equalization to be
     applied to the sum and variance projections be a command-line
     argument, but that would be incrementally more work... */
  if (ninspect_doit(nout, nin, 0.5)) {
    err=biffGetDone(ninspect_me);
    airMopAdd(mop, err, airFree, airMopAlways);
    fprintf(stderr, "%s: trouble creating output:\n%s", ninspect_me, err);
    airMopError(mop); return 1;
  }

  if (nrrdSave(outS, nout, NULL)) {
    err=biffGetDone(NRRD);
    airMopAdd(mop, err, airFree, airMopAlways);
    fprintf(stderr, "%s: trouble saving output image \"%s\":\n%s",
	    ninspect_me, outS, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
