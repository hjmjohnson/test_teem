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

char *reshapeName = "reshape";
#define INFO "Superficially change dimension and/or axes sizes"
char *reshapeInfo = INFO;
char *reshapeInfoL = (INFO
		      ". The underlying linear ordering of the samples is "
		      "unchanged, but the reported dimension or axes sizes "
		      "are changed.  Identical in concept to Matlab's "
		      "\"reshape\" command.");

int
reshapeMain(int argc, char **argv, char *me) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int *size, sizeLen;
  airArray *mop;

  OPT_ADD_NIN(nin, "input nrrd");
  hestOptAdd(&opt, "s", "sz0 sz1 ", airTypeInt, 1, -1, &size, NULL,
	     "new axes sizes", &sizeLen);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopInit();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(reshapeInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdReshape_nva(nout, nin, sizeLen, size)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error reshaping nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(NULL);

  airMopOkay(mop);
  return 0;
}
