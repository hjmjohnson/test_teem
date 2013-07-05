/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
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

#define INFO "Information about this program and its use"

int
unrrdu_aboutMain(int argc, const char **argv, const char *me,
                 hestParm *hparm) {
  char buff[AIR_STRLEN_MED], fmt[AIR_STRLEN_MED];
  char par1[] = "\t\t\t\t"
    "\"unu\" is a command-line interface to much of the functionality "
    "in \"nrrd\", a C library for raster data processing. Nrrd is one "
    "library in the \"Teem\" collection of libraries.  More information "
    "about Teem is at <http://teem.sf.net>. A checkout of Teem source "
    "is available via:\n "
    "svn co http://teem.svn.sf.net/svnroot/teem/teem/trunk teem\n ";
  char par2[] = "\t\t\t\t"
    "Long-term maintenance of this software depends on funding, and "
    "funding depends on being able to document who is using it for what.  "
    "If unu or Nrrd has helped in your research, including for simple one-off "
    "experiments or mundane data hacking, the developers of Teem would love "
    "to know. There are multiple ways of communicating this.  "
    "In your publications, consider adding a line such as this "
    "in the Acknowledgments: "
    "\"Data processing performed with the unu tool, "
    "part of the Teem toolkit available at "
    "http://teem.sf.net\". "
    "Alternatively, please email glk@uchicago.edu and briefly describe "
    "how Teem software has helped in your work. "
    "Please also consider joining the teem-users mailing list: "
    "<http://lists.sourceforge.net/lists/listinfo/teem-users>. This is "
    "the primary forum for feedback, questions, and feature requests.\n ";
  char par3[] = "\t\t\t\t"
    "A summary list of unu commands is generated by running simply \"unu\". "
    "Running a unu command without additional arguments "
    "(e.g. \"unu slice\") generates its description and usage information. "
    "This includes information (marked with \"* Uses ...\") "
    "about the underling Nrrd library calls "
    "(e.g. nrrdSlice()) that implement the basic functionality in "
    "that unu command.\n ";
  char par4[] = "\t\t\t\t"
    "The utility of unu is mainly as a pre-processing tool for getting "
    "data into a type, encoding, format, or dimensions best suited for some "
    "visualization or rendering task.  Also, slices and projections are "
    "effective ways to visually inspect the contents of a dataset. "
    "Especially useful commands include make, resample, crop, slice, "
    "project, histo, dhisto, quantize, and save. Unu can process "
    "CT and MRI volume datasets, grayscale and color images, "
    "time-varying volumes of vector fields (5-D arrays), and more. "
    "Currently supported formats are plain text files (2-D float arrays), "
    "NRRD, VTK structured points, and PNG and PNM images.  "
    "\"unu make -bs -1\" can read from most DICOM files.  "
    "\"unu save\" can generate EPS files. "
    "Supported encodings are raw, ascii, hex, gzip, and bzip2.\n";
  char par5[] = "\t\t\t\t"
    "Much of the functionality of unu derives from chaining multiple "
    "invocations together with pipes (\"|\"), minimizing the "
    "need to save out intermediate files. For example, if "
    "\"data.raw.gz\" is a gzip'ed 256\tx\t256\tx\t80 volume of raw floats "
    "written from a PC, "
    "then the following will save to \"zsum.png\" a histogram "
    "equalized summation projection along the slowest axis:\n";
  char par6[] = "\tunu make -i data.raw.gz -t float -s 256 256 80 "
    "-e gzip -en little \\\n "
    "  | unu project -a 2 -m sum \\\n "
    "  | unu heq -b 2000 -s 1 \\\n "
    "  | unu quantize -b 8 -o zsum.png"
    "\n";
  int enc, form, miss;

  AIR_UNUSED(argc);
  AIR_UNUSED(argv);
  AIR_UNUSED(me);

  fprintf(stdout, "\n");
  sprintf(buff, "--- unu: Utah Nrrd Utilities command-line interface ---");
  sprintf(fmt, "%%%ds\n",
          (int)((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stdout, fmt, buff);
  sprintf(buff, "(Teem version %s, %s)",
          airTeemVersion, airTeemReleaseDate);
  sprintf(fmt, "%%%ds\n",
          (int)((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
  fprintf(stdout, fmt, buff);
  fprintf(stdout, "\n");

  _hestPrintStr(stdout, 1, 0, 78, par1, AIR_FALSE);
  _hestPrintStr(stdout, 1, 0, 78, par2, AIR_FALSE);
  _hestPrintStr(stdout, 1, 0, 78, par3, AIR_FALSE);
  _hestPrintStr(stdout, 1, 0, 78, par4, AIR_FALSE);
  _hestPrintStr(stdout, 1, 0, 78, par5, AIR_FALSE);
  _hestPrintStr(stdout, 2, 0, 78, par6, AIR_FALSE);

  printf(" Formats available:");
  miss = AIR_FALSE;
  for (form=nrrdFormatTypeUnknown+1; form<nrrdFormatTypeLast; form++) {
    if (nrrdFormatArray[form]->available()) {
      printf(" %s", airEnumStr(nrrdFormatType, form));
    } else {
      miss = AIR_TRUE;
    }
  }
  printf("\n");
  if (miss) {
    printf("   (not available:");
    for (enc=nrrdFormatTypeUnknown+1; enc<nrrdFormatTypeLast; enc++) {
      if (!nrrdFormatArray[enc]->available()) {
        printf(" %s", airEnumStr(nrrdFormatType, enc));
      }
    }
    printf(")\n");
  }

  printf(" Nrrd data encodings available:");
  miss = AIR_FALSE;
  for (enc=nrrdEncodingTypeUnknown+1; enc<nrrdEncodingTypeLast; enc++) {
    if (nrrdEncodingArray[enc]->available()) {
      printf(" %s", airEnumStr(nrrdEncodingType, enc));
    } else {
      miss = AIR_TRUE;
    }
  }
  printf("\n");
  if (miss) {
    printf("   (not available:");
    for (enc=nrrdEncodingTypeUnknown+1; enc<nrrdEncodingTypeLast; enc++) {
      if (!nrrdEncodingArray[enc]->available()) {
        printf(" %s", airEnumStr(nrrdEncodingType, enc));
      }
    }
    printf(")\n");
  }

  printf("\n");
  /* NOTE: this is an exceptional unu command that doesn't rely on
     privateUnrrdu.h USAGE() macro; so we determine our own return value */
  return 0;
}

UNRRDU_CMD(about, INFO);
