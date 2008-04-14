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

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "pull.h"
#include "privatePull.h"

/*
** issues:
** does everything work on the first iteration
** how to handle the needed extra probe for d strength / d scale
** does it eventually catch non-existant energy or force
** how are force/energy along scale handled differently than in space?
*/

unsigned int
_neighBinPoints(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="_neighBinPoints";
  unsigned int nn, herPointIdx, herBinIdx;
  pullBin *herBin;
  pullPoint *herPoint;

  nn = 0;
  herBinIdx = 0;
  while ((herBin = bin->neighBin[herBinIdx])) {
    for (herPointIdx=0; herPointIdx<herBin->pointNum; herPointIdx++) {
      herPoint = herBin->point[herPointIdx];
      /*
      fprintf(stderr, "!%s(%u): neighbin %u has point %u\n", me,
              point->idtag, herBinIdx, herPoint->idtag);
      */
      /* can't interact with myself */
      if (point != herPoint) {
        if (nn < _PULL_NEIGH_MAXNUM) {
          task->neighPoint[nn++] = herPoint;
          /*
          fprintf(stderr, "%s(%u): neighPoint[%u] = %u\n", 
                  me, point->idtag, nn-1, herPoint->idtag);
          */
        } else {
          fprintf(stderr, "%s: maxed out at %u points!\n", me, 
                  _PULL_NEIGH_MAXNUM);
        }
      }
    }
    herBinIdx++;
  }
  return nn;
}


double
_energyInterParticle(pullTask *task, pullPoint *me, pullPoint *she, 
                     /* output */
                     double egrad[4]) {
  char meme[]="_energyInterParticle";
  double spadist, sparad, diff[4], rr, enr, frc, *parm;

  ELL_4V_SUB(diff, she->pos, me->pos);
  spadist = ELL_3V_LEN(diff);
  sparad = task->pctx->radiusSpace;
  
  rr = spadist/(2*sparad);
  /*
  fprintf(stderr, "!%s: rr(%u,%u) = %g\n", meme, me->idtag, she->idtag, rr);
  */
  if (rr > 1) {
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }
  if (rr == 0) {
    fprintf(stderr, "%s: pos of pts %u, %u equal: (%g,%g,%g,%g)\n",
            meme, me->idtag, she->idtag, 
            me->pos[0], me->pos[1], me->pos[2], me->pos[3]);
    ELL_4V_SET(egrad, 0, 0, 0, 0);
    return 0;
  }

  parm = task->pctx->energySpec->parm;
  enr = task->pctx->energySpec->energy->eval(&frc, rr, parm);
  frc *= -1.0/(2*sparad*spadist);
  ELL_3V_SCALE(egrad, frc, diff);
  egrad[3] = 0;
  /*
  fprintf(stderr, "%s: %u <-- %u = %g,%g,%g -> egrad = %g,%g,%g, enr = %g\n",
          meme, me->idtag, she->idtag, 
          diff[0], diff[1], diff[2],
          egrad[0], egrad[1], egrad[2], enr);
  */
  return enr;
}

/*
** meanNeighDist is allowed to be NULL. If it is non-NULL, 
** it must be set, and must be < 0 if there are no neighbors
*/
double
_energyFromPoints(pullTask *task, pullBin *bin, pullPoint *point, 
                  /* output */
                  double egradSum[4], double *meanNeighDist) {
  /* char me[]="_energyFromPoints"; */
  double energySum, distSqSum, spaDistSqMax;
  int nopt,     /* optimiziation: we sometimes re-use neighbor lists */
    ntrue;      /* we search all possible neighbors, stored in the bins
                   (either because !nopt, or, this iter we learn true
                   subset of interacting neighbors).  This could also
                   be called "dontreuse" or something like that */
  unsigned int nidx, neighCount,
    nnum;       /* how much of task->neigh[] we use */

  if (meanNeighDist && !egradSum) {
    /* shouldn't happen */
    return AIR_NAN;
  }
  
  /* set nopt and ntrue */
  if (task->pctx->neighborTrueProb < 1) {
    nopt = AIR_TRUE;
    if (egradSum) {
      /* We allow the neighbor list optimization only when we're also asked
         to compute the energy gradient.  When we're not getting the energy
         gradient, we're being called to test the waters at possible new
         locations, in which case we can't be changing the effective particle 
         neighborhood */
      ntrue = (0 == task->pctx->iter
               || airDrandMT_r(task->rng) < task->pctx->neighborTrueProb);
    } else {
      ntrue = AIR_FALSE;
    }
  } else {
    nopt = AIR_FALSE;
    ntrue = AIR_TRUE;
  }
  /*
  fprintf(stderr, "!%s(%u), nopt = %d, ntrue = %d\n", me, point->idtag,
          nopt, ntrue);
  */
  /* set nnum and task->neigh[] */
  if (ntrue) {
    nnum = _neighBinPoints(task, bin, point);
    if (nopt) {
      airArrayLenSet(point->neighArr, 0);
    }
  } else {
    /* (nopt true) this iter we re-use existing neighbor list */
    nnum = point->neighNum;
    for (nidx=0; nidx<nnum; nidx++) {
      task->neighPoint[nidx] = point->neighPoint[nidx];
    }
  }

  /* loop through neighbor points */
  spaDistSqMax = 4*task->pctx->radiusSpace*task->pctx->radiusSpace;
  /*
  fprintf(stderr, "%s: radiusSpace = %g -> spaDistSqMax = %g\n", me,
          task->pctx->radiusSpace, spaDistSqMax);
  */
  energySum = 0;
  distSqSum = 0;
  neighCount = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  for (nidx=0; nidx<nnum; nidx++) {
    double diff[4], spaDistSq, enr, egrad[4];
    pullPoint *herPoint;
    herPoint = task->neighPoint[nidx];
    ELL_4V_SUB(diff, herPoint->pos, point->pos);
    spaDistSq = ELL_3V_DOT(diff, diff);
    /*
    fprintf(stderr, "!%s: %u:%g,%g,%g <-- %u:%g,%g,%g = sqd %g %s %g\n", me,
            point->idtag, point->pos[0], point->pos[1], point->pos[2], 
            herPoint->idtag,
            herPoint->pos[0], herPoint->pos[1], herPoint->pos[2],
            spaDistSq, spaDistSq > spaDistSqMax ? ">" : "<=", spaDistSqMax);
    */
    if (spaDistSq > spaDistSqMax) {
      continue;
    }
    if (AIR_ABS(diff[3] > task->pctx->radiusScale)) {
      continue;
    }
    enr = _energyInterParticle(task, point, herPoint, egrad);
    /*
    fprintf(stderr, "!%s: energySum = %g + %g = %g\n", me, 
            energySum, enr, energySum + enr);
    */
    energySum += enr;
    if (egradSum) {
      ELL_4V_INCR(egradSum, egrad);
      if (ELL_4V_DOT(egrad, egrad)) {
        if (meanNeighDist) {
          neighCount++;
          distSqSum += spaDistSq;
        }
        if (nopt && ntrue) {
          unsigned int ii;
          ii = airArrayLenIncr(point->neighArr, 1);
          point->neighPoint[ii] = herPoint;
        }
      }
    }
  }
  
  /* finish computing mean distance to neighbors */
  if (meanNeighDist) {
    if (neighCount) {
      *meanNeighDist = sqrt(distSqSum/neighCount);
    } else {
      *meanNeighDist = -1;
    }
  }

  return energySum;
}

double
_energyFromImage(pullTask *task, pullPoint *point,
                 /* output */
                 double egradSum[4]) {
  char me[]="_energyFromImage";
  double energy, egrad[4];

  /* not sure I have the logic for this right 
  int ptrue;
  if (task->pctx->probeProb < 1 && allowProbeProb) {
    if (egrad) {
      ptrue = (0 == task->pctx->iter
               || airDrandMT_r(task->rng) < task->pctx->probeProb);
    } else {
      ptrue = AIR_FALSE;
    }
  } else {
    ptrue = AIR_TRUE;
  }
  */
  if (_pullProbe(task, point)) {
    fprintf(stderr, "%s: problem probing!!!\n%s\n", me, biffGetDone(PULL));
  }
  energy = 0;
  if (egradSum) {
    ELL_4V_SET(egradSum, 0, 0, 0, 0);
  }
  if (task->pctx->ispec[pullInfoHeight]
      && !task->pctx->ispec[pullInfoHeight]->constraint) {
    energy += _pullPointScalar(task->pctx, point, pullInfoHeight,
                               egrad, NULL);
    if (egradSum) {
      ELL_4V_INCR(egradSum, egrad);
    }
  }
  if (task->pctx->ispec[pullInfoIsosurfaceValue]
      && !task->pctx->ispec[pullInfoIsosurfaceValue]->constraint) {
    /* we're only going towards an isosurface, not constrained to it
       ==> set up a parabolic potential well around the isosurface */
    double val, grd[4];
    val = _pullPointScalar(task->pctx, point, pullInfoIsosurfaceValue,
                           grd, NULL);
    energy += val*val;
    grd[3] = 0;
    if (egradSum) {
      ELL_4V_SCALE_INCR(egradSum, 2*val, grd);
    }
  }
  return energy;
}

/* HEY: have to make sure that scale position point->pos[3] 
** is not modified anywhere in here: constraints are ONLY spatial
*/
int
_constraintSatisfy(pullTask *task, pullPoint *point,
                   /* output */
                   int *constrFailP) {
  char me[]="_constraintSatisfy", err[BIFF_STRLEN];
  double 
    stepMax,      /* maximum step size allowed */
    step,         /* current step size */
    valLast, val, /* last and current function values */
    avalLast,     /* absolute value of last function value */
    hack,         /* how to control re-tries in the context of a single
                     for-loop, instead of a nested do-while loop */
    grad[4], dir[3], len, 
    posOld[3];
  unsigned int iter,  /* 0: initial probe, 1..iterMax: probes in loop */
    iterMax;
#define CHECK_LEN if (!len) { \
    sprintf(err, "%s: got zero-length gradient at (%g,%g,%g,%g)\n", me, \
            point->pos[0], point->pos[1], point->pos[2], point->pos[3]); \
    biffAdd(PULL, err); return 1;                                       \
  }
#define PROBE(i, v, g, h)  if (_pullProbe(task, point)) {  \
    sprintf(err, "%s: on iter %u", me, iter);              \
    biffAdd(PULL, err); return 1;                          \
  }                                                        \
  (v) = _pullPointScalar(task->pctx, point, (i), (g), (h));

  /* if constraint satisfaction fails (without calling biff), we'll
     change this to AIR_TRUE */
  *constrFailP = AIR_FALSE;
  iter = 0;
  stepMax = task->pctx->constraintVoxelSize;
  iterMax = task->pctx->constraintIterMax;
  /* initial probe is not done for us in _pullPointProcess */
  switch (task->pctx->constraint) {
  case pullInfoHeight:
    break;
  case pullInfoIsosurfaceValue:
    PROBE(pullInfoIsosurfaceValue, val, grad, NULL);
    valLast = val;
    avalLast = AIR_ABS(val);
    ELL_3V_NORM(dir, grad, len);
    CHECK_LEN;
    hack = 1;
    for (iter=0; iter<iterMax; iter++) {
      double aval;
      step = -val/len; /* the newton-raphson step */
      step = step > 0 ? AIR_MIN(stepMax, step) : AIR_MAX(-stepMax, step);
      ELL_3V_COPY(posOld, point->pos);
      ELL_3V_SCALE_INCR(point->pos, hack*step, dir);
      PROBE(pullInfoIsosurfaceValue, val, grad, NULL);
      aval = AIR_ABS(val);
      if (aval <= avalLast) {  /* we're no further from the root */
        if (AIR_ABS(step) < stepMax*task->pctx->constraintStepMin) {
          /* we have converged! */
          break;
        }
        valLast = val;
        avalLast = aval;
        ELL_3V_NORM(dir, grad, len);
        CHECK_LEN;
        hack = 1;
      } else { /* oops, try again, don't update dir or len, reset val */
        hack *= task->pctx->stepScale;
        val = valLast;
        ELL_3V_COPY(point->pos, posOld);
      }
    }
    if (iter == iterMax) {
      *constrFailP = AIR_TRUE;
    }
    break;
  default:
    fprintf(stderr, "%s: constraint on %s (%d) unimplemented!!\n", me,
            airEnumStr(pullInfo, task->pctx->constraint),
            task->pctx->constraint);
  }
  /*
  fprintf(stderr, "!%s bye, fail = %d\n", me, *constrFailP);
  */
  return 0;
#undef CHECK_LEN
#undef PROBE
}

/*
** its in here that we scale from "energy gradient" to "force"
*/
double
_energyTotal(pullTask *task, pullBin *bin, pullPoint *point,
             /* output */
             double force[4], double *neighDist) {
  /* char me[]="_energyTotal"; */
  double enrIm, enrPt, egradIm[4], egradPt[4], energy;
    
  ELL_4V_SET(egradIm, 0, 0, 0, 0); /* sssh */
  ELL_4V_SET(egradPt, 0, 0, 0, 0); /* sssh */
  enrIm = _energyFromImage(task, point, 
                           force ? egradIm : NULL);
  enrPt = _energyFromPoints(task, bin, point,
                            force ? egradPt : NULL, neighDist);
  energy = AIR_LERP(task->pctx->alpha, enrIm, enrPt);
  /*
  fprintf(stderr, "!%s(%u): energy = lerp(%g, im %g, pt %g) = %g\n", me,
          point->idtag, task->pctx->alpha, enrIm, enrPt, energy);
  */
  if (force) {
    ELL_4V_LERP(force, task->pctx->alpha, egradIm, egradPt);
    ELL_4V_SCALE(force, -1, force);
    /*
    fprintf(stderr, "!%s(%u): egradIm = %g %g %g %g\n", me, point->idtag,
            egradIm[0], egradIm[1], egradIm[2], egradIm[3]);
    fprintf(stderr, "!%s(%u): egradPt = %g %g %g %g\n", me, point->idtag,
            egradPt[0], egradPt[1], egradPt[2], egradPt[3]);
    fprintf(stderr, "!%s(%u): ---> force = %g %g %g %g\n", me,
            point->idtag, force[0], force[1], force[2], force[3]);
    */
  }
  if (task->pctx->wall) {
    unsigned int axi;
    double frc;
    for (axi=0; axi<4; axi++) {
      frc = task->pctx->bboxMin[axi] - point->pos[axi];
      if (frc < 0) {
        /* not below min */
        frc = task->pctx->bboxMax[axi] - point->pos[axi];
        if (frc > 0) {
          /* not above max */
          frc = 0;
        }
      } 
      energy += task->pctx->wall*frc*frc/2;
      if (force) {
        force[axi] += task->pctx->wall*frc;
      }
    }
  }
  return energy;
}

int
_pullPointProcess(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="pullPointProcess", err[BIFF_STRLEN];
  double energyOld, energyNew, force[4], distLimit, posOld[4],
    testvec[3], testlen;
  int stepBad, giveUp;

  if (0 && 3 == task->pctx->iter && 1 == point->idtag) {
    Nrrd *neout, *nfout;
    double *eout, *fout;
    unsigned int su, sv, ui, vi; 
    double min, max, xx, yy, zz, posSave[4];

    ELL_4V_COPY(posSave, point->pos);
    su = sv = 300;
    neout = nrrdNew();
    nfout = nrrdNew();
    nrrdAlloc_va(neout, nrrdTypeDouble, 2,
                 AIR_CAST(size_t, su), AIR_CAST(size_t, sv));
    nrrdAlloc_va(nfout, nrrdTypeDouble, 3,
                 3, AIR_CAST(size_t, su), AIR_CAST(size_t, sv));
    eout = AIR_CAST(double *, neout->data);
    fout = AIR_CAST(double *, nfout->data);
    min = -40;
    max = 40;
    zz = 0;
    for (vi=0; vi<sv; vi++) {
      yy = AIR_AFFINE(0, vi, sv-1, min, max);
      fprintf(stderr, "%s: %u / %u\n", me, vi, sv);
      for (ui=0; ui<su; ui++) {
        xx = AIR_AFFINE(0, ui, su-1, min, max);
        ELL_4V_SET(point->pos, xx, yy, zz, 0);
        eout[ui + su*vi] = _energyTotal(task, bin, point, 
                                        force, &distLimit);
        ELL_3V_COPY(fout + 3*(ui + su*vi), force);
      }
    }
    nrrdSave("enr.nrrd", neout, NULL);
    nrrdSave("frc.nrrd", nfout, NULL);
    nrrdNuke(neout);
    nrrdNuke(nfout);
    ELL_4V_COPY(point->pos, posSave);
  }

  /*
  fprintf(stderr, "%s: =============================== (%u) hi @ %g %g %g\n",
          me, point->idtag, point->pos[0], point->pos[1], point->pos[2]);
  */
  energyOld = _energyTotal(task, bin, point, force, &distLimit);
  /*
  fprintf(stderr, "!%s: =================== point %u has:\n "
          "     energy = %g ; ndist = %g, force %g %g %g %g\n", me,
          point->idtag, energyOld, distLimit, 
          force[0], force[1], force[2], force[3]);
  */
  if (!( AIR_EXISTS(energyOld) && ELL_4V_EXISTS(force) )) {
    sprintf(err, "%s: point %u non-exist energy or force", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }
  if (distLimit < 0 /* no neighbors! */
      || pullEnergyZero == task->pctx->energySpec->energy) {
    distLimit = task->pctx->radiusSpace;
  }
  /* maybe voxel size should also be considered for finding distLimit */

  ELL_4V_COPY(posOld, point->pos);
  ELL_3V_SCALE(testvec, point->stepEnergy, force);
  testlen = ELL_3V_LEN(testvec);
  if (testlen > distLimit) {
    point->stepEnergy *= distLimit/testlen;
  }
  do {
    int constrFail;

    giveUp = AIR_FALSE;
    ELL_4V_SCALE_ADD2(point->pos, 1.0, posOld, point->stepEnergy, force);
    /*
    fprintf(stderr, "!%s: ======= trying step %g to pos %g %g %g %g\n", me,
            point->stepEnergy, 
            point->pos[0], point->pos[1], point->pos[2], point->pos[3]);
    */
    if (task->pctx->constraint) {
      if (_constraintSatisfy(task, point, &constrFail)) {
        sprintf(err, "%s: trouble", me);
        biffAdd(PULL, err); return 1;
      }
    } else {
      constrFail = AIR_FALSE;
    }
    if (constrFail) {
      energyNew = AIR_NAN;
    } else {
      energyNew = _energyTotal(task, bin, point,  NULL, NULL);
      /*
      fprintf(stderr, "!%s: ======= e new = %g %s old %g %s\n", me, energyNew,
              energyNew > energyOld ? ">" : "<=", energyOld,
              energyNew > energyOld ? "!! BADSTEP !!" : "ok");
      */
    }
    stepBad = constrFail || (energyNew > energyOld);
    if (stepBad) {
      point->stepEnergy *= task->pctx->stepScale;
      /* you have a problem if you had a non-trivial force, but you can't
         ever seem to take a small enough step to reduce energy */
      if (point->stepEnergy < 0.000000000001
          && ELL_4V_LEN(force) > 0.000001) {
        fprintf(stderr, "%s: %u (%g,%g,%g,%g) stepEnr %g stuck\n", me,
                point->idtag, 
                point->pos[0], point->pos[1], point->pos[2], point->pos[3],
                point->stepEnergy);
        /* This point is fuct, may as well reset its step, maybe things
           will go better next time.  Without this resetting, it will stay
           effectively frozen */
        point->stepEnergy = task->pctx->stepInitial;
        giveUp = AIR_TRUE;
      }
    }
  } while (stepBad && !giveUp);
  /* now: energy decreased, and, if we have one, constraint has been met */

  /* not recorded for the sake of this function, but for system accounting */
  point->energy = energyNew;

  return 0;
}

int
pullBinProcess(pullTask *task, unsigned int myBinIdx) {
  char me[]="pullBinProcess", err[BIFF_STRLEN];
  pullBin *myBin;
  unsigned int myPointIdx;

  if (task->pctx->verbose > 2) {
    fprintf(stderr, "%s(%u): doing bin %u\n", me, task->threadIdx, myBinIdx);
  }
  myBin = task->pctx->bin + myBinIdx;
  for (myPointIdx=0; myPointIdx<myBin->pointNum; myPointIdx++) {
    
    if (_pullPointProcess(task, myBin, myBin->point[myPointIdx])) {
      sprintf(err, "%s: on point %u of bin %u\n", me, 
              myPointIdx, myBinIdx);
      biffAdd(PULL, err); return 1;
    }

  } /* for myPointIdx */

  return 0;
}
