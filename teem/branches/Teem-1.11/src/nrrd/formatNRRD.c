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

#include "nrrd.h"
#include "privateNrrd.h"

#define MAGIC "NRRD"
#define MAGIC0 "NRRD00.01"
#define MAGIC1 "NRRD0001"
#define MAGIC2 "NRRD0002"
#define MAGIC3 "NRRD0003"
#define MAGIC4 "NRRD0004"
#define MAGIC5 "NRRD0005"

const char *
_nrrdFormatURLLine0 = "Complete NRRD file format specification at:";
const char *
_nrrdFormatURLLine1 = "http://teem.sourceforge.net/nrrd/format.html";

void
nrrdIoStateDataFileIterBegin(NrrdIoState *nio) {

  nio->dataFNIndex = -1;
  return;
}

/* this macro suggested by Bryan Worthen */
/* if str = '-', strcmp() is 0, && short circuits, return false
** else str != '-'
** if str[1] = ':', its probably a windows full path, != is 0, return false
** else str[1] != ':'
** if str[0] = '/', its a normal full path, return false
*/
#define _NEED_PATH(str) (strcmp("-", (str)) \
                         && ':' != (str)[1] \
                         && '/' != (str)[0])

/*
** this is responsible for the header-relative path processing
**
** NOTE: if the filename is "-", then because it does not start with '/',
** it would normally be prefixed by nio->path, so it needs special handling
**
** NOTE: this should work okay with nio->headerStringRead, I think ...
*/
int
nrrdIoStateDataFileIterNext(FILE **fileP, NrrdIoState *nio, int reading) {
  static const char me[]="nrrdIoStateDataFileIterNext";
  char *fname=NULL;
  int ii, needPath;
  unsigned int num, fi;
  size_t maxl;
  airArray *mop;

  mop = airMopNew();
  airMopAdd(mop, (void*)fileP, (airMopper)airSetNull, airMopOnError);

  if (!fileP) {
    biffAddf(NRRD, "%s: got NULL pointer", me);
    airMopError(mop); return 1;
  }
  if (!_nrrdDataFNNumber(nio)) {
    biffAddf(NRRD, "%s: there appear to be zero datafiles!", me);
    airMopError(mop); return 1;
  }
  
  nio->dataFNIndex++;
  if (nio->dataFNIndex >= (int)_nrrdDataFNNumber(nio)) {
    /* there is no next data file, but we don't make that an error */
    nio->dataFNIndex = _nrrdDataFNNumber(nio);
    airMopOkay(mop);
    *fileP = NULL;
    return 0;
  }

  /* HEY: some of this error checking is done far more often than needed */
  if (nio->dataFNFormat || nio->dataFNArr->len) {
    needPath = AIR_FALSE;
    maxl = 0;
    if (nio->dataFNFormat) {
      needPath = _NEED_PATH(nio->dataFNFormat);
      /* assuming 10-digit integers is plenty big */
      maxl = 10 + strlen(nio->dataFNFormat);
    } else {
      for (fi=0; fi<nio->dataFNArr->len; fi++) {
        needPath |= _NEED_PATH(nio->dataFN[fi]);
        maxl = AIR_MAX(maxl, strlen(nio->dataFN[fi]));
      }
    }
    if (needPath && !airStrlen(nio->path)) {
      biffAddf(NRRD, "%s: need nio->path for header-relative datafiles", me);
      airMopError(mop); return 1;
    }
    fname = (char*)malloc(airStrlen(nio->path) + strlen("/") + maxl + 1);
    if (!fname) {
      biffAddf(NRRD, "%s: couldn't allocate filename buffer", me);
      airMopError(mop); return 1;
    }
    airMopAdd(mop, fname, airFree, airMopAlways);
  }

  if (nio->dataFNFormat) {
    /* ---------------------------------------------------------- */
    /* --------- base.%d <min> <max> <step> [<dim>] ------------- */
    /* ---------------------------------------------------------- */
    num = 0;
    for (ii = nio->dataFNMin; 
         ((nio->dataFNStep > 0 && ii <= nio->dataFNMax)
          || (nio->dataFNStep < 0 && ii >= nio->dataFNMax));
         ii += nio->dataFNStep) {
      if ((int)num == nio->dataFNIndex) {  /* HEY scrutinize cast */
        break;
      }
      num += 1;
    }
    if (_NEED_PATH(nio->dataFNFormat)) {
      strcpy(fname, nio->path);
      strcat(fname, "/");
      sprintf(fname + strlen(nio->path) + strlen("/"), nio->dataFNFormat, ii);
    } else {
      sprintf(fname, nio->dataFNFormat, ii);
    }
  } else if (nio->dataFNArr->len) {
    /* ---------------------------------------------------------- */
    /* ------------------- LIST or single ----------------------- */
    /* ---------------------------------------------------------- */
    if (_NEED_PATH(nio->dataFN[nio->dataFNIndex])) {
      sprintf(fname, "%s/%s", nio->path, nio->dataFN[nio->dataFNIndex]);
    } else {
      strcpy(fname, nio->dataFN[nio->dataFNIndex]);
    }
  }
  /* else data file is attached */
  
  if (nio->dataFNFormat || nio->dataFNArr->len) {
    *fileP = airFopen(fname, reading ? stdin : stdout, reading ? "rb" : "wb");
    if (!(*fileP)) {
      biffAddf(NRRD, "%s: couldn't open \"%s\" (data file %d of %d) for %s",
               me, fname, nio->dataFNIndex+1, (int)_nrrdDataFNNumber(nio),
               reading ? "reading" : "writing");
      airMopError(mop); return 1;
    }
  } else {
    /* data file is attached */
    if (nio->headerStringRead) {
      /* except we were never reading from a file to begin with, but this
         isn't an error */
      *fileP = NULL;
    } else {
      *fileP = nio->headerFile;
    }
  }
  
  airMopOkay(mop);
  return 0;
}

/* 
** we try to use the oldest format that will hold the nrrd 
*/
int
_nrrdFormatNRRD_whichVersion(const Nrrd *nrrd, NrrdIoState *nio) {
  int ret;

  if (_nrrdFieldInteresting(nrrd, nio, nrrdField_measurement_frame)) {
    ret = 5;
  } else if (_nrrdFieldInteresting(nrrd, nio, nrrdField_thicknesses)
             || _nrrdFieldInteresting(nrrd, nio, nrrdField_space)
             || _nrrdFieldInteresting(nrrd, nio, nrrdField_space_dimension)
             || _nrrdFieldInteresting(nrrd, nio, nrrdField_sample_units)
             || airStrlen(nio->dataFNFormat) || nio->dataFNArr->len > 1) {
    ret = 4;
  } else if (_nrrdFieldInteresting(nrrd, nio, nrrdField_kinds)) {
    ret = 3;
  } else if (nrrdKeyValueSize(nrrd)) {
    ret = 2;
  } else {
    ret = 1;
  }
  return ret;
}

int
_nrrdFormatNRRD_available(void) {
  
  return AIR_TRUE;
}

int
_nrrdFormatNRRD_nameLooksLike(const char *filename) {
  
  return (airEndsWith(filename, NRRD_EXT_NRRD)
          || airEndsWith(filename, NRRD_EXT_NHDR));
}

int
_nrrdFormatNRRD_fitsInto(const Nrrd *nrrd, const NrrdEncoding *encoding,
                         int useBiff) {
  static const char me[]="_nrrdFormatNRRD_fitsInto";

  if (!( nrrd && encoding )) {
    biffMaybeAddf(useBiff, NRRD, "%s: got NULL nrrd (%p) or encoding (%p)",
                  me, AIR_CAST(void*, nrrd), AIR_CAST(void*, encoding));
    return AIR_FALSE;
  }

  /* everything fits in a nrrd */
  return AIR_TRUE;
}

int
_nrrdFormatNRRD_contentStartsLike(NrrdIoState *nio) {
  
  return (!strcmp(MAGIC0, nio->line)
          || !strcmp(MAGIC1, nio->line)
          || !strcmp(MAGIC2, nio->line)
          || !strcmp(MAGIC3, nio->line)
          || !strcmp(MAGIC4, nio->line)
          || !strcmp(MAGIC5, nio->line)
          );
}

/*
** _nrrdHeaderCheck()
**
** minimal consistency checks on relationship between fields of nrrd,
** only to be used after the headers is parsed, and before the data is
** read, to make sure that information required for reading data is in
** fact known.
**
** NOTE: this is not the place to do the sort of checking done by 
** nrrdCheck(), because it includes I/O-specific stuff
**
*/
int
_nrrdHeaderCheck(Nrrd *nrrd, NrrdIoState *nio, int checkSeen) {
  static const char me[]="_nrrdHeaderCheck";
  int i;

  if (checkSeen) {
    for (i=1; i<=NRRD_FIELD_MAX; i++) {
      if (_nrrdFieldRequired[i] && !nio->seen[i]) {
        biffAddf(NRRD, "%s: didn't see required field: %s",
                 me, airEnumStr(nrrdField, i));
        return 1;
      }
    }
  }
  if (nrrdTypeBlock == nrrd->type && !nrrd->blockSize) {
    biffAddf(NRRD, "%s: type is %s, but missing field: %s", me,
             airEnumStr(nrrdType, nrrdTypeBlock),
             airEnumStr(nrrdField, nrrdField_block_size));
    return 1;
  }
  if (!nrrdElementSize(nrrd)) {
    biffAddf(NRRD, "%s: nrrd reports zero element size!", me);
    return 1;
  }
  /* _nrrdReadNrrdParse_sizes() checks axis[i].size, which completely
     determines the return of nrrdElementNumber() */
  if (airEndianUnknown == nio->endian
      && nio->encoding->endianMatters
      && 1 != nrrdElementSize(nrrd)) {
    biffAddf(NRRD, "%s: type (%s) and encoding (%s) require %s info", me,
             airEnumStr(nrrdType, nrrd->type),
             nio->encoding->name,
             airEnumStr(nrrdField, nrrdField_endian));
    return 1;    
  }

  /* we don't really try to enforce consistency with the
     min/max/center/size information on each axis, other than the
     value checking done by the _nrrdReadNrrdParse_* functions,
     because we only really care that we know each axis size.  Past
     that, if the user messes it up, its not really our problem ... */

  return 0;
}

/*
** NOTE: currently, this will read, without complaints or errors,
** newer NRRD format features from older NRRD files (as indicated by
** magic), such as key/value pairs from a NRRD0001 file, even though
** strictly speaking these are violations of the format.
**
** NOTE: by giving a NULL "file", you can make this function basically
** do the work of reading in datafiles, without any header parsing 
*/
int
_nrrdFormatNRRD_read(FILE *file, Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[]="_nrrdFormatNRRD_read";
  /* Dynamically allocated for space reasons. */
  /* MWC: These strlen usages look really unsafe. */
  int ret;
  unsigned int llen;
  size_t valsPerPiece;
  char *data;
  FILE *dataFile=NULL;

  /* record where the header is being read from for the sake of
     nrrdIoStateDataFileIterNext() */
  nio->headerFile = file;

  /* HEY: GLK forgets the context in which file might be reasonably NULL
     but on Fri Sep 23 09:48:41 EDT 2005 this was "if (file) { ..." */
  /* nio->headerStringRead is NULL whenever IO from string is not being done */
  if (file || nio->headerStringRead) {
    if (!_nrrdFormatNRRD_contentStartsLike(nio)) {
      biffAddf(NRRD, "%s: this doesn't look like a %s file", me,
               nrrdFormatNRRD->name);
      return 1;
    }
    /* parse all the header lines */
    do {
      nio->pos = 0;
      if (_nrrdOneLine(&llen, nio, file)) {
        biffAddf(NRRD, "%s: trouble getting line of header", me);
        return 1;
      }
      if (llen > 1) {
        ret = _nrrdReadNrrdParseField(nio, AIR_TRUE);
        if (!ret) {
          biffAddf(NRRD, "%s: trouble parsing field in \"%s\"",
                   me, nio->line);
          return 1;
        }
        /* comments and key/values are allowed multiple times */
        if (nio->seen[ret]
            && !(ret == nrrdField_comment || ret == nrrdField_keyvalue)) {
          biffAddf(NRRD, "%s: already set field %s", me, 
                   airEnumStr(nrrdField, ret));
          return 1;
        }
        if (nrrdFieldInfoParse[ret](file, nrrd, nio, AIR_TRUE)) {
          /* HEY: this error message should be printing out all the
             per-axis fields, not just the first
             HEY: if your stupid parsing functions didn't modify
             nio->line then you wouldn't have this problem ... */
          biffAddf(NRRD, "%s: trouble parsing %s info \"%s\"", me,
                   airEnumStr(nrrdField, ret), nio->line + nio->pos);
          return 1;
        }
        nio->seen[ret] = AIR_TRUE;
      }
    } while (llen > 1);
    /* either
       0 == llen: we're at EOF (or end of nio->headerStringRead), or
       1 == llen: we just read the empty line seperating header from data */
    if (0 == llen
        && !nio->headerStringRead
        && !nio->dataFNFormat
        && 0 == nio->dataFNArr->len) { 
      /* we're at EOF, we're not reading from a string, but there's
         apparently no seperate data file */
      biffAddf(NRRD, "%s: hit end of header, but no \"%s\" given", me,
               airEnumStr(nrrdField, nrrdField_data_file));
      return 1;
    }
  }
  if (_nrrdHeaderCheck(nrrd, nio, !!file)) {
    biffAddf(NRRD, "%s: %s", me, 
             (llen ? "finished reading header, but there were problems"
              : "hit EOF before seeing a complete valid header"));
    return 1;
  }

  /* we seemed to have read in a valid header; now allocate the memory.
     For directIO-compatible allocation we need to get the first datafile */
  nrrdIoStateDataFileIterBegin(nio);
  /* NOTE: if nio->headerStringRead, this may set dataFile to NULL */
  if (nrrdIoStateDataFileIterNext(&dataFile, nio, AIR_TRUE)) {
    biffAddf(NRRD, "%s: couldn't open the first datafile", me);
    return 1;
  }
  if (nio->skipData) {
    nrrd->data = NULL;
    data = NULL;
  } else {
    if (_nrrdCalloc(nrrd, nio, dataFile)) {
      biffAddf(NRRD, "%s: couldn't allocate memory for data", me);
      return 1;
    }
    data = (char*)nrrd->data;
  }

  /* iterate through datafiles and read them in */
  /* NOTE: you have to open dataFile even in the case of skipData, because
     caller might have set keepNrrdDataFileOpen, in which case you need to
     do any line or byte skipping if it is specified */
  valsPerPiece = nrrdElementNumber(nrrd)/_nrrdDataFNNumber(nio);
  while (dataFile) {
    /* ---------------- skip, if need be */
    if (nrrdLineSkip(dataFile, nio)) {
      biffAddf(NRRD, "%s: couldn't skip lines", me);
      return 1;
    }
    if (!nio->encoding->isCompression) {
      /* bytes are skipped here for non-compression encodings, but are
         skipped within the decompressed stream for compression encodings */
      if (nrrdByteSkip(dataFile, nrrd, nio)) {
        biffAddf(NRRD, "%s: couldn't skip bytes", me);
        return 1;
      }
    }
    /* ---------------- read the data itself */
    if (2 <= nrrdStateVerboseIO) {
      fprintf(stderr, "(%s: reading %s data ... ", me, nio->encoding->name);
      fflush(stderr);
    }
    if (!nio->skipData) {
      if (nio->encoding->read(dataFile, data, valsPerPiece, nrrd, nio)) {
        if (2 <= nrrdStateVerboseIO) {
          fprintf(stderr, "error!\n");
        }
        biffAddf(NRRD, "%s:", me);
        return 1;
      }
    }
    if (2 <= nrrdStateVerboseIO) {
      fprintf(stderr, "done)\n");
    }
    /* ---------------- go to next data file */
    if (nio->keepNrrdDataFileOpen && _nrrdDataFNNumber(nio) == 1) {
      nio->dataFile = dataFile;
    } else {
      if (dataFile != nio->headerFile) {
        dataFile = airFclose(dataFile);
      }
    }
    data += valsPerPiece*nrrdElementSize(nrrd);
    if (nrrdIoStateDataFileIterNext(&dataFile, nio, AIR_TRUE)) {
      biffAddf(NRRD, "%s: couldn't get the next datafile", me);
      return 1;
    }
  }

  if (airEndianUnknown != nio->endian && nrrd->data) {
    /* we positively know the endianness of data just read */
    if (1 < nrrdElementSize(nrrd)
        && nio->encoding->endianMatters
        && nio->endian != AIR_ENDIAN) {
      /* endianness exposed in encoding, and its wrong */
      if (2 <= nrrdStateVerboseIO) {
        fprintf(stderr, "(%s: fixing endianness ... ", me);
        fflush(stderr);
      }
      nrrdSwapEndian(nrrd);
      if (2 <= nrrdStateVerboseIO) {
        fprintf(stderr, "done)\n");
        fflush(stderr);
      }
    }
  }
    
  return 0;
}

int
_nrrdFormatNRRD_write(FILE *file, const Nrrd *nrrd, NrrdIoState *nio) {
  static const char me[]="_nrrdFormatNRRD_write"; 
  char strbuf[AIR_STRLEN_MED], *strptr, *tmp;
  int ii;
  unsigned int jj;
  airArray *mop;
  FILE *dataFile=NULL;
  size_t valsPerPiece;
  char *data;

  mop = airMopNew();

  if (!(file
        || nio->headerStringWrite
        || nio->learningHeaderStrlen)) {
    biffAddf(NRRD, "%s: have no file or string to write to, nor are "
             "learning header string length", me);
    airMopError(mop); return 1;
  }
  if (nrrdTypeBlock == nrrd->type && nrrdEncodingAscii == nio->encoding) {
    biffAddf(NRRD, "%s: can't write nrrd type %s with %s encoding", me,
             airEnumStr(nrrdType, nrrdTypeBlock),
             nrrdEncodingAscii->name);
    airMopError(mop); return 1;
  }

  /* record where the header is being written to for the sake of
     nrrdIoStateDataFileIterNext(). This may be NULL if
     nio->headerStringWrite is non-NULL */
  nio->headerFile = file;

  /* we have to make sure that the data filename information is set
     (if needed), so that it can be printed by _nrrdFprintFieldInfo */
  if (nio->detachedHeader 
      && !nio->dataFNFormat
      && 0 == nio->dataFNArr->len) {
    /* NOTE: this means someone requested a detached header, but we
       don't already have implicit (via dataFNFormat) or explicit
       (via dataFN[]) information about the data file */
    /* NOTE: whether or not nio->skipData, we have to contrive a filename to 
       say in the "data file" field, which is stored in nio->dataFN[0],
       because the data filename will be "interesting", according to
       _nrrdFieldInteresting() */
    /* NOTE: Fri Feb  4 01:42:20 EST 2005 the way this is now set up, having
       a name in dataFN[0] will trump the name implied by nio->{path,base},
       which is a useful way for the user to explicitly set the output
       data filename (as with unu make -od) */
    if (!( !!airStrlen(nio->path) && !!airStrlen(nio->base) )) {
      biffAddf(NRRD, "%s: can't create data file name: nio's "
               "path and base empty", me);
      airMopError(mop); return 1;
    }
    tmp = (char*)malloc(strlen(nio->base) 
                        + strlen(".")
                        + strlen(nio->encoding->suffix) + 1);
    if (!tmp) {
      biffAddf(NRRD, "%s: couldn't allocate data filename", me);
      airMopError(mop); return 1;
    }
    airMopAdd(mop, tmp, airFree, airMopOnError);
    sprintf(tmp, "%s.%s", nio->base, nio->encoding->suffix);
    jj = airArrayLenIncr(nio->dataFNArr, 1); /* HEY error checking */
    nio->dataFN[jj] = tmp;
  }

  /* the magic is in fact the first thing to be written */
  if (file) {
    fprintf(file, "%s%04d\n", MAGIC, _nrrdFormatNRRD_whichVersion(nrrd, nio));
  } else if (nio->headerStringWrite) {
    sprintf(nio->headerStringWrite, "%s%04d\n",
            MAGIC, _nrrdFormatNRRD_whichVersion(nrrd, nio));
  } else {
    nio->headerStrlen = strlen(MAGIC) + strlen("0000") + 1;
  }

  /* write the advertisement about where to get the file format */
  if (file) {
    fprintf(file, "# %s\n", _nrrdFormatURLLine0);
    fprintf(file, "# %s\n", _nrrdFormatURLLine1);
  } else if (nio->headerStringWrite) {
    sprintf(strbuf, "# %s\n", _nrrdFormatURLLine0);
    strcat(nio->headerStringWrite, strbuf);
    sprintf(strbuf, "# %s\n", _nrrdFormatURLLine1);
    strcat(nio->headerStringWrite, strbuf);
  } else {
    nio->headerStrlen += sprintf(strbuf, "# %s\n", _nrrdFormatURLLine0);
    nio->headerStrlen += sprintf(strbuf, "# %s\n", _nrrdFormatURLLine1);
  }

  /* this is where the majority of the header printing happens */
  for (ii=1; ii<=NRRD_FIELD_MAX; ii++) {
    if (_nrrdFieldInteresting(nrrd, nio, ii)) {
      if (file) {
        _nrrdFprintFieldInfo (file, "", nrrd, nio, ii);
      } else if (nio->headerStringWrite) {
        _nrrdSprintFieldInfo(&strptr, "", nrrd, nio, ii);
        if (strptr) {
          strcat(nio->headerStringWrite, strptr);
          strcat(nio->headerStringWrite, "\n");
          free(strptr);
          strptr = NULL;
        }
      } else {
        _nrrdSprintFieldInfo(&strptr, "", nrrd, nio, ii);
        if (strptr) {
          nio->headerStrlen += strlen(strptr);
          nio->headerStrlen += strlen("\n");
          free(strptr);
          strptr = NULL;
        }
      }
    }
  }

  /* comments and key/values handled differently */
  for (jj=0; jj<nrrd->cmtArr->len; jj++) {
    if (file) {
      fprintf(file, "%c %s\n", NRRD_COMMENT_CHAR, nrrd->cmt[jj]);
    } else if (nio->headerStringWrite) {
      strptr = (char*)malloc(1 + strlen(" ") 
                             + strlen(nrrd->cmt[jj]) + strlen("\n") + 1);
      sprintf(strptr, "%c %s\n", NRRD_COMMENT_CHAR, nrrd->cmt[jj]);
      strcat(nio->headerStringWrite, strptr);
      free(strptr);
      strptr = NULL;
    } else {
      nio->headerStrlen += (1 + strlen(" ") 
                            + strlen(nrrd->cmt[jj]) + strlen("\n") + 1);
    }
  }
  for (jj=0; jj<nrrd->kvpArr->len; jj++) {
    if (file) {
      _nrrdKeyValueWrite(file, NULL,
                         NULL, nrrd->kvp[0 + 2*jj], nrrd->kvp[1 + 2*jj]);
    } else if (nio->headerStringWrite) {
      _nrrdKeyValueWrite(NULL, &strptr,
                         NULL, nrrd->kvp[0 + 2*jj], nrrd->kvp[1 + 2*jj]);
      if (strptr) {
        strcat(nio->headerStringWrite, strptr);
        free(strptr);
        strptr = NULL;
      }
    } else {
      _nrrdKeyValueWrite(NULL, &strptr,
                         NULL, nrrd->kvp[0 + 2*jj], nrrd->kvp[1 + 2*jj]);
      if (strptr) {
        nio->headerStrlen += strlen(strptr);
        free(strptr);
        strptr = NULL;
      }
    }
  }

  if (file) {
    if (!( nio->detachedHeader || _nrrdDataFNNumber(nio) > 1 )) {
      fprintf(file, "\n");
    }
  }

  if (file && !nio->skipData) {
    nrrdIoStateDataFileIterBegin(nio);
    if (nrrdIoStateDataFileIterNext(&dataFile, nio, AIR_FALSE)) {
      biffAddf(NRRD, "%s: couldn't write the first datafile", me);
      airMopError(mop); return 1;
    }
    
    valsPerPiece = nrrdElementNumber(nrrd)/_nrrdDataFNNumber(nio);
    data = (char*)nrrd->data;
    do {
      /* ---------------- write data */
      if (2 <= nrrdStateVerboseIO) {
        fprintf(stderr, "(%s: writing %s data ", me, nio->encoding->name);
        fflush(stderr);
      }
      if (nio->encoding->write(dataFile, data, valsPerPiece, nrrd, nio)) {
        if (2 <= nrrdStateVerboseIO) {
          fprintf(stderr, "error!\n");
        }
        biffAddf(NRRD, "%s: couldn't write %s data", me, nio->encoding->name);
        airMopError(mop); return 1;
      }
      if (2 <= nrrdStateVerboseIO) {
        fprintf(stderr, "done)\n");
      }
      /* ---------------- go to next data file */
      if (dataFile != nio->headerFile) {
        dataFile = airFclose(dataFile);
      }
      data += valsPerPiece*nrrdElementSize(nrrd);
      if (nrrdIoStateDataFileIterNext(&dataFile, nio, AIR_TRUE)) {
        biffAddf(NRRD, "%s: couldn't get the next datafile", me);
        airMopError(mop); return 1;
      }
    } while (dataFile);
  }

  airMopOkay(mop); 
  return 0;
}

const NrrdFormat
_nrrdFormatNRRD = {
  "NRRD",
  AIR_FALSE,  /* isImage */
  AIR_TRUE,   /* readable */
  AIR_TRUE,   /* usesDIO */
  _nrrdFormatNRRD_available,
  _nrrdFormatNRRD_nameLooksLike,
  _nrrdFormatNRRD_fitsInto,
  _nrrdFormatNRRD_contentStartsLike,
  _nrrdFormatNRRD_read,
  _nrrdFormatNRRD_write
};

const NrrdFormat *const
nrrdFormatNRRD = &_nrrdFormatNRRD;
