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

#include "nrrd.h"
#include "privateNrrd.h"

#define MAGIC_P6 "P6"
#define MAGIC_P5 "P5"
#define MAGIC_P3 "P3"
#define MAGIC_P2 "P2"

int
_nrrdFormatPNM_available(void) {
  
  return AIR_TRUE;
}

int
_nrrdFormatPNM_nameLooksLike(const char *filename) {
  
  return (airEndsWith(filename, NRRD_EXT_PGM)
	  || airEndsWith(filename, NRRD_EXT_PPM));
}

int
_nrrdFormatPNM_fitsInto(const Nrrd *nrrd, const NrrdEncoding *encoding,
			int useBiff) {
  char me[]="_nrrdFormatPNM_fitsInto", err[AIR_STRLEN_MED];
  int ret;
  
  if (!( nrrd && encoding )) {
    sprintf(err, "%s: got NULL nrrd (%p) or encoding (%p)",
	    me, nrrd, encoding);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  if (nrrdTypeUChar != nrrd->type) {
    sprintf(err, "%s: type must be %s (not %s)", me,
	    airEnumStr(nrrdType, nrrdTypeUChar),
	    airEnumStr(nrrdType, nrrd->type));
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  if (!( nrrdEncodingRaw == encoding || nrrdEncodingAscii == encoding)) {
    sprintf(err, "%s: encoding can only be %s or %s", me,
	    nrrdEncodingRaw->name, nrrdEncodingAscii->name);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  if (2 == nrrd->dim) {
    /* its a gray-scale image */
    ret = 2;
  } else if (3 == nrrd->dim) {
    if (1 == nrrd->axis[0].size) {
      /* its a faux-3D image, really grayscale */
      ret = 2;
    } else if (3 == nrrd->axis[0].size) {
      /* its a real color image */
      ret = 3;
    } else {
      /* else its no good */
      sprintf(err, "%s: dim is 3, but 1st axis size is %d, not 1 or 3",
	      me, nrrd->axis[0].size);
      biffMaybeAdd(NRRD, err, useBiff); 
      return AIR_FALSE;
    }
  } else {
    sprintf(err, "%s: dimension is %d, not 2 or 3", me, nrrd->dim);
    biffMaybeAdd(NRRD, err, useBiff); 
    return AIR_FALSE;
  }
  return ret;
}

int
_nrrdFormatPNM_contentStartsLike(NrrdIO *nio) {
  
  return (!strcmp(MAGIC_P6, nio->line)
	  || !strcmp(MAGIC_P5, nio->line)
	  || !strcmp(MAGIC_P3, nio->line)
	  || !strcmp(MAGIC_P2, nio->line));
}

int
_nrrdFormatPNM_read(FILE *file, Nrrd *nrrd, NrrdIO *nio) {
  char me[]="_nrrdFormatPNM_read", err[AIR_STRLEN_MED], *perr;
  const char *fs;
  int i, color, got, want, len, ret, val[5], sx, sy, max, magic;

  if (!_nrrdFormatPNM_contentStartsLike(nio)) {
    sprintf(err, "%s: this doesn't look like a %s file", me, 
	    nrrdFormatPNM->name);
    biffAdd(NRRD, err); return 1;
  }
  nrrd->type = nrrdTypeUChar;
  if (!strcmp(MAGIC_P6, nio->line)) {
    magic = 6;
  } else if (!strcmp(MAGIC_P5, nio->line)) {
    magic = 5;
  } else if (!strcmp(MAGIC_P3, nio->line)) {
    magic = 3;
  } else if (!strcmp(MAGIC_P2, nio->line)) {
    magic = 2;
  } else {
    fprintf(stderr, "%s: PANIC: magic \"%s\" not handled\n", me, nio->line);
    biffAdd(NRRD, err); return 1;
  }

  switch(magic) {
  case 2:
    color = AIR_FALSE;
    nio->encoding = nrrdEncodingAscii;
    nrrd->dim = 2;
    break;
  case 3:
    color = AIR_TRUE;
    nio->encoding = nrrdEncodingAscii;
    nrrd->dim = 3;
    break;
  case 5:
    color = AIR_FALSE;
    nio->encoding = nrrdEncodingRaw;
    nrrd->dim = 2;
    break;
  case 6:
    color = AIR_TRUE;
    nio->encoding = nrrdEncodingRaw;
    nrrd->dim = 3;
    break;
  default:
    sprintf(err, "%s: sorry, PNM magic %d not implemented", me, magic);
    biffAdd(NRRD, err); return 1;
    break;
  }
  val[0] = val[1] = val[2] = 0;
  got = 0;
  want = 3;
  while (got < want) {
    nio->pos = 0;
    if (_nrrdOneLine(&len, nio, file)) {
      sprintf(err, "%s: failed to get line from PNM header", me);
      biffAdd(NRRD, err); return 1;
    }
    if (!(0 < len)) {
      sprintf(err, "%s: hit EOF in header with %d of %d ints parsed",
	      me, got, want);
      biffAdd(NRRD, err); return 1;
    }
    if ('#' == nio->line[0]) {
      if (strncmp(nio->line, NRRD_PNM_COMMENT, strlen(NRRD_PNM_COMMENT))) {
	/* this is a generic comment */
	ret = 0;
	goto plain;
      }
      /* else this PNM comment is trying to tell us something */
      nio->pos = strlen(NRRD_PNM_COMMENT);
      nio->pos += strspn(nio->line + nio->pos, _nrrdFieldSep);
      ret = _nrrdReadNrrdParseField(nrrd, nio, AIR_FALSE);
      if (!ret) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: unparsable field \"%s\" --> plain comment)\n",
		  me, nio->line);
	}
	goto plain;
      }
      if (nrrdField_comment == ret) {
	ret = 0;
	goto plain;
      }
      fs = airEnumStr(nrrdField, ret);
      if (!_nrrdFieldValidInImage[ret]) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: field \"%s\" (not allowed in PNM) "
		  "--> plain comment)\n", me, fs);
	}
	ret = 0;
	goto plain;
      }
      if (!nio->seen[ret] 
	  && _nrrdReadNrrdParseInfo[ret](nrrd, nio, AIR_TRUE)) {
	perr = biffGetDone(NRRD);
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: unparsable info for field \"%s\" "
		  "--> plain comment:\n%s)\n", me, fs, perr);
	}
	free(perr);
	ret = 0;
	goto plain;
      }
      nio->seen[ret] = AIR_TRUE;
    plain:
      if (!ret) {
	if (nrrdCommentAdd(nrrd, nio->line+1)) {
	  sprintf(err, "%s: couldn't add comment", me);
	  biffAdd(NRRD, err); return 1;
	}
      }
      continue;
    }
    
    if (3 == sscanf(nio->line, "%d%d%d", val+got+0, val+got+1, val+got+2)) {
      got += 3;
      continue;
    }
    if (2 == sscanf(nio->line, "%d%d", val+got+0, val+got+1)) {
      got += 2;
      continue;
    }
    if (1 == sscanf(nio->line, "%d", val+got+0)) {
      got += 1;
      continue;
    }

    /* else, we couldn't parse ANY numbers on this line, which is okay
       as long as the line contains nothing but white space */
    for (i=0; i<=strlen(nio->line)-1 && isspace(nio->line[i]); i++)
      ;
    if (i != strlen(nio->line)) {
      sprintf(err, "%s: \"%s\" has no integers but isn't just whitespace", 
	      me, nio->line);
      biffAdd(NRRD, err); return 1;
    }
  }
  /* this assumes 3 == want */
  sx = val[0];
  sy = val[1];
  max = val[2];
  if (!(sx > 0 && sy > 0 && max > 0)) {
    sprintf(err, "%s: sx,sy,max of %d,%d,%d has problem", me, sx, sy, max);
    biffAdd(NRRD, err); return 1;
  }
  if (255 != max) {
    sprintf(err, "%s: sorry, can only deal with max value 255 (not %d)", 
	    me, max);
    biffAdd(NRRD, err); return 1;
  }

  /* we know what we need in order to set nrrd fields and read data */
  if (color) {
    nrrdAxisInfoSet(nrrd, nrrdAxisInfoSize, 3, sx, sy);
  } else {
    nrrdAxisInfoSet(nrrd, nrrdAxisInfoSize, sx, sy);
  }
  nio->dataFile = file;
  if (!nio->skipData) {
    if (nio->encoding->read(nrrd, nio)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); return 1;
    }
  } else {
    nrrd->data = NULL;
  }

  return 0;
}

int
_nrrdFormatPNM_write(FILE *file, const Nrrd *_nrrd, NrrdIO *nio) {
  char me[]="_nrrdFormatPNM_write", err[AIR_STRLEN_MED];
  int i, color, sx, sy, magic;
  Nrrd *nrrd;
  airArray *mop;
  
  mop = airMopNew();
  airMopAdd(mop, nrrd = nrrdNew(), (airMopper)nrrdNuke, airMopAlways);
  if (nrrdCopy(nrrd, _nrrd)) {
    sprintf(err, "%s: couldn't make private copy", me);
    biffAdd(NRRD, err); airMopError(mop); return 1;
  }
  if (3 == nrrd->dim && 1 == nrrd->axis[0].size) {
    if (nrrdAxesDelete(nrrd, nrrd, 0)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
  }
  color = (3 == nrrd->dim);
  if (!color) {
    magic = (nrrdEncodingAscii == nio->encoding ? 2 : 5);
    sx = nrrd->axis[0].size;
    sy = nrrd->axis[1].size;
  } else {
    magic = (nrrdEncodingAscii == nio->encoding ? 3 : 6);
    sx = nrrd->axis[1].size;
    sy = nrrd->axis[2].size;
  }
  
  fprintf(file, "P%d\n", magic);
  fprintf(file, "%d %d\n", sx, sy);
  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (_nrrdFieldValidInImage[i] && _nrrdFieldInteresting(nrrd, nio, i)) {
      _nrrdFprintFieldInfo(file, NRRD_PNM_COMMENT, nrrd, nio, i);
    }
  }
  for (i=0; i<nrrd->cmtArr->len; i++) {
    fprintf(file, "# %s\n", nrrd->cmt[i]);
  }
  fprintf(file, "255\n");

  nio->dataFile = file;
  if (!nio->skipData) {
    if (nio->encoding->write(nrrd, nio)) {
      sprintf(err, "%s:", me);
      biffAdd(NRRD, err); airMopError(mop); return 1;
    }
  }
  
  airMopError(mop); 
  return 0;
}

const NrrdFormat
_nrrdFormatPNM = {
  "PNM",
  AIR_TRUE,   /* isImage */
  AIR_TRUE,   /* readable */
  AIR_FALSE,  /* usesDIO */
  _nrrdFormatPNM_available,
  _nrrdFormatPNM_nameLooksLike,
  _nrrdFormatPNM_fitsInto,
  _nrrdFormatPNM_contentStartsLike,
  _nrrdFormatPNM_read,
  _nrrdFormatPNM_write
};

const NrrdFormat *const
nrrdFormatPNM = &_nrrdFormatPNM;
