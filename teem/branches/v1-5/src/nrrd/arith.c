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

#include "nrrd.h"
#include "privateNrrd.h"

/*
******** nrrdArithGamma()
**
** map the values in a nrrd through a power function; essentially:
** val = pow(val, 1/gamma), but this is after the val has been normalized
** to be in the range of 0.0 to 1.0 (assuming that the given min and
** max really are the full range of the values in the nrrd).  Thus,
** the given min and max values are fixed points of this
** transformation.  Using a negative gamma means that after the pow()
** function has been applied, the value is inverted with respect to
** min and max (like in xv).
*/
int
nrrdArithGamma(Nrrd *nout, Nrrd *nin, double gamma, double min, double max) {
  char me[]="nrrdArithGamma", func[]="gamma", err[AIR_STRLEN_MED];
  double val;
  size_t I, num;
  double (*lup)(void *, size_t);
  double (*ins)(void *, size_t, double);

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s: couldn't initialize by copy to output", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  if (!( AIR_EXISTS(gamma) && AIR_EXISTS(min) && AIR_EXISTS(max) )) {
    sprintf(err, "%s: not all of gamma, min, max exist", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nrrdTypeBlock != nin->type && nrrdTypeBlock != nout->type )) {
    sprintf(err, "%s: can't deal with %s type", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }

  lup = nrrdDLookup[nin->type];
  ins = nrrdDInsert[nout->type];
  gamma = 1/gamma;
  num = nrrdElementNumber(nin);
  if (gamma < 0.0) {
    gamma = -gamma;
    for (I=0; I<num; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(1.0, val, 0.0, min, max);
      ins(nout->data, I, val);
    }
  } else {
    for (I=0; I<num; I++) {
      val = lup(nin->data, I);
      val = AIR_AFFINE(min, val, max, 0.0, 1.0);
      val = pow(val, gamma);
      val = AIR_AFFINE(0.0, val, 1.0, min, max);
      ins(nout->data, I, val);
    }
  }
  if (nrrdContentSet(nout, func, nin, "%g,%g,%g", gamma, min, max)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_NONE);
  }
  
  return 0;
}

/* ---------------------------- unary -------------- */

double _nrrdUnaryOpNegative(double a)   {return -a;}
double _nrrdUnaryOpReciprocal(double a) {return 1.0/a;}
double _nrrdUnaryOpSin(double a)        {return sin(a);}
double _nrrdUnaryOpCos(double a)        {return cos(a);}
double _nrrdUnaryOpTan(double a)        {return tan(a);}
double _nrrdUnaryOpAsin(double a)       {return asin(a);}
double _nrrdUnaryOpAcos(double a)       {return acos(a);}
double _nrrdUnaryOpAtan(double a)       {return atan(a);}
double _nrrdUnaryOpExp(double a)        {return exp(a);}
double _nrrdUnaryOpLog(double a)        {return log(a);}
double _nrrdUnaryOpLog10(double a)      {return log10(a);}
double _nrrdUnaryOpLog1p(double a)      {
  double b;

  b = 1 + a;
  if (b == 1) {
    return a;
  } else {
    return log(b)*a/(b-1);
  }
}
double _nrrdUnaryOpSqrt(double a)       {return sqrt(a);}
double _nrrdUnaryOpCbrt(double a)       {return airCbrt(a);}
double _nrrdUnaryOpErf(double a)        {return airErf(a);}
double _nrrdUnaryOpCeil(double a)       {return ceil(a);}
double _nrrdUnaryOpFloor(double a)      {return floor(a);}
double _nrrdUnaryOpRoundUp(double a)    {return AIR_ROUNDUP(a);}
double _nrrdUnaryOpRoundDown(double a)  {return AIR_ROUNDDOWN(a);}
double _nrrdUnaryOpAbs(double a)        {return AIR_ABS(a);}
double _nrrdUnaryOpSgn(double a) {
  return (a < 0.0 ? -1 : (a > 0.0 ? 1 : 0));}
double _nrrdUnaryOpExists(double a)     {return AIR_EXISTS(a);}

double (*_nrrdUnaryOp[NRRD_UNARY_OP_MAX+1])(double) = {
  NULL,
  _nrrdUnaryOpNegative,
  _nrrdUnaryOpReciprocal,
  _nrrdUnaryOpSin,
  _nrrdUnaryOpCos,
  _nrrdUnaryOpTan,
  _nrrdUnaryOpAsin,
  _nrrdUnaryOpAcos,
  _nrrdUnaryOpAtan,
  _nrrdUnaryOpExp,
  _nrrdUnaryOpLog,
  _nrrdUnaryOpLog10,
  _nrrdUnaryOpLog1p,
  _nrrdUnaryOpSqrt,
  _nrrdUnaryOpCbrt,
  _nrrdUnaryOpErf,
  _nrrdUnaryOpCeil,
  _nrrdUnaryOpFloor,
  _nrrdUnaryOpRoundUp,
  _nrrdUnaryOpRoundDown,
  _nrrdUnaryOpAbs,
  _nrrdUnaryOpSgn,
  _nrrdUnaryOpExists
};

int
nrrdArithUnaryOp(Nrrd *nout, int op, Nrrd *nin) {
  char me[]="nrrdArithUnaryOp", err[AIR_STRLEN_MED];
  size_t N, I;
  int size[NRRD_DIM_MAX];
  double (*insert)(void *v, size_t I, double d), 
    (*lookup)(void *v, size_t I), (*uop)(double), val;

  if (!(nout && nin)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdTypeBlock == nin->type) {
    sprintf(err, "%s: can't operate on type %s", me,
	    airEnumStr(nrrdType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdUnaryOp, op)) {
    sprintf(err, "%s: unary op %d invalid", me, op);
    biffAdd(NRRD, err); return 1;
  }
  if (nout != nin) {
    if (nrrdCopy(nout, nin)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  nrrdPeripheralInit(nout);
  uop = _nrrdUnaryOp[op];

  N = nrrdElementNumber(nin);
  lookup = nrrdDLookup[nin->type];
  insert = nrrdDInsert[nin->type];
  for (I=0; I<N; I++) {
    val = lookup(nin->data, I);
    insert(nout->data, I, uop(val));
  }
  if (nrrdContentSet(nout, airEnumStr(nrrdUnaryOp, op), nin, "")) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/* ---------------------------- binary -------------- */

double _nrrdBinaryOpAdd(double a, double b)       {return a + b;}
double _nrrdBinaryOpSubtract(double a, double b)  {return a - b;}
double _nrrdBinaryOpMultiply(double a, double b)  {return a * b;}
double _nrrdBinaryOpDivide(double a, double b)    {return a / b;}
double _nrrdBinaryOpPow(double a, double b)       {return pow(a,b);}
double _nrrdBinaryOpMod(double a, double b) {
  return AIR_MOD((int)a,(int)b);}
double _nrrdBinaryOpFmod(double a, double b)      {return fmod(a,b);}
double _nrrdBinaryOpAtan2(double a, double b)     {return atan2(a,b);}
double _nrrdBinaryOpMin(double a, double b)       {return AIR_MIN(a,b);}
double _nrrdBinaryOpMax(double a, double b)       {return AIR_MAX(a,b);}
double _nrrdBinaryOpLT(double a, double b)        {return (a < b);}
double _nrrdBinaryOpLTE(double a, double b)       {return (a <= b);}
double _nrrdBinaryOpGT(double a, double b)        {return (a > b);}
double _nrrdBinaryOpGTE(double a, double b)       {return (a >= b);}
double _nrrdBinaryOpCompare(double a, double b) {
  return (a < b ? -1 : (a > b ? 1 : 0));}
double _nrrdBinaryOpEqual(double a, double b)     {return (a == b);}
double _nrrdBinaryOpNotEqual(double a, double b)  {return (a != b);}
double _nrrdBinaryOpExists(double a, double b)    {return (AIR_EXISTS(a) 
							   ? a : b);}

double (*_nrrdBinaryOp[NRRD_BINARY_OP_MAX+1])(double, double) = {
  NULL,
  _nrrdBinaryOpAdd,
  _nrrdBinaryOpSubtract,
  _nrrdBinaryOpMultiply,
  _nrrdBinaryOpDivide,
  _nrrdBinaryOpPow,
  _nrrdBinaryOpMod,
  _nrrdBinaryOpFmod,
  _nrrdBinaryOpAtan2,
  _nrrdBinaryOpMin,
  _nrrdBinaryOpMax,
  _nrrdBinaryOpLT,
  _nrrdBinaryOpLTE,
  _nrrdBinaryOpGT,
  _nrrdBinaryOpGTE,
  _nrrdBinaryOpCompare,
  _nrrdBinaryOpEqual,
  _nrrdBinaryOpNotEqual,
  _nrrdBinaryOpExists
};

/*
******** nrrdArithBinaryOp
**
** this is a simplified version of nrrdArithIterBinaryOp, written after
** that, in a hurry, to operate directly on two nrrds, instead with
** the NrrdIter nonsense
*/
int
nrrdArithBinaryOp(Nrrd *nout, int op, Nrrd *ninA, Nrrd *ninB) {
  char me[]="nrrdArithBinaryOp", err[AIR_STRLEN_MED], *contA, *contB;
  size_t N, I;
  int size[NRRD_DIM_MAX];
  double (*ins)(void *v, size_t I, double d), (*lupA)(void *v, size_t I), 
    (*lupB)(void *v, size_t I), (*bop)(double a, double b), valA, valB;

  if (!( nout && !nrrdCheck(ninA) && !nrrdCheck(ninB) )) {
    sprintf(err, "%s: NULL pointer or invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!nrrdSameSize(ninA, ninB, AIR_TRUE)) {
    sprintf(err, "%s: size mismatch between arguments", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdBinaryOp, op)) {
    sprintf(err, "%s: binary op %d invalid", me, op);
    biffAdd(NRRD, err); return 1;
  }
  
  nrrdAxisInfoGet_nva(ninA, nrrdAxisInfoSize, size);
  if (!( nout == ninA || nout == ninB)) {
    if (nrrdMaybeAlloc_nva(nout, ninA->type, ninA->dim, size)) {
      sprintf(err, "%s: couldn't allocate output nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
    if (nrrdAxisInfoCopy(nout, ninA, NULL, NRRD_AXIS_INFO_NONE)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
    nrrdPeripheralCopy(nout, ninA);
  }
  bop = _nrrdBinaryOp[op];

  N = nrrdElementNumber(ninA);
  lupA = nrrdDLookup[ninA->type];
  lupB = nrrdDLookup[ninB->type];
  ins = nrrdDInsert[nout->type];
  for (I=0; I<N; I++) {
    valA = lupA(ninA->data, I);
    valB = lupB(ninB->data, I);
    ins(nout->data, I, bop(valA, valB));
  }

  contA = _nrrdContentGet(ninA);
  contB = _nrrdContentGet(ninB);
  if (_nrrdContentSet(nout, airEnumStr(nrrdBinaryOp, op),
		      contA, "%s", contB)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(contA); free(contB); return 1;
  }
  free(contA);
  free(contB); 
  return 0;
}

int
nrrdArithIterBinaryOp(Nrrd *nout, int op, NrrdIter *inA, NrrdIter *inB) {
  char me[]="nrrdArithIterBinaryOp", err[AIR_STRLEN_MED], *contA, *contB;
  size_t N, I;
  int type, size[NRRD_DIM_MAX];
  double (*insert)(void *v, size_t I, double d), 
    (*bop)(double a, double b), valA, valB;
  Nrrd *nin;

  if (!(nout && inA && inB)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdBinaryOp, op)) {
    sprintf(err, "%s: binary op %d invalid", me, op);
    biffAdd(NRRD, err); return 1;
  }
  nin = inA->nrrd ? inA->nrrd : inB->nrrd;
  if (!nin) {
    sprintf(err, "%s: can't operate on two fixed values", me);
    biffAdd(NRRD, err); return 1;
  }
  type = nin->type;
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim, size)) {
    sprintf(err, "%s: couldn't allocate output nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdPeripheralInit(nout);
  bop = _nrrdBinaryOp[op];

  /*
  fprintf(stderr, "!%s: inA->left = %d, inB->left = %d\n", me, 
	  (int)(inA->left), (int)(inB->left));
  */
  N = nrrdElementNumber(nin);
  insert = nrrdDInsert[type];
  for (I=0; I<N; I++) {
    valA = nrrdIterValue(inA);
    valB = nrrdIterValue(inB);
    insert(nout->data, I, bop(valA, valB));
  }
  contA = nrrdIterContent(inA);
  contB = nrrdIterContent(inB);
  if (_nrrdContentSet(nout, airEnumStr(nrrdBinaryOp, op),
		      contA, "%s", contB)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(contA); free(contB); return 1;
  }
  if (nout != nin) {
    nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_NONE);
  }
  free(contA);
  free(contB); 
  return 0;
}

/* ---------------------------- ternary -------------- */

double _nrrdTernaryOpClamp(double a, double b, double c) {
  return AIR_CLAMP(a, b, c);}
double _nrrdTernaryOpLerp(double a, double b, double c) {
  /* we do something more than the simple lerp here because
     we want to facilitate usage as something which can get around
     non-existant values (b and c as NaN or Inf) without
     getting polluted by them. */

  if (0.0 == a) {
    return b;
  } else if (1.0 == a) {
    return c;
  } else {
    return AIR_LERP(a, b, c);
  }
}
double _nrrdTernaryOpExists(double a, double b, double c) {
  return (AIR_EXISTS(a) ? b : c);
}
double _nrrdTernaryOpInOpen(double a, double b, double c) {
  return (AIR_IN_OP(a, b, c));
}
double _nrrdTernaryOpInClosed(double a, double b, double c) {
  return (AIR_IN_CL(a, b, c));
}
double (*_nrrdTernaryOp[NRRD_TERNARY_OP_MAX+1])(double, double, double) = {
  NULL,
  _nrrdTernaryOpClamp,
  _nrrdTernaryOpLerp,
  _nrrdTernaryOpExists,
  _nrrdTernaryOpInOpen,
  _nrrdTernaryOpInClosed
};

/*
******** nrrdArithTerneryOp
**
** UNTESTED UNTESTED UNTESTED UNTESTED UNTESTED UNTESTED UNTESTED UNTESTED
**
** this is a simplified version of nrrdArithIterTernaryOp, written after
** that, in a hurry, to operate directly on three nrrds, instead with
** the NrrdIter nonsense
*/
int
nrrdArithTernaryOp(Nrrd *nout, int op, Nrrd *ninA, Nrrd *ninB, Nrrd *ninC) {
  char me[]="nrrdArithTernaryOp", err[AIR_STRLEN_MED], *contA, *contB, *contC;
  size_t N, I;
  int size[NRRD_DIM_MAX];
  double (*ins)(void *v, size_t I, double d), (*lupA)(void *v, size_t I), 
    (*lupB)(void *v, size_t I), (*lupC)(void *v, size_t I),
    (*top)(double a, double b, double c), valA, valB, valC;

  if (!( nout && !nrrdCheck(ninA) && !nrrdCheck(ninB) && !nrrdCheck(ninC) )) {
    sprintf(err, "%s: NULL pointer or invalid args", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( nrrdSameSize(ninA, ninB, AIR_TRUE) &&
	 nrrdSameSize(ninA, ninC, AIR_TRUE) )) {
    sprintf(err, "%s: size mismatch between arguments", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdTernaryOp, op)) {
    sprintf(err, "%s: ternary op %d invalid", me, op);
    biffAdd(NRRD, err); return 1;
  }
  
  nrrdAxisInfoGet_nva(ninA, nrrdAxisInfoSize, size);
  if (!( nout == ninA || nout == ninB || nout == ninC)) {
    if (nrrdMaybeAlloc_nva(nout, ninA->type, ninA->dim, size)) {
      sprintf(err, "%s: couldn't allocate output nrrd", me);
      biffAdd(NRRD, err); return 1;
    }
    if (nrrdAxisInfoCopy(nout, ninA, NULL, NRRD_AXIS_INFO_NONE)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
    nrrdPeripheralCopy(nout, ninA);
  }
  top = _nrrdTernaryOp[op];

  N = nrrdElementNumber(ninA);
  lupA = nrrdDLookup[ninA->type];
  lupB = nrrdDLookup[ninB->type];
  lupC = nrrdDLookup[ninC->type];
  ins = nrrdDInsert[nout->type];
  for (I=0; I<N; I++) {
    valA = lupA(ninA->data, I);
    valB = lupB(ninB->data, I);
    valC = lupC(ninC->data, I);
    ins(nout->data, I, top(valA, valB, valC));
  }

  contA = _nrrdContentGet(ninA);
  contB = _nrrdContentGet(ninB);
  contC = _nrrdContentGet(ninC);
  if (_nrrdContentSet(nout, airEnumStr(nrrdTernaryOp, op),
		      contA, "%s,%s", contB, contC)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(contA); free(contB); free(contC); return 1;
  }
  free(contA);
  free(contB); 
  free(contC); 

  return 0;
}

int
nrrdArithIterTernaryOp(Nrrd *nout, int op,
		       NrrdIter *inA, NrrdIter *inB, NrrdIter *inC) {
  char me[]="nrrdArithIterTernaryOp", err[AIR_STRLEN_MED],
    *contA, *contB, *contC;
  size_t N, I;
  int type, size[NRRD_DIM_MAX];
  double (*insert)(void *v, size_t I, double d), 
    (*top)(double a, double b, double c), valA, valB, valC;
  Nrrd *nin;

  if (!(nout && inA && inB && inC)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (airEnumValCheck(nrrdTernaryOp, op)) {
    sprintf(err, "%s: ternary op %d invalid", me, op);
    biffAdd(NRRD, err); return 1;
  }
  nin = inA->nrrd ? inA->nrrd : (inB->nrrd ? inB->nrrd : inC->nrrd);
  if (!nin) {
    sprintf(err, "%s: can't operate on three fixed values", me);
    biffAdd(NRRD, err); return 1;
  }
  type = nin->type;
  nrrdAxisInfoGet_nva(nin, nrrdAxisInfoSize, size);
  if (nrrdMaybeAlloc_nva(nout, type, nin->dim, size)) {
    sprintf(err, "%s: couldn't allocate output nrrd", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrdPeripheralInit(nout);
  top = _nrrdTernaryOp[op];

  /*
  fprintf(stderr, "%!s: inA->left = %d, inB->left = %d\n", me, 
	  (int)(inA->left), (int)(inB->left));
  */
  N = nrrdElementNumber(nin);
  insert = nrrdDInsert[type];
  for (I=0; I<N; I++) {
    valA = nrrdIterValue(inA);
    valB = nrrdIterValue(inB);
    valC = nrrdIterValue(inC);
    /*
    if (!(I % 1000)) {
      fprintf(stderr, "!%s: %d: top(%g,%g,%g) = %g\n", me, (int)I,
	      valA, valB, valC,
	      top(valA, valB, valC));
    }
    */
    insert(nout->data, I, top(valA, valB, valC));
  }
  contA = nrrdIterContent(inA);
  contB = nrrdIterContent(inB);
  contC = nrrdIterContent(inC);
  if (_nrrdContentSet(nout, airEnumStr(nrrdTernaryOp, op),
		      contA, "%s,%s", contB, contC)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(contA); free(contB); free(contC); return 1;
  }
  if (nout != nin) {
    nrrdAxisInfoCopy(nout, nin, NULL, NRRD_AXIS_INFO_NONE);
  }
  free(contA);
  free(contB); 
  free(contC); 
  return 0;
}

