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


#include "../bane.h"

char *me;

void
usage() {
  /*                      0     1        2       3          4   (5) */
  fprintf(stderr, "usage: %s <infoIn> <sigma> <gthresh> <posOut>\n", me);
  exit(1);
}

int
main(int argc, char *argv[]) {
  Nrrd *info, *pos;
  float sigma, gthresh;
  char *iStr, *oStr, *sigStr, *gthStr;

  me = argv[0];
  if (argc != 5) {
    usage();
  }
  iStr = argv[1];
  sigStr = argv[2];
  gthStr = argv[3];
  oStr = argv[4];

  if (1 != sscanf(sigStr, "%g", &sigma) ||
      1 != sscanf(gthStr, "%g", &gthresh)) {
    fprintf(stderr, "%s: couldn't parse %s and %s as floats\n", me, 
	    sigStr, gthStr);
    usage();
  }

  if (nrrdLoad(info=nrrdNew(), iStr)) {
    fprintf(stderr, "%s: trouble reading \"%s\" :\n%s\n", me, 
	    iStr, biffGet(NRRD));
    exit(1);
  }
  if (banePosCalc(pos = nrrdNew(), sigma, gthresh, info)) {
    fprintf(stderr, "%s: trouble calculating %s:\n%s\n", me,
	    2 == info->dim ? "p(v,g)" : "p(v)", biffGet(BANE));
    exit(1);
  }
  if (nrrdSave(oStr, pos, NULL)) {
    fprintf(stderr, "%s: trouble writing output to \"%s\"\n", me, oStr);
    exit(1);
  }
  nrrdNuke(info);
  nrrdNuke(pos);
  exit(0);
}

