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

#include "ten.h"
#include "tenPrivate.h"

#define INFO "Register diffusion-weighted echo-planar images"
char *_tend_epiregInfoL =
  (INFO
   ". This registration corrects the shear, scale, and translate along "
   "the phase encoding direction (assumed to be the Y (second) axis of "
   "the image) caused by eddy currents from the diffusion-encoding "
   "gradients with echo-planar imaging.  The method is based on calculating "
   "moments of segmented images, where the segmentation is a simple "
   "procedure based on blurring (optional), thresholding and "
   "connected component analysis. "
   "The registered DWIs are resampled with the "
   "chosen kernel, with the seperate DWIs stacked along axis 0.");

int
tend_epiregMain(int argc, char **argv, char *me, hestParm *hparm) {
  int pret;
  hestOpt *hopt = NULL;
  char *perr, *err;
  airArray *mop;
  char *outS, *buff;

  NrrdKernelSpec *ksp;
  Nrrd **nin, **nout, *ngrad;
  int ref, ninLen, noverbose, progress, nocc, ni, baseNum;
  float bw[2], thr, fitFrac;
  
  hestOptAdd(&hopt, "i", "dwi0 dwi1", airTypeOther, 3, -1, &nin, NULL,
	     "all the diffusion-weighted images (DWIs), as seperate nrrds",
	     &ninLen, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "g", "grads", airTypeOther, 1, 1, &ngrad, NULL,
	     "array of gradient directions, in the same order as the "
	     "associated DWIs were given to \"-i\"", NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&hopt, "r", "reference", airTypeInt, 1, 1, &ref, "-1",
	     "which of the DW volumes (zero-based numbering) should be used "
	     "as the standard, to which all other images are transformed. "
	     "Using -1 (the default) means that 9 intrinsic parameters "
	     "governing the relationship between the gradient direction "
	     "and the resulting distortion are estimated and fitted, "
	     "ensuring good registration with the non-diffusion-weighted "
	     "T2 image (which is never explicitly used in registration). "
	     "Otherwise, by picking a specific DWI, no distortion parameter "
	     "estimation is done. ");
  hestOptAdd(&hopt, "nv", NULL, airTypeInt, 0, 0, &noverbose, NULL,
	     "turn OFF verbose mode, and "
	     "have no idea what stage processing is at.");
  hestOptAdd(&hopt, "p", NULL, airTypeInt, 0, 0, &progress, NULL,
	     "save out intermediate steps of processing");
  hestOptAdd(&hopt, "bw", "x,y blur", airTypeFloat, 2, 2, bw, "1.0 2.0",
	     "standard devs in X and Y directions of gaussian filter used "
	     "to blur the DWIs prior to doing segmentation. This blurring "
	     "does not effect the final resampling of registered DWIs. "
	     "Use \"0.0 0.0\" to say \"no blurring\"");
  hestOptAdd(&hopt, "t", "DWI thresh", airTypeFloat, 1, 1, &thr, "nan",
	     "Threshold value to use on DWIs, "
	     "to do initial seperation of brain and non-brain.  By default, "
	     "the threshold is determined automatically by histogram "
	     "analysis. ");
  hestOptAdd(&hopt, "ncc", NULL, airTypeInt, 0, 0, &nocc, NULL,
	     "do *NOT* do connected component (CC) analysis, after "
	     "thresholding and before moment calculation.  Doing CC analysis "
	     "usually gives better results because it converts the "
	     "thresholding output into something much closer to a "
	     "real segmentation");
  hestOptAdd(&hopt, "f", "fit frac", airTypeFloat, 1, 1, &fitFrac, "0.70",
	     "(only meaningful with \"-r 0\") When doing linear fitting "
	     "of the intrinsic distortion parameters, it is good "
	     "to ignore the slices for which the segmentation was poor.  A "
	     "heuristic is used to rank the slices according to segmentation "
	     "quality.  This option controls how many of the (best) slices "
	     "contribute to the fitting.  Use \"0\" to disable distortion "
	     "parameter fitting. ");
  hestOptAdd(&hopt, "k", "kernel", airTypeOther, 1, 1, &ksp, "cubic:0,0.5",
	     "kernel for resampling DWIs along the phase-encoding "
	     "direction during final registration stage",
	     NULL, NULL, nrrdHestKernelSpec);
  hestOptAdd(&hopt, "bn", "base #", airTypeInt, 1, 1, &baseNum, "1",
	     "first number to use in numbered sequence of output files.");
  hestOptAdd(&hopt, "o", "prefix", airTypeString, 1, 1, &outS, NULL,
	     "prefix for output filenames.  Will save out one (registered) "
	     "DWI for each input DWI, using the same type as the input.");

  mop = airMopNew();
  airMopAdd(mop, hopt, (airMopper)hestOptFree, airMopAlways);
  USAGE(_tend_epiregInfoL);
  PARSE();
  airMopAdd(mop, hopt, (airMopper)hestParseFree, airMopAlways);
  
  nout = (Nrrd **)calloc(ninLen, sizeof(Nrrd *));
  buff = (char *)calloc(airStrlen(outS) + 10, sizeof(char));
  if (!( nout && buff )) {
    fprintf(stderr, "%s: couldn't allocate buffers", me);
    airMopError(mop); return 1;
  }
  airMopAdd(mop, nout, airFree, airMopAlways);
  airMopAdd(mop, buff, airFree, airMopAlways);
  for (ni=0; ni<ninLen; ni++) {
    airMopAdd(mop, nout[ni]=nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  }
  if (tenEpiRegister(nout, nin, ninLen, ngrad,
		     ref,
		     bw[0], bw[1], fitFrac, thr, !nocc,
		     ksp->kernel, ksp->parm,
		     progress, !noverbose)) {
    airMopAdd(mop, err = biffGetDone(TEN), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble:\n%s\n", me, err);
    airMopError(mop); exit(1);
  }
  for (ni=0; ni<ninLen; ni++) {
    if (ninLen+baseNum > 99) {
      sprintf(buff, "%s%05d.nrrd", outS, ni+baseNum);
    } else if (ninLen+baseNum > 9) {
      sprintf(buff, "%s%02d.nrrd", outS, ni+baseNum);
    } else {
      sprintf(buff, "%s%d.nrrd", outS, ni+baseNum);
    }
    if (nrrdSave(buff, nout[ni], NULL)) {
      airMopAdd(mop, err=biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble writing \"%s\":\n%s\n", me, buff, err);
      airMopError(mop); return 1;
    }
  }
  
  airMopOkay(mop);
  return 0;
}
/* TEND_CMD(epireg, INFO); */
unrrduCmd tend_epiregCmd = { "epireg", INFO, tend_epiregMain };
