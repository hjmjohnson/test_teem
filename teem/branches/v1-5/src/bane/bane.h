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

#if defined(_WIN32) && !defined(TEEM_STATIC)
#define bane_export __declspec(dllimport)
#else
#define bane_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define BANE baneBiffKey

/*
** The idea is that the baneRange, baneInc, baneClip, and baneMeasr,
** structs, and the pointers to them declared below, are glorified
** constants.  There is certainly nothing dynamically allocated in any
** of them.  All the parameters to these things (the various double
** *xxxParm arrays) are kept in things which are dynamically allocated
** by the user.
*/

/* -------------------------- ranges -------------------------- */

/*
******** baneRange..._e enum
**
** Range: nature of the values generated by a measure- are they
** strictly positive (such as gradient magnitude), should they be
** considered to be centered around zero (2nd directional
** derivative) or could they be anywhere (data
** value).
**
** The job of the ans() function in the range is not to exclude
** any data.  Indeed, if the range is set correctly for the type
** of data used, then range->ans() should always return a range
** that is as large or larger than the one which was passed.  
** Doing otherwise would make ranges too complicated (such as
** requiring a parm array), and besides, its the job of the
** inclusion methods to be smart about things like this.
*/
enum {
  baneRangeUnknown_e,  /* 0: nobody knows */
  baneRangePos_e,      /* 1: always positive: enforce that min == 0 */
  baneRangeNeg_e,      /* 2: always negative: enforce that max == 0 */
  baneRangeZeroCent_e, /* 3: positive and negative, centered around
			     zero: enforce (conservative) centering of
			     interval around 0 */
  baneRangeFloat_e,   /*  4: anywhere: essentially a no-op */
  baneRangeLast_e
};
#define BANE_RANGE_MAX    4
/*
******** baneRange struct
**
** things used to operate on ranges
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int which;
  int (*ans)(double *ominP, double *omaxP,
	     double imin, double imax);
} baneRange;

/* -------------------------- inc -------------------------- */

/*
******** baneInc..._e enum
**
** Inc: methods for determing what range of measured values deserves
** to be included along one axes of a histogram volume.  Each
** inclusion method has some parameters (at most BANE_INC_PARM_NUM)
** which are (or can be harmlessly cast to) floats.  Some of them need
** a histogram (a Nrrd) in order to determine the new min and max,
** some just use a Nrrd as a place to store some information.
**
** To make matters confusing, the behavior of some of these varies with
** the baneRange they are associated with... 
*/
enum {
  baneIncUnknown_e,     /* 0: nobody knows */
  baneIncAbsolute_e,    /* 1: within explicitly specified bounds 
			   incParm[0]: new min
			   incParm[1]: new max */
  baneIncRangeRatio_e,  /* 2: some fraction of the total range
			   incParm[0]: scales the size of the range
			   after it has been sent through the
			   associated range function.
			   incParm[1]: (optional) for baneRangeFloat:
			   midpoint of scaling; if doesn't exist,
			   average of min and max is used.  For all
			   other range types, 0 is always used. */
  baneIncPercentile_e,  /* 3: exclude some percentile
			   incParm[0]: resolution of histogram
			   generated
			   incParm[1]: PERCENT of hits to throw away,
			   by nibbling away at lower and upper ends of
			   range, in a manner dependant on the range
			   type
			   incParm[2]: (optional) for baneRangeFloat,
			   center value that we nibble towards.  If it
			   doesn't exist, we use the average of the
			   min and max (though perhaps the mode value
			   would be better).  For all other range
			   types, we nibble towards 0. */
  baneIncStdv_e,        /* 4: some multiple of the standard deviation
			   incParm[0]: range is standard deviation
			   times this
			   incParm[1]: (optional) for baneRangeFloat:
			   if exists, the midpoint of the range,
			   otherwise the mean is used.  For all other
			   range types, the range is positioned in the
			   logical way. */
  baneIncLast_e
};
#define BANE_INC_MAX       4
#define BANE_INC_PARM_NUM 3

typedef void (baneIncPass)(Nrrd *hist, double val, double *incParm);

/*
******** baneInc struct
**
** things used to calculate and describe inclusion ranges.  The return
** from histNew should be eventually passed to nrrdNuke.
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int which;
  int numParm;           /* assumed length of incParm in this ans() */
  Nrrd *(*histNew)(double *incParm);
  baneIncPass *passA;
  baneIncPass *passB;
  int (*ans)(double *minP, double *maxP,
	     Nrrd *hist, double *incParm,
	     baneRange *range);
} baneInc;

/* -------------------------- clip -------------------------- */

/*
******** baneClip..._e enum
**
** Clip: how to map values in the "raw" histogram volume to the more
** convenient 8-bit version.  The number of hits for the semi-constant
** background of a large volume can be huge, so some scheme for dealing
** with this is needed.
*/
enum {
  baneClipUnknown_e,     /* 0: nobody knows */
  baneClipAbsolute_e,    /* 1: clip at explicitly specified bin count */
  baneClipPeakRatio_e,   /* 2: some fraction of maximum #hits in any bin */
  baneClipPercentile_e,  /* 3: percentile of values, sorted by hits */
  baneClipTopN_e,        /* 4: ignore the N bins with the highest counts */
  baneClipLast
};
#define BANE_CLIP_MAX       4
#define BANE_CLIP_PARM_NUM 1
/*
******** baneClip struct
**
** things used to calculate and describe clipping
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int which;
  int numParm;           /* assumed length of clipParm in this ans() */
  int (*ans)(Nrrd *hvol, double *clipParm);
} baneClip;

/* -------------------------- measr -------------------------- */

/*
******** baneMeasr..._e enum
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
** in gage.  Hence the measrParm arrays, even though nothing currently
** uses them.
*/
enum {
  baneMeasrUnknown_e,    /* 0: nobody knows */
  baneMeasrVal_e,        /* 1: the data value */
  baneMeasrGradMag_e,    /* 2: gradient magnitude */
  baneMeasrLapl_e,       /* 3: Laplacian */
  baneMeasrHess_e,       /* 4: Hessian-based measure of 2nd DD along
                               gradient */
  baneMeasrCurvedness_e, /* 5: L2 norm of K1, K2 principal curvatures
			      (gageSclCurvedness) */
  baneMeasrShapeTrace_e, /* 6: shape indicator (gageSclShapeTrace) */
  baneMeasrLast
};
#define BANE_MEASR_MAX      6
#define BANE_MEASR_PARM_NUM 1
/*
******** baneMeasr struct
**
** things used to calculate and describe measurements
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  int which;
  int numParm;           /* assumed length of measrParm in this ans() */
  int query;             /* the gageScl query needed for this measure.
			    Does not need to be the recursive
			    prerequisite expansion). */
  baneRange *range;
  float (*ans)(gage_t *, double *measrParm);
} baneMeasr;

/* -------------------- histogram volumes, etc. ---------------------- */

/*
******** baneAxis struct
** 
** Information for how to do measurement and inclusion along each axis
** of the histogram volume.
**
** No dynamically allocated stuff in here!!
*/
typedef struct {
  int res;                            /* resolution = number of bins */
  baneMeasr *measr;
  double measrParm[BANE_MEASR_PARM_NUM];
  baneInc *inc;
  double incParm[BANE_INC_PARM_NUM];
} baneAxis;

/*
******** baneHVolParm struct
** 
** Information for how to create a histogram volume.  NB: We have an
** array of baneAxis structs, not pointers to them.  baneHVolParmNew
** does initialization of them.
**
** No dynamically allocated stuff in here!!
*/
typedef struct {
  int verbose;                         /* status messages to stderr */
  int makeMeasrVol;                    /* create a 3 x X x Y x Z volume of
					  measurements, so that they aren't
					  measured (as many as) three times */
  Nrrd *measrVol;
  int measrVolDone;                    /* values in measrVol are filled */
  baneAxis ax[3];                      /* NB: not pointers to baneAxis */
  int k3pack;
  int renormalize;                     /* use gage's mask renormalization */
  NrrdKernel *k[GAGE_KERNEL_NUM];
  double kparm[GAGE_KERNEL_NUM][NRRD_KERNEL_PARMS_NUM];
  baneClip *clip;
  double clipParm[BANE_CLIP_PARM_NUM];
  double incLimit;                     /* lowest permissible fraction of the
					  data remaining after new inclusion
					  has been determined */
} baneHVolParm;

/* defaultsBane.c */
extern bane_export const char *baneBiffKey;
extern bane_export int baneDefVerbose;
extern bane_export int baneDefMakeMeasrVol;
extern bane_export float baneDefIncLimit;
extern bane_export int baneDefRenormalize;
extern bane_export int baneDefPercHistBins;
extern bane_export int baneStateHistEqBins;
extern bane_export int baneStateHistEqSmart;
extern bane_export int baneHack;

/* range.c */
extern bane_export baneRange *baneRangeUnknown;
extern bane_export baneRange *baneRangePos;
extern bane_export baneRange *baneRangeNeg;
extern bane_export baneRange *baneRangeZeroCent;
extern bane_export baneRange *baneRangeFloat;
extern bane_export baneRange *baneRangeArray[BANE_RANGE_MAX+1]; 

/* inc.c */
extern bane_export baneInc *baneIncUnknown;
extern bane_export baneInc *baneIncAbsolute;
extern bane_export baneInc *baneIncRangeRatio;
extern bane_export baneInc *baneIncPercentile;
extern bane_export baneInc *baneIncStdv;
extern bane_export baneInc *baneIncArray[BANE_INC_MAX+1];

/* measr.c */
extern bane_export baneMeasr *baneMeasrUnknown;
extern bane_export baneMeasr *baneMeasrVal;
extern bane_export baneMeasr *baneMeasrGradMag;
extern bane_export baneMeasr *baneMeasrLapl;
extern bane_export baneMeasr *baneMeasrHess;
extern bane_export baneMeasr *baneMeasrCurvedness;
extern bane_export baneMeasr *baneMeasrShadeTrace;
extern bane_export baneMeasr *baneMeasrArray[BANE_MEASR_MAX+1];

/* clip.c */
extern bane_export baneClip *baneClipUnknown;
extern bane_export baneClip *baneClipAbsolute;
extern bane_export baneClip *baneClipPeakRatio;
extern bane_export baneClip *baneClipPercentile;
extern bane_export baneClip *baneClipTopN;
extern bane_export baneClip *baneClipArray[BANE_CLIP_MAX+1];

/* methodsBane.c */
extern baneHVolParm *baneHVolParmNew();
extern void baneHVolParmGKMSInit(baneHVolParm *hvp);
extern baneHVolParm *baneHVolParmNix(baneHVolParm *hvp);

/* valid.c */
extern int baneInputCheck(Nrrd *nin, baneHVolParm *hvp);
extern int baneHVolCheck(Nrrd *hvol);
extern int baneInfoCheck(Nrrd *info2D, int wantDim);
extern int banePosCheck(Nrrd *pos, int wantDim);
extern int baneBcptsCheck(Nrrd *Bcpts);

/* hvol.c */
extern void baneProbe(double val[3],
		      Nrrd *nin, baneHVolParm *hvp, gageContext *ctx,
		      int x, int y, int z);
extern int baneFindInclusion(double min[3], double max[3], 
			     Nrrd *nin, baneHVolParm *hvp, gageContext *ctx);
extern int baneMakeHVol(Nrrd *hvol, Nrrd *nin, baneHVolParm *hvp);
extern Nrrd *baneGKMSHVol(Nrrd *nin, float gradPerc, float hessPerc);

/* trnsf.c */
extern int baneOpacInfo(Nrrd *info, Nrrd *hvol, int dim, int measr);
extern int bane1DOpacInfoFrom2D(Nrrd *info1D, Nrrd *info2D);
extern int baneSigmaCalc(float *sP, Nrrd *info);
extern int banePosCalc(Nrrd *pos, float sigma, float gthresh, Nrrd *info);
extern void _baneOpacCalcA(int lutLen, float *opacLut, 
			   int numCpts, float *xo,
			   float *pos);
extern void _baneOpacCalcB(int lutLen, float *opacLut, 
			   int numCpts, float *x, float *o,
			   float *pos);
extern int baneOpacCalc(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos);

/* trex.c */
extern float *_baneTRexRead(char *fname);
extern void _baneTRexDone();

/* scat.c */
extern int baneRawScatterplots(Nrrd *nvg, Nrrd *nvh, Nrrd *hvol, int histEq);

/* gkms{Flotsam,Hvol,Scat,Pvg,Opac,Mite}.c */
#define BANE_GKMS_DECLARE(C) extern bane_export unrrduCmd baneGkms_##C##Cmd;
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
extern bane_export airEnum *baneGkmsMeasr;
extern bane_export unrrduCmd *baneGkmsCmdList[]; 
extern void baneGkmsUsage(char *me, hestParm *hparm);
extern bane_export hestCB *baneGkmsHestIncStrategy;
extern bane_export hestCB *baneGkmsHestBEF;
extern bane_export hestCB *baneGkmsHestGthresh;

#ifdef __cplusplus
}
#endif

#endif /* BANE_HAS_BEEN_INCLUDED */
