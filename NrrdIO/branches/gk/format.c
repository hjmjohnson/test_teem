/*
  NrrdIO: stand-alone code for basic nrrd functionality
  Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah
 
  These source files have been copied and/or modified from teem,
  Gordon Kindlmann's research software; <http://teem.sourceforge.net>.
  Teem is licensed under the GNU Lesser Public License. The
  non-copyleft licensing defined here applies to only the source files
  in the NrrdIO distribution (not the rest of teem), and only to the
  files originating with NrrdIO (not analogous files in teem).
 
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any
  damages arising from the use of this software.
 
  Permission is granted to anyone to use this software for any
  purpose, including commercial applications, and to alter it and
  redistribute it freely, subject to the following restrictions:
 
  1. The origin of this software must not be misrepresented; you must
     not claim that you wrote the original software. If you use this
     software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
 
  2. Altered source versions must be plainly marked as such, and must
     not be misrepresented as being the original software.
 
  3. This notice may not be removed or altered from any source distribution.
*/

#include "NrrdIO.h"
#include "privateNrrd.h"

/*
** what a NrrdFormat can assume:
** -- that nio->format has been set to you already
** -- for read(): that nio->path has been set to the path of the file being
**    read in, if the information was ever available
** -- for contentStartsLike() and read(): that nio->line contains the
**    first line of of the file, in order to determine the file type
**
** what a NrrdFormat has to do:
** -- respect nio->skipData to whatever extent makes sense on top of how the
**    NrrdEncoding respects it (by making read and write no-ops).  
**    nrrdFormatNRRD, for instance, won't create empty detached data files
**    if nio->skipData.
** -- determing what the NrrdEncoding to use, if there's a choice
** -- respect nrrdStateVerboseIO with messages to stderr, if possible
**
** The "unknown" format is intended as a template for writing new formats.
*/

int
_nrrdFormatUnknown_available(void) {

  /* insert code here */

  return AIR_FALSE;
}

int
_nrrdFormatUnknown_nameLooksLike(const char *filename) {
  
  /* insert code here */

  return AIR_FALSE;
}

int
_nrrdFormatUnknown_fitsInto(const Nrrd *nrrd, const NrrdEncoding *encoding,
			    int useBiff) {
  char me[]="_nrrdFormatUnknown_fitsInto", err[AIR_STRLEN_MED];
  
  if (!(nrrd && encoding)) {
    sprintf(err, "%s: got NULL nrrd (%p) or encoding (%p)",
	    me, nrrd, encoding);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }

  /* insert code here */

  return AIR_FALSE;
}

int
_nrrdFormatUnknown_contentStartsLike(NrrdIoState *nio) {
  
  /* insert code here */

  return AIR_FALSE;
}

int
_nrrdFormatUnknown_read(FILE *file, Nrrd *nrrd, NrrdIoState *nio) {
  char me[]="_nrrdFormatUnknown_read", err[AIR_STRLEN_MED];

  /* insert code here, and remove error handling below */

  sprintf(err, "%s: ERROR!!! trying to read unknown format", me);
  biffAdd(NRRD, err);
  return 1;
}

int
_nrrdFormatUnknown_write(FILE *file, const Nrrd *nrrd, NrrdIoState *nio) {
  char me[]="_nrrdFormatUnknown_write", err[AIR_STRLEN_MED];

  /* insert code here, and remove error handling below */

  sprintf(err, "%s: ERROR!!! trying to write unknown format", me);
  biffAdd(NRRD, err);
  return 1;
}

const NrrdFormat
_nrrdFormatUnknown = {
  "unknown",
  AIR_FALSE,  /* isImage */
  AIR_TRUE,   /* readable */
  AIR_FALSE,  /* usesDIO */
  _nrrdFormatUnknown_available,
  _nrrdFormatUnknown_nameLooksLike,
  _nrrdFormatUnknown_fitsInto,
  _nrrdFormatUnknown_contentStartsLike,
  _nrrdFormatUnknown_read,
  _nrrdFormatUnknown_write
};

const NrrdFormat *const
nrrdFormatUnknown = &_nrrdFormatUnknown;

const NrrdFormat *const
nrrdFormatArray[NRRD_FORMAT_TYPE_MAX+1] = {
  &_nrrdFormatUnknown,
  &_nrrdFormatNRRD,
  &_nrrdFormatPNM,
  &_nrrdFormatPNG,
  &_nrrdFormatVTK,
  &_nrrdFormatText,
  &_nrrdFormatEPS
};
