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

#include "ten.h"
#include "tenPrivate.h"

#define INFO "Calculate one or more eigenvectors in a DT volume"
char *_tend_evecInfoL =
  (INFO
   ". ");

int
tend_evecMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  int aniso, ret, *comp, compLen, cc, sx, sy, sz;
  Nrrd *nin, *nout;
  char *outS;
  float thresh, *edata, *tdata, eval[3], evec[9], scl, c[TEN_ANISO_MAX+1];
  size_t N, I;

  hestOptAdd(&hopt, "c", "c0 ", airTypeInt, 1, 3, &comp, NULL,
	     "which eigenvalues should be saved out. \"0\" for the "
	     "largest, \"1\" for the middle, \"2\" for the smallest, "
	     "\"01\", \"12\", \"012\" or similar for more than one",
	     &compLen);
  hestOptAdd(&hopt, "t", "thresh", airTypeFloat, 1, 1, &thresh, "0.5",
	     "confidence threshold");
  hestOptAdd(&hopt, "a", "aniso", airTypeEnum, 1, 1, &aniso, "unknown",
	     "Scale the vector(s) by this anisotropy metric.  By "
	     "default (not using this option), the vector(s) are "
	     "unit-length. " TEN_ANISO_DESC,
	     NULL, tenAniso);
  hestOptAdd(&hopt, "i", "nin", airTypeOther, 1, 1, &nin, NULL,
	     "input diffusion tensor volume", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, NULL,
	     "output image (floating point)");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_evecInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  for (cc=0; cc<compLen; cc++) {
    if (!AIR_IN_CL(0, comp[cc], 2)) {
      fprintf(stderr, "%s: requested component %d (%d of 3) not in [0..2]\n",
	      me, comp[cc], cc+1);
      airMopError(mop); exit(1);
    }
  }
  if (!tenValidTensor(nin, nrrdTypeFloat, AIR_TRUE)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: didn't get a valid DT volume:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  
  sx = nin->axis[1].size;
  sy = nin->axis[2].size;
  sz = nin->axis[3].size;

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  ret = nrrdAlloc(nout, nrrdTypeFloat, 4, 3*compLen, sx, sy, sz);
  if (ret) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble allocating output:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }

  N = sx*sy*sz;
  edata = nout->data;
  tdata = nin->data;
  if (1 == compLen) {
    for (I=0; I<N; I++) {
      tenEigensolve(eval, evec, tdata);
      scl = tdata[0] >= thresh;
      if (aniso) {
	tenAnisoCalc(c, eval);
	scl *= c[aniso];
      }
      ELL_3V_SCALE(edata, scl, evec+3*comp[0]);
      edata += 3;
      tdata += 7;
    }
  } else {
    for (I=0; I<N; I++) {
      tenEigensolve(eval, evec, tdata);
      scl = tdata[0] >= thresh;
      if (aniso) {
	tenAnisoCalc(c, eval);
	scl *= c[aniso];
      }
      for (cc=0; cc<compLen; cc++) {
	ELL_3V_SCALE(edata+3*cc, scl, evec+3*comp[cc]);
      }
      edata += 3*compLen;
      tdata += 7;
    }
  }

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }

  airMopOkay(mop);
  return 0;
}
TEND_CMD(evec, INFO);
