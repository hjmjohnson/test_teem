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

#ifndef GAGE_HAS_BEEN_INCLUDED
#define GAGE_HAS_BEEN_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <teem/air.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GAGE gageBiffKey

/*
** the only extent to which gage treats different axes differently is
** the spacing between samples along the axis.  To have different
** filters for the same function, but along different axes, would be
** too messy.  Thus, gage is not very useful as the engine for
** downsampling: it can't tell that along one axis samples should be
** blurred while they should be interpolated along another.  Rather,
** it assumes that the main task of probing is *reconstruction*: of
** values, of derivatives, or lots of different quantities
*/

/*
** terminology: gage is intended to measure multiple things at one
** point in a volume.  The SET of ALL the things that you want
** gage to measure is called the "QUERY".  Each of the many quantities
** comprising the query are called "ITEM"s.  
*/

/*
******** gage_t
** 
** this is the very crude means by which you can control the type
** of values that gage works with: "float" or "double".  It is an
** unfortunate but greatly simplifying restriction that this type
** is used for all types of probing (scalar, vector, etc).
**
** So: choose double by defining TEEM_GAGE_TYPE_DOUBLE and float
** otherwise.
*/

#ifdef TEEM_GAGE_TYPE_DOUBLE
typedef double gage_t;
#define gage_nrrdType nrrdTypeDouble
#define GAGE_TYPE_FLOAT 0
#else
typedef float gage_t;
#define gage_nrrdType nrrdTypeFloat
#define GAGE_TYPE_FLOAT 1
#endif

/*
******** GAGE_FD, GAGE_FR #defines
**
** these macros replace fields that used to be in the gageContext
*/
#define GAGE_FD(ctx) (2*((ctx)->havePad + 1))
#define GAGE_FR(ctx) ((ctx)->havePad + 1)

/*
******** GAGE_PERVOLUME_NUM
**
** max number of pervolumes that can be associated with a context.
** Since this is so often just 1, it makes no sense to adopt a more
** general mechanism to allow an unlimited number of pervolumes.
*/
#define GAGE_PERVOLUME_NUM 4

/*
******** gageParm... enum
**
** these are passed to gageSet.  Look for like-wise named field of
** gageParm for documentation on what these mean.
**
** The following things have to agree:
** - gageParm* enum
** - fields of gageParm struct
** - analagous gageDef* defaults (their declaration and setting)
** - action of gageParmSet
** - action of gageParmReset
*/
enum {
  gageParmUnknown,
  gageParmVerbose,                /* non-boolean int */
  gageParmRenormalize,            /* int */
  gageParmCheckIntegrals,         /* int */
  gageParmNoRepadWhenSmaller,     /* int */
  gageParmK3Pack,                 /* int */
  gageParmGradMagMin,             /* gage_t */
  gageParmGradMagCurvMin,         /* gage_t */
  gageParmDefaultSpacing,         /* gage_t */
  gageParmCurvNormalSide,         /* int */
  gageParmKernelIntegralNearZero, /* gage_t */
  gageParmRequireAllSpacings,     /* int */
  gageParmRequireEqualCenters,    /* int */
  gageParmDefaultCenter,          /* int */
  gageParmLast
};

/*
******** gage{Ctx,Pvl}Flag... enum
**
** organizes all the dependendies within a context and between a
** context and pervolumes.  Basically, all of this complexity is here
** to the handle the fact that different queries need different
** kernels, and different kernels have different supports, which
** requires different amounts of padding around volumes.  The user
** should not have to be concerned about any of this; it should be
** useful only to gageUpdate().
**
** A change to the "nixer" could never require re-padding the original
** volume, so it is not included as a flag below.
*/
enum {
  gageCtxFlagUnknown=-1,
  gageCtxFlagNeedD,      /*  0: derivatives required for query */
  gageCtxFlagK3Pack,     /*  1: whether to use 3 or 6 kernels */
  gageCtxFlagNeedK,      /*  2: which of the kernels will actually be used */
  gageCtxFlagKernel,     /*  3: any one of the kernels or its parameters */
  gageCtxFlagHavePad,    /*  4: padding we'll actually use (may not want to
                             repad when going to a smaller padding) */
  gageCtxFlagShape,      /*  5: a new pervolume shape was set */
  gageCtxFlagLast
};
#define GAGE_CTX_FLAG_NUM    6

enum {
  gagePvlFlagUnknown=-1,
  gagePvlFlagVolume,     /*  0: got a new volume */
  gagePvlFlagQuery,      /*  1: what do you really care about */
  gagePvlFlagNeedD,      /*  2: derivatives required for query */
  gagePvlFlagPadder,     /*  3: how to pad volumes */
  gagePvlFlagPadInfo,    /*  4: supplemental pad/nix info */
  gagePvlFlagLast
};
#define GAGE_PVL_FLAG_NUM    5
  

/*
******** gageKernel... enum
**
** these are the different kernels that might be used in gage, regardless
** of what kind of volume is being probed.
*/
enum {
  gageKernelUnknown=-1, /* -1: nobody knows */
  gageKernel00,         /*  0: measuring values */
  gageKernel10,         /*  1: reconstructing 1st derivatives */
  gageKernel11,         /*  2: measuring 1st derivatives */
  gageKernel20,         /*  3: reconstructing 1st partials and 2nd deriv.s */
  gageKernel21,         /*  4: measuring 1st partials for a 2nd derivative */
  gageKernel22,         /*  5: measuring 2nd derivatives */
  gageKernelLast
};
#define GAGE_KERNEL_NUM     6

#define GAGE_ITEM_PREREQ_NUM 5
/*
******** gageItemEntry struct
**
** necessary information about each item supported by the kind, which 
** is defined at compile-time in a per-kind table (at least it is for
** the scalar, vector, and tensor kinds)
*/
typedef struct {
  int enumVal,          /* the item's enum value */
    answerLength,       /* how many gage_t's are needed to store the answer */
    needDeriv,          /* what kind of derivative info is immediately needed
                           for this item (not recursively expanded). This is
                           NO LONGER a bitvector: values are 0, 1, 2, ... */
    prereq[GAGE_ITEM_PREREQ_NUM],
                        /* what are the other items this item depends on
                           (not recusively expanded).  The move away from
                           bit-vectors based on primitive types is what
                           necessitates conveying this with a stupid little
                           array. */
    parentItem,         /* the enum value of an item (answerLength > 1)
                           containing the smaller value for which we are
                           merely an alias
                           OR: -1 if there's no parent */
    parentIndex;        /* where our value starts in parents value
                           OR: -1 if there's no parent */
} gageItemEntry;

/*
** modifying the enums below (scalar, vector, etc query quantities)
** necesitates modifying:
** - the central item table 
** - the associated arrays in arrays.c
** - the arrays in enums.c, 
** - the "answer" method itself.
*/

/*
******** gageScl* enum
**
** all the "items" that gage can measure in a scalar volume.
**
** NOTE: although it is currently listed that way, it is not necessary
** that prerequisite measurements are listed before the other measurements
** which need them (that is represented by _gageSclPrereq)
**
** The description for each enum value starts with the numerical value
** followed by a string which identifies the value in the gageScl airEnum.
** GT means gage_t; this is used to indicate how many scalars are associated
** with each enum value.
*/
enum {
  gageSclUnknown=-1,  /* -1: nobody knows */
  gageSclValue,       /*  0: "v", data value: GT[1] */
  gageSclGradVec,     /*  1: "grad", gradient vector, un-normalized: GT[3] */
  gageSclGradMag,     /*  2: "gm", gradient magnitude: GT[1] */
  gageSclNormal,      /*  3: "n", gradient vector, normalized: GT[3] */
  gageSclNPerp,       /*  4: "np", projection onto tangent plane: GT[9] */
  gageSclHessian,     /*  5: "h", Hessian: GT[9] (column-order) */
  gageSclLaplacian,   /*  6: "l", Laplacian: Dxx + Dyy + Dzz: GT[1] */
  gageSclHessFrob,    /*  7: "hf", Frobenius normal of Hessian: GT[1] */
  gageSclHessEval,    /*  8: "heval", Hessian's eigenvalues: GT[3] */
  gageSclHessEval0,   /*  9: "heval0", Hessian's 1st eigenvalue: GT[1] */
  gageSclHessEval1,   /* 10: "heval1", Hessian's 2nd eigenvalue: GT[1] */
  gageSclHessEval2,   /* 11: "heval2", Hessian's 3rd eigenvalue: GT[1] */
  gageSclHessEvec,    /* 12: "hevec", Hessian's eigenvectors: GT[9] */
  gageSclHessEvec0,   /* 13: "hevec0", Hessian's 1st eigenvector: GT[3] */
  gageSclHessEvec1,   /* 14: "hevec1", Hessian's 2nd eigenvector: GT[3] */
  gageSclHessEvec2,   /* 15: "hevec2", Hessian's 3rd eigenvector: GT[3] */
  gageScl2ndDD,       /* 16: "2d", 2nd dir.deriv. along gradient: GT[1] */
  gageSclGeomTens,    /* 17: "gten", sym. matx w/ evals {0, K1, K2} and
                             evecs {grad, cdir0, cdir1}: GT[9] */
  gageSclK1,          /* 18: "k1", 1st principle curvature: GT[1] */
  gageSclK2,          /* 19: "k2", 2nd principle curvature (k2 <= k1): GT[1] */
  gageSclTotalCurv,   /* 20: "tc", L2 norm(K1,K2) (not Koen.'s "C"): GT[1] */
  gageSclShapeTrace,  /* 21, "st", (K1+K2)/Curvedness: GT[1] */
  gageSclShapeIndex,  /* 22: "si", Koen.'s shape index, ("S"): GT[1] */
  gageSclMeanCurv,    /* 23: "mc", mean curvature (K1 + K2)/2: GT[1] */
  gageSclGaussCurv,   /* 24: "gc", gaussian curvature K1*K2: GT[1] */
  gageSclCurvDir1,    /* 25: "cdir1", 1st principle curv direction: GT[3] */
  gageSclCurvDir2,    /* 26: "cdir2", 2nd principle curv direction: GT[3] */
  gageSclFlowlineCurv,/* 27: "fc", curvature of normal streamline: GT[1] */
  gageSclMedian,      /* 28: "med", median filter */
  gageSclLast
};
#define GAGE_SCL_ITEM_MAX     28

/*
******** gageVec* enum
**
** all the "items" that gage knows how to measure in a 3-vector volume
**
** The strings gives one of the gageVec airEnum identifiers, and GT[x]
** says how many scalars are associated with this value.
*/
enum {
  gageVecUnknown=-1, /* -1: nobody knows */
  gageVecVector,     /*  0: "v", component-wise-intrpolated (CWI) vec: GT[3] */
  gageVecVector0,    /*  1: "v0", vector[0]: GT[1] */
  gageVecVector1,    /*  2: "v1", vector[0]: GT[1] */
  gageVecVector2,    /*  3: "v2", vector[0]: GT[1] */
  gageVecLength,     /*  4: "l", length of CWI vector: GT[1] */
  gageVecNormalized, /*  5: "n", normalized CWI vector: GT[3] */
  gageVecJacobian,   /*  6: "j", component-wise Jacobian: GT[9]
                            0:dv_x/dx  3:dv_x/dy  6:dv_x/dz
                            1:dv_y/dx  4:dv_y/dy  7:dv_y/dz
                            2:dv_z/dx  5:dv_z/dy  8:dv_z/dz */
  gageVecDivergence, /*  7: "d", divergence (based on Jacobian): GT[1] */
  gageVecCurl,       /*  8: "c", curl (based on Jacobian): GT[3] */
  gageVecCurlNorm,   /*  9: "cm", curl magnitude: GT[1] */
  gageVecHelicity,   /* 10: "h", helicity: vec . curl: GT[1] */
  gageVecNormHelicity,/*11: "nh", normalized helicity: GT[1] */
  gageVecLambda2,    /* 12: "lmbda2", lambda2 criterion: GT[1] */
  gageVecImaginaryPart,/* 13: "imag", imag. part of jacobian's eigenv: GT[1] */
  gageVecHessian,    /* 14: "vh", second-order derivative: GT[27] 
                            0:d2v_x/dxdx   1:d2v_x/dxdy   2:d2v_x/dxdz
                            3:d2v_x/dydx   4:d2v_x/dydy   5:d2v_x/dydz
                            6:d2v_x/dzdx   7:d2v_x/dzdy   8:d2v_x/dzdz
                            9:d2v_y/dxdx       [...]
                                [...]
                           24:dv2_z/dzdx  25:d2v_z/dzdy  26:d2v_z/dzdz */
  gageVecDivGradient,/* 15: "dg", divergence gradient: GT[3] */
  gageVecCurlGradient,/*16: "cg", curl gradient: GT[9] */
  gageVecCurlNormGrad,/*17: "cng", curl norm gradient: GT[3] */
  gageVecNCurlNormGrad, /* 18: "ncng", normalized curl norm gradient: GT[3] */
  gageVecHelGradient,/* 19: "hg", helicity gradient: GT[3] */
  gageVecDirHelDeriv,/* 20: "dhd", directional derivative of helicity: GT[1] */ 
  gageVecProjHelGradient, /* 21: "phg", projected helicity gradient: GT[3] */
  gageVecGradient0,  /* 22: "g0", gradient of 1st component of vector: GT[3] */
  gageVecGradient1,  /* 23: "g1", gradient of 2nd component of vector: GT[3] */
  gageVecGradient2,  /* 24: "g2", gradient of 3rd component of vector: GT[3] */
  gageVecMultiGrad,  /* 25: "mg", sum of outer products of gradients: GT[9] */
  gageVecMGFrob,     /* 26: "mgfrob", frob norm of multi-gradient: GT[1] */
  gageVecMGEval,     /* 27: "mgeval", eigenvalues of multi-gradient: GT[3] */
  gageVecMGEvec,     /* 28: "mgevec", eigenvectors of multi-gradient: GT[9] */
  gageVecLast
};
#define GAGE_VEC_ITEM_MAX     28

struct gageKind_t;       /* dumb forward declaraction, ignore */
struct gagePerVolume_t;  /* dumb forward declaraction, ignore */

/*
******** gagePadder_t, gageNixer_t
**
** type of functions used to pad volumes and to remove padded volumes.
** Chances are, no one has to worry about these, since the default
** padder (_gageStandardPadder) and nixer (_gageStandardNixer) probably
** do exactly what you want.
*/
typedef Nrrd* (gagePadder_t)(Nrrd *nin, struct gageKind_t *kind,
                             int padding, struct gagePerVolume_t *pvl);
typedef void (gageNixer_t)(Nrrd *npad, struct gageKind_t *kind,
                           struct gagePerVolume_t *pvl);

/*
******** gageShape struct
**
** just a container for all the information related to the "shape"
** of all the volumes associated with a context
*/
typedef struct gageShape_t {
  int size[3],                /* dimensions of UNPADDED volume */
    defaultCenter,            /* default centering to use when given volume
                                 has no centering set */
    center;                   /* the sample centering of the volume(s)- this
                                 is an issue for determing the extent of the
                                 volume, and in cell centering, an extra
                                 sample of padding */
  gage_t spacing[3],          /* spacings for each axis */
    fwScale[GAGE_KERNEL_NUM][3];
                              /* how to rescale weights for each of the
                                 kernels according to non-unity-ness of
                                 sample spacing (0=X, 1=Y, 2=Z) */
  double volHalfLen[3],       /* half the lengths along each axis in order
                                 to bound the volume in a bi-unit cube */
    voxLen[3];                /* when bound in bi-unit cube, the dimensions
                                 of a single voxel */
} gageShape;

/*
******** gageParm struct
**
** a container for the various switches and knobs which control
** gage, aside from the obvious inputs (kernels, queries, volumes)
*/
typedef struct gageParm_t {
  int renormalize;            /* hack to make sure that sum of
                                 discrete value reconstruction weights
                                 is same as kernel's continuous
                                 integral, and that the 1nd and 2nd
                                 deriv weights really sum to 0.0 */
  int checkIntegrals;         /* call the "integral" method of the
                                 kernel to verify that it is
                                 appropriate for the task for which
                                 the kernel is being set:
                                 reconstruction: 1.0, derivatives: 0.0 */
  int noRepadWhenSmaller;     /* if a change in parameters leads to a newer and
                                 smaller amount of padding, don't generate a
                                 new padded volume, use the somewhat overly
                                 padded volume (a.k.a "NRWS") */
  int k3pack;                 /* non-zero (true) iff we do not use
                                 kernels for gageKernelIJ with I != J.
                                 So, we use the value reconstruction
                                 kernel (gageKernel00) for 1st and 2nd
                                 derivative reconstruction, and so on.
                                 This is faster because we can re-use
                                 results from low-order convolutions. */
  gage_t gradMagMin,          /* pre-normalized vector lengths can't be
                                 smaller than this */
    gradMagCurvMin,           /* punt on computing curvature information if
                                 gradient magnitude is less than this. Yes,
                                 this is scalar-kind-specific, but there's
                                 no other good place for it */
    kernelIntegralNearZero,   /* tolerance with checkIntegrals on derivative
                                 kernels */
    defaultSpacing;           /* when requireAllSpacings is zero, what spacing
                                 to use when we have to invent one */
  int curvNormalSide,         /* determines direction of gradient that is used
                                 as normal in curvature calculations, exactly
                                 the same as miteUser's normalSide: 1 for
                                 normal pointing to lower values (higher
                                 values are more "inside"); -1 for normal
                                 pointing to higher values (low values more
                                 "inside") */
    requireAllSpacings,       /* if non-zero, require that spacings on all 3
                                 spatial axes are set, and are equal; this is
                                 the traditional way of gage.  If zero, then 
                                 one, two, or all three axes' spacing can be
                                 unset, and we'll use defaultSpacing instead */
    requireEqualCenters,      /* if non-zero, all centerings on spatial axes 
                                 must be the same (including the possibility 
                                 of all being nrrdCenterUnknown). If zero, its
                                 okay for axes' centers to be unset, but two
                                 that are set cannot be unequal */
    defaultCenter;            /* only meaningful when requireAllSpacings is
                                 zero- what centering to use when you have to
                                 invent one, because its not set */
} gageParm;

/*
******** gagePoint struct
**
** stores location of last query location, in terms of coordinates
** in the PADDED volume
*/
typedef struct gagePoint_t {
  gage_t xf, yf, zf;          /* fractional voxel location, used to
                                 short-circuit calculation of filter sample
                                 locations and weights */
  int xi, yi, zi;             /* integral voxel location */
} gagePoint;

/*
******** gageQuery typedef
** 
** this short, fixed-length array is used as a bit-vector to store
** all the items that comprise a query.  Its length sets an upper
** bound on the maximum item value that a gageKind may use.
**
** The important thing to keep in mind is that a variable of type
** gageKind ends up being passed by reference, even though the
** syntax of its usage doesn't immediately announce that.
**
** gageKindCheck currently has the role of verifying that a gageKind's
** maximum item value is within the bounds set here. Using
** GAGE_QUERY_BYTES_NUM == 8 gives a max item value of 63, which is 
** far above anything being used now.
*/
#define GAGE_QUERY_BYTES_NUM 8
#define GAGE_ITEM_MAX ((8*GAGE_QUERY_BYTES_NUM)-1)
typedef unsigned char gageQuery[GAGE_QUERY_BYTES_NUM];

/*
******** GAGE_QUERY_RESET, GAGE_QUERY_TEST
******** GAGE_QUERY_ON, GAGE_QUERY_OFF
**
** Macros for manipulating a gageQuery.
**
** airSanity ensures that an unsigned char is in fact 8 bits
*/
#define GAGE_QUERY_RESET(q) (q[0]=q[1]=q[2]=q[3]=q[4]=q[5]=q[6]=q[7]=0)
#define GAGE_QUERY_COPY(p, q) \
  p[0] = q[0]; p[1] = q[1]; p[2] = q[2]; p[3] = q[3]; \
  p[4] = q[4]; p[5] = q[5]; p[6] = q[6]; p[7] = q[7]
#define GAGE_QUERY_ADD(p, q) \
  p[0] |= q[0]; p[1] |= q[1]; p[2] |= q[2]; p[3] |= q[3]; \
  p[4] |= q[4]; p[5] |= q[5]; p[6] |= q[6]; p[7] |= q[7]
#define GAGE_QUERY_EQUAL(p, q) \
  (p[0] == q[0] && p[1] == q[1] && p[2] == q[2] && p[3] == q[3] \
   && p[4] == q[4] && p[5] == q[5] && p[6] == q[6] && p[7] == q[7])
#define GAGE_QUERY_NONZERO(q) \
  (q[0] | q[1] | q[2] | q[3] | q[4] | q[5] | q[6] | q[7])
#define GAGE_QUERY_ITEM_TEST(q, i) (q[i/8] & (1 << (i % 8)))
#define GAGE_QUERY_ITEM_ON(q, i) (q[i/8] |= (1 << (i % 8)))
#define GAGE_QUERY_ITEM_OFF(q, i) (q[i/8] &= ^(1 << (i % 8)))

/*
******** gageContext struct
**
** The information here is specific to the dimensions, scalings, and
** padding of a volume, but not to kind of volume (all kind-specific
** information is in the gagePerVolume).  One context can be used in
** conjuction with probing multiple volumes.
*/
typedef struct gageContext_t {
  int verbose,                /* verbosity */
    thisIsACopy;              /* I am the result of gageContextCopy */
  gageParm parm;              /* parameters */
  NrrdKernelSpec *ksp[GAGE_KERNEL_NUM];  /* all the kernels we'll ever need */
  struct gagePerVolume_t *pvl[GAGE_PERVOLUME_NUM];
                              /* the pervolumes attached to this context */
  int numPvl;                 /* number of pervolumes currently attached */
  gageShape *shape;           /* sizes, spacings, centering, and other 
                                 geometric aspects of the volume */
  int flag[GAGE_CTX_FLAG_NUM];/* all the flags used by gageUpdate() used to
                                 describe what changed in this context */
  int needD[3];               /* which value/derivatives need to be calculated
                                 for all pervolumes (doV, doD1, doD2) */
  int needK[GAGE_KERNEL_NUM]; /* which kernels are needed for all pervolumes */
  int needPad;                /* amount of boundary margin required for current
                                 queries (in all pervolumes), kernels, and
                                 the value of k3pack */
  int havePad;                /* amount of padding currently in pervolumes
                                 Note: fd == 2*(havePad + 1)
                                 where fd is the "filter diameter", the length
                                 of the filter weight arrays.  It is an 
                                 unnecessary but GREATLY simplifying constraint
                                 that ties fd to havePad according to above,
                                 unnecessary since with noRepadWhenSmaller we
                                 could have a volume that is padded more than
                                 needed for the given set of filters */
  gage_t *fsl,                /* filter sample locations (all axes):
                                 logically a fd x 3 array */
    *fw;                      /* filter weights (all axes, all kernels):
                                 logically a fd x 3 x GAGE_KERNEL_NUM array */
  unsigned int *off;          /* offsets to other fd^3 samples needed to fill
                                 3D intermediate value cache. Allocated size is
                                 dependent on kernels, values inside are
                                 dependent on the dimensions of the volume. It
                                 may be more correct to be using size_t
                                 instead of uint, but the X and Y dimensions of
                                 the volume would have to be super-outrageous
                                 for that to be a problem */
  gagePoint point;            /* last probe location */
} gageContext;

/*
******** gagePerVolume
**
** information that is specific to one volume, and to one kind of
** volume.
*/
typedef struct gagePerVolume_t {
  int verbose,                /* verbosity */
    thisIsACopy;              /* I'm a copy */
  struct gageKind_t *kind;    /* what kind of volume is this pervolume for */
  gageQuery query;            /* the query, recursively expanded */
  int needD[3];               /* which derivatives need to be calculated for
                                 the query (above) in this volume */
  Nrrd *nin;                  /* the original, unpadded volume, passed to the
                                 padder below, but never nixed or nuked by
                                 gage, and never passed to anything else */
  gagePadder_t *padder;       /* how to pad nin to produce npad; use NULL
                                 to signify no-op */  
  gageNixer_t *nixer;         /* for when npad is to be replaced or removed;
                                 use NULL to signify no-op */
  void *padInfo;              /* supplemental information intended for use
                                 by the padder/nixer */
  Nrrd *npad;                 /* the padded nrrd which is probed */
  int flag[GAGE_PVL_FLAG_NUM];/* for the kind-specific flags ... */
  gage_t *iv3, *iv2, *iv1;    /* 3D, 2D, 1D, value caches.  These are cubical,
                                 square, and linear arrays, all length fd on
                                 each edge.  Exactly how values are arranged
                                 in iv3 (non-scalar volumes can have the
                                 component axis be the slowest or the fastest)
                                 is not strictly speaking gage's concern, as
                                 filling iv3 is up to iv3Fill in the gageKind
                                 struct.  Use of iv2 and iv1 is entirely up
                                 the kind's filter method. */
  gage_t (*lup)(const void *ptr, size_t I); 
                              /* nrrd{F,D}Lookup[] element, according to
                                 npad->type and gage_t */
  gage_t *answer;             /* main buffer to hold all the answers */
  gage_t **directAnswer;      /* array of pointers into answer */
} gagePerVolume;

/*
******** gageKind struct
**
** all the information and functions that are needed to handle one
** kind of volume (such as scalar, vector, etc.)
*/
typedef struct gageKind_t {
  char name[AIR_STRLEN_SMALL];      /* short identifying string for kind */
  airEnum *enm;                     /* such as gageScl.  NB: the "unknown"
                                       value in the enum MUST be -1 (since
                                       queries are formed as bitflags) */
  int baseDim,                      /* dimension that x,y,z axes start on
                                       (0 for scalars, 1 for vectors) */
    valLen,                         /* number of scalars per data point */
    itemMax;                        /* such as GAGE_SCL_ITEM_MAX */
  gageItemEntry *table;             /* array of gageItemEntry's, indexed
                                       by the item value */
  void (*iv3Print)(FILE *,          /* such as _gageSclIv3Print() */
                   gageContext *,
                   gagePerVolume *),
    (*filter)(gageContext *,        /* such as _gageSclFilter() */
              gagePerVolume *),
    (*answer)(gageContext *,        /* such as _gageSclAnswer() */
              gagePerVolume *);
} gageKind;

/*
******** gageItemSpec struct
**
** dumb little way to store a kind/item pair.  Formerly known
** as a gageQuerySpec.  
*/
typedef struct {
  gageKind *kind;             /* the kind of the volume to measure */
  int item;                   /* the quantity to measure */
} gageItemSpec;

/* defaultsGage.c */
TEEM_API const char *gageBiffKey;
TEEM_API int gageDefVerbose;
TEEM_API gage_t gageDefGradMagMin;
TEEM_API gage_t gageDefGradMagCurvMin;
TEEM_API int gageDefRenormalize;
TEEM_API int gageDefCheckIntegrals;
TEEM_API int gageDefNoRepadWhenSmaller;
TEEM_API int gageDefK3Pack;
TEEM_API gage_t gageDefDefaultSpacing;
TEEM_API int gageDefCurvNormalSide;
TEEM_API double gageDefKernelIntegralNearZero;
TEEM_API int gageDefRequireAllSpacings;
TEEM_API int gageDefRequireEqualCenters;
TEEM_API int gageDefDefaultCenter;

/* miscGage.c */
/* gageErrStr and gageErrNum are for describing errors that happen in
   gageProbe(): using biff is too heavy-weight for this, and the idea is
   that no ill should occur if the error is repeatedly ignored */
TEEM_API char gageErrStr[AIR_STRLEN_LARGE];
TEEM_API int gageErrNum;
TEEM_API gage_t gageZeroNormal[3];
TEEM_API airEnum *gageKernel;
TEEM_API void gageParmReset(gageParm *parm);
TEEM_API void gagePointReset(gagePoint *point);
TEEM_API gageItemSpec *gageItemSpecNew(void);
TEEM_API gageItemSpec *gageItemSpecNix(gageItemSpec *isp);

/* kind.c */
TEEM_API int gageKindCheck(gageKind *kind);
TEEM_API int gageKindTotalAnswerLength(gageKind *kind);
TEEM_API int gageKindAnswerOffset(gageKind *kind, int item);

/* print.c */
TEEM_API void gageQueryPrint(FILE *file, gageKind *kind, gageQuery query);

/* sclfilter.c */
TEEM_API void gageScl3PFilter2(gage_t *iv3, gage_t *iv2, gage_t *iv1,
                               gage_t *fw00, gage_t *fw11, gage_t *fw22,
                               gage_t *val, gage_t *gvec, gage_t *hess,
                               int doV, int doD1, int doD2);
TEEM_API void gageScl3PFilter4(gage_t *iv3, gage_t *iv2, gage_t *iv1,
                               gage_t *fw00, gage_t *fw11, gage_t *fw22,
                               gage_t *val, gage_t *gvec, gage_t *hess,
                               int doV, int doD1, int doD2);
TEEM_API void gageScl3PFilterN(int fd,
                               gage_t *iv3, gage_t *iv2, gage_t *iv1,
                               gage_t *fw00, gage_t *fw11, gage_t *fw22,
                               gage_t *val, gage_t *gvec, gage_t *hess,
                               int doV, int doD1, int doD2);

/* scl.c */
TEEM_API airEnum *gageScl;
TEEM_API gageKind *gageKindScl;

/* vecGage.c (together with vecprint.c, these contain everything to
   implement the "vec" kind, and could be used as examples of what it
   takes to create a new gageKind) */
TEEM_API airEnum *gageVec;
TEEM_API gageKind *gageKindVec;

/* shape.c */
TEEM_API void gageShapeReset(gageShape *shp);
TEEM_API gageShape *gageShapeNew();
TEEM_API gageShape *gageShapeNix(gageShape *shape);
TEEM_API int gageShapeSet(gageShape *shp, Nrrd *nin, int baseDim);
TEEM_API void gageShapeUnitWtoI(gageShape *shape,
                                double index[3], double world[3]);
TEEM_API void gageShapeUnitItoW(gageShape *shape,
                                double world[3], double index[3]);
TEEM_API int gageShapeEqual(gageShape *shp1, char *name1,
                            gageShape *shp2, char *name2);

/* the organization of the next two files used to be according to
   what the first argument is, not what appears in the function name,
   but that's just a complete mess now */
/* pvl.c */
TEEM_API int gageVolumeCheck(gageContext *ctx, Nrrd *nin, gageKind *kind);
TEEM_API gagePerVolume *gagePerVolumeNew(gageContext *ctx,
                                         Nrrd *nin, gageKind *kind);
TEEM_API gagePerVolume *gagePerVolumeNix(gagePerVolume *pvl);
TEEM_API void gagePadderSet(gageContext *ctx,
                            gagePerVolume *pvl, gagePadder_t *padder);
TEEM_API void gageNixerSet(gageContext *ctx, 
                           gagePerVolume *pvl, gageNixer_t *nixer);
TEEM_API gage_t *gageAnswerPointer(gageContext *ctx, 
                                   gagePerVolume *pvl, int item);
TEEM_API int gageQueryReset(gageContext *ctx, gagePerVolume *pvl);
TEEM_API int gageQuerySet(gageContext *ctx, gagePerVolume *pvl,
                          gageQuery query);
TEEM_API int gageQueryAdd(gageContext *ctx, gagePerVolume *pvl,
                          gageQuery query);
TEEM_API int gageQueryItemOn(gageContext *ctx, gagePerVolume *pvl, int item);

/* ctx.c */
TEEM_API gageContext *gageContextNew();
TEEM_API gageContext *gageContextCopy(gageContext *ctx);
TEEM_API gageContext *gageContextNix(gageContext *ctx);
TEEM_API void gageParmSet(gageContext *ctx, int which, gage_t val);
TEEM_API int gagePerVolumeIsAttached(gageContext *ctx, gagePerVolume *pvl);
TEEM_API int gagePerVolumeAttach(gageContext *ctx, gagePerVolume *pvl);
TEEM_API int gagePerVolumeDetach(gageContext *ctx, gagePerVolume *pvl);
TEEM_API int gageKernelSet(gageContext *ctx,
                           int which, const NrrdKernel *k, double *kparm);
TEEM_API void gageKernelReset(gageContext *ctx);
TEEM_API int gageProbe(gageContext *ctx, gage_t x, gage_t y, gage_t z);

/* update.c */
TEEM_API int gageUpdate(gageContext *ctx);

/* st.c */
TEEM_API int gageStructureTensor(Nrrd *nout, Nrrd *nin,
                                 int dScale, int iScale, int dsmp);

#ifdef __cplusplus
}
#endif

#endif /* GAGE_HAS_BEEN_INCLUDED */
