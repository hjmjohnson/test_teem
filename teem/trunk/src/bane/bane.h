/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/


#ifndef BANE_HAS_BEEN_INCLUDED
#define BANE_HAS_BEEN_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <air.h>
#include <biff.h>
#include <nrrd.h>

#define BANE_SMALL_STRLEN 65
#define BANE "bane"

/*
******** baneRange enum
******** baneRangeStr[][]
**
** Range: nature of the values generated by a measure- are they strictly
** positive (such as gradient magnitude), should they be considered to
** be centered around zero (2nd directional derivative) or could they
** be anywhere (data value)
*/
#define BANE_RANGE_MAX 4
typedef enum {
  baneRangeUnknown,    /* 0: nobody knows */
  baneRangePos,        /* 1: always positive */
  baneRangeNeg,        /* 2: always negative */
  baneRangeZeroCent,   /* 3: positive and negative, centered around zero */
  baneRangeFloat,      /* 4: anywhere */
  baneRangeLast
} baneRanges;
extern char baneRangeStr[][BANE_SMALL_STRLEN];
typedef void (*baneRangeType)(double *minP, double *maxP, 
			      double min, double max);
extern baneRangeType baneRange[BANE_RANGE_MAX+1];

/*
******** baneMeasr enum
******** baneMeasrStr[][]
******** baneMeasrRange[]
******** baneMeasrMargin[]
**
** Measr: one of the kind of measurement which determines location along
** one of the axes of the histogram volume.  Each measure has an associated
** baneRange as indicated by baneMeasrRange[].  Some measurements can't
** be taken on the boundary of the volume, baneMeasrMargin[] tells how
** many voxels in you have to be before the measurement can safely happen
*/
#define BANE_MEASR_MAX 5
typedef enum {
  baneMeasrUnknown,    /* 0: nobody knows */
  baneMeasrVal,        /* 1: the data value */
  baneMeasrGradMag_cd, /* 2: gradient magnitude from central differences */
  baneMeasrLapl_cd,    /* 3: Laplacian, 2nd central differences */
  baneMeasrHess_cd,    /* 4: Hessian-based measure of 2nd DD along gradient
			  using central differences */
  baneMeasrGMG_cd,     /* 5: gradient of magnitude of gradient,
			  using central differences */
  baneMeasrLast
} baneMeasrs;
extern char baneMeasrStr[][BANE_SMALL_STRLEN];
extern int baneMeasrRange[BANE_MEASR_MAX+1];
extern int baneMeasrMargin[BANE_MEASR_MAX+1];
typedef double (*baneMeasrType)(Nrrd *n, NRRD_BIG_INT idx);
extern baneMeasrType baneMeasr[BANE_MEASR_MAX+1];

/*
******** baneInc enum
******** baneIncStr[][]
******** baneIncNeedsHisto
**
** Inc: methods for determing what range of measurement values deserves
** to be included in the histogram volume.  Each inclusion method has
** some parameters (at most BANE_INC_NUM_PARM) which are (or can be
** harmlessly cast to) floats.  Some of them need a histogram in order
** to determing the new min and max, as indicated by baneIncNeedsHisto[]
*/
#define BANE_INC_NUM_PARM 3
#define BANE_INC_MAX 4
typedef enum {
  baneIncUnknown,      /* 0: nobody knows */
  baneIncAbsolute,     /* 1: within explicitly specified bounds */
  baneIncRangeRatio,   /* 2: some fraction of the total range */
  baneIncPercentile,   /* 3: exclude some percentile */
  baneIncStdv,         /* 4: some multiple of the standard deviation */
  baneIncLast
} baneIncs;
extern char baneIncStr[BANE_INC_MAX+1][BANE_SMALL_STRLEN];
extern int baneIncNumParm[BANE_INC_MAX+1];
extern Nrrd *(*baneIncNrrd[BANE_INC_MAX+1])(double *parm);
typedef void (*baneIncInitType)(Nrrd *n, double val);
extern baneIncInitType baneIncInitA[BANE_INC_MAX+1];
extern baneIncInitType baneIncInitB[BANE_INC_MAX+1];
typedef void (*baneIncType)(double *minP, double *maxP,
			    Nrrd *n, double *parm, int range);
extern baneIncType baneInc[BANE_INC_MAX+1];

/*
******** baneClip enum
******** bandClipStr[][]
**
** Clip: how to map values in the "raw" histogram volume to the more
** convenient 8-bit version.  The number of hits for the semi-constant
** background of a large volume can be huge, so some scheme for dealing
** with this is needed.
*/
#define BANE_CLIP_NUM_PARM 1
#define BANE_CLIP_MAX 4
typedef enum {
  baneClipUnknown,     /* 0: nobody knows */
  baneClipAbsolute,    /* 1: clip at explicitly specified bin count */
  baneClipPeakRatio,   /* 2: some fraction of maximum #hits in any bin */
  baneClipPercentile,  /* 3: percentile of values, sorted by hits */
  baneClipTopN,        /* 4: ignore the N bins with the highest counts */
  baneClipLast
} baneClips;
extern char baneClipStr[][BANE_SMALL_STRLEN];
extern int baneClipNumParm[BANE_CLIP_MAX+1];
typedef int (*baneClipType)(Nrrd *, double *parm);
extern baneClipType baneClip[BANE_CLIP_MAX+1];

/*
******** baneMeasrParm struct
** 
** Information for how to do measurement along each axis of the
** histogram volume.  
**
** No dynamically allocated stuff in here
*/
typedef struct {
  int res,                             /* resolution = number of bins */
    measr,                             /* from baneMeasrs enum */
    inc;                               /* from baneIncs enum */
  double incParm[BANE_INC_NUM_PARM];   /* parameters for inclusion method */
} baneMeasrParm;

/*
******** baneHVolParm struct
** 
** Information for how to create a histogram volume
**
*/
typedef struct {
  int verb;                            /* status messages to stderr */
  baneMeasrParm axp[3];                /* parameters for axes' measurement */
  baneClips clip;                      /* how to clip hit counts in hvol */
  double clipParm[BANE_CLIP_NUM_PARM], /* parameter(s) to clip method */
    incLimit;                          /* lowest permissible fraction of the
					  data remaining after new inclusion
					  has been determined */
} baneHVolParm;

/*
******** BANE_DEF_INCLIMIT
**
** the default minimum percentage of voxels which must be included in
** the histogram volume volume (baneHVolParm's incLimit is set to 100
** times this by default).  If the inclusion is set such that that you
** get less than this percentage of hits, then baneMakeHVol() will
** fail 
*/
#define BANE_DEF_INCLIMIT 80

typedef struct {
  nrrdKernel *k[3];
  float param[3][NRRD_MAX_KERNEL_PARAMS];
} baneProbeK3Pack;

/* methods.c */
extern baneHVolParm *baneHVolParmNew();
extern baneHVolParm *baneHVolParmNix(baneHVolParm *hvp);
extern void baneHVolParmGKMSInit(baneHVolParm *hvp);
extern baneProbeK3Pack *baneProbeK3PackNew();
extern baneProbeK3Pack *baneProbeK3PackNix(baneProbeK3Pack *);

/* hvol.c */
extern int baneMakeHVol(Nrrd *hvol, Nrrd *nin, baneHVolParm *hvp);
extern int baneApplyMeasr(Nrrd *nout, Nrrd *nin, int measr);
extern Nrrd *baneGKMSHVol(Nrrd *nin, float perc);

/* probe.c */
/* 
** obviously the baneProbes enum, the BANE_PROBE_* #defines, and 
** the baneProbAnsLen, baneProbeAnsOffset arrays (in probe.c) 
** MUST be kept in sync 
*/
#define BANE_PROBE_MAX 11
#define BANE_PROBE_ANSLEN 40
typedef enum {
  baneProbeVal,          /* 0: data value (*float) */
  baneProbeGradVec,      /* 1: gradient vector, un-normalized (float[3])*/
  baneProbeGradMag,      /* 2: gradient magnitude (*float) */
  baneProbeNormal,       /* 3: gradient vector, normalized (float[3]) */
  baneProbeHess,         /* 4: Hessian (float[9]) */
  baneProbeHessEval,     /* 5: Hessian's eigenvalues (float[3]) */
  baneProbeHessEvec,     /* 6: Hessian's eigenvectors (float[9]) */
  baneProbe2ndDD,        /* 7: 2nd dir.deriv. along gradient (*float) */
  baneProbeCurvVecs,     /* 8: principle curvature directions (float[6]) */
  baneProbeK1K2,         /* 9: principle curvature magnitudes (float[2]) */
  baneProbeShapeIndex,   /* 10: Koen.'s shape index, (S") (*float) */
  baneProbeCurvedness    /* 11: L2 norm of K1, K2 (not Koen.'s "C") (*float) */
} baneProbes;
#define BANE_PROBE_VAL        (1<<0)
#define BANE_PROBE_GRADVEC    (1<<1)
#define BANE_PROBE_GRADMAG    (1<<2)
#define BANE_PROBE_NORMAL     (1<<3)
#define BANE_PROBE_HESS       (1<<4)
#define BANE_PROBE_HESSEVAL   (1<<5)
#define BANE_PROBE_HESSEVEC   (1<<6)
#define BANE_PROBE_2NDDD      (1<<7)
#define BANE_PROBE_CURVVECS   (1<<8)
#define BANE_PROBE_K1K2       (1<<9)
#define BANE_PROBE_SHAPEINDEX (1<<10)
#define BANE_PROBE_CURVEDNESS (1<<11)
extern int baneProbeAnsLen[BANE_PROBE_MAX+1];
extern int baneProbeAnsOffset[BANE_PROBE_MAX+1];
extern void baneProbe(float *ans, Nrrd *nin, int query,
		      baneProbeK3Pack *pack,
		      float x, float y, float z);
extern int baneProbeDebug;

/* valid.c */
extern int baneValidHVol(Nrrd *hvol);
extern int baneValidInfo2D(Nrrd *info2D);
extern int baneValidInfo1D(Nrrd *info1D);
extern int baneValidPos1D(Nrrd *pos1D);
extern int baneValidPos2D(Nrrd *pos2D);
extern int baneValidBcpts(Nrrd *Bcpts);

/* trnsf.c */
extern int bane2DOpacInfo(Nrrd *info2D, Nrrd *hvol);
extern int bane1DOpacInfo(Nrrd *info1D, Nrrd *hvol);
extern int bane1DOpacInfoFrom2D(Nrrd *info1D, Nrrd *info2D);
extern int baneSigmaCalc1D(float *sP, Nrrd *info1D);
extern int baneSigmaCalc2D(float *sP, Nrrd *info2D);
extern int banePosCalc1D(Nrrd *pos1D, 
			 float sigma, float gthresh, Nrrd *info1D);
extern int banePosCalc2D(Nrrd *pos2D, 
			 float sigma, float gthresh, Nrrd *info2D);
extern int baneOpacCalc1Dcpts(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos1D);
extern void _baneOpacCalcA(int lutLen, float *opacLut, 
			   int numCpts, float *xo,
			   float *pos);
extern void _baneOpacCalcB(int lutLen, float *opacLut, 
			   int numCpts, float *x, float *o,
			   float *pos);
extern int baneOpacCalc2Dcpts(Nrrd *opac, Nrrd *Bcpts, Nrrd *pos2D);

/* trex.c */
extern float *_baneTRexRead(char *fname);
extern void _baneTRexDone();


#ifdef __cplusplus
}
#endif
#endif /* BANE_HAS_BEEN_INCLUDED */
