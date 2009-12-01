/*
  Teem: Tools to process and visualize scientific data and images              
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

#ifndef PULL_HAS_BEEN_INCLUDED
#define PULL_HAS_BEEN_INCLUDED

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/ell.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/limn.h>
#include <teem/ten.h>

#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(TEEM_STATIC)
#  if defined(TEEM_BUILD) || defined(pull_EXPORTS) || defined(teem_EXPORTS)
#    define PULL_EXPORT extern __declspec(dllexport)
#  else
#    define PULL_EXPORT extern __declspec(dllimport)
#  endif
#else /* TEEM_STATIC || UNIX */
#  define PULL_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULL pullBiffKey
#define PULL_THREAD_MAXNUM 512
#define PULL_VOLUME_MAXNUM 4
#define PULL_POINT_NEIGH_INCR 16
#define PULL_BIN_MAXNUM 128*128*128*30 /* sanity check on max number bins */
#define POINT_NUM_INCR 1024

#define PULL_PHIST 0

/*
******** pullInfo enum
**
** all the things that might be learned via gage from some kind, that
** can be used to control particle dynamics.
**
** There are multiple scalars (and associated) derivatives that can
** be used for dynamics:
** - Inside: just for nudging things to stay inside a mask
** - Height: value for computer-vision-y features of ridges, valleys,
**   and edges.  Setting pullInfoHeight as a constraint does valley
**   sampling (flip the sign to get ridges), based on Tangent1, Tangent2,
**   and TangentMode.  Setting pullInfoHeightLaplacian as a constraint
*    does zero-crossing edge detection.
** - Isovalue: just for implicit surfaces f=0
** - Strength: some measure of feature strength, with the assumption
**   that it can't be analytically differentiated in space or scale.
*/
enum {
  pullInfoUnknown,            /*  0 */
  pullInfoTensor,             /*  1: [7] tensor here */
  pullInfoTensorInverse,      /*  2: [7] inverse of tensor here */
  pullInfoHessian,            /*  3: [9] hessian used for force distortion */
  pullInfoInside,             /*  4: [1] containment scalar */
  pullInfoInsideGradient,     /*  5: [3] containment vector */
  pullInfoHeight,             /*  6: [1] for gravity, and edge and crease 
                                         feature detection */
  pullInfoHeightGradient,     /*  7: [3] */
  pullInfoHeightHessian,      /*  8: [9] */
  pullInfoHeightLaplacian,    /*  9: [1] for zero-crossing edge detection */
  pullInfoSeedPreThresh,      /* 10: [1] scalar for pre-thresholding seeding,
                                 so that points can be quickly eliminated
                                 (e.g. prior to constraint satisfaction) */
  pullInfoSeedThresh,         /* 11: [1] scalar for thresholding seeding */
  pullInfoLiveThresh,         /* 12: [1] scalar for thresholding extent 
                                 particles, AND for future additions from
                                 population control */
  pullInfoLiveThresh2,        /* 13: [1] another pullInfoLiveThresh */
  pullInfoTangent1,           /* 14: [3] first tangent to constraint surf */
  pullInfoTangent2,           /* 15: [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* 16: [1] for morphing between co-dim 1 and 2;
                                 User must set scale so mode from -1 to 1
                                 means co-dim 1 (surface) to 2 (line) */
  pullInfoIsovalue,           /* 17: [1] for isosurface extraction */
  pullInfoIsovalueGradient,   /* 18: [3] */
  pullInfoIsovalueHessian,    /* 19: [9] */
  pullInfoStrength,           /* 20: [1] */
  pullInfoQuality,            /* 21: [1] */
  pullInfoLast
};
#define PULL_INFO_MAX            21

/*
******** pullVal* enum
**
** the item values for the pseudo gageKind pullVal
*/
enum {
  pullValUnknown,             /* 0 */
  pullValSlice,               /* 1: position along slice axis */
  pullValSliceGradVec,        /* 2: gradient of slice pos == slice normal */
  pullValLast
};
#define PULL_VAL_ITEM_MAX        2

/*
** the various properties of particles in the system 
**
** consider adding: dot between normalized directions of force and movmt 
*/
enum {
  pullPropUnknown,            /*  0: nobody knows */
  pullPropIdtag,              /*  1: [1] idtag (unsigned int) */
  pullPropIdCC,               /*  2: [1] idcc (unsigned int) */
  pullPropEnergy,             /*  3: [1] energy from last iteration */
  pullPropStepEnergy,         /*  4: [1] step size for minimizing energy */
  pullPropStepConstr,         /*  5: [1] step size for constraint satis. */
  pullPropStuck,              /*  6: [1] how many iters its been stuck */
  pullPropPosition,           /*  7: [4] position */
  pullPropForce,              /*  8: [4] force accumulation */
  pullPropNeighDistMean,      /*  9: [1] "mean distance" to neighbors */
  pullPropScale,              /* 10: [1] scale position */
  pullPropLast
};

/*
** the components of a point's status that are set as a bitflag 
** in point->status
*/
enum {
  pullStatusUnknown,             /* 0: nobody knows */
  pullStatusStuck,               /* 1: couldn't move to decrease energy */
#define PULL_STATUS_STUCK_BIT  (1<< 1)
  pullStatusNewbie,              /* 2: not binned, testing if the system
                                    would be better with me in it */
#define PULL_STATUS_NEWBIE_BIT (1<< 2)
  pullStatusNixMe,               /* 3: nix me at the *end* of this iter,
                                    and don't look at me for energy 
                                    during this iteraction */
#define PULL_STATUS_NIXME_BIT  (1<< 3)
  pullStatusEdge,                /* 4: at the spatial edge of one of the
                                    volumes: gage had to invent values for 
                                    some samples in the kernel support */
#define PULL_STATUS_EDGE_BIT   (1<< 4)
  pullStatusLast
};

/*
******** pullInterType* enum
**
** the different types of scale-space interaction that can happen
** in scale-space.  The descriptions here overlook the normalization
** by radiusScale and radiusSpace
*/
enum {
  pullInterTypeUnknown,      /* 0 */
  pullInterTypeJustR,        /* 1: phi(r,s) = phi_r(r) */
  pullInterTypeUnivariate,   /* 2: phi(r,s) = phi_r(u); u = sqrt(r*r+s*s) */
  pullInterTypeSeparable,    /* 3: phi(r,s) = phi_r(r)*phi_s(s) */
  pullInterTypeAdditive,     /* 4: phi(r,s) = beta*phi_r(r)*win(s) 
                                              + (1-beta)*win(r)*phi_s(s) */
  pullInterTypeLast
};
#define PULL_INTER_TYPE_MAX     4

/*
******** pullEnergyType* enum
**
** the different shapes of potential energy profiles that can be used
*/
enum {
  pullEnergyTypeUnknown,       /* 0 */
  pullEnergyTypeSpring,        /* 1 */
  pullEnergyTypeGauss,         /* 2 */
  pullEnergyTypeButterworth,   /* 3 */
  pullEnergyTypeCotan,         /* 4 */
  pullEnergyTypeCubic,         /* 5 */
  pullEnergyTypeQuartic,       /* 6 */
  pullEnergyTypeCubicWell,     /* 7 */
  pullEnergyTypeZero,          /* 8 */
  pullEnergyTypeButterworthParabola, /* 9 */
  pullEnergyTypeLast
};
#define PULL_ENERGY_TYPE_MAX      9
#define PULL_ENERGY_PARM_NUM 3

enum {
  pullProcessModeUnknown,      /* 0 */
  pullProcessModeDescent,      /* 1 */
  pullProcessModeNeighLearn,   /* 2 */
  pullProcessModeAdding,       /* 3 */
  pullProcessModeNixing,       /* 4 */
  pullProcessModeLast
};
#define PULL_PROCESS_MODE_MAX     4

/*
** the conditions under which a point may find itself at some position 
*/
enum {
  pullCondUnknown,            /* 0 */
  pullCondOld,                /* 1 */
  pullCondConstraintSatA,     /* 2 */
  pullCondConstraintSatB,     /* 3 */
  pullCondEnergyTry,          /* 4 */
  pullCondConstraintFail,     /* 5 */
  pullCondEnergyBad,          /* 6 */
  pullCondNew,                /* 7 */
  pullCondLast
};

/* 
** how the gageItem for a pullInfo is specified
*/
typedef struct pullInfoSpec_t {
  /* ------ INPUT ------ */
  int info;                     /* from the pullInfo* enum */
  char *volName;                /* volume name */
  int item;                     /* which item */
  double scale,                 /* scaling factor (including sign) */
    zero;                       /* for height and inside: where is zero,
                                   for seedThresh, threshold value */
  int constraint;               /* (for scalar items) minimizing this
                                   is a constraint to enforce per-point
                                   per-iteration, not merely a contribution 
                                   to the point's energy */
  /* ------ INTERNAL ------ */
  unsigned int volIdx;          /* which volume */
} pullInfoSpec;

/*
******** pullPoint
**
*/
typedef struct pullPoint_t {
  unsigned int idtag,         /* unique point ID */
    idCC;                     /* id for connected component analysis */
  struct pullPoint_t **neighPoint; /* list of neighboring points */
  unsigned int neighPointNum;
  airArray *neighPointArr;    /* airArray around neighPoint and neighNum
                                 (no callbacks used here) */
  double neighDistMean,       /* average of distance to neighboring
                                 points with whom this point interacted,
                                 in rs-normalized space */
    neighMode;                /* some average of mode of nearby points */
  unsigned int neighInterNum, /* number of particles with which I had some
				 non-zero interaction on last iteration */
    stuckIterNum;             /* how many iterations I've been stuck */
#if PULL_PHIST
  double *phist;              /* history of positions tried in the last iter,
                                 in sets of 5 doubles: (x,y,z,t,info) */
  unsigned int phistNum;      /* number of positions stored */
  airArray *phistArr;         /* airArray around phist */
#endif
  unsigned int status;        /* bit-flag of status info, though right now
                                 its just a boolean for having gotten stuck */
  double pos[4],              /* position in space and scale */
    energy,                   /* energy accumulator for this iteration */
    force[4],                 /* force accumulator for this iteration */
    stepEnergy,               /* step size for energy minimization */
    stepConstr,               /* step size for constraint satisfaction */
    info[1];                  /* all information learned from gage that matters
                                 for particle dynamics.  This is sneakily
                                 allocated for *more*, depending on needs,
                                 so this MUST be last field */
} pullPoint;

/*
******** pullBin
**
** the data structure for doing spatial binning.
*/
typedef struct pullBin_t {
  pullPoint **point;         /* dyn. alloc. array of point pointers */
  unsigned int pointNum;     /* # of points in this bin */
  airArray *pointArr;        /* airArray around point and pointNum 
                                (no callbacks used here) */
  struct pullBin_t **neighBin;  /* pre-computed NULL-terminated list of all
                                neighboring bins, including myself */
} pullBin;

/*
******** pullEnergy
**
** the functions which determine inter-point forces
**
** NOTE: the eval() function probably does NOT check to see it was passed
** a non-NULL pointer into which to store the derivative of energy ("denr")
**
** Thu Apr 10 12:40:08 EDT 2008: nixed the "support" function, since it
** was annoying to deal with variable support potentials.  Now everything
** cuts off at dist=1.  You can still use the parm vector to change the
** shape inside the support.
*/
typedef struct {
  char name[AIR_STRLEN_SMALL];
  unsigned int parmNum;
  double (*eval)(double *denr, double dist,
                 const double parm[PULL_ENERGY_PARM_NUM]);
} pullEnergy;

typedef struct {
  const pullEnergy *energy;
  double parm[PULL_ENERGY_PARM_NUM];
} pullEnergySpec;

/*
** In the interests of simplicity (and with the cost of some redundancy), 
** this is going to copied per-task, which is why it contains the gageContext
** The idea is that the first of these is somehow set up by the user
** or something, and the rest of them are created within pull per-task.
*/
typedef struct {
  int verbose;                 /* blah blah blah */
  char *name;                  /* how the volume will be identified
                                  (like its a variable name) */
  const gageKind *kind;
  const Nrrd *ninSingle;       /* don't own */
  const Nrrd *const *ninScale; /* don't own;
                                  NOTE: only one of ninSingle and ninScale
                                  can be non-NULL */
  unsigned int scaleNum;       /* number of scale-space samples (volumes) */
  double *scalePos;            /* location of all samples in scale */
  int scaleDerivNorm;          /* normalize derivatives based on scale */
  NrrdKernelSpec *ksp00,       /* for sampling tensor field */
    *ksp11,                    /* for gradient of mask, other 1st derivs */
    *ksp22,                    /* for 2nd derivatives */
    *kspSS;                    /* for reconstructing from scale-space
                                  samples */
  gageQuery pullValQuery;      /* if this is a pullValGageKind volume,
                                  then we don't have a real gageContext,
                                  and we have to manage our own query */
  gageContext *gctx;           /* do own, and set based on info here */
  gagePerVolume *gpvl,         /* stupid gage API ... */
    **gpvlSS;                  /* stupid gage API ... */
  int seedOnly;                /* volume only required for seeding, for
                                  either pullInfoSeedThresh or
                                  pullInfoSeedPreThresh */
} pullVolume;

/*
******** pullTask
**
** The information specific for a thread.  
*/
typedef struct pullTask_t {
  struct pullContext_t
    *pctx;                      /* parent's context; not const because the
                                   tasks assign themselves bins to do work */
  pullVolume
    *vol[PULL_VOLUME_MAXNUM];   /* volumes copied from parent */
  double *pullValAnswer;        /* buffer for holding answers to all pullVal
                                   items (like gagePerVolume->answer) */
  double **pullValDirectAnswer; /* convenience pointers into pullValAnswer
                                   (like gagePerVolume->directAnswer) */
  const double
    *ans[PULL_INFO_MAX+1];      /* answer *pointers* for all possible infos,
                                   pointing into per-task per-volume gctxs
                                   (or into above per-task pullValAnswer),
                                   OR: NULL if that info is not being used */
  int processMode;              /* what kind of point processing is being
                                   done by this task right now */
  airThread *thread;            /* my thread */
  unsigned int threadIdx;       /* which thread am I */
  airRandMTState *rng;          /* state for my RNG */
  pullPoint *pointBuffer,       /* place for copying point into during 
                                   strength ascent computation; can't be
                                   statically allocated because pullPoint
                                   size is known only at run-time */
    **neighPoint;               /* array of point pointers, either all
                                   possible points from neighbor bins, or
                                   last learned interacting neighbors */
  pullPoint **addPoint;         /* points to add before next iter */
  unsigned int addPointNum;     /* # of points to add */
  airArray *addPointArr;        /* airArray around addPoint, addPointNum */
  pullPoint **nixPoint;         /* points to nix before next iter */
  unsigned int nixPointNum;     /* # of points to nix */
  airArray *nixPointArr;        /* airArray around nixPoint, nixPointNum */
  void *returnPtr;              /* for airThreadJoin */
  unsigned int stuckNum;        /* # stuck particles seen by this task */
} pullTask;

/*
******** pullInitMethod* enum
**
** the different ways pull can be initialized
*/
enum {
  pullInitMethodUnknown,       /* 0 */
  pullInitMethodRandom,        /* 1 */
  pullInitMethodPointPerVoxel, /* 2 */
  pullInitMethodGivenPos,      /* 3 */
  pullInitMethodLast
};

/*
******** pullInitParm
**
** none of this is directly user-set
*/
typedef struct {
  int method;               /* from pullInitMethod* enum */
  int liveThreshUse,        /* use the liveThresh info as a criterion
                               for seeding */
    unequalShapesAllow;     /* when taking in volumes, allow their shapes
                               to be unuequal.  Some confusing usage problems
                               are due to using differently shaped volumes */
  double jitter;            /* w/ PointPerVoxel,
                               how much to jitter index space positions */
  unsigned int numInitial,  /* w/ Random, # pts to start with */
    samplesAlongScaleNum,   /* w/ PointPerVoxel,
                               # of samples along scale (distributed
                               uniformly in scale's *index* space*/
    ppvZRange[2];           /* w/ PointPerVoxel,
                               range of indices along Z to do seeding
                               by pointPerVoxel, or, {1,0} to do the
                               whole volume as normal */
  int pointPerVoxel;        /* w/ PointPerVoxel,
                               if this (ppv) is > 0, then use
                               ppv points per voxel. If ppv < 0, then jitter
                               point point every -ppv'th voxel
                               (so ppv=-1 is same as ppv=1) */
  const Nrrd *npos;         /* positions (4xN array) to start with */
} pullInitParm;

/*
******** pullIterParm* enum
**
** parameters that related to iterations and their periods
*/
enum {
  pullIterParmUnknown,

  /* if non-zero, max number of iterations for whole system. 
     if zero: no explicit on the number of iterations */
  pullIterParmMax,
  
  /* if non-zero, max number of iterations we allow something to be
     continuously stuck before nixing it */
  pullIterParmStuckMax,

  /* if non-zero, max number of iterations for enforcing each constraint */
  pullIterParmConstraintMax,

  /* how many intervals to wait between attemps at population control,
     or, 0 to say: "no pop cntl" */
  pullIterParmPopCntlPeriod,

  /* if non-zero, interval between iters at which output snapshots are saved */
  pullIterParmSnap,
  pullIterParmLast
};

typedef struct {
  unsigned int max,
    popCntlPeriod,
    constraintMax,
    stuckMax,
    snap;
} pullIterParm;

/*
******** pullSysParm* enum
**
** various continuous parameters.  It should be understood that the number
** of these is a reflection of the experimentation and exploration that
** went into the creation of these particles systems, rather than a need
** for onerous parameter tweaking in order to get something useful done.
** Except for alpha, beta, and gamma, these have reasonable defaults.
*/
enum {
  pullSysParmUnknown,

  /* balance between particle-image energy E_i and inter-particle energy E_ij,
     the most important parameter in the governing equation of the system.
     alpha = 0: only particle-image; alpha = 1: only inter-particle */
  pullSysParmAlpha,

  /* when using pullInterAdditive inter-particle energy, determine the 
     balance between spatial repulsion and scale attraction.
     beta = 0: only spatial repulsion; beta = 1: only scale attraction */
  pullSysParmBeta,

  /* when energyFromStrength is non-zero: scaling factor on energy
     from strength */
  pullSysParmGamma,

  /* initial (time) step for dynamics */
  pullSysParmStepInitial,

  /* radius/scaling of inter-particle interactions in the spatial domain */
  pullSysParmRadiusSpace,

  /* radius/scaling of inter-particle interactions in the scale domain */
  pullSysParmRadiusScale,

  /* probability that we find the true neighbors of the particle, as
     opposed to using a cached list */
  pullSysParmNeighborTrueProb,

  /* probability that we do image probing to find out what's really going on */
  pullSysParmProbeProb,

  /* (>= 1.0) how much to opportunistically scale up step size (for
     energy minimization) with every iteration */
  pullSysParmOpporStepScale,

  /* (< 1.0) when energy goes up instead of down, or when constraint
     satisfaction seems to be going the wrong way, how to scale (down)
     step size */
  pullSysParmStepScale,

  /* pseudo-convergence threshold that controls when population control is
     activated (has to be higher than (less strict) energyDecreaseMin */
  pullSysParmEnergyDecreasePopCntlMin,

  /* epsilon amount by which its okay for particle energy to increase,
     in the context of gradient descent */
  pullSysParmEnergyIncreasePermit,
  
  /* convergence threshold: stop when fractional improvement
     (decrease) in total system energy dips below this */
  pullSysParmEnergyDecreaseMin,

  /* convergence threshold for constraint satisfaction: finished if
     stepsize goes below this times constraintVoxelSize */
  pullSysParmConstraintStepMin,
  
  /* spring constant on bbox wall */
  pullSysParmWall,

  pullSysParmLast
};

typedef struct {
  double alpha, beta, gamma, wall,
    radiusSpace, radiusScale,
    neighborTrueProb, probeProb,
    stepInitial, opporStepScale, stepScale, constraintStepMin,
    energyDecreaseMin,
    energyDecreasePopCntlMin,
    energyIncreasePermit;
} pullSysParm;  

/*
******** pullFlag* enum
**
** the various booleans
*/
enum {
  pullFlagUnknown,

  /* permute points during rebinning between iters, to randomize their
     ordering within their bin (bins are still processed in scanline
     order) */
  pullFlagPermuteOnRebin,

  /* when alpha is exactly zero, perhaps with the purpose of migrating
     particles along scale towards the scale of maximal strength while
     avoiding all inter-particle interaction, don't do any population
     control */
  pullFlagNoPopCntlWithZeroAlpha,

  /* whether or not to deny adding points to bins where there are
     close points already */
  pullFlagRestrictiveAddToBins,   

  /* if non-zero, strength is a particle- image energy term that is
     minimized by motion along scale, which in turn requires extra
     probing to determine the strength gradient along scale. */
  pullFlagEnergyFromStrength,

  /* if non-zero, nix points that got near enough to the volume edge
     that gage had to invent values for the kernel support */
  pullFlagNixAtVolumeEdgeSpace,

  /* if non-zero, during initialization, try constraint satisfaction
     (if there is a constraint) before testing whether the seedThresh
     is met.  Doing the constraint will take longer, but a point is
     more likely to meet a threshold based on feature strength */
  pullFlagConstraintBeforeSeedThresh,

  /* no binning: all particles can potentially interact (for debugging) */
  pullFlagBinSingle,

  pullFlagLast
};

typedef struct {
  int permuteOnRebin,
    noPopCntlWithZeroAlpha,
    restrictiveAddToBins,
    energyFromStrength,
    nixAtVolumeEdgeSpace,
    constraintBeforeSeedThresh,
    binSingle;
} pullFlag;

/*
******** pullContext
**
** everything for doing one computation
**
*/
typedef struct pullContext_t {
  /* INPUT ----------------------------- */

  pullInitParm initParm;           /* parms for initialization, set with
                                      the pullInit*Set() functions */
  pullIterParm iterParm;           /* parms about iterations and periods, set 
                                      with pullIterParmSet() */
  pullSysParm sysParm;             /* continuous parameters for system, set
                                      with pullSysParmSet() */
  pullFlag flag;                   /* all flags, set with pullFlagSet() */
  int verbose;                     /* verbosity level, set with 
                                      pullVerboseSet() */
  unsigned int threadNum,          /* number of threads to use, set with
                                      pullThreadNumSet() */
    rngSeed,                       /* seed value for random number generator,
                                      NOT directly related to seed point
                                      placement (as name might suggest),
                                      set with pullRngSeedSet() */
    progressBinMod;                /* progress indication by printing "."
                                      is given when the bin index mod'd by
                                      this is zero; higher numbers give
                                      less feedback, set with
                                      pullProgressBinModSet() */
  void (*iter_cb)(void *data_cb);  /* callback to call once per iter
                                      from pullRun().  This and data_cb are
                                      set with pullCallbackSet() */
  void *data_cb;                   /* data to pass to callback */
  pullVolume 
    *vol[PULL_VOLUME_MAXNUM];      /* the volumes we analyze (we DO OWN),
                                      set by either pullVolumeSingleAdd()
                                      or pullVolumeStackAdd() */
  unsigned int volNum;             /* actual length of vol[] used */
  pullInfoSpec
    *ispec[PULL_INFO_MAX+1];       /* info ii is in effect if ispec[ii] is
                                      non-NULL (and we DO OWN ispec[ii]),
                                      set by pullInfoSpecAdd() */
  int interType;                   /* from the pullInterType* enum.  This
                                      and the energy specs below are set by
                                      pullInterEnergySet() */
  pullEnergySpec *energySpecR,     /* starting point for radial potential
                                      energy function, phi_r */
    *energySpecS,                  /* second energy potential function, for
                                      scale-space behavior, phi_s */
    *energySpecWin;                /* function used to window phi_r along s,
                                      and phi_s along r, for use with 
                                      pullInterTypeAdditive */

  double sliceNormal[3];           /* until this is set per-volume as it
                                      should be, it lives here. Zero-length
                                      normal means "not set". NOTE: Since this
                                      is currently a hack, there isn't a 
                                      pull*Set() function for it */

  /* INTERNAL -------------------------- */

  double bboxMin[4], bboxMax[4];   /* scale-space bounding box of all volumes:
                                      region over which binning is defined.
                                      In 3-D space, the bbox is axis aligned,
                                      even when the volume is not so aligned,
                                      which means that some bins might be
                                      under- or un- utilized, oh well. 
                                      bboxMin[3] and bboxMax[3] are the 
                                      bounds of the volume in *scale* (sigma),
                                      not t, or tau */
  unsigned int infoTotalLen,       /* total length of the info buffers needed,
                                      which determines size of allocated
                                      binPoint */
    infoIdx[PULL_INFO_MAX+1];      /* index of answer within pullPoint->info */
  unsigned int idtagNext;          /* next per-point igtag value */
  int haveScale,                   /* non-zero iff one of the volumes is in
                                      scale-space */
    constraint,                    /* if non-zero, we have a constraint to
                                      satisfy, and this is its info number  */
    finished;                      /* used to signal all threads to return */
  double maxDistSpace,             /* max dist of point-point interaction in 
                                      the spatial axes.*/
    maxDistScale,                  /* max dist of point-point interaction 
                                      along scale */
    constraintDim,                 /* dimension (possibly non-integer) of
				      (spatial) constraint surface we're
                                      working on, or 0 if no constraint, or
                                      if we have a constraint, but we're using
                                      tangent mode, so the constraint dim is
                                      per-point */
    targetDim,                     /* dimension (possibly non-integer) of
                                      surface we'd like to be sampling, which
                                      can be different than constraintDim
                                      because of scale-space, and either 
                                      repulsive (+1) or attractive (+0)
                                      behavior along scale */
    voxelSizeSpace,                /* mean spatial voxel edge length, for
                                      limiting travel distance for descent
                                      and constraint satisfaction */
    voxelSizeScale;                /* mean voxel edge length in space, for
                                      limiting travel (along scale) distance
                                      during descent */
  pullBin *bin;                    /* volume of bins (see binsEdge, binNum) */
  unsigned int binsEdge[4],        /* # bins along each volume edge,
                                      determined by maxEval and scale */
    binNum,                        /* total # bins in grid */
    binNextIdx;                    /* next bin of points to be processed,
                                      we're done when binNextIdx == binNum */
  double _sliceNormal[3];          /* normalized version of sliceNormal.
                                      Still, zero-length means "not set". */
  unsigned int *tmpPointPerm;      /* storing points during rebinning */
  pullPoint **tmpPointPtr;
  unsigned int tmpPointNum;

  airThreadMutex *binMutex;        /* mutex around bin, needed because bins
                                      are the unit of work for the tasks */
  pullTask **task;                 /* dynamically allocated array of tasks */
  airThreadBarrier *iterBarrierA;  /* barriers between iterations */
  airThreadBarrier *iterBarrierB;  /* barriers between iterations */

  /* OUTPUT ---------------------------- */

  double timeIteration,            /* time needed for last (single) iter */
    timeRun,                       /* total time spent in pullRun() */
    energy;                        /* final energy of system */
  unsigned int addNum,             /* # prtls added by PopCntl in last iter */
    nixNum,                        /* # prtls nixed by PopCntl in last iter */
    stuckNum,                      /* # stuck particles in last iter */
    pointNum,                      /* total # particles */
    CCNum,                         /* # connected components */
    iter;                          /* how many iterations were needed */
} pullContext;

/* defaultsPull.c */
PULL_EXPORT int pullPhistEnabled;
PULL_EXPORT const char *pullBiffKey;

/* initPull.c */
PULL_EXPORT int pullInitRandomSet(pullContext *pctx, unsigned int numInitial);
PULL_EXPORT int pullInitPointPerVoxelSet(pullContext *pctx, int pointPerVoxel,
                                         unsigned int zSlcMin,
                                         unsigned int zSlcMax,
                                         unsigned int alongScaleNum,
                                         double jitter);
PULL_EXPORT int pullInitGivenPosSet(pullContext *pctx, const Nrrd *npos);
PULL_EXPORT int pullInitLiveThreshUseSet(pullContext *pctx, int liveThreshUse);
PULL_EXPORT int pullInitUnequalShapesAllowSet(pullContext *pctx, int allow);

/* parmPull.c */
PULL_EXPORT int pullIterParmSet(pullContext *pctx, int which,
                                unsigned int pval);
PULL_EXPORT int pullSysParmSet(pullContext *pctx, int which,
                               double pval);
PULL_EXPORT int pullFlagSet(pullContext *pctx, int which, int flag);
PULL_EXPORT int pullVerboseSet(pullContext *pctx, int verbose);
PULL_EXPORT int pullThreadNumSet(pullContext *pctx, unsigned int threadNum);
PULL_EXPORT int pullRngSeedSet(pullContext *pctx, unsigned int rngSeed);
PULL_EXPORT int pullProgressBinModSet(pullContext *pctx, unsigned int bmod);
PULL_EXPORT int pullCallbackSet(pullContext *pctx,
                                void (*iter_cb)(void *data_cb),
                                void *data_cb);
PULL_EXPORT int pullInterEnergySet(pullContext *pctx, int interType,
                                   const pullEnergySpec *enspR,
                                   const pullEnergySpec *enspS,
                                   const pullEnergySpec *enspWin);

/* energy.c */
PULL_EXPORT const airEnum *const pullInterType;
PULL_EXPORT const airEnum *const pullEnergyType;
PULL_EXPORT const pullEnergy *const pullEnergyUnknown;
PULL_EXPORT const pullEnergy *const pullEnergySpring;
PULL_EXPORT const pullEnergy *const pullEnergyGauss;
PULL_EXPORT const pullEnergy *const pullEnergyButterworth;
PULL_EXPORT const pullEnergy *const pullEnergyCotan;
PULL_EXPORT const pullEnergy *const pullEnergyCubic;
PULL_EXPORT const pullEnergy *const pullEnergyQuartic;
PULL_EXPORT const pullEnergy *const pullEnergyCubicWell;
PULL_EXPORT const pullEnergy *const pullEnergyZero;
PULL_EXPORT const pullEnergy *const pullEnergyButterworthParabola;
PULL_EXPORT const pullEnergy *const pullEnergyAll[PULL_ENERGY_TYPE_MAX+1];
PULL_EXPORT pullEnergySpec *pullEnergySpecNew();
PULL_EXPORT void pullEnergySpecSet(pullEnergySpec *ensp,
                                   const pullEnergy *energy,
                                   const double parm[PULL_ENERGY_PARM_NUM]);
PULL_EXPORT void pullEnergySpecCopy(pullEnergySpec *esDst,
                                    const pullEnergySpec *esSrc);
PULL_EXPORT pullEnergySpec *pullEnergySpecNix(pullEnergySpec *ensp);
PULL_EXPORT int pullEnergySpecParse(pullEnergySpec *ensp, const char *str);
PULL_EXPORT hestCB *pullHestEnergySpec;

/* volumePull.c */
PULL_EXPORT pullVolume *pullVolumeNew();
PULL_EXPORT pullVolume *pullVolumeNix(pullVolume *vol);
PULL_EXPORT int pullVolumeSingleAdd(pullContext *pctx, 
                                    const gageKind *kind, 
                                    char *name, const Nrrd *nin,
                                    const NrrdKernelSpec *ksp00,
                                    const NrrdKernelSpec *ksp11,
                                    const NrrdKernelSpec *ksp22);
PULL_EXPORT int pullVolumeStackAdd(pullContext *pctx,
                                   const gageKind *kind, 
                                   char *name, 
                                   const Nrrd *nin,
                                   const Nrrd *const *ninSS,
                                   double *scalePos,
                                   unsigned int ninNum,
                                   int scaleDerivNorm,
                                   const NrrdKernelSpec *ksp00,
                                   const NrrdKernelSpec *ksp11,
                                   const NrrdKernelSpec *ksp22,
                                   const NrrdKernelSpec *kspSS);
PULL_EXPORT const pullVolume *pullVolumeLookup(const pullContext *pctx,
                                               const char *volName);

/* kindPull.c */
PULL_EXPORT const airEnum *const pullVal;
PULL_EXPORT gageKind *pullValGageKind;

/* infoPull.c */
PULL_EXPORT const airEnum *const pullInfo;
PULL_EXPORT unsigned int pullInfoAnswerLen(int info);
PULL_EXPORT pullInfoSpec *pullInfoSpecNew();
PULL_EXPORT pullInfoSpec *pullInfoSpecNix(pullInfoSpec *ispec);
PULL_EXPORT int pullInfoSpecAdd(pullContext *pctx, pullInfoSpec *ispec);

/* contextPull.c */
PULL_EXPORT pullContext *pullContextNew(void);
PULL_EXPORT pullContext *pullContextNix(pullContext *pctx);
PULL_EXPORT int pullOutputGet(Nrrd *nPosOut, Nrrd *nTenOut,
                              Nrrd *nStrengthOut,
                              const double scaleVec[3], double scaleRad,
                              pullContext *pctx);
PULL_EXPORT int pullPositionHistoryGet(limnPolyData *pld, pullContext *pctx);
PULL_EXPORT int pullPropGet(Nrrd *nprop, int prop, pullContext *pctx);

/* pointPull.c */
PULL_EXPORT unsigned int pullPointNumber(const pullContext *pctx);
PULL_EXPORT pullPoint *pullPointNew(pullContext *pctx);
PULL_EXPORT pullPoint *pullPointNix(pullPoint *pnt);

/* binningPull.c */
PULL_EXPORT int pullBinsPointAdd(pullContext *pctx, pullPoint *point,
                                 /* output */
                                 pullBin **binUsed);
PULL_EXPORT int pullBinsPointMaybeAdd(pullContext *pctx, pullPoint *point, 
                                      /* output */
                                      pullBin **binUsed, int *added);
PULL_EXPORT void pullBinsNeighborSet(pullContext *pctx);

/* actionPull.c */
PULL_EXPORT const airEnum *const pullProcessMode;
PULL_EXPORT int pullBinProcess(pullTask *task, unsigned int myBinIdx);
PULL_EXPORT int pullGammaLearn(pullContext *pctx);

/* corePull.c */
PULL_EXPORT int pullStart(pullContext *pctx);
PULL_EXPORT int pullRun(pullContext *pctx);
PULL_EXPORT int pullFinish(pullContext *pctx);

/* ccPull.c */
PULL_EXPORT int pullCCFind(pullContext *pctx);
PULL_EXPORT int pullCCMeasure(pullContext *pctx, Nrrd *nmeas,
                              int measrInfo, double rho);
PULL_EXPORT int pullCCSort(pullContext *pctx, int measrInfo, double rho);

#ifdef __cplusplus
}
#endif

#endif /* PULL_HAS_BEEN_INCLUDED */
