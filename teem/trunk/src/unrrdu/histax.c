/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/

#include "private.h"

char *histaxName = "histax";
#define INFO "Replace each scanline along an axis with its histogram"
char *histaxInfo = INFO;
char *histaxInfoL = (INFO
		   ". ");

int
histaxMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int axis, type, bins;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_AXIS(axis, "axis to histogram along");
  hestOptAdd(&opt, "b", "bins", airTypeInt, 1, 1, &bins, NULL,
	     "# of bins in histogram");
  OPT_ADD_TYPE(type, "output type");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(histaxInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoAxis(nout, nin, axis, bins, type)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing axis histogramming:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  

  SAVE(nout, NULL);

  airMopOkay(mop);
  return 0;
}
