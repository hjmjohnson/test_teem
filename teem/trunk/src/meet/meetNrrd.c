/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "meet.h"

typedef union {
  const NrrdKernel ***k;
  void **v;
} _kernu;

/*
** ALLOCATES and returns a NULL-terminated array of all the 
** NrrdKernels in Teem
*/
const NrrdKernel **
meetNrrdKernelAll(void) {
  airArray *arr;
  const NrrdKernel **kern;
  unsigned int ii;
  int di, ci, ai, dmax, cmax, amax;
  _kernu ku;
  
  ku.k = &kern;
  arr = airArrayNew(ku.v, NULL, sizeof(NrrdKernel *), 2);

  /* kernel.c */
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelZero;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBox;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBoxSupportDebug;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCos4SupportDebug;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCos4SupportDebugD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCos4SupportDebugDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCos4SupportDebugDDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCheap;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelHermiteScaleSpaceFlag;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelTent;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelForwDiff;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCentDiff;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBCCubic;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBCCubicD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBCCubicDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCatmullRom;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelCatmullRomD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelAQuartic;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelAQuarticD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelAQuarticDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC3Quintic;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC3QuinticD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC3QuinticDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC4Hexic;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC4HexicD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC4HexicDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC4HexicDDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelC4HexicApproxInverse;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelGaussian;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelGaussianD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelGaussianDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelDiscreteGaussian;

  /* winKernel.c */
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelHann;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelHannD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelHannDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBlackman;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBlackmanD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBlackmanDD;

  /* bsplKernel.c */
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline3;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline3D;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline3DD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline3DDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline3ApproxInverse;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline5;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline5D;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline5DD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline5DDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline5ApproxInverse;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline7;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline7D;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline7DD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline7DDD;
  ii = airArrayLenIncr(arr, 1); kern[ii] = nrrdKernelBSpline7ApproxInverse;

  /* tmfKernel.c
   nrrdKernelTMF[D+1][C+1][A] is d<D>_c<C>_<A>ef:
   Dth-derivative, C-order continuous ("smooth"), A-order accurate
   (for D and C, index 0 accesses the function for -1) 
    NRRD_EXPORT NrrdKernel *const nrrdKernelTMF[4][5][5];
  */
  dmax = AIR_CAST(int, nrrdKernelTMF_maxD);
  cmax = AIR_CAST(int, nrrdKernelTMF_maxC);
  amax = AIR_CAST(int, nrrdKernelTMF_maxA);
  for (di=-1; di<=dmax; di++) {
    for (ci=-1; ci<=cmax; ci++) {
      for (ai=1; ai<=amax; ai++) {
        ii = airArrayLenIncr(arr, 1);
        kern[ii] = nrrdKernelTMF[di+1][ci+1][ai];
      }
    }
  }

  /* NULL-terminate the list */
  ii = airArrayLenIncr(arr, 1); kern[ii] = NULL;
  /* nix, not nuke the airArray */
  airArrayNix(arr);
  return kern;
}

/*
** Does more than call nrrdKernelCheck on all kernels:
** makes sure that all kernels have unique names
** makes sure that derivative relationships are correct
** Also, simply calling nrrdKernelCheck requires some knowledge 
** of the kernel's needed parameters
*/
int
meetNrrdKernelAllCheck(void) {
  static const char me[]="meetNrrdKernelAllCheck";
  const NrrdKernel **kern, *kk, *ll;
  unsigned int ki, kj, pnum;
  airArray *mop;
  double step, epsl, XX, YY,
    parm0[NRRD_KERNEL_PARMS_NUM], parm1_0[NRRD_KERNEL_PARMS_NUM], 
    parm1_1[NRRD_KERNEL_PARMS_NUM], parm1_X[NRRD_KERNEL_PARMS_NUM],
    parm[NRRD_KERNEL_PARMS_NUM];
  size_t evalNum;
  int EE;

  mop = airMopNew();
  kern = meetNrrdKernelAll();
  airMopAdd(mop, AIR_CAST(void*, kern), airFree, airMopAlways);
  evalNum = 120000; /* success of kernel integral test is surprisingly
                       dependent on this, likely due to the naive way
                       the integral is numerically computed; the current
                       value here represents some experimentation */
  epsl = 0.9e-5;
  XX = 7.0/3.0;  /* 2.333.. */
  YY = 43.0/9.0; /* 4.777.. */
  parm0[0] = AIR_NAN; /* shouldn't be read */
  parm1_0[0] = 0.0;
  parm1_1[0] = 1.0;
  parm1_X[0] = XX;
  ki = 0;
  while ((kk = kern[ki])) {
    kj = 0;
    while (kj < ki) {
      ll = kern[kj];
      if (kk == ll) {
        biffAddf(MEET, "%s: kern[%u] and [%u] were identical (%s)", 
                 me, kj, ki, kk->name);
        airMopError(mop); return 1;
      }
      if (!airStrcmp(kk->name, ll->name)) {
        biffAddf(MEET, "%s: kern[%u] and [%u] have same name (%s)", 
                 me, kj, ki, kk->name);
        airMopError(mop); return 1;
      }
      kj++;
    }
    pnum = kk->numParm;
    EE = 0;
    /* the second argument to CHECK is how much to scale up the
       permissible error in kernel evaluations (between float and double)
       The kernels for which this is higher should be targets for
       re-coding with an eye towards numerical accuracy */
#define CHECK(P, S)                                                     \
    if (!EE) EE |= nrrdKernelCheck(kk, (P), evalNum, epsl*(S), NULL, NULL);
    if (nrrdKernelBCCubic == kk ||
        nrrdKernelBCCubicD == kk ||
        nrrdKernelBCCubicDD == kk) {
      /* try a few settings of the 3 parms */
      ELL_3V_SET(parm, 1.0, 0.0, 0.0); CHECK(parm, 1);
      ELL_3V_SET(parm, XX, 0.0, 0.0); CHECK(parm, 1);
      ELL_3V_SET(parm, 1.0, 1.0/3.0, 1.0/3.0); CHECK(parm, 1);
      ELL_3V_SET(parm, XX, 1.0/3.0, 1.0/3.0); CHECK(parm, 1);
      ELL_3V_SET(parm, 1.0, 0.0, 1.0); CHECK(parm, 1);
      ELL_3V_SET(parm, XX, 0.0, 1.0); CHECK(parm, 1);
      ELL_3V_SET(parm, 1.0, 0.5, 0.0); CHECK(parm, 1);
      ELL_3V_SET(parm, XX, 0.5, 0.0); CHECK(parm, 1);
    } else if (2 == pnum) {
      if (nrrdKernelAQuartic == kk ||
          nrrdKernelAQuarticD == kk ||
          nrrdKernelAQuarticDD == kk) {
        ELL_2V_SET(parm, 1.0, 0.0); CHECK(parm, 10);
        ELL_2V_SET(parm, 1.0, 0.5); CHECK(parm, 10);
        ELL_2V_SET(parm, XX, 0.0);  CHECK(parm, 10);
        ELL_2V_SET(parm, XX, 0.5);  CHECK(parm, 10);
      } else if (nrrdKernelGaussian == kk ||
                 nrrdKernelGaussianD == kk ||
                 nrrdKernelGaussianDD == kk) {
        ELL_2V_SET(parm, 0.1, XX); CHECK(parm, 10);
        ELL_2V_SET(parm, 0.1, YY); CHECK(parm, 10);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 10);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 10);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 10);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 10);
      } else if (nrrdKernelHann == kk ||
                 nrrdKernelHannD == kk ||
                 nrrdKernelBlackman == kk) {
        ELL_2V_SET(parm, 0.5, XX); CHECK(parm, 100);
        ELL_2V_SET(parm, 0.5, YY); CHECK(parm, 100);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 100);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 100);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 100);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 100);
      } else if (nrrdKernelHannDD == kk ||
                 nrrdKernelBlackmanD == kk ||
                 nrrdKernelBlackmanDD == kk) {
        /* there are apparently bugs in these kernels */
        ELL_2V_SET(parm, 0.5, XX); CHECK(parm, 10000000);
        ELL_2V_SET(parm, 0.5, YY); CHECK(parm, 10000000);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 1000000);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 1000000);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 100000);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 100000);
      } else if (nrrdKernelDiscreteGaussian == kk) {
        ELL_2V_SET(parm, 0.1, XX); CHECK(parm, 1);
        ELL_2V_SET(parm, 0.1, YY); CHECK(parm, 1);
        ELL_2V_SET(parm, 1.0, XX); CHECK(parm, 1);
        ELL_2V_SET(parm, 1.0, YY); CHECK(parm, 1);
        ELL_2V_SET(parm, XX, XX);  CHECK(parm, 1);
        ELL_2V_SET(parm, XX, YY);  CHECK(parm, 1);
      }
    } else if (1 == pnum) {
      if (strstr(kk->name, "TMF")) {
        /* these take a single parm, but its not support */
        parm[0] = 0.0;     CHECK(parm, 10);
        parm[0] = 1.0/3.0; CHECK(parm, 10);
      } else {
        /* zero, box, boxsup, cos4sup{,D,DD,DDD}, cheap,
           tent, fordif, cendif, 
           C3Quintic{,D,DD,DD}, C4Hexic{,D,DD,DDD} */
        /* takes a single support/scale parm[0], try two different values */
        if (nrrdKernelCos4SupportDebug == kk ||
            nrrdKernelCos4SupportDebugD == kk ||
            nrrdKernelCos4SupportDebugDD == kk ||
            nrrdKernelCos4SupportDebugDDD == kk) {
          CHECK(parm1_1, 10);
          CHECK(parm1_X, 10);
        } else if (nrrdKernelC3Quintic == kk ||
                   nrrdKernelC3QuinticD == kk ||
                   nrrdKernelC3QuinticDD == kk ||
                   nrrdKernelC4Hexic == kk ||
                   nrrdKernelC4HexicD == kk ||
                   nrrdKernelC4HexicDD == kk ||
                   nrrdKernelC4HexicDDD == kk
                   ) {
          CHECK(parm1_1, 1);
          CHECK(parm1_X, 1);
        } else {
          CHECK(parm1_1, 1);
          CHECK(parm1_X, 1);
        }
      }
    } else if (0 == pnum) {
      /* hermiteSS, catmull-rom{,D}, bspl{3,5,7}{,D,DD,DDD} */
      if (nrrdKernelBSpline5DD == kk ||
          nrrdKernelBSpline5DDD == kk ||
          nrrdKernelBSpline7DD == kk ) {
        CHECK(parm0, 100);
      } else {
        CHECK(parm0, 10);
      }
    } else {
      biffAddf(MEET, "%s: sorry, didn't expect %u parms for %s",
               me, pnum, kk->name);
      airMopError(mop); return 1;
    }
    if (EE) {
      biffMovef(MEET, NRRD, "%s: problem with kern[%u] \"%s\"", me, ki,
                kk->name ? kk->name : "(NULL name)");
      airMopError(mop); return 1;
    }
    ki++;
#undef CHECK
  }

  airMopOkay(mop);
  return 0;
}
