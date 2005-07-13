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

#ifndef BANE_HAS_BEEN_INCLUDED
#define BANE_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/nrrd.h>
#include <teem/unrrdu.h>
#include <teem/gage.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BANE baneBiffKey

/*
******** #define BANE_PARM_NUM
**
** maximum number of parameters that may be needed by a baneInc, baneClip...
*/
#define BANE_PARM_NUM 5


/* -------------------------- ranges -------------------------- */

/*
******** baneRange... enum
**
** Range: nature of the values generated by a measure- are they
** strictly positive (such as gradient magnitude), should they be
** considered to be centered around zero (2nd directional
** derivative) or could they be anywhere (data
** value).
**
** The job of the answer() function in the range is not to exclude
** any data.  Indeed, if the range is set correctly for the type
** of data used, then range->ans() should always return a range
** that is as large or larger than the one which was passed.  
** Doing otherwise would make ranges too complicated (such as
** requiring a parm array), and besides, its the job of the
** inclusion methods to be smart about things like this.
*/
enum {
  baneRangeUnknown,      /* 0: nobody knows */
  baneRangePositive,     /* 1: always positive: enforce that min == 0 */
  baneRangeNegative,     /* 2: always negative: enforce that max == 0 */
  baneRangeZeroCentered, /* 3: positive and negative, centered around
                            zero: enforce (conservative) centering of
                            interval around 0 */
  baneRangeAnywhere,     /* 4: anywhere: essentially a no-op */
  baneRangeLast
};

/*
******** baneRange struct
**
** things used to operate on ranges
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int type;
  double center;  /* for baneRangeAnywhere: nominal center of value range
                     NOTE: there is currently no API for setting this,
                     it has to be set manually */
  int (*answer)(double *ominP, double *omaxP,
                double imin, double imax);
} baneRange;

/* -------------------------- inc -------------------------- */

/*
******** baneInc... enum
**
** Inc: methods for determing what range of measured values deserves
** to be included along one axes of a histogram volume.  Each
** inclusion method has some parameters (at most BANE_PARM_NUM)
** which are (or can be harmlessly cast to) floats.  Some of them need
** a histogram (a Nrrd) in order to determine the new min and max,
** some just use a Nrrd as a place to store some information.
**
** To make matters confusing, the behavior of some of these varies with
** the baneRange they are associated with... 
**
** As version 1.7, the incParm[] is no longer used to communicate the
** center of a baneRangeAnywhere- that is now (as it should be) in
** the baneRange itself.
*/
enum {
  baneIncUnknown,     /* 0: nobody knows */
  baneIncAbsolute,    /* 1: within explicitly specified bounds 
                         -- incParm[0]: new min
                         -- incParm[1]: new max */
  baneIncRangeRatio,  /* 2: some fraction of the total range
                         -- incParm[0]: scales the size of the range, after
                            it has been sent through the associated range
                            function. */
  baneIncPercentile,  /* 3: exclude some percentile
                         -- incParm[0]: resolution of histogram generated
                         -- incParm[1]: PERCENT of hits to throw away,
                            by nibbling away at lower and upper ends of
                            range, in a manner dependant on the range type */
  baneIncStdv,        /* 4: some multiple of the standard deviation
                         -- incParm[0]: range is standard deviation 
                            times this */
  baneIncLast
};

/*
******** baneInc struct
**
** things used to calculate and describe inclusion ranges.  The return
** from histNew should be eventually passed to nrrdNuke.
*/
typedef struct baneInc_t {
  char name[AIR_STRLEN_SMALL];
  int type;
  double S, SS; int num;  /* used for calculating standard dev */
  Nrrd *nhist;
  baneRange *range;
  double parm[BANE_PARM_NUM];
  void (*process[2])(struct baneInc_t *inc, double val);
  int (*answer)(double *minP, double *maxP,
                Nrrd *hist, double *parm, baneRange *range);
} baneInc;

/* -------------------------- clip -------------------------- */

/*
******** baneClip... enum
**
** Clip: how to map values in the "raw" histogram volume to the more
** convenient 8-bit version.  The number of hits for the semi-constant
** background of a large volume can be huge, so some scheme for dealing
** with this is needed.
*/
enum {
  baneClipUnknown,     /* 0: nobody knows */
  baneClipAbsolute,    /* 1: clip at explicitly specified bin count */
  baneClipPeakRatio,   /* 2: some fraction of maximum #hits in any bin */
  baneClipPercentile,  /* 3: percentile of values, sorted by hits */
  baneClipTopN,        /* 4: ignore the N bins with the highest counts */
  baneClipLast
};

/*
******** baneClip struct
**
** things used to calculate and describe clipping
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int type;
  double parm[BANE_PARM_NUM];
  int (*answer)(int *countP, Nrrd *hvol, double *clipParm);
} baneClip;

/* -------------------------- measr -------------------------- */

/*
******** baneMeasr... enum
**
** Measr: one of the kind of measurement which determines location along
** one of the axes of the histogram volume.
**
** In this latest version of bane (1.4), I nixed the "gradient of magnitude
** of gradient" (GMG) based measure of the second directional
** derivative.  I felt that the benefits of using gage for value and
** derivative measurement (allowing arbitrary kernels), combined with
** the fact that doing GMG can't be done directly in gage (because its
** a derivative of pre-computed derivatives), outweighed the loss of
** GMG.  Besides, according to Appendix C of my Master's thesis the
** only thing its really good at avoiding is quantization noise in
** 8-bit data, but gage isn't limited to 8-bit data anyway.
**
** The reason for not simply using the pre-defined gageScl values is
** that eventually I'll want to do things which modify/combine those
** values in a parameter-controlled way, something which will never be
** in gage.  Hence the parm array, even though nothing currently
** uses them.
*/
enum {
  baneMeasrUnknown,           /* 0: nobody knows */
  baneMeasrValuePositive,     /* 1: the data value, with positive range
                                 (gageSclValue) */
  baneMeasrValueZeroCentered, /* 2: the data value, with zero-centered range
                                 (gageSclValue) */
  baneMeasrValueAnywhere,     /* 3: the data value, with anywhere range
                                 (gageSclValue) */
  baneMeasrGradMag,           /* 4: gradient magnitude (gageSclGradMag) */
  baneMeasrLaplacian,         /* 5: Laplacian (gageSclLaplacian) */
  baneMeasr2ndDD,             /* 6: Hessian-based measure of 2nd DD along
                                 gradient (gageScl2ndDD) */
  baneMeasrTotalCurv,         /* 7: L2 norm of K1, K2 principal curvatures
                                 (gageSclTotalCurv) */
  baneMeasrFlowlineCurv,      /* 8: curvature of normal streamline
                                 (gageSclFlowlineCurv) */
  baneMeasrLast
};

/*
******** baneMeasr struct
**
** things used to calculate and describe measurements
*/
typedef struct baneMeasr_t {
  char name[AIR_STRLEN_SMALL];
  int type;
  double parm[BANE_PARM_NUM];
  gageQuery query;       /* the gageScl query needed for this measure,
                            but NOT its recursive prerequisite expansion). */
  baneRange *range;
  int offset0;
  double (*answer)(struct baneMeasr_t *, gage_t *, double *parm);
} baneMeasr;

/* -------------------- histogram volumes, etc. ---------------------- */

/*
******** baneAxis struct
** 
** Information for how to do measurement and inclusion along each axis
** of the histogram volume.
*/
typedef struct {
  unsigned int res;                     /* resolution = number of bins */
  baneMeasr *measr;
  baneInc *inc;
} baneAxis;

/*
******** baneHVolParm struct
** 
** Information for how to create a histogram volume.
**
*/
typedef struct {
  /* -------------- input */
  int verbose,                         /* status messages to stderr */
    makeMeasrVol,                      /* create a 3 x X x Y x Z volume of
                                          measurements, so that they aren't
                                          measured (as many as) three times */
    renormalize,                       /* use gage's mask renormalization */
    k3pack;        
  const NrrdKernel *k[GAGE_KERNEL_NUM];
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
  baneClip *clip;
  double incLimit;                     /* lowest permissible fraction of the
                                          data remaining after new inclusion
                                          has been determined */
  baneAxis axis[3];
  /* -------------- internal */
  Nrrd *measrVol;
  int measrVolDone;                    /* values in measrVol are filled */
} baneHVolParm;

/* defaultsBane.c */
TEEM_API const char *baneBiffKey;
TEEM_API int baneDefVerbose;
TEEM_API int baneDefMakeMeasrVol;
TEEM_API double baneDefIncLimit;
TEEM_API int baneDefRenormalize;
TEEM_API int baneDefPercHistBins;
TEEM_API int baneStateHistEqBins;
TEEM_API int baneStateHistEqSmart;
TEEM_API int baneHack;

/* rangeBane.c */
TEEM_API baneRange *baneRangeNew(int type);
TEEM_API baneRange *baneRangeCopy(baneRange *range);
TEEM_API int baneRangeAnswer(baneRange *range,
                             double *ominP, double *omaxP,
                             double imin, double imax);
TEEM_API baneRange *baneRangeNix(baneRange *range);

/* inc.c */
TEEM_API baneInc *baneIncNew(int type, baneRange *range, double *parm);
TEEM_API void baneIncProcess(baneInc *inc, int passIdx, double val);
TEEM_API int baneIncAnswer(baneInc *inc, double *minP, double *maxP);
TEEM_API baneInc *baneIncCopy(baneInc *inc);
TEEM_API baneInc *baneIncNix(baneInc *inc);

/* clip.c */
TEEM_API baneClip *baneClipNew(int type, double *parm);
TEEM_API int baneClipAnswer(int *countP, baneClip *clip, Nrrd *hvol);
TEEM_API baneClip *baneClipCopy(baneClip *clip);
TEEM_API baneClip *baneClipNix(baneClip *clip);

/* measr.c */
TEEM_API baneMeasr *baneMeasrNew(int type, double *parm);
TEEM_API double baneMeasrAnswer(baneMeasr *measr, gageContext *gctx);
TEEM_API baneMeasr *baneMeasrCopy(baneMeasr *measr);
TEEM_API baneMeasr *baneMeasrNix(baneMeasr *measr);

/* methodsBane.c */
/* NOTE: this is NOT a complete API, like gage has.  Currently there
   is only API for things that have to be allocated internally */
TEEM_API baneHVolParm *baneHVolParmNew();
TEEM_API void baneHVolParmGKMSInit(baneHVolParm *hvp);
TEEM_API void baneHVolParmAxisSet(baneHVolParm *hvp, unsigned int axisIdx,
                                  unsigned int res,
                                  baneMeasr *measr, baneInc *inc);
TEEM_API void baneHVolParmClipSet(baneHVolParm *hvp, baneClip *clip);
TEEM_API baneHVolParm *baneHVolParmNix(baneHVolParm *hvp);

/* valid.c */
TEEM_API int baneInputCheck(Nrrd *nin, baneHVolParm *hvp);
TEEM_API int baneHVolCheck(Nrrd *hvol);
TEEM_API int baneInfoCheck(Nrrd *info2D, int wantDim);
TEEM_API int banePosCheck(Nrrd *pos, int wantDim);
TEEM_API int baneBcptsCheck(Nrrd *Bcpts);

/* hvol.c */
TEEM_API void baneProbe(double val[3],
                        Nrrd *nin, baneHVolParm *hvp, gageContext *ctx,
                        unsigned int x, unsigned int y, unsigned int z);
TEEM_API int baneFindInclusion(double min[3], double max[3], 
                               Nrrd *nin, baneHVolParm *hvp, gageContext *ctx);
TEEM_API int baneMakeHVol(Nrrd *hvol, Nrrd *nin, baneHVolParm *hvp);
TEEM_API Nrrd *baneGKMSHVol(Nrrd *nin, float gradPerc, float hessPerc);

/* trnsf.c */
TEEM_API int baneOpacInfo(Nrrd *info, Nrrd *hvol, int dim, int measr);
TEEM_API int bane1DOpacInfoFrom2D(Nrrd *info1D, Nrrd *info2D);
TEEM_API int baneSigmaCalc(float *sP, Nrrd *info);
TEEM_API int banePosCalc(Nrrd *pos, float sigma, float gthresh, Nrrd *info);
TEEM_API void _baneOpacCalcA(unsigned int lutLen, float *opacLut, 
                             unsigned int numCpts, float *xo,
                             float *pos);
TEEM_API void _baneOpacCalcB(unsigned int lutLen, float *opacLut, 
                             unsigned int numCpts, float *x, float *o,
                             float *pos);
TEEM_API int baneOpacCalc(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos);

/* trex.c */
TEEM_API float *_baneTRexRead(char *fname);
TEEM_API void _baneTRexDone();

/* scat.c */
TEEM_API int baneRawScatterplots(Nrrd *nvg, Nrrd *nvh, Nrrd *hvol, int histEq);

/* gkms{Flotsam,Hvol,Scat,Pvg,Opac,Mite}.c */
#define BANE_GKMS_DECLARE(C) TEEM_API unrrduCmd baneGkms_##C##Cmd;
#define BANE_GKMS_LIST(C) &baneGkms_##C##Cmd,
#define BANE_GKMS_MAP(F) \
F(hvol) \
F(scat) \
F(info) \
F(pvg) \
F(opac) \
F(mite) \
F(txf)
BANE_GKMS_MAP(BANE_GKMS_DECLARE)
TEEM_API airEnum *baneGkmsMeasr;
TEEM_API unrrduCmd *baneGkmsCmdList[]; 
TEEM_API void baneGkmsUsage(char *me, hestParm *hparm);
TEEM_API hestCB *baneGkmsHestIncStrategy;
TEEM_API hestCB *baneGkmsHestBEF;
TEEM_API hestCB *baneGkmsHestGthresh;

#ifdef __cplusplus
}
#endif

#endif /* BANE_HAS_BEEN_INCLUDED */
