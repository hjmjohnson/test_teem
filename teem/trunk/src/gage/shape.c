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

#include "gage.h"
#include "privateGage.h"

void
gageShapeReset(gageShape *shape) {
  
  if (shape) {
    shape->defaultCenter = gageDefDefaultCenter;
    ELL_3V_SET(shape->size, 0, 0, 0);
    shape->center = nrrdCenterUnknown;
    shape->fromOrientation = AIR_FALSE;
    ELL_3V_SET(shape->spacing, AIR_NAN, AIR_NAN, AIR_NAN);
  }
  return;
}

gageShape *
gageShapeNew() {
  gageShape *shape;
  
  shape = (gageShape *)calloc(1, sizeof(gageShape));
  if (shape) {
    gageShapeReset(shape);
  }
  return shape;
}

gageShape *
gageShapeCopy(gageShape *shp) {
  gageShape *nhp;

  if ((nhp = gageShapeNew())) {
    /* no pointers, so this is easy */
    memcpy(nhp, shp, sizeof(gageShape));
  }
  return nhp;
}

gageShape *
gageShapeNix(gageShape *shape) {
  
  airFree(shape);
  return NULL;
}

static void
shapeUnitItoW(gageShape *shape, double world[3], double index[3],
              double volHalfLen[3]) {
  unsigned int i;
  
  if (nrrdCenterNode == shape->center) {
    for (i=0; i<=2; i++) {
      world[i] = NRRD_NODE_POS(-volHalfLen[i], volHalfLen[i],
                               shape->size[i], index[i]);
    }
  } else {
    for (i=0; i<=2; i++) {
      world[i] = NRRD_CELL_POS(-volHalfLen[i], volHalfLen[i],
                               shape->size[i], index[i]);
    }
  }
}
/*
** _gageShapeSet
**
** we are serving two masters here.  If ctx is non-NULL, we are being called
** from within gage, and we are to be lax or strict according to the settings
** of ctx->parm.requireAllSpacings and ctx->parm.requireEqualCenters.  If
** ctx is NULL, gageShapeSet was called, in which case we go with lax
** behavior (nothing "required")
**
** This function has subsumed the old gageVolumeCheck, and hence has 
** become this weird beast- part error checker and part (gageShape)
** initializer.  Oh well...
*/
int
_gageShapeSet(const gageContext *ctx, gageShape *shape,
              const Nrrd *nin, unsigned int baseDim) {
  char me[]="_gageShapeSet", err[BIFF_STRLEN];
  int ai, cx, cy, cz, statCalc[3], status;
  unsigned int minsize;
  const NrrdAxisInfo *ax[3];
  double vecA[4], vecB[3], vecC[3], vecD[4], matA[9],
    spcCalc[3], vecCalc[3][NRRD_SPACE_DIM_MAX], orig[NRRD_SPACE_DIM_MAX];

  /* ------ basic error checking */
  if (!( shape && nin )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(GAGE, err);  if (shape) { gageShapeReset(shape); }
    return 1;
  }
  if (nrrdCheck(nin)) {
    sprintf(err, "%s: basic nrrd validity check failed", me);
    biffMove(GAGE, err, NRRD); gageShapeReset(shape);
    return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: need a non-block type nrrd", me);
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  if (!(nin->dim == 3 + baseDim)) {
    sprintf(err, "%s: nrrd should be %u-D, not %u-D",
            me, 3 + baseDim, nin->dim);
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  ax[0] = &(nin->axis[baseDim+0]);
  ax[1] = &(nin->axis[baseDim+1]);
  ax[2] = &(nin->axis[baseDim+2]);

  statCalc[0] = nrrdSpacingCalculate(nin, baseDim + 0,
                                     spcCalc + 0, vecCalc[0]);
  statCalc[1] = nrrdSpacingCalculate(nin, baseDim + 1,
                                     spcCalc + 1, vecCalc[1]);
  statCalc[2] = nrrdSpacingCalculate(nin, baseDim + 2,
                                     spcCalc + 2, vecCalc[2]);
  /* see if nrrdSpacingCalculate ever *failed* */
  if (nrrdSpacingStatusUnknown == statCalc[0]
      || nrrdSpacingStatusUnknown == statCalc[1]
      || nrrdSpacingStatusUnknown == statCalc[2]) {
    sprintf(err, "%s: nrrdSpacingCalculate trouble on axis %d, %d, or %d",
            me, baseDim + 0, baseDim + 1, baseDim + 2);
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  if (!( statCalc[0] == statCalc[1] && statCalc[1] == statCalc[2] )) {
    sprintf(err, "%s: inconsistent spacing information on axes "
            "%u (%s), %u (%s), and %u (%s)", me,
            baseDim + 0, airEnumDesc(nrrdSpacingStatus, statCalc[0]),
            baseDim + 1, airEnumDesc(nrrdSpacingStatus, statCalc[1]),
            baseDim + 2, airEnumDesc(nrrdSpacingStatus, statCalc[2]));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  /* this simplifies reasoning in the code that follows */
  status = statCalc[0];
  /* zero spacing would be problematic */
  if (0 == spcCalc[0] && 0 == spcCalc[1] && 0 == spcCalc[2]) {
    sprintf(err, "%s: spacings (%g,%g,%g) for axes %d,%d,%d not all "
            "non-zero", me, spcCalc[1], spcCalc[1], spcCalc[2],
            baseDim+0, baseDim+1, baseDim+2);
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }

  /* error checking based on status */
  if (nrrdSpacingStatusScalarWithSpace == status) {
    sprintf(err, "%s: sorry, can't handle per-axis spacing that isn't part "
            "of a surrounding world space (%s)",
            me, airEnumStr(nrrdSpacingStatus, status));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  /* we no longer allow a nrrd to come in with no spacing info at all */
  if (nrrdSpacingStatusNone == status) {
    sprintf(err, "%s: sorry, need some spacing info for spatial axes "
            "%u, %u, %u", me,
            baseDim+0, baseDim+1, baseDim+2); 
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  /* actually, there shouldn't be any other options for spacing status
     besides these too; this is just being careful */
  if (!( nrrdSpacingStatusDirection == status
         || nrrdSpacingStatusScalarNoSpace == status )) {
    sprintf(err, "%s: sorry, can only handle spacing status %d (%s) "
            "or %d (%s), not %d (%s)", me,
            nrrdSpacingStatusDirection,
            airEnumStr(nrrdSpacingStatus, nrrdSpacingStatusDirection),
            nrrdSpacingStatusScalarNoSpace,
            airEnumStr(nrrdSpacingStatus, nrrdSpacingStatusScalarNoSpace),
            status, airEnumStr(nrrdSpacingStatus, status));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }

  if (nrrdSpacingStatusDirection == status) {
    shape->fromOrientation = AIR_TRUE;
    if (3 != nin->spaceDim) {
      sprintf(err, "%s: orientation space dimension %d != 3",
              me, nin->spaceDim);
      biffAdd(GAGE, err); gageShapeReset(shape);
      return 1;
    }
  } else {
    shape->fromOrientation = AIR_FALSE;
  }

  /* ------ find centering (set shape->center) */
  /* NOTE: when the volume is being crammed in a bi-unit cube, the centering
     will actually affect the positions of the samples.  Otherwise,
     (having full orientation, or using ctx->parm.orientationFromSpacing)
     the centering will only affect the probe-able bounds of the volume,
     but the sample positions in space don't depend on centering */
  cx = ax[0]->center;
  cy = ax[1]->center;
  cz = ax[2]->center;
  if (!( cx == cy && cy == cz )) {
    sprintf(err, "%s: axes %d,%d,%d centerings (%s,%s,%s) not all equal", me,
            baseDim+0, baseDim+1, baseDim+2,
            airEnumStr(nrrdCenter, cx),
            airEnumStr(nrrdCenter, cy),
            airEnumStr(nrrdCenter, cz));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }
  /* HEY: this logic is a little odd; we hope that
     shape->defaultCenter never disagrees w/ ctx->parm.defaultCenter! */
  shape->center = (nrrdCenterUnknown != cx
                   ? cx /* cx == cy == cz, by above */
                   : (ctx 
                      ? ctx->parm.defaultCenter
                      : shape->defaultCenter));

  /* ------ find sizes (set shape->size[0,1,2]) */
  shape->size[0] = ax[0]->size;
  shape->size[1] = ax[1]->size;
  shape->size[2] = ax[2]->size;
  minsize = (nrrdCenterCell == shape->center ? 1 : 2);
  /* this can't be relaxed in the face of having full orientation info,
     because even then, you can't have a non-zero probe-able volume if
     there's only one sample along a node-centered axis */
  if (!(shape->size[0] >= minsize 
        && shape->size[1] >= minsize
        && shape->size[2] >= minsize )) {
    sprintf(err, "%s: sizes (%u,%u,%u) must all be >= %u "
            "(min number of %s-centered samples)", me, 
            shape->size[0], shape->size[1], shape->size[2],
            minsize, airEnumStr(nrrdCenter, shape->center));
    biffAdd(GAGE, err); gageShapeReset(shape);
    return 1;
  }

  /* ------ find spacings[0,1,2] and ItoW matrix */
  if (shape->fromOrientation
      || (ctx && ctx->parm.orientationFromSpacing)) {
    if (ctx && ctx->parm.orientationFromSpacing) {
      /* need abs() in case an axis had negative spacing */
      ELL_3V_ABS(shape->spacing, spcCalc);
      ELL_3V_SET(vecCalc[0], airSgn(spcCalc[0]), 0.0, 0.0);
      ELL_3V_SET(vecCalc[1], 0.0, airSgn(spcCalc[1]), 0.0);
      ELL_3V_SET(vecCalc[2], 0.0, 0.0, airSgn(spcCalc[2]));
    } else {
      ELL_3V_COPY(shape->spacing, spcCalc);
      /* vecCalc set by nrrdSpacingCalculate */
    }
    if (shape->fromOrientation) {
      /* if the spaceOrigin isn't set, this will be all NaNs */
      nrrdSpaceOriginGet(nin, orig);
    } else {
      /* sorry, if you want to specify an image origin that over-rides the
         behavior of centering the volume at (0,0,0), then it has to be
         done through the full orientation info.  That is, we don't want
         to use nrrdOriginCalculate() because otherwise the logic gets
         too complicated */
      ELL_3V_SET(orig, AIR_NAN, AIR_NAN, AIR_NAN);
    }
    if (!ELL_3V_EXISTS(orig)) {
      /* don't have origin, for whatever reason; center volume on (0,0,0) */
      ELL_3V_SET(orig, 0.0, 0.0, 0.0);
      ELL_3V_SCALE_INCR(orig, -(shape->size[0] - 1.0)*shape->spacing[0]/2.0,
                        vecCalc[0]);
      ELL_3V_SCALE_INCR(orig, -(shape->size[1] - 1.0)*shape->spacing[1]/2.0,
                        vecCalc[1]);
      ELL_3V_SCALE_INCR(orig, -(shape->size[2] - 1.0)*shape->spacing[2]/2.0,
                        vecCalc[2]);
    }
    vecD[3] = 0;
    ELL_3V_SCALE(vecD, spcCalc[0], vecCalc[0]);
    ELL_4MV_COL0_SET(shape->ItoW, vecD);
    ELL_3V_SCALE(vecD, spcCalc[1], vecCalc[1]);
    ELL_4MV_COL1_SET(shape->ItoW, vecD);
    ELL_3V_SCALE(vecD, spcCalc[2], vecCalc[2]);
    ELL_4MV_COL2_SET(shape->ItoW, vecD);
    vecD[3] = 1;
    ELL_3V_COPY(vecD, orig);
    ELL_4MV_COL3_SET(shape->ItoW, vecD);
    /*
    printf("%s: %g (%g,%g,%g)\n", me,
           spcCalc[0], vecCalc[0][0], vecCalc[0][1], vecCalc[0][2]);
    printf("%s: %g (%g,%g,%g)\n", me,
           spcCalc[1], vecCalc[1][0], vecCalc[1][1], vecCalc[1][2]);
    printf("%s: %g (%g,%g,%g)\n", me,
           spcCalc[2], vecCalc[2][0], vecCalc[2][1], vecCalc[2][2]);
    */
  } else {
    double maxLen, volHalfLen[3];
    size_t num[3];
    /* ------ learn lengths for bounding nrrd in bi-unit cube */
    ELL_3V_ABS(shape->spacing, spcCalc);
    maxLen = 0.0;
    for (ai=0; ai<=2; ai++) {
      num[ai] = (nrrdCenterNode == shape->center
                 ? shape->size[ai]-1
                 : shape->size[ai]);
      volHalfLen[ai] = num[ai]*shape->spacing[ai];
      maxLen = AIR_MAX(maxLen, volHalfLen[ai]);
    }
    /* Thu Dec 13 02:45:01 EST 2007
       fixed long-standing bug in handling vols without full orientation info:
       spacing[ai] was never scaled to account for being crammed into
       the bi-unit cube!! */
    for (ai=0; ai<=2; ai++) {
      volHalfLen[ai] /= maxLen;
      shape->spacing[ai] = 2*volHalfLen[ai]/num[ai];
    }
    ELL_3V_SET(vecC, 0, 0, 0);
    shapeUnitItoW(shape, vecA, vecC, volHalfLen);
    ELL_3V_SET(vecC, 1, 0, 0);
    shapeUnitItoW(shape, vecB, vecC, volHalfLen);
    ELL_3V_SUB(vecD, vecB, vecA);
    vecD[3] = 0;
    ELL_4MV_COL0_SET(shape->ItoW, vecD);

    ELL_3V_SET(vecC, 0, 1, 0);
    shapeUnitItoW(shape, vecB, vecC, volHalfLen);
    ELL_3V_SUB(vecD, vecB, vecA);
    vecD[3] = 0;
    ELL_4MV_COL1_SET(shape->ItoW, vecD);

    ELL_3V_SET(vecC, 0, 0, 1);
    shapeUnitItoW(shape, vecB, vecC, volHalfLen);
    ELL_3V_SUB(vecD, vecB, vecA);
    vecD[3] = 0;
    ELL_4MV_COL2_SET(shape->ItoW, vecD);

    vecA[3] = 1;
    ELL_4MV_COL3_SET(shape->ItoW, vecA);
  }
  
  /* ------ set the rest of the matrices */
  ell_4m_inv_d(shape->WtoI, shape->ItoW);
  ELL_34M_EXTRACT(matA, shape->ItoW);
  ell_3m_inv_d(shape->ItoWSubInv, matA);
  ELL_3M_TRANSPOSE(shape->ItoWSubInvTransp, shape->ItoWSubInv);

  return 0;
}

int
gageShapeSet(gageShape *shape, const Nrrd *nin, int baseDim) {
  char me[]="gageShapeSet", err[BIFF_STRLEN];

  if (_gageShapeSet(NULL, shape, nin, baseDim)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(GAGE, err); return 1;
  }
  return 0;
}

/*
** this wasn't being used at all
void
gageShapeUnitWtoI(gageShape *shape, double index[3], double world[3]) {
  int i;
  
  if (nrrdCenterNode == shape->center) {
    for (i=0; i<=2; i++) {
      index[i] = NRRD_NODE_IDX(-shape->volHalfLen[i], shape->volHalfLen[i],
                               shape->size[i], world[i]);
    }
  } else {
    for (i=0; i<=2; i++) {
      index[i] = NRRD_CELL_IDX(-shape->volHalfLen[i], shape->volHalfLen[i],
                               shape->size[i], world[i]);
    }
  }
}
*/

void
gageShapeWtoI(gageShape *shape, double _index[3], double _world[3]) {
  /* char me[]="gageShapeWtoI"; */
  double index[4], world[4];

  /*
  printf("!%s: hello %p %p %p; %p\n", me, 
         shape, _index, _world, shape->WtoI);
  */
  ELL_3V_COPY(world, _world);
  world[3] = 1.0;
  ELL_4MV_MUL(index, shape->WtoI, world);
  ELL_3V_SCALE(_index, 1.0/index[3], index);
}

void
gageShapeItoW(gageShape *shape, double _world[3], double _index[3]) {
  double world[4], index[4];

  ELL_3V_COPY(index, _index);
  index[3] = 1.0;
  ELL_4MV_MUL(world, shape->ItoW, index);
  ELL_3V_SCALE(_world, 1.0/world[3], world);
}

int
gageShapeEqual(gageShape *shape1, char *_name1,
               gageShape *shape2, char *_name2) {
  char me[]="_gageShapeEqual", err[BIFF_STRLEN],
    *name1, *name2, what[] = "???";

  name1 = _name1 ? _name1 : what;
  name2 = _name2 ? _name2 : what;
  if (!( shape1->fromOrientation == shape2->fromOrientation )) {
    sprintf(err, "%s: fromOrientation of %s (%s) != %s's (%s)", me,
            name1, shape1->fromOrientation ? "true" : "false",
            name2, shape2->fromOrientation ? "true" : "false");
    biffAdd(GAGE, err); return 0;
  }
  if (!( shape1->size[0] == shape2->size[0] &&
         shape1->size[1] == shape2->size[1] &&
         shape1->size[2] == shape2->size[2] )) {
    sprintf(err, "%s: dimensions of %s (%u,%u,%u) != %s's (%u,%u,%u)", me,
            name1, 
            shape1->size[0], shape1->size[1], shape1->size[2],
            name2,
            shape2->size[0], shape2->size[1], shape2->size[2]);
    biffAdd(GAGE, err); return 0;
  }
  if (shape1->fromOrientation) {
    if (!( ELL_4M_EQUAL(shape1->ItoW, shape2->ItoW) )) {
      sprintf(err, "%s: ItoW matrices of %s and %s not the same", me,
              name1, name2);
      biffAdd(GAGE, err); return 0;
    }
  } else {
    if (!( shape1->spacing[0] == shape2->spacing[0] &&
           shape1->spacing[1] == shape2->spacing[1] &&
           shape1->spacing[2] == shape2->spacing[2] )) {
      sprintf(err, "%s: spacings of %s (%g,%g,%g) != %s's (%g,%g,%g)", me,
              name1,
              shape1->spacing[0], shape1->spacing[1], shape1->spacing[2],
              name2,
              shape2->spacing[0], shape2->spacing[1], shape2->spacing[2]);
      biffAdd(GAGE, err); return 0;
    }
    if (!( shape1->center == shape2->center )) {
      sprintf(err, "%s: centering of %s (%s) != %s's (%s)", me,
              name1, airEnumStr(nrrdCenter, shape1->center),
              name2, airEnumStr(nrrdCenter, shape2->center));
      biffAdd(GAGE, err); return 0;
    }
  }

  return 1;
}

void
gageShapeBoundingBox(double min[3], double max[3],
                     gageShape *shape) {
  /* char me[]="gageShapeBoundingBox"; */
  double minIdx[3], maxIdx[3], cornerIdx[8][3], tmp[3];
  unsigned int ii;
  
  if (!( min && max && shape )) {
    return;
  }
  if (nrrdCenterNode == shape->center) {
    ELL_3V_SET(minIdx, 0, 0, 0);
    ELL_3V_SET(maxIdx,
               shape->size[0]-1,
               shape->size[1]-1,
               shape->size[2]-1);
  } else {
    ELL_3V_SET(minIdx, -0.5, -0.5, -0.5);
    ELL_3V_SET(maxIdx,
               shape->size[0]-0.5,
               shape->size[1]-0.5,
               shape->size[2]-0.5);
  }
  ELL_3V_SET(cornerIdx[0], minIdx[0], minIdx[1], minIdx[2]);
  ELL_3V_SET(cornerIdx[1], maxIdx[0], minIdx[1], minIdx[2]);
  ELL_3V_SET(cornerIdx[2], minIdx[0], maxIdx[1], minIdx[2]);
  ELL_3V_SET(cornerIdx[3], maxIdx[0], maxIdx[1], minIdx[2]);
  ELL_3V_SET(cornerIdx[4], minIdx[0], minIdx[1], maxIdx[2]);
  ELL_3V_SET(cornerIdx[5], maxIdx[0], minIdx[1], maxIdx[2]);
  ELL_3V_SET(cornerIdx[6], minIdx[0], maxIdx[1], maxIdx[2]);
  ELL_3V_SET(cornerIdx[7], maxIdx[0], maxIdx[1], maxIdx[2]);
  gageShapeItoW(shape, tmp, cornerIdx[0]);
  ELL_3V_COPY(min, tmp);
  ELL_3V_COPY(max, tmp);
  for (ii=1; ii<8; ii++) {
    gageShapeItoW(shape, tmp, cornerIdx[ii]);
    ELL_3V_MIN(min, min, tmp);
    ELL_3V_MAX(max, max, tmp);
  }
  return;
}
