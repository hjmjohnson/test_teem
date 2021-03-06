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

#define INFO "Map nrrd through *irregular* univariate map (\"colormap\")"
char *_unrrdu_imapInfoL =
(INFO
 ". A map is irregular if the control points are not evenly "
 "spaced along the domain, and hence their position must be "
 "explicitly represented in the map.  As nrrds, these maps "
 "are necessarily 2D.  Along axis 0, the first value is the "
 "location of the control point, and the remaining values "
 "give are the range of the map for that control point. "
 "The output value(s) is the result of linearly "
 "interpolating between value(s) from the map.");

int
unrrdu_imapMain(int argc, char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nmap, *nacl, *nout;
  airArray *mop;
  int typeOut, rescale, aclLen, pret;
  double min, max;

  hestOptAdd(&opt, "m", "map", airTypeOther, 1, 1, &nmap, NULL,
	     "irregular map to map input nrrd through",
	     NULL, NULL, nrrdHestNrrd);
  hestOptAdd(&opt, "l", "aclLen", airTypeInt, 1, 1, &aclLen, "0",
	     "length of accelerator array, used to try to speed-up "
	     "task of finding between which pair of control points "
	     "a given value lies.  Not terribly useful for small maps "
	     "(about 10 points or less).  Use 0 to turn accelorator off. ");
  hestOptAdd(&opt, "r", NULL, airTypeInt, 0, 0, &rescale, NULL,
	     "rescale the input values from the input range to the "
	     "map domain");
  hestOptAdd(&opt, "min", "value", airTypeDouble, 1, 1, &min, "nan",
	     "Value to map to low end of map. Defaults to lowest value "
	     "found in input nrrd.  Explicitly setting this (and the "
	     "same for the max) is useful only with rescaling (\"-r\")");
  hestOptAdd(&opt, "max", "value", airTypeDouble, 1, 1, &max, "nan",
	     "Value to map to high end of map. Defaults to highest value "
	     "found in input nrrd.");
  hestOptAdd(&opt, "t", "type", airTypeOther, 1, 1, &typeOut, "default",
	     "specify the type (\"int\", \"float\", etc.) of the "
	     "output nrrd. "
	     "By default (not using this option), the output type "
	     "is the map's type.",
             NULL, NULL, &unrrduHestMaybeTypeCB);
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_imapInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (aclLen) {
    nacl = nrrdNew();
    airMopAdd(mop, nacl, (airMopper)nrrdNuke, airMopAlways);
    if (nrrd1DIrregAclGenerate(nacl, nmap, aclLen)) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: trouble generating accelerator:\n%s", me, err);
      airMopError(mop);
      return 1;
    }
  } else {
    nacl = NULL;
  }
  if (AIR_EXISTS(min))
    nin->min = min;
  if (AIR_EXISTS(max))
    nin->max = max;
  if (rescale) {
    nrrdMinMaxCleverSet(nin);
  }
  if (nrrdTypeDefault == typeOut) {
    typeOut = nmap->type;
  }
  /* some very non-exhaustive tests seemed to indicate that the
     accelerator does not in fact reliably speed anything up.
     This of course depends on the size of the imap (# points),
     but chances are most imaps will have only a handful of points,
     in which case the binary search in _nrrd1DIrregFindInterval()
     will finish quickly ... */
  if (nrrdApply1DIrregMap(nout, nin, nmap, nacl, typeOut, rescale)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: trouble applying map:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(imap, INFO);

