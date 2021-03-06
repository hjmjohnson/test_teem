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

#include <teem/air.h>
#include <teem/hest.h>
#include <teem/biff.h>
#include <teem/nrrd.h>
#include <teem/gage.h>
#include <teem/limn.h>
#include <teem/hoover.h>


#define MREND "mrender"

char *info = ("A demonstration of hoover, gage, and nrrd measures. "
	      "Uses hoover to cast rays through a scalar volume, gage to "
	      "measure one of various quantities along the rays, and a "
	      "specified nrrd measure to reduce all the values along a ray "
	      "down to one scalar, which is saved in the output (float) "
	      "image.");

/* -------------------------------------------------------------- */

/* Even though the gageContext is really thread-specific, and
   therefore doesn't really belong in mrendUser, the first context
   from which all others is copied is logically shared across threads,
   as are the input parameter it contains. There is a per-thread
   gageContext pointer in mrendThread */

typedef struct {
  Nrrd *nin;            /* input volume to render */
  double rayStep;       /* distance between sampling planes */
  int whatq,            /* what to measure along the ray */
    measr;              /* how to reduce the ray samples to a scalar */
  /* we have a seperate copy of the kernel specs so that hest can
     set these, and then we'll gageKernelSet() them in the context
     in order to do the proper error checking- hest can't do the
     error checking that we need... */
  NrrdKernelSpec *ksp[GAGE_KERNEL_NUM];
  gageContext *gctx0;   /* gage input and parent thread state */
  hooverContext *hctx;  /* hoover input and state */
  char *outS;           /* (managed by hest) output filename */

  airArray *mrmop;
} mrendUser;

mrendUser *
mrendUserNew() {
  mrendUser *uu;
  int i;

  uu = (mrendUser *)calloc(1, sizeof(mrendUser));
  uu->nin = NULL;
  uu->rayStep = 0.0;
  uu->whatq = gageSclUnknown;
  uu->measr = nrrdMeasureUnknown;
  for (i=0; i<GAGE_KERNEL_NUM; i++) {
    uu->ksp[i] = NULL;
  }
  uu->gctx0 = gageContextNew();
  uu->hctx = hooverContextNew();
  uu->outS = NULL;
  uu->mrmop = airMopNew();
  airMopAdd(uu->mrmop, uu->gctx0, (airMopper)gageContextNix, airMopAlways);
  airMopAdd(uu->mrmop, uu->hctx, (airMopper)hooverContextNix, airMopAlways);
  return uu;
}

mrendUser *
mrendUserNix(mrendUser *uu) {

  if (uu) {
    airMopOkay(uu->mrmop);
    AIR_FREE(uu);
  }
  return NULL;
}

int
mrendUserCheck(mrendUser *uu) {
  char me[]="mrendUserCheck", err[AIR_STRLEN_MED];
  
  if (3 != uu->nin->dim) {
    sprintf(err, "%s: input nrrd needs 3 dimensions, not %d", 
	    me, uu->nin->dim);
    biffAdd(MREND, err); return 1;
  }
  if (!( uu->nin->axis[0].center == uu->nin->axis[1].center &&
	 uu->nin->axis[0].center == uu->nin->axis[2].center )) {
    sprintf(err, "%s: axes 0,1,2 centerings (%s,%s,%s) not equal", me,
	    airEnumStr(nrrdCenter, uu->nin->axis[0].center),
	    airEnumStr(nrrdCenter, uu->nin->axis[1].center),
	    airEnumStr(nrrdCenter, uu->nin->axis[2].center));
    biffAdd(MREND, err); return 1;
  }
  if (1 != gageKindScl->ansLength[uu->whatq]) {
    sprintf(err, "%s: quantity %s isn't a scalar; can't render it\n",
	    me, airEnumStr(gageKindScl->enm, uu->whatq));
    biffAdd(MREND, err); return 1;
  }
  
  return 0;
}

/* -------------------------------------------------------------- */

typedef struct mrendRender_t {
  double time0, time1;  /* render start and end times */
  Nrrd *nout;           /* output image: always 2D array of floats */
  float *imgData;       /* output image data */
  int sx, sy,           /* image dimensions */
    totalSamples;       /* total number of samples used for all rays */
  struct mrendThread_t *tinfo[HOOVER_THREAD_MAX];
} mrendRender;

typedef struct mrendThread_t {
  float *val,        /* array of ray samples */
    rayLen,          /* length of ray segment between near and far */
    rayStep;         /* ray step needed FOR THIS RAY, to acheive sampling on
			planes (same scaling of uu->rayStep) */
  int thrid,         /* thread ID */
    valLen,          /* allocated size of val */
    valNum,          /* number of values set in val (index of next value) */
    ui, vi,          /* image coords */
    numSamples,      /* total number of samples this thread has done */
    verbose;         /* blah blah blah blah */
  gageContext *gctx; /* thread-specific gage context (or copy of uu->gctx0
			for the first thread) */
  gage_t *answer;    /* pointer to the SINGLE answer we care about */
} mrendThread;

int
mrendRenderBegin(mrendRender **rrP, mrendUser *uu) {
  char me[]="mrendRenderBegin", err[AIR_STRLEN_MED];
  gagePerVolume *pvl;
  int E, thr;

  /* this assumes that mrendUserCheck(uu) has passed */

  *rrP = (mrendRender *)calloc(1, sizeof(mrendRender));
  airMopAdd(uu->mrmop, *rrP, airFree, airMopAlways);
  /* pvl managed via parent context */

  (*rrP)->time0 = airTime();

  E = 0;
  if (!E) E |= !(pvl = gagePerVolumeNew(uu->gctx0, uu->nin, gageKindScl));
  if (!E) E |= gagePerVolumeAttach(uu->gctx0, pvl);
  if (!E) E |= gageKernelSet(uu->gctx0, gageKernel00,
			     uu->ksp[gageKernel00]->kernel,
			     uu->ksp[gageKernel00]->parm);
  if (!E) E |= gageKernelSet(uu->gctx0, gageKernel11,
			     uu->ksp[gageKernel11]->kernel,
			     uu->ksp[gageKernel11]->parm);
  if (!E) E |= gageKernelSet(uu->gctx0, gageKernel22,
			     uu->ksp[gageKernel22]->kernel,
			     uu->ksp[gageKernel22]->parm);
  if (!E) E |= gageQuerySet(uu->gctx0, pvl, 1 << uu->whatq);
  if (!E) E |= gageUpdate(uu->gctx0);
  if (E) {
    sprintf(err, "%s: gage trouble", me);
    biffMove(MREND, err, GAGE);
    return 1;
  }
  fprintf(stderr, "%s: kernel support = %d^3 samples\n", me,
	  GAGE_FD(uu->gctx0));

  if (nrrdMaybeAlloc((*rrP)->nout=nrrdNew(), nrrdTypeFloat, 2,
		     uu->hctx->imgSize[0], uu->hctx->imgSize[1])) {
    sprintf(err, "%s: nrrd trouble", me);
    biffMove(MREND, err, NRRD);
    return 1;
  }
  (*rrP)->nout->axis[0].min = uu->hctx->cam->uRange[0];
  (*rrP)->nout->axis[0].max = uu->hctx->cam->uRange[1];
  (*rrP)->nout->axis[1].min = uu->hctx->cam->vRange[0];
  (*rrP)->nout->axis[1].max = uu->hctx->cam->vRange[1];
  airMopAdd(uu->mrmop, (*rrP)->nout, (airMopper)nrrdNuke, airMopAlways);
  (*rrP)->imgData = (*rrP)->nout->data;
  (*rrP)->sx = uu->hctx->imgSize[0];
  (*rrP)->sy = uu->hctx->imgSize[1];

  for (thr=0; thr<uu->hctx->numThreads; thr++) {
    (*rrP)->tinfo[thr] = (mrendThread *)calloc(1, sizeof(mrendThread));
    airMopAdd(uu->mrmop, (*rrP)->tinfo[thr], airFree, airMopAlways);
  }

  return 0;
}

int
mrendRenderEnd(mrendRender *rr, mrendUser *uu) {
  char me[]="mrendRenderEnd", err[AIR_STRLEN_MED];
  int thr;

  /* add up # samples from all threads */
  rr->totalSamples = 0;
  for (thr=0; thr<uu->hctx->numThreads; thr++) {
    rr->totalSamples += rr->tinfo[thr]->numSamples;
  }

  rr->time1 = airTime();
  fprintf(stderr, "\n");
  fprintf(stderr, "%s: rendering time = %g secs\n", me,
	  rr->time1 - rr->time0);
  fprintf(stderr, "%s: sampling rate = %g KHz\n", me,
	  rr->totalSamples/(1000.0*(rr->time1 - rr->time0)));
  if (nrrdSave(uu->outS, rr->nout, NULL)) {
    sprintf(err, "%s: trouble saving image", me);
    biffMove(MREND, err, NRRD);
    return 1;
  }

  return 0;
}

/* -------------------------------------------------------------- */

int
mrendThreadBegin(mrendThread **ttP,
		 mrendRender *rr, mrendUser *uu, int whichThread) {

  /* allocating the mrendThreads should be part of the thread body,
     but as long as there isn't a mutex around registering them with
     the airMop in the mrendRender, then all that needs to be done
     as part of mrendRenderBegin (see above) */
  (*ttP) = rr->tinfo[whichThread];
  if (!whichThread) {
    /* this is the first thread- it just points to the parent gageContext */
    (*ttP)->gctx = uu->gctx0;
  } else {
    /* we have to generate a new gageContext */
    (*ttP)->gctx = gageContextCopy(uu->gctx0);
  }
  (*ttP)->answer = gageAnswerPointer((*ttP)->gctx,
				     (*ttP)->gctx->pvl[0], uu->whatq);
  (*ttP)->val = NULL;
  (*ttP)->valLen = 0;
  (*ttP)->valNum = 0;
  (*ttP)->rayLen = 0;
  (*ttP)->thrid = whichThread;
  (*ttP)->numSamples = 0;
  return 0;
}

int
mrendThreadEnd(mrendThread *tt, mrendRender *rr, mrendUser *uu) {
  
  AIR_FREE(tt->val);

  return 0;
}

/* -------------------------------------------------------------- */

int
mrendRayBegin(mrendThread *tt, mrendRender *rr, mrendUser *uu,
	      int uIndex,
	      int vIndex,
	      double rayLen,
	      double rayStartWorld[3],
	      double rayStartIndex[3],
	      double rayDirWorld[3],
	      double rayDirIndex[3]) {
  int newLen;

  tt->ui = uIndex;
  tt->vi = vIndex;
  tt->rayLen = rayLen;
  tt->rayStep = (uu->rayStep*tt->rayLen /
		 (uu->hctx->cam->vspFaar - uu->hctx->cam->vspNeer));
  newLen = AIR_ROUNDUP(rayLen/tt->rayStep) + 1;
  if (!tt->val || newLen > tt->valLen) {
    AIR_FREE(tt->val);
    tt->valLen = newLen;
    tt->val = (float*)calloc(newLen, sizeof(float));
  }
  tt->valNum = 0;
  if (!uIndex) {
    fprintf(stderr, "%d/%d ", vIndex, uu->hctx->imgSize[1]);
    fflush(stderr);
  }

  fflush(stderr);
  return 0;
}

int
mrendRayEnd(mrendThread *tt, mrendRender *rr, mrendUser *uu) {
  float answer;

  if (tt->valNum) {
    nrrdMeasureLine[uu->measr](&answer,
			       nrrdTypeFloat,
			       tt->val, nrrdTypeFloat,
			       tt->valNum,
			       0, tt->rayLen);
    /* this silliness is because when using a histo-based
       nrrdMeasure, and if the values where all zero,
       then the answer will probably be NaN */
    answer = AIR_EXISTS(answer) ? answer : 0.0;
    rr->imgData[(tt->ui) + (rr->sx)*(tt->vi)] = answer;
  } else {
    rr->imgData[(tt->ui) + (rr->sx)*(tt->vi)] = 0.0;
  }

  return 0;
}

/* -------------------------------------------------------------- */

double
mrendSample(mrendThread *tt, mrendRender *rr, mrendUser *uu,
	    int num, double rayT,
	    int inside,
	    double samplePosWorld[3],
	    double samplePosIndex[3]) {
  char me[]="mrendSample", err[AIR_STRLEN_MED];

  if (inside) {
    if (gageProbe(tt->gctx,
		  samplePosIndex[0], samplePosIndex[1], samplePosIndex[2])) {
      sprintf(err, "%s: gage trouble: %s (%d)", me, gageErrStr, gageErrNum);
      biffAdd(MREND, err);
      return AIR_NAN;
    }
    tt->val[tt->valNum++] = *(tt->answer);
    tt->numSamples++;
  }
  
  return tt->rayStep;
}

/* -------------------------------------------------------------- */

/*
** learned: if you're playing games with strings with two passes, where
** you first generate the set of strings in order to calculate their
** cumulative length, and then (2nd pass) concatenate the strings
** together, be very sure that the generation of the strings on the
** two passes is identical.  Had a very troublesome memory error because
** I was using short version of the description string to determine
** allocation, and then the long version in the second pass...
*/
char *
mrendGage(char *prefix) {
  char *line, *ret;
  int i, len;
  
  /* 1st pass through- determine needed buffer size */
  len = 0;
  for (i=airEnumUnknown(gageScl)+1; !airEnumValCheck(gageScl, i); i++) {
    if (1 == gageKindScl->ansLength[i]) {
      line = airEnumFmtDesc(gageScl, i, AIR_FALSE, "\n \b\bo \"%s\": %s");
      len += strlen(line);
      free(line);
    }
  }
  ret = (char*)calloc(strlen(prefix) + len + 1, sizeof(char));
  if (ret) {
    strcpy(ret, prefix);
    /* 2nd pass through: create output */
    for (i=airEnumUnknown(gageScl)+1; !airEnumValCheck(gageScl, i); i++) {
      if (1 == gageKindScl->ansLength[i]) {
	line = airEnumFmtDesc(gageScl, i, AIR_FALSE, "\n \b\bo \"%s\": %s");
	strcat(ret, line);
	free(line);
      }
    }
  }
  return ret;
}

int
main(int argc, char *argv[]) {
  hestOpt *hopt=NULL;
  hestParm *hparm;
  int E, Ecode, renorm;
  char *me, *errS, *buff;
  mrendUser *uu;
  airArray *mop;
  double gmc;

  me = argv[0];
  mop = airMopNew();
  hparm = hestParmNew();
  hparm->respFileEnable = AIR_TRUE;
  uu = mrendUserNew();

  airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
  airMopAdd(mop, uu, (airMopper)mrendUserNix, airMopAlways);

  buff = mrendGage("the quantity to measure at sample points along " \
		   "rays. Possibilities include:");
  airMopAdd(mop, buff, airFree, airMopAlways);

  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &(uu->nin), NULL,
	     "input nrrd to render", NULL, NULL, nrrdHestNrrd);
  limnHestCamOptAdd(&hopt, uu->hctx->cam,
		    NULL, "0 0 0", "0 0 1",
		    NULL, NULL, NULL,
		    "-1 1", "-1 1");
  hestOptAdd(&hopt, "is", "image size", airTypeInt, 2, 2, uu->hctx->imgSize,
	     "256 256", "image dimensions");
  hestOptAdd(&hopt, "k00", "kernel", airTypeOther, 1, 1,
	     &(uu->ksp[gageKernel00]), "tent",
	     "value reconstruction kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k11", "kernel", airTypeOther, 1, 1,
	     &(uu->ksp[gageKernel11]), "cubicd:1,0",
	     "first derivative kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "k22", "kernel", airTypeOther, 1, 1,
	     &(uu->ksp[gageKernel22]), "cubicdd:1,0",
	     "second derivative kernel",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "rn", NULL, airTypeBool, 0, 0, &renorm, NULL,
	     "renormalize kernel weights at each new sample location. "
	     "\"Accurate\" kernels don't need this; doing it always "
	     "makes things go slower");
  hestOptAdd(&hopt, "q", "quantity", airTypeEnum, 1, 1, &(uu->whatq), NULL,
	     buff,
	     NULL, gageKindScl->enm);
  hestOptAdd(&hopt, "m", "measure", airTypeEnum, 1, 1, &(uu->measr), NULL,
	     "how to collapse list of ray samples into one scalar. "
	     NRRD_MEASURE_DESC,
	     NULL, nrrdMeasure);
  hestOptAdd(&hopt, "gmc", "min gradmag", airTypeDouble, 1, 1, &gmc, "0.0",
	     "For curvature-related queries, set answer to zero when "
	     "gradient magnitude is below this");
  hestOptAdd(&hopt, "step", "size", airTypeDouble, 1, 1, &(uu->rayStep),
	     "0.01", "step size along ray in world space");
  hestOptAdd(&hopt, "nt", "# threads", airTypeInt, 1, 1,
	     &(uu->hctx->numThreads),
	     "1", "number of threads hoover should use");
  hestOptAdd(&hopt, "o", "filename", airTypeString, 1, 1, &(uu->outS),
	     NULL, "file to write output nrrd to");
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);

  hestParseOrDie(hopt, argc-1, argv+1, hparm,
		 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  if (mrendUserCheck(uu)) {
    fprintf(stderr, "%s: problem with input parameters:\n%s\n",
	    me, errS = biffGetDone(MREND)); free(errS);
    airMopError(mop);
    return 1;
  }
  gageParmSet(uu->gctx0, gageParmGradMagCurvMin, gmc);
  gageParmSet(uu->gctx0, gageParmRenormalize, renorm);
  fprintf(stderr, "%s: will render %s of %s\n", me,
	  airEnumStr(nrrdMeasure, uu->measr),
	  airEnumStr(gageKindScl->enm, uu->whatq));
  
  /* set remaining fields of hoover context */
  nrrdAxisInfoGet_nva(uu->nin, nrrdAxisInfoSize, uu->hctx->volSize);
  nrrdAxisInfoGet_nva(uu->nin, nrrdAxisInfoSpacing, uu->hctx->volSpacing);
  if (nrrdCenterUnknown != uu->nin->axis[0].center) {
    uu->hctx->volCentering = uu->nin->axis[0].center;
    fprintf(stderr, "%s: setting volCentering to %s\n", me,
	    airEnumStr(nrrdCenter, uu->nin->axis[0].center));
  }
  /* this is reasonable for now */
  uu->hctx->imgCentering = nrrdCenterCell;
  uu->hctx->user = uu;
  uu->hctx->renderBegin = (hooverRenderBegin_t *)mrendRenderBegin;
  uu->hctx->threadBegin = (hooverThreadBegin_t *)mrendThreadBegin;
  uu->hctx->rayBegin = (hooverRayBegin_t *)mrendRayBegin;
  uu->hctx->sample = (hooverSample_t *)mrendSample;
  uu->hctx->rayEnd = (hooverRayEnd_t *)mrendRayEnd;
  uu->hctx->threadEnd = (hooverThreadEnd_t *)mrendThreadEnd;
  uu->hctx->renderEnd = (hooverRenderEnd_t *)mrendRenderEnd;

  if (!hooverMyPthread) {
    fprintf(stderr, "%s: This teem not compiled with pthread support.\n", me);
    fprintf(stderr, "%s: --> can't use %d threads; only using 1\n",
	    me, uu->hctx->numThreads);
    uu->hctx->numThreads = 1;
  }

  E = hooverRender(uu->hctx, &Ecode, NULL);
  if (E) {
    if (hooverErrInit == E) {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(HOOVER)); free(errS);
    } else {
      fprintf(stderr, "%s: ERROR:\n%s\n",
	      me, errS = biffGetDone(MREND)); free(errS);
    }
    airMopError(mop);
    return 1;
  }
  
  airMopOkay(mop);
  return 0;
}
