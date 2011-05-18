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

#include "unrrdu.h"
#include "privateUnrrdu.h"

#define INFO "Ternary operation on three nrrds or constants"
char *_unrrdu_3opInfoL =
(INFO
 ". Can have one, two, or three nrrds, but not zero. "
 "Use \"-\" for an operand to signify "
 "a nrrd to be read from stdin (a pipe).  Note, however, "
 "that \"-\" can probably only be used once (reliably).");

int
unrrdu_3opMain(int argc, const char **argv, char *me, hestParm *hparm) {
  hestOpt *opt = NULL;
  char *out, *err;
  NrrdIter *in1, *in2, *in3;
  Nrrd *nout, *ntmp=NULL;
  int op, type, E, pret;
  airArray *mop;

  hestOptAdd(&opt, NULL, "operator", airTypeEnum, 1, 1, &op, NULL,
             "Ternary operator. Possibilities include:\n "
             "\b\bo \"+\", \"x\": sum or product of three values\n "
             "\b\bo \"min\", \"max\": minimum, maximum\n "
             "\b\bo \"min_sm\": smoothed minimum function; "
             "min_sm(x, w, M) is like min(x,M) but for x > M-w (with w > 0) "
             "there is a smooth transition from x to asymptotic to M"
             "\b\bo \"max_sm\": smoothed maximum function; "
             "max_sm(m, w, x) is like max(M,x) but for x < m+w (with w > m) "
             "there is a smooth transition from x to asymptotic to m"
             "\b\bo \"clamp\": second value is clamped to range between "
             "the first and the third\n "
             "\b\bo \"ifelse\": if 1st value non-zero, then 2nd value, else "
             "3rd value\n "
             "\b\bo \"lerp\": linear interpolation between the 2nd and "
             "3rd values, as the 1st value varies between 0.0 and 1.0, "
             "respectively\n "
             "\b\bo \"exists\": if the first value exists, use the second "
             "value, otherwise use the third\n "
             "\b\bo \"in_op\": 1 iff second value is > first and < "
             "third, 0 otherwise\n "
             "\b\bo \"in_cl\": 1 iff second value is >= first and <= "
             "third, 0 otherwise\n "
             "\b\bo \"gauss\": evaluate (at 1st value) Gaussian with mean=2nd "
             "and stdv=3rd value",
             NULL, nrrdTernaryOp);
  hestOptAdd(&opt, NULL, "in1", airTypeOther, 1, 1, &in1, NULL,
             "First input.  Can be a single value or a nrrd.",
             NULL, NULL, nrrdHestIter);
  hestOptAdd(&opt, NULL, "in2", airTypeOther, 1, 1, &in2, NULL,
             "Second input.  Can be a single value or a nrrd.",
             NULL, NULL, nrrdHestIter);
  hestOptAdd(&opt, NULL, "in3", airTypeOther, 1, 1, &in3, NULL,
             "Third input.  Can be a single value or a nrrd.",
             NULL, NULL, nrrdHestIter);
  hestOptAdd(&opt, "t,type", "type", airTypeOther, 1, 1, &type, "default",
             "type to convert all nrrd inputs to, prior to "
             "doing operation.  This also determines output type. "
             "By default (not using this option), the types of the input "
             "nrrds are left unchanged.",
             NULL, NULL, &unrrduHestMaybeTypeCB);
  OPT_ADD_NOUT(out, "output nrrd");

  mop = airMopNew();
  airMopAdd(mop, opt, (airMopper)hestOptFree, airMopAlways);

  USAGE(_unrrdu_3opInfoL);
  PARSE();
  airMopAdd(mop, opt, (airMopper)hestParseFree, airMopAlways);

  nout = nrrdNew();
  airMopAdd(mop, nout, (airMopper)nrrdNuke, airMopAlways);

  /*
  fprintf(stderr, "%s: op = %d\n", me, op);
  fprintf(stderr, "%s: in1->left = %d, in2->left = %d\n", me, 
          (int)(in1->left), (int)(in2->left));
  */
  if (nrrdTypeDefault != type) {
    /* they wanted to convert nrrds to some other type first */
    E = 0;
    if (in1->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in1->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in1, ntmp);
    }
    if (in2->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in2->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in2, ntmp);
    }
    if (in3->ownNrrd) {
      if (!E) E |= nrrdConvert(ntmp=nrrdNew(), in3->ownNrrd, type);
      if (!E) nrrdIterSetOwnNrrd(in3, ntmp);
    }
    if (E) {
      airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
      fprintf(stderr, "%s: error converting input nrrd(s):\n%s", me, err);
      airMopError(mop);
      return 1;
    }
    /* this will still leave a nrrd in the NrrdIter for nrrdIterNix()
       (called by hestParseFree() called be airMopOkay()) to clear up */
  }
  if (nrrdArithIterTernaryOp(nout, op, in1, in2, in3)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: error doing ternary operation:\n%s", me, err);
    airMopError(mop);
    return 1;
  }
  
  SAVE(out, nout, NULL);

  airMopOkay(mop);
  return 0;
}

UNRRDU_CMD(3op, INFO);

