/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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

#ifndef TEN_HAS_BEEN_INCLUDED
#define TEN_HAS_BEEN_INCLUDED

#include <math.h>

#include <air.h>
#include <biff.h>
#include <ell.h>
#include <nrrd.h>
#include <unrrdu.h>
#include <dye.h>
#include <gage.h>
#include <limn.h>
#include <echo.h>

#include "tenMacros.h"

#if defined(_WIN32) && defined(TEEM_DLL)
#define ten_export __declspec(dllimport)
#else
#define ten_export
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TEN "ten"

enum {
  tenAnisoUnknown,    /*  0: nobody knows */
  tenAniso_Cl1,       /*  1: Westin's linear (first version) */
  tenAniso_Cp1,       /*  2: Westin's planar (first version) */
  tenAniso_Ca1,       /*  3: Westin's linear + planar (first version) */
  tenAniso_Cs1,       /*  4: Westin's spherical (first version) */
  tenAniso_Ct1,       /*  5: gk's anisotropy type (first version) */
  tenAniso_Cl2,       /*  6: Westin's linear (second version) */
  tenAniso_Cp2,       /*  7: Westin's planar (second version) */
  tenAniso_Ca2,       /*  8: Westin's linear + planar (second version) */
  tenAniso_Cs2,       /*  9: Westin's spherical (second version) */
  tenAniso_Ct2,       /* 10: gk's anisotropy type (second version) */
  tenAniso_RA,        /* 11: Bass+Pier's relative anisotropy */
  tenAniso_FA,        /* 12: (Bass+Pier's fractional anisotropy)/sqrt(2) */
  tenAniso_VF,        /* 13: volume fraction = 1-(Bass+Pier's volume ratio) */
  tenAniso_Q,         /* 14: radius of root circle is 2*sqrt(Q/9) 
		             (this is 9 times proper Q in cubic solution) */
  tenAniso_R,         /* 15: phase of root circle is acos(R/Q^3) */
  tenAniso_S,         /* 16: sqrt(Q^3 - R^2) */
  tenAniso_Th,        /* 17: R/Q^3 */
  tenAniso_Cz,        /* 18: Zhukov's invariant-based anisotropy metric */
  tenAniso_Tr,        /* 19: plain old trace */
  tenAnisoLast
};
#define TEN_ANISO_MAX    19

typedef struct {
  int glyphType;          /* from tenGlyphType* enum */
  float sqSharp;          /* how much to turn on edges in superquadrics */
  Nrrd *nmask;
  int anisoType, colEvec, res;
  float colSat, colGamma;
  float edgeWidth[5];     /* same as limnOptsPS */
  float anisoThresh, confThresh, useColor;
  float maskThresh, glyphScale;
  float sumFloor, sumCeil;
  float fakeSat;
  int dim;
} tenGlyphParm;

#define TEN_ANISO_DESC \
  "All the Westin metrics come in two versions.  Currently supported:\n " \
  "\b\bo \"cl1\", \"cl2\": Westin's linear\n " \
  "\b\bo \"cp1\", \"cp2\": Westin's planar\n " \
  "\b\bo \"ca1\", \"ca2\": Westin's linear + planar\n " \
  "\b\bo \"cs1\", \"cs2\": Westin's spherical (1-ca)\n " \
  "\b\bo \"ct1\", \"ct2\": GK's anisotropy type (cp/ca)\n " \
  "\b\bo \"ra\": Basser/Pierpaoli relative anisotropy\n " \
  "\b\bo \"fa\": Basser/Pierpaoli fractional anisotropy/sqrt(2)\n " \
  "\b\bo \"vf\": volume fraction = 1-(Basser/Pierpaoli volume ratio)\n " \
  "\b\bo \"tr\": trace"

enum {
  tenGageUnknown = -1,  /* -1: nobody knows */
  tenGageTensor,        /*  0: "t", the reconstructed tensor: GT[7] */
  tenGageTrace,         /*  1: "tr", trace of tensor: *GT */
  tenGageFrobTensor,    /*  2: "fro", frobenius norm of tensor: *GT */
  tenGageEval,          /*  3: "eval", eigenvalues of tensor
			       (sorted descending) : GT[3] */
  tenGageEvec,          /*  4: "evec", eigenvectors of tensor: GT[9] */
  tenGageTensorGrad,    /*  5: "tg", all tensor component gradients, starting
			       with the confidence value gradient: GT[21] */
  tenGageQ,             /*  6: "q", Q anisotropy (or 9 times it): *GT */
  tenGageQGradVec,      /*  7: "qv", gradient of Q anisotropy: GT[3] */
  tenGageQGradMag,      /*  8: "qg", grad mag of Q anisotropy: *GT */
  tenGageQNormal,       /*  9: "qn", normalized gradient of Q
			       anisotropy: GT[3] */
  tenGageMultiGrad,     /* 10: "mg", sum of outer products of the tensor 
			       matrix elements, correctly counting the
			       off-diagonal entries twice, but not counting
			       the confidence value: GT[9] */
  tenGageFrobMG,        /* 11: "frmg", frobenius norm of multi gradient: *GT */
  tenGageMGEval,        /* 12: "mgeval", eigenvalues of multi gradient: GT[3]*/
  tenGageMGEvec,        /* 13: "mgevec", eigenvectors of multi
			       gradient: GT[9] */
  tenGageAniso,         /* 14: "an", all anisotropies: GT[TEN_ANISO_MAX+1] */
  tenGageLast
};
#define TEN_GAGE_MAX       14
#define TEN_GAGE_TOTAL_ANS_LENGTH (72+TEN_ANISO_MAX+1)

/*
******** tenFiberType* enum
**
** the different kinds of fiber tractography that we do
*/
enum {
  tenFiberTypeUnknown,    /* 0: nobody knows */
  tenFiberTypeEvec1,      /* 1: standard following of principal eigenvector */
  tenFiberTypeTensorLine, /* 2: Weinstein-Kindlmann tensorlines */
  tenFiberTypePureLine,   /* 3: "pure" tensorlines- multiplication only */
  tenFiberTypeZhukov,     /* 4: Zhukov's oriented tensor reconstruction */
  tenFiberTypeLast
};
#define TEN_FIBER_TYPE_MAX   4

/*
******** tenFiberStop* enum
**
** the different reasons why fibers stop going
*/
enum {
  tenFiberStopUnknown,  /* 0: nobody knows */
  tenFiberStopAniso,    /* 1: specified anisotropy got below specified level */
  tenFiberStopLen,      /* 2: fiber got too long */
  tenFiberStopBounds,   /* 3: fiber position stepped outside volume */
  tenFiberStopLast
};
#define TEN_FIBER_STOP_MAX 3

/*
******** tenGlyphType* enum
** 
** the different types of glyphs that may be used for tensor viz
*/
enum {
  tenGlyphTypeUnknown,    /* 0: nobody knows */
  tenGlyphTypeBox,        /* 1 */
  tenGlyphTypeSphere,     /* 2 */
  tenGlyphTypeCylinder,   /* 3 */
  tenGlyphTypeSuperquad,  /* 4 */
  tenGlyphTypeLast
};
#define TEN_GLYPH_TYPE_MAX   4

typedef struct {
  /* ---- input -------- */
  Nrrd *dtvol;          /* the volume being analyzed */
  NrrdKernelSpec *ksp;  /* how to interpolate tensor values in dtvol */
  int type;            /* from the tenFiber* enum */
  double step,          /* step size in world space */
    maxHalfLen;         /* longest propagation (forward or backward) allowed
			   from midpoint */
  /* ---- internal ----- */
  gageContext *gtx;     /* wrapped around dtvol */
  gage_t *dten,         /* gageAnswerPointer(gtx->pvl[0], tenGageTensor) */
    *evec;              /* gageAnswerPointer(gtx->pvl[0], tenGageEvec) */
  /* ---- output ------- */
  double len;           /* total fiber length in world space */
  int stop[2];          /* why backward/forward (0/1) tracing stopped
			   (from tenFiberStop* enum) */
} tenFiberContext;

/* defaultsTen.c */
extern ten_export const char tenDefFiberKernel[];
extern ten_export double tenDefFiberStep;
extern ten_export double tenDefFiberMaxHalfLen;

/* enumsTen.c */
extern ten_export airEnum *tenAniso;
extern ten_export airEnum _tenGage;
extern ten_export airEnum *tenGage;
extern ten_export airEnum *tenFiberType;
extern ten_export airEnum *tenFiberStop;
extern ten_export airEnum *tenGlyphType;

/* glyph.c */
extern tenGlyphParm *tenGlyphParmNew();
extern tenGlyphParm *tenGlyphParmNix(tenGlyphParm *parm);
extern int tenGlyphParmCheck(tenGlyphParm *parm, Nrrd *nten);
extern int tenGlyphGen(limnObj *glyphs, echoScene *scene,
		       Nrrd *nten, tenGlyphParm *parm);

/* tensor.c */
extern ten_export int tenVerbose;
extern int tenTensorCheck(Nrrd *nin, int wantType, int useBiff);
extern int tenExpand(Nrrd *tnine, Nrrd *tseven, float scale, float thresh);
extern int tenShrink(Nrrd *tseven, Nrrd *nconf, Nrrd *tnine);
extern int tenEigensolve(float eval[3], float evec[9], float t[7]);
extern int tenTensorMake(Nrrd *nout, Nrrd *nconf, Nrrd *neval, Nrrd *nevec);

/* chan.c */
extern void tenCalcOneTensor1(float tens[7], float chan[7], 
			      float thresh, float slope, float b);
extern void tenCalcOneTensor2(float tens[7], float chan[7], 
			      float thresh, float slope, float b);
extern int tenCalcTensor(Nrrd *nout, Nrrd *nin, int version,
			 float thresh, float slope, float b);

/* aniso.c */
extern ten_export float tenAnisoSigma;  /* added to denominator
					   in Westin anisos */
extern void tenAnisoCalc(float c[TEN_ANISO_MAX+1], float eval[3]);
extern int tenAnisoPlot(Nrrd *nout, int aniso, int res, int whole);
extern int tenAnisoVolume(Nrrd *nout, Nrrd *nin, int aniso, float thresh);
extern int tenAnisoHistogram(Nrrd *nout, Nrrd *nin,
			     int version, int resolution);

/* miscTen.c */
extern short tenEvqOne(float vec[3], float scl);
extern int tenEvqVolume(Nrrd *nout, Nrrd *nin, int which,
			int aniso, int scaleByAniso);

/* fiberMethods.c */
extern const char tenDefFiberKernel[];
extern tenFiberContext *tenFiberContextNew(Nrrd *dtvol);
extern int tenFiberTypeSet(tenFiberContext *tfx, int type);
extern int tenFiberKernelSet(tenFiberContext *tfx,
			     NrrdKernel *kern,
			     double parm[NRRD_KERNEL_PARMS_NUM]);
extern int tenFiberUpdate(tenFiberContext *tfx);
extern tenFiberContext *tenFiberContextNix(tenFiberContext *tfx);

/* fiber.c */
extern int tenFiberTrace(tenFiberContext *tfx, Nrrd *fiber,
			 double startX, double startY, double startZ);

/* tenGage.c */
extern ten_export gageKind *tenGageKind;

/* tend{Flotsam,Anplot,Anvol,Evec,Eval,...}.c */
#define TEND_DECLARE(C) extern ten_export unrrduCmd tend_##C##Cmd;
#define TEND_LIST(C) &tend_##C##Cmd,
#define TEND_MAP(F) \
F(make) \
F(calc) \
F(sten) \
F(glyph) \
F(anplot) \
F(anvol) \
F(anhist) \
F(point) \
F(eval) \
F(evec) \
F(evq) \
F(expand) \
F(shrink) \
F(satin)
TEND_MAP(TEND_DECLARE)
extern ten_export unrrduCmd *tendCmdList[]; 
extern void tendUsage(char *me, hestParm *hparm);


#ifdef __cplusplus
}
#endif

#endif /* TEN_HAS_BEEN_INCLUDED */
