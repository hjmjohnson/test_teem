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

#include "../moss.h"

int
main(int argc, char *argv[]) {
  char *me, *info="inverts a moss transform";
  hestOpt *hopt=NULL;
  double *mat, inv[6];
  
  me = argv[0];
  hestOptAdd(&hopt, "t", "transform", airTypeOther, 1, 1, &mat, "identity",
	     "transform(s) to apply to image",
	     NULL, NULL, mossHestTransform);
  hestParseOrDie(hopt, argc-1, argv+1, NULL,
		 me, info, AIR_TRUE, AIR_TRUE, AIR_TRUE);

  fprintf(stderr, "%s: got transform:\n", me);
  mossMatPrint(stderr, mat);
  mossMatInvert(inv, mat);
  fprintf(stderr, "\n%s: inverse:\n", me);
  mossMatPrint(stderr, inv);
  
  exit(0);
}

