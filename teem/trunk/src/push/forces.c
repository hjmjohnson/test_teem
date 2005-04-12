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

#include "push.h"
#include "privatePush.h"

enum {
  pushForceUnknown,
  pushForceSpring,        /* 1 */
  pushForceGauss,         /* 2 */
  pushForceCharge,        /* 3 */
  pushForceCotan,         /* 4 */
  pushForceLast
};
#define PUSH_FORCE_MAX       4

char
_pushForceStr[PUSH_FORCE_MAX+1][AIR_STRLEN_SMALL] = {
  "(unknown_force)",
  "spring",
  "gauss",
  "charge",
  "cotan",
};

char
_pushForceDesc[PUSH_FORCE_MAX+1][AIR_STRLEN_MED] = {
  "unknown_force",
  "Hooke's law, with a tunable region of attraction",
  "derivative of a Gaussian energy function",
  "inverse square law, with tunable cut-off",
  "Cotangent based energy function (from Meyer et al. SMI 05)",
};

airEnum
_pushForceEnum = {
  "force",
  PUSH_FORCE_MAX,
  _pushForceStr,  NULL,
  _pushForceDesc,
  NULL, NULL,
  AIR_FALSE
};
airEnum *
pushForceEnum = &_pushForceEnum;

/* ----------------------------------------------------------------
** ------------------------------ SPRING --------------------------
** ----------------------------------------------------------------
** 2 parms:
** 0: spring constant (formerly known as pctx->stiff)
** 1: pull distance
*/
push_t
_pushForceSpringEval(push_t dist, push_t scale, const push_t *parm) {
  push_t ret, pull;

  pull = parm[1]*scale;
  if (dist > pull) {
    ret = 0;
  } else if (dist > 0) {
    ret = dist*(dist*dist/(pull*pull) - 2*dist/pull + 1);
  } else {
    ret = dist;
  }
  ret *= parm[0];
  return ret;
}

push_t
_pushForceSpringMaxDist(push_t scale, push_t maxEval, const push_t *parm) {

  return scale*maxEval + parm[1];
}

/* ----------------------------------------------------------------
** ------------------------------ arrays ... ----------------------
** ----------------------------------------------------------------
*/

int
_pushForceParmNum[PUSH_FORCE_MAX+1] = {
  0,
  2,
  2,
  1,
  0
};

push_t
(*_pushForceEval[PUSH_FORCE_MAX+1])(push_t dist, push_t scale,
                                    const push_t *parm) = {
                                      _pushForceSpringEval,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL
};

push_t
(*_pushForceMaxDist[PUSH_FORCE_MAX+1])(push_t dist, push_t scale,
                                       const push_t *parm) = {
                                         _pushForceSpringMaxDist,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL
};

pushForce *
pushForceNew() {
  pushForce *force;
  int pi;

  force = (pushForce *)calloc(1, sizeof(pushForce));
  if (force) {
    force->eval = NULL;
    force->maxDist = NULL;
    for (pi=0; pi<PUSH_FORCE_PARM_MAXNUM; pi++) {
      force->parm[pi] = AIR_NAN;
    }
  }
  return force;
}

pushForce *
pushForceNix(pushForce *force) {

  airFree(force);
  return NULL;
}

pushForce *
pushForceParse(const char *_str) {
  char me[]="pushForceParse", err[AIR_STRLEN_MED];
  char *str, *col, *_pstr, *pstr;
  pushForce *force;
  int fri, needParm, haveParm;
  airArray *mop;
  double pval;

  if (!_str) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(PUSH, err); return NULL;
  }

  mop = airMopNew();
  str = airStrdup(_str);
  airMopAdd(mop, str, (airMopper)airFree, airMopAlways);
  force = pushForceNew();
  airMopAdd(mop, force, (airMopper)pushForceNix, airMopOnError);

  col = strchr(str, ':');
  if (!col) {
    sprintf(err, "%s: didn't see colon separator in \"%s\"", me, str);
    biffAdd(PUSH, err); airMopError(mop); return NULL;
  }
  *col = '\0';
  fri = airEnumVal(pushForceEnum, str);
  if (pushForceUnknown == fri) {
    sprintf(err, "%s: didn't recognize \"%s\" as a force", me, str);
  }
  needParm = _pushForceParmNum[fri];
  _pstr = pstr = col+1;
  /* code lifted from teem/src/nrrd/kernel.c, should probably refactor... */
  for (haveParm=0; haveParm<needParm; haveParm++) {
    if (!pstr) {
      break;
    }
    if (1 != sscanf(pstr, "%lg", &pval)) {
      sprintf(err, "%s: trouble parsing \"%s\" as double (in \"%s\")",
              me, _pstr, _str);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
    }
    force->parm[haveParm] = pval;
    if ((pstr = strchr(pstr, ','))) {
      pstr++;
      if (!*pstr) {
        sprintf(err, "%s: nothing after last comma in \"%s\" (in \"%s\")",
                me, _pstr, _str);
        biffAdd(PUSH, err); airMopError(mop); return NULL;
      }
    }
  }
  /* haveParm is now the number of parameters that were parsed. */
  if (haveParm < needParm) {
    sprintf(err, "%s: parsed only %d of %d required parameters (for %s force)"
            "from \"%s\" (in \"%s\")",
            me, haveParm, needParm,
            airEnumStr(pushForceEnum, fri), _pstr, _str);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
  } else {
    if (pstr) {
      sprintf(err, "%s: \"%s\" (in \"%s\") has more than %d doubles",
              me, _pstr, _str, needParm);
      biffAdd(PUSH, err); airMopError(mop); return NULL;
    }
  }
  
  /* parameters have been set, now set the rest of the force info */
  force->eval = _pushForceEval[fri];
  force->maxDist = _pushForceMaxDist[fri];

  airMopOkay(mop);
  return force;
}

int
_pushHestForceParse(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  pushForce **fcP;
  char me[]="_pushHestForceParse", *perr;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  fcP = (pushForce **)ptr;
  *fcP = pushForceParse(str);
  if (!(*fcP)) {
    perr = biffGetDone(PUSH);
    strncpy(err, perr, AIR_STRLEN_HUGE-1);
    free(perr);
    return 1;
  }
  return 0;
}

hestCB
_pushHestForce = {
  sizeof(pushForce*),
  "force specification",
  _pushHestForceParse,
  (airMopper)pushForceNix
};

hestCB *
pushHestForce = &_pushHestForce;
