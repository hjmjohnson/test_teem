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

#include "ten.h"
#include "privateTen.h"

#define PARM_NUM 6
static const tenModelParmDesc
parmDesc[] = {
  /* 0 */ {"B0", 0.0, TEN_MODEL_B0_MAX, AIR_FALSE, 0},
  /* 1 */ {"length", 0.0, TEN_MODEL_DIFF_MAX, AIR_FALSE, 0},
  /* 2 */ {"radius", 0.0, TEN_MODEL_DIFF_MAX, AIR_FALSE, 0},
  /* 3 */ {"x", -1.0, 1.0, AIR_TRUE, 0},
  /* 4 */ {"y", -1.0, 1.0, AIR_TRUE, 1},
  /* 5 */ {"z", -1.0, 1.0, AIR_TRUE, 2}
};

static void 
simulate(double *dwiSim, const double *parm, const tenExperSpec *espec) {
  unsigned int ii;
  double b0, length, radius, vec[3], ten[7],
    ident[7] = {1, 1, 0, 0, 1, 0, 1};

  b0 = parm[0];
  length = parm[1];
  radius = parm[2];
  vec[0] = parm[3];
  vec[1] = parm[4];
  vec[2] = parm[5];
  TEN_T3V_OUTER(ten, vec);
  TEN_T_SCALE_ADD2(ten, length - radius, ten, radius, ident);
  for (ii=0; ii<espec->imgNum; ii++) {
    double adc, bb;
    bb = espec->bval[ii];
    adc = TEN_T3V_CONTR(ten, espec->grad + 3*ii);
    dwiSim[ii] = b0*exp(-bb*adc);
  }
  return;
}

_TEN_PARM_ALLOC
_TEN_PARM_RAND
_TEN_PARM_STEP
_TEN_PARM_DIST
_TEN_PARM_COPY

static char *
parmSprint(char str[AIR_STRLEN_MED], const double *parm) {
  sprintf(str, "(%g) %g x %g (%g,%g,%g)", parm[0],
          parm[1], parm[2], parm[3], parm[4], parm[5]);
  return str;
}

static int
parmConvert(double *parmDst, const double *parmSrc,
            const tenModel *modelSrc) {
  int ret;

  ret = 0;
  parmDst[0] = parmSrc[0];
  if (modelSrc == tenModelBall) {
    
  } else if (modelSrc == tenModel1Stick) {

  } else if (modelSrc == tenModelBall1Stick) {

  } else if (modelSrc == tenModelCylinder) {

  } else if (modelSrc == tenModelTensor2) {

  } else {
    unsigned int ii;
    for (ii=0; ii<PARM_NUM; ii++) {
      parmDst[ii] = AIR_NAN;
    }
    ret = 2;
  }
  return ret;
}

_TEN_SQE
_TEN_SQE_GRAD_CENTDIFF
_TEN_SQE_FIT(tenModelCylinder)

_TEN_NLL
_TEN_NLL_GRAD_STUB
_TEN_NLL_FIT_STUB

tenModel
_tenModelCylinder = {
  TEN_MODEL_STR_CYLINDER,
  _TEN_MODEL_FIELDS
};
const tenModel *const tenModelCylinder = &_tenModelCylinder;
