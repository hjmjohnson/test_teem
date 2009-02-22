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


#include "pull.h"
#include "privatePull.h"

int
_pullPointProcessNeighLearn(pullTask *task, pullBin *bin, pullPoint *point) {
  
  /* sneaky: we just learn (and discard) the energy, and this function
     will do the work of learning the neighbors */
  _pullEnergyFromPoints(task, bin, point, NULL);
  return 0;
}

static double
_pointEnergyOfNeighbors(pullTask *task, pullBin *bin, pullPoint *point,
                        double *fracNixed) {
  double enr;
  unsigned int ii, xx;
  pullPoint *her;
  
  enr = 0;
  xx = 0;
  for (ii=0; ii<point->neighPointNum; ii++) {
    her = point->neighPoint[ii];
    if (her->status & PULL_STATUS_NIXME_BIT) {
      xx += 1;
    } else {
      enr += _pullEnergyFromPoints(task, bin, her, NULL);
    }
  }
  *fracNixed = (point->neighPointNum
                ? AIR_CAST(double, xx)/point->neighPointNum
                : 0);
  return enr;
}

int
_pullPointProcessAdding(pullTask *task, pullBin *bin, pullPoint *point) {
  char me[]="_pullPointProcessAdding", err[BIFF_STRLEN];
  unsigned int npi, iter, api;
  double noffavg[4], enrWith, enrWithout, fracNixed, newSpcDist, tmp;
  pullPoint *newpnt;
  int E;

  if (point->neighPointNum && task->pctx->constraintDim) {
    unsigned int plenty;
    /* HEY, doesn't take into account scale space! */
    plenty = (2 == task->pctx->constraintDim
              ? 7
              : 3);
    if (point->neighPointNum >= plenty) {
      /* there's little chance that adding points will reduce energy */
      return 0;
    }
  }
  ELL_4V_SET(noffavg, 0, 0, 0, 0);
  for (npi=0; npi<point->neighPointNum; npi++) {
    double off[4];
    ELL_4V_SUB(off, point->pos, point->neighPoint[npi]->pos);
    /* normalize the offset */
    ELL_3V_SCALE(off, 1/task->pctx->radiusSpace, off);
    if (task->pctx->haveScale) {
      off[3] /= task->pctx->radiusScale;
    }
    ELL_4V_INCR(noffavg, off);
  }
  if (point->neighPointNum
      && (ELL_4V_LEN(noffavg)/point->neighPointNum 
          < _PULL_NEIGH_OFFSET_SUM_THRESH)) {
    /* we have neighbors, and they seem to be balanced well enough;
       don't try to add */
    return 0;
  }
  /* else we'll try adding something */
  newpnt = pullPointNew(task->pctx);
  if (!newpnt) {
    sprintf(err, "%s: couldn't spawn new point from %u", me, point->idtag);
    biffAdd(PULL, err); return 1;
  }
  /* set status to indicate this is an unbinned point, with no 
     knowledge of its neighbors */
  newpnt->status |= PULL_STATUS_NEWBIE_BIT;
  if (pullEnergyCubicWell == task->pctx->energySpec->energy) {
    newSpcDist = task->pctx->radiusSpace*task->pctx->energySpec->parm[0];
  } else {
    newSpcDist = task->pctx->radiusSpace*_PULL_NEWPNT_DIST;
  }
  /* compute offset (normalized) direction from current point location */
  if (!point->neighPointNum) {
    /* we had no neighbors, have to pretend like we did */
    airNormalRand_r(noffavg + 0, noffavg + 1, task->rng);
    airNormalRand_r(noffavg + 2, noffavg + 3, task->rng);
  }
  if (task->pctx->constraint) {
    double proj[9], tmpvec[3];
    _pullConstraintTangent(task, point, proj);
    ELL_3MV_MUL(tmpvec, proj, noffavg);
    ELL_3V_COPY(noffavg, tmpvec);
  }
  ELL_3V_NORM(noffavg, noffavg, tmp);
  /* set new point location */
  ELL_3V_SCALE_ADD2(newpnt->pos, newSpcDist, noffavg,
                    1.0, point->pos);
  if (task->pctx->haveScale) {
    /* HEY: should have a smarter way of placing along scale */
    newpnt->pos[3] = (0.5*task->pctx->radiusScale*noffavg[3]
                      + point->pos[3]);
  } else {
    newpnt->pos[3] = point->pos[3];
  }
  /* satisfy constraint if needed */
  if (task->pctx->constraint) {
    int constrFail;
    if (_pullConstraintSatisfy(task, newpnt, &constrFail)) {
      sprintf(err, "%s: on newbie point %u (spawned from %u)", me,
              newpnt->idtag, point->idtag);
      biffAdd(PULL, err); pullPointNix(newpnt); return 1;
    }
    if (constrFail) {
      /* constraint satisfaction failed, which isn't an error for us,
         we just don't try to add this point */
      pullPointNix(newpnt);
      return 0;
    }
  }
  /* do some descent, on this point only, which we do by sneakily
     changing the per-task process mode ... */
  task->processMode = pullProcessModeDescent;
  E = 0;
  for (iter=0; iter<task->pctx->popCntlPeriod; iter++) {
    if (!E) E |= _pullPointProcessDescent(task, bin, newpnt,
                                          AIR_TRUE /* ignoreImage */);
  }
  /* now that newbie point is final test location, learn neighbors */
  /* we have to add newpnt to add queue, just so that its neighbors
     can see it as a possible neighbor */
  api = airArrayLenIncr(task->addPointArr, 1);
  task->addPoint[api] = newpnt;
  task->processMode = pullProcessModeNeighLearn;
  _pullEnergyFromPoints(task, bin, newpnt, NULL);
  /* and teach the neighbors their neighbors (possibly including newpnt) */
  for (npi=0; npi<newpnt->neighPointNum; npi++) {
    _pullEnergyFromPoints(task, bin, newpnt->neighPoint[npi], NULL);
  }
  /* now back to the actual mode we came here with */
  task->processMode = pullProcessModeAdding;
  enrWith = (_pullEnergyFromPoints(task, bin, newpnt, NULL)
             + _pointEnergyOfNeighbors(task, bin, newpnt, &fracNixed));
  newpnt->status |= PULL_STATUS_NIXME_BIT;
  enrWithout = _pointEnergyOfNeighbors(task, bin, newpnt, &fracNixed);
  newpnt->status &= ~PULL_STATUS_NIXME_BIT;
  if (enrWith <= enrWithout) {
    /* energy is lower (or the same) *with* newpnt, so we keep it, which
       means keeping it in the add queue where it already is */
  } else {
    /* its not good to add the point, remove it from the add queue */
    airArrayLenIncr(task->addPointArr, -1);
    /* ugh, have to signal to neighbors that its no longer their neighbor */
    task->processMode = pullProcessModeNeighLearn;
    newpnt->status |= PULL_STATUS_NIXME_BIT;
    for (npi=0; npi<newpnt->neighPointNum; npi++) {
      _pullEnergyFromPoints(task, bin, newpnt->neighPoint[npi], NULL);
    }
    pullPointNix(newpnt);
    task->processMode = pullProcessModeAdding;
  }
  return 0;
}

int
_pullPointProcessNixing(pullTask *task, pullBin *bin, pullPoint *point) {
  double enrWith, enrWithout, fracNixed;

  enrWith = (_pullEnergyFromPoints(task, bin, point, NULL)
             + _pointEnergyOfNeighbors(task, bin, point, &fracNixed));
  /* if many neighbors have been nixed, then system is far from convergence,
     so the energy is a less meaningful description of the neighborhood,
     and so its a bad idea to try to nix this point */
  if (fracNixed < _PULL_FRAC_NIXED_THRESH) {
    point->status |= PULL_STATUS_NIXME_BIT;
    enrWithout = _pointEnergyOfNeighbors(task, bin, point, &fracNixed);
    if (enrWith <= enrWithout) {
      /* Energy isn't distinctly lowered without the point, so keep it;
         turn off nixing.  If enrWith == enrWithout == 0, as happens to
         isolated points, then the difference between "<=" and "<" 
         keeps them from getting nixed */
      point->status &= ~PULL_STATUS_NIXME_BIT;
    } 
  }
  
  return 0;
}

int
_pullIterFinishAdding(pullContext *pctx) {
  char me[]="_pullIterFinishAdding", err[BIFF_STRLEN];
  unsigned int taskIdx;
  
  pctx->addNum = 0;
  for (taskIdx=0; taskIdx<pctx->threadNum; taskIdx++) {
    pullTask *task;
    task = pctx->task[taskIdx];
    if (task->addPointNum) {
      unsigned int pointIdx;
      int added;
      for (pointIdx=0; pointIdx<task->addPointNum; pointIdx++) {
        pullPoint *point;
        point = task->addPoint[pointIdx];
        point->status &= ~PULL_STATUS_NEWBIE_BIT;
        if (pullBinsPointMaybeAdd(pctx, point, &added)) {
          sprintf(err, "%s: trouble binning new point %u", me, point->idtag);
          biffAdd(PULL, err); return 1;
        }
        if (added) {
          pctx->addNum++;
        } else {
          if (pctx->verbose) {
            printf("%s: decided NOT to add new point %u\n", me, point->idtag);
          }
          point = pullPointNix(point);
        }
      }
      airArrayLenSet(task->addPointArr, 0);
    }
  }
  if (pctx->verbose && pctx->addNum) {
    printf("%s: ADDED %u\n", me, pctx->addNum);
  }
  return 0;
}

int
_pullIterFinishNixing(pullContext *pctx) {
  char me[]="_pullIterFinishNixing";
  unsigned int binIdx;

  pctx->nixNum = 0;
  for (binIdx=0; binIdx<pctx->binNum; binIdx++) {
    pullBin *bin;
    unsigned int pointIdx;
    bin = pctx->bin + binIdx;
    pointIdx = 0;
    while (pointIdx < bin->pointNum) {
      pullPoint *point;
      point = bin->point[pointIdx];
      if (point->status & PULL_STATUS_NIXME_BIT) {
        pullPointNix(point);
        /* copy last point pointer to this slot */
        bin->point[pointIdx] = bin->point[bin->pointNum-1];
        airArrayLenIncr(bin->pointArr, -1); /* will decrement bin->pointNum */
	pctx->nixNum++;
      } else {
        pointIdx++;
      }
    }
  }
  if (pctx->verbose && pctx->nixNum) {
    printf("%s: NIXED %u\n", me, pctx->nixNum);
  }
  return 0;
}

