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

#define INFO "Estimate models from a set of DW images"
char *_tend_mfitInfoL =
  (INFO
   ". More docs here.");

int
tend_mfitMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;

  Nrrd *nin, *nout;
  char *outS, *modS;
  int knownB0, saveB0, verbose, mlfit, typeOut;
  unsigned int maxIter, minIter, starts;
  double sigma, eps;
  const tenModel *model;
  tenExperSpec *espec;

  hestOptAdd(&hopt, "v", "verbose", airTypeInt, 1, 1, &verbose, "0",
             "verbosity level");
  hestOptAdd(&hopt, "m", "model", airTypeString, 1, 1, &modS, NULL,
             "which model to fit. Use optional \"b0+\" prefix to "
             "indicate that the B0 image should also be saved.");
  hestOptAdd(&hopt, "ns", "# starts", airTypeUInt, 1, 1, &starts, "1",
             "number of random starting points at which to initialize "
             "fitting");
  hestOptAdd(&hopt, "ml", NULL, airTypeInt, 0, 0, &mlfit, NULL,
             "do ML fitting, rather than least-squares, which also "
             "requires setting \"-sigma\"");
  hestOptAdd(&hopt, "sigma", "sigma", airTypeDouble, 1, 1, &sigma, "nan",
             "Rician noise parameter");
  hestOptAdd(&hopt, "eps", "eps", airTypeDouble, 1, 1, &eps, "0.01",
             "convergence epsilon");
  hestOptAdd(&hopt, "mini", "min iters", airTypeUInt, 1, 1, &minIter, "3",
             "minimum required # iterations for fitting.");
  hestOptAdd(&hopt, "maxi", "max iters", airTypeUInt, 1, 1, &maxIter, "100",
             "maximum allowable # iterations for fitting.");
  hestOptAdd(&hopt, "knownB0", "bool", airTypeBool, 1, 1, &knownB0, NULL,
             "Indicates if the B=0 non-diffusion-weighted reference image "
             "is known (\"true\"), or if it has to be estimated along with "
             "the other model parameters (\"false\")");
  hestOptAdd(&hopt, "t", "type", airTypeEnum, 1, 1, &typeOut, "float",
             "output type of DWIs",
             NULL, nrrdType);
  hestOptAdd(&hopt, "i", "dwi", airTypeOther, 1, 1, &nin, "-",
             "all the diffusion-weighted images in one 4D nrrd",
             NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "o", "nout", airTypeString, 1, 1, &outS, "-",
             "output tensor volume");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_mfitInfoL);
  JUSTPARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);

  espec = tenExperSpecNew();
  airMopAdd(mop, espec, (airMopper)tenExperSpecNix, airMopAlways);
  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);
  if (tenModelParse(&model, &saveB0, AIR_FALSE, modS)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble parsing model \"%s\":\n%s\n", me, modS, err);
    airMopError(mop); return 1;
  }
  if (tenExperSpecFromKeyValueSet(espec, nin)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble getting exper from kvp:\n%s\n", me, err);
    airMopError(mop); return 1;
  }  
  if (tenModelSqeFit(nout, NULL, model, espec, nin,
                     knownB0, saveB0, typeOut, 
                     minIter, maxIter, starts, eps, NULL)) {
    airMopAdd(mop, err=biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble fitting:\n%s\n", me, err);
    airMopError(mop); return 1;
  }  

  if (nrrdSave(outS, nout, NULL)) {
    airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble writing:\n%s\n", me, err);
    airMopError(mop); return 1;
  }

  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(mfit, INFO); */
unrrduCmd tend_mfitCmd = { "mfit", INFO, tend_mfitMain };
