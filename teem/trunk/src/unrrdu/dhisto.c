/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2009--2019  University of Chicago
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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Create image of 1-D value histogram"
static const char *_unrrdu_dhistoInfoL =
  (INFO
   ". With \"-nolog\", this becomes a quick & dirty way of plotting a function.\n "
   "* Uses nrrdHistoDraw");

int
unrrdu_dhistoMain(int argc, const char **argv, const char *me,
                  hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  Nrrd *nin, *nout;
  int pret, nolog, notick;
  unsigned int size;
  airArray *mop;
  double max;

  hestOptAdd(&opt, "h,height", "height", airTypeUInt, 1, 1, &size, NULL,
             "height of output image (horizontal size is determined by "
             "number of bins in input histogram).");
  hestOptAdd(&opt, "nolog", NULL, airTypeInt, 0, 0, &nolog, NULL,
             "do not show the log-scaled histogram with decade tick-marks");
  hestOptAdd(&opt, "notick", NULL, airTypeInt, 0, 0, &notick, NULL,
             "do not draw the log decade tick marks");
  hestOptAdd(&opt, "max,maximum", "max # hits", airTypeDouble, 1, 1,
             &max, "nan",
             "constrain the top of the drawn histogram to be at this "
             "number of hits.  This will either scale the drawn histogram "
             "downward or clip its top, depending on whether the given max "
             "is higher or lower than the actual maximum bin count.  By "
             "not using this option (the default), the actual maximum bin "
             "count is used");
  OPT_ADD_NIN(nin, "input nrrd");
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_dhistoInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  if (nrrdHistoDraw(nout, nin, size,
                    nolog ? AIR_FALSE : (notick ? 2 : AIR_TRUE),
                    max)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error drawing histogram nrrd:\n%s", me, err);
    airMopError(mop);
    return 1;
  }

  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(dhisto, INFO);
