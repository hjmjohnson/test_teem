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

/* --------------------------------------------------------- */

static const char *
_pullSourceStr[] = {
  "(unknown pullSource)",
  "gage",
  "prop"
};

static const airEnum
_pullSource = {
  "pullSource",
  PULL_SOURCE_MAX,
  _pullSourceStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
const airEnum *const
pullSource = &_pullSource;

/* --------------------------------------------------------- */

static const char *
_pullInfoStr[] = {
  "(unknown pullInfo)",
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght",
  "hghtgradvec",
  "hghthessian",
  "hghtlapl",
  "seedprethresh",
  "seedthresh",
  "livethresh",
  "livethresh2",
  "livethresh3",
  "tan1",
  "tan2",
  "tanmode",
  "isoval",
  "isogradvec",
  "isohessian",
  "strength",
  "quality"
};

static const int
_pullInfoVal[] = {
  pullInfoUnknown,
  pullInfoTensor,             /* [7] tensor here */
  pullInfoTensorInverse,      /* [7] inverse of tensor here */
  pullInfoHessian,            /* [9] hessian used for force distortion */
  pullInfoInside,             /* [1] containment scalar */
  pullInfoInsideGradient,     /* [3] containment vector */
  pullInfoHeight,             /* [1] for gravity */
  pullInfoHeightGradient,     /* [3] for gravity */
  pullInfoHeightHessian,      /* [9] for gravity */
  pullInfoHeightLaplacian,    /* [1]  */
  pullInfoSeedPreThresh,      /* [1] */
  pullInfoSeedThresh,         /* [1] scalar for thresholding seeding */
  pullInfoLiveThresh,         /* [1] */ 
  pullInfoLiveThresh2,        /* [1] */ 
  pullInfoLiveThresh3,        /* [1] */ 
  pullInfoTangent1,           /* [3] first tangent to constraint surf */
  pullInfoTangent2,           /* [3] second tangent to constraint surf */
  pullInfoTangentMode,        /* [1] for morphing between co-dim 1 and 2 */
  pullInfoIsovalue,           /* [1] */
  pullInfoIsovalueGradient,   /* [3] */
  pullInfoIsovalueHessian,    /* [9] */
  pullInfoStrength,           /* [1] */
  pullInfoQuality             /* [1] */
};

static const char *
_pullInfoStrEqv[] = {
  "ten",
  "teni",
  "hess",
  "in",
  "ingradvec",
  "hght", "h",
  "hghtgradvec", "hgvec",
  "hghthessian", "hhess",
  "hghtlapl", "hlapl",
  "seedprethresh", "spthr",
  "seedthresh", "sthr",
  "livethresh", "lthr",
  "livethresh2", "lthr2",
  "livethresh3", "lthr3",
  "tan1",
  "tan2",
  "tanmode", "tmode",
  "isoval", "iso",
  "isogradvec", "isogvec",
  "isohessian", "isohess",
  "strength", "strn",
  "quality", "qual",
  ""
};

static const int
_pullInfoValEqv[] = {
  pullInfoTensor,
  pullInfoTensorInverse,
  pullInfoHessian,
  pullInfoInside,
  pullInfoInsideGradient,
  pullInfoHeight, pullInfoHeight,
  pullInfoHeightGradient, pullInfoHeightGradient,
  pullInfoHeightHessian, pullInfoHeightHessian,
  pullInfoHeightLaplacian, pullInfoHeightLaplacian,
  pullInfoSeedPreThresh, pullInfoSeedPreThresh,
  pullInfoSeedThresh, pullInfoSeedThresh,
  pullInfoLiveThresh, pullInfoLiveThresh,
  pullInfoLiveThresh2, pullInfoLiveThresh2,
  pullInfoLiveThresh3, pullInfoLiveThresh3,
  pullInfoTangent1,
  pullInfoTangent2,
  pullInfoTangentMode, pullInfoTangentMode,
  pullInfoIsovalue, pullInfoIsovalue,
  pullInfoIsovalueGradient, pullInfoIsovalueGradient,
  pullInfoIsovalueHessian, pullInfoIsovalueHessian,
  pullInfoStrength, pullInfoStrength,
  pullInfoQuality, pullInfoQuality
};

static const airEnum
_pullInfo = {
  "pullInfo",
  PULL_INFO_MAX,
  _pullInfoStr, _pullInfoVal,
  NULL,
  _pullInfoStrEqv, _pullInfoValEqv,
  AIR_FALSE
};
const airEnum *const
pullInfo = &_pullInfo;

/* --------------------------------------------------------- */

const char *
_pullPropStr[] = {
  "(unknown_prop)",
  "idtag",
  "idcc",
  "energy",
  "stepEnergy",
  "stepConstr",
  "stuck",
  "position",
  "force",
  "neighDistMean",
  "scale",
  "neighCovar",
  "neighCovar7Ten",
  "neighTanCovar",
  "stability"
};

static const airEnum
_pullProp = {
  "pullProp",
  PULL_PROP_MAX,
  _pullPropStr, NULL,
  NULL,
  NULL, NULL,
  AIR_FALSE
};
const airEnum *const
pullProp = &_pullProp;

/* --------------------------------------------------------- */

const char *
_pullProcessModeStr[] = {
  "(unknown_mode)",
  "descent",
  "nlearn",
  "adding",
  "nixing"
};

const airEnum
_pullProcessMode = {
  "process mode",
  PULL_PROCESS_MODE_MAX,
  _pullProcessModeStr,  NULL,
  NULL, NULL, NULL,
  AIR_FALSE
};
const airEnum *const
pullProcessMode = &_pullProcessMode;

