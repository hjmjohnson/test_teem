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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Replace a sub-region with a different nrrd"
char *_unrrdu_insetInfoL = 
(INFO ". This is functionally the opposite of \"crop\".");

int
unrrdu_insetMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout, *nsub;
  int *minOff, minLen, ax, pret, min[NRRD_DIM_MAX];
  airArray *mop;

  OPT_ADD_BOUND("min", minOff,
		"coordinates of where to locate sub-volume within "
		"input nrrd.\n "
		"\b\bo <int> gives 0-based index\n "
		"\b\bo M, M+<int>, M-<int> give index relative "
		"to the last sample on the axis (M == #samples-1).",
		minLen);
  hestOptAdd(&opt, "s", "nsub", airTypeOther, 1, 1, &(nsub), NULL,
	     "sub-region nrrd.  This the data to be inset in \"nin\"",
             NULL, NULL, nrrdHestNrrd);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_insetInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  if (!( minLen == nin->dim )) {
    fprintf(stderr,
	    "%s: # min coords (%d) != nrrd dim (%d)\n",
	    me, minLen, nin->dim);
    airMopError(mop);
    return 1;
  }
  for (ax=0; ax<=nin->dim-1; ax++) {
    if (-1 == minOff[0 + 2*ax]) {
      fprintf(stderr, "%s: can't use m+<int> specification for axis %d min\n",
	      me, ax);
      airMopError(mop);
      return 1;
    }
  }
  for (ax=0; ax<=nin->dim-1; ax++) {
    min[ax] = minOff[0 + 2*ax]*(nin->axis[ax].size-1) + minOff[1 + 2*ax];
  }

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdInset(nout, nin, nsub, min)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error insetting nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(inset, INFO);
