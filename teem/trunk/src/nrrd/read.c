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


#include "nrrd.h"
#include "private.h"

/*
  #include <sys/types.h>
  #include <unistd.h>
*/

char _nrrdRelDirFlag[] = "./";
char _nrrdFieldSep[] = " \t";
char _nrrdTableSep[] = " ,\t";

/*
** _nrrdValidHeader
**
** consistency checks on relationship between fields of nrrd
** 
** ALSO, it is here that we do the courtesy setting of the
** "num" field (to the product of the axes' sizes)
*/
int
_nrrdValidHeader(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdValidHeader", err[NRRD_STRLEN_MED];
  int i;

  for (i=1; i<=NRRD_FIELD_MAX; i++) {
    if (!_nrrdFieldRequired[i])
      continue;
    if (!io->seen[i]) {
      sprintf(err, "%s: didn't see required field: %s",
	      me, nrrdEnumValToStr(nrrdEnumField, i));
      biffAdd(NRRD, err); return 0;
    }
  }
  if (nrrdTypeBlock == nrrd->type && 0 == nrrd->blockSize) {
    sprintf(err, "%s: type is %s, but missing field: %s", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock),
	    nrrdEnumValToStr(nrrdEnumField, nrrdField_block_size));
    biffAdd(NRRD, err); return 0;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 0;
  }
  if (airEndianUnknown == io->endian &&
      nrrdEncodingEndianMatters[io->encoding] &&
      1 != nrrdElementSize(nrrd)) {
    sprintf(err, "%s: type (%s) and encoding (%s) require %s info", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrd->type),
	    nrrdEnumValToStr(nrrdEnumEncoding, io->encoding),
	    nrrdEnumValToStr(nrrdEnumField, nrrdField_endian));
    biffAdd(NRRD, err); return 0;    
  }
  
  /* we don't really try to enforce anything with the min/max/center/size
     information on each axis, because we only really care that we know
     each axis size.  Past that, if the user messes it up, its not really
     our problem ... */

  return 1;
}

int
_nrrdCalloc(Nrrd *nrrd) {
  char me[]="_nrrdCalloc", err[NRRD_STRLEN_MED];
  nrrdBigInt num;

  nrrd->data = airFree(nrrd->data);
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  nrrd->data = calloc(num, nrrdElementSize(nrrd));
  if (!nrrd->data) {
    sprintf(err, "%s: couldn't calloc(" NRRD_BIG_INT_PRINTF
	    ", %d)", me, num, nrrdElementSize(nrrd));
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

int
_nrrdReadDataRaw(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadDataRaw", err[NRRD_STRLEN_MED];
  nrrdBigInt num, bsize;
  size_t size, ret, dio;
  
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  printf("!%s: nrrd->axis[0].size = %d\n", me, nrrd->axis[0].size);
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  printf("!%s: num = " NRRD_BIG_INT_PRINTF "\n", me, num);
  printf("!%s: element size = %d\n", me, nrrdElementSize(nrrd));
  bsize = num * nrrdElementSize(nrrd);
  size = bsize;
  printf("!%s: bsize = " NRRD_BIG_INT_PRINTF "\n", me, bsize);
  printf("!%s: size = " NRRD_BIG_INT_PRINTF "\n", me, (nrrdBigInt)size);
  if (size != bsize) {
    fprintf(stderr,
	    "%s: PANIC: \"size_t\" can't represent byte-size of data.\n", me);
    exit(1);
  }

  if (_nrrdFormatUsesDIO[io->format]) {
    dio = airDioTest(size, io->dataFile, NULL);
  }
  else {
    dio = airNoDio_format;
  }
  if (airNoDio_okay == dio) {
    if (_nrrdFormatUsesDIO[io->format]) {
      if (3 <= nrrdStateVerboseIO) {
	fprintf(stderr, "with direct I/O ");
      }
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "... ");
	fflush(stderr);
      }
    }
    /* airDioRead includes the memory allocation */
    ret = airDioRead(io->dataFile, &(nrrd->data), size);
    if (size != ret) {
      sprintf(err, "%s: airDioRead() failed", me);
      biffAdd(NRRD, err); return 1;
    }
  }
  else {
    if (_nrrdCalloc(nrrd)) {
      sprintf(err, "%s: trouble", me); biffAdd(NRRD, err); return 1;
    }
    if (AIR_DIO && _nrrdFormatUsesDIO[io->format]) {
      if (3 <= nrrdStateVerboseIO) {
	fprintf(stderr, "with fread()");
	if (4 <= nrrdStateVerboseIO) {
	  fprintf(stderr, " (why no DIO: %s)", airNoDioErr(dio));
	}
      }
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "... ");
	fflush(stderr);
      }
    }
    ret = fread(nrrd->data, nrrdElementSize(nrrd), num, io->dataFile);
    if (ret != num) {
      sprintf(err, "%s: fread() returned " NRRD_BIG_INT_PRINTF
	      " (not " NRRD_BIG_INT_PRINTF ")", me,
	      (nrrdBigInt)ret, num);
      biffAdd(NRRD, err); return 1;
    }
  }

  if (airEndianUnknown != io->endian) {
    /* we positively know the endianness of data just read */
    if (io->endian != AIR_ENDIAN && 1 < nrrdElementSize(nrrd)) {
      /* the endiannesses of the data and the architecture are different,
	 and, the size of the data elements is bigger than a byte */
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: fixing endianness ... ", me);
	fflush(stderr);
      }
      nrrdSwapEndian(nrrd);
      if (2 <= nrrdStateVerboseIO) {
	fprintf(stderr, "done)");
	fflush(stderr);
      }
    }
  }
  
  return 0;
}

int
_nrrdReadDataAscii(Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadDataAscii", err[NRRD_STRLEN_MED],
    numStr[NRRD_STRLEN_LINE];
  nrrdBigInt I, num;
  char *data;
  int size, tmp;
  
  if (nrrdTypeBlock == nrrd->type) {
    sprintf(err, "%s: can't read nrrd type %s from ascii", me,
	    nrrdEnumValToStr(nrrdEnumType, nrrdTypeBlock));
    biffAdd(NRRD, err); return 1;
  }
  num = nrrdElementNumber(nrrd);
  if (!num) {
    sprintf(err, "%s: calculated number of elements to be zero!", me);
    biffAdd(NRRD, err); return 1;
  }
  /* this shouldn't actually be necessary ... */
  if (!nrrdElementSize(nrrd)) {
    sprintf(err, "%s: nrrd reports zero element size!", me);
    biffAdd(NRRD, err); return 1;
  }
  
  if (_nrrdCalloc(nrrd)) {
    sprintf(err, "%s: trouble", me); biffAdd(NRRD, err); return 1;
  }
  data = nrrd->data;
  size = nrrdElementSize(nrrd);
  for (I=0; I<=num-1; I++) {
    if (1 != fscanf(io->dataFile, "%s", numStr)) {
      sprintf(err, "%s: couldn't parse element " NRRD_BIG_INT_PRINTF
	      " of "NRRD_BIG_INT_PRINTF, me, I+1, num);
      biffAdd(NRRD, err); return 1;
    }
    if (nrrd->type >= nrrdTypeInt) {
      /* sscanf supports putting value directly into this type */
      if (1 != airSingleSscanf(numStr, nrrdTypeConv[nrrd->type], 
			       (void*)(data + I*size))) {
	sprintf(err, "%s: couln't parse %s "NRRD_BIG_INT_PRINTF
		" of " NRRD_BIG_INT_PRINTF " (\"%s\")", me,
		nrrdEnumValToStr(nrrdEnumType, nrrd->type),
		I+1, num, numStr);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      /* sscanf value into an int first */
      if (1 != airSingleSscanf(numStr, "%d", &tmp)) {
	sprintf(err, "%s: couln't parse element "NRRD_BIG_INT_PRINTF
		" of " NRRD_BIG_INT_PRINTF  " (\"%s\")",
		me, I+1, num, numStr);
	biffAdd(NRRD, err); return 1;
      }
      nrrdIInsert[nrrd->type](data, I, tmp);
    }
  }
  
  return 0;
}

/*
** The data readers are responsible for memory allocation.
** This is necessitated by the memory restrictions of direct I/O
*/
int
(*_nrrdReadData[NRRD_ENCODING_MAX+1])(Nrrd *, nrrdIO *) = {
  NULL,
  _nrrdReadDataRaw,
  _nrrdReadDataAscii
};

int
_nrrdReadNrrd(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadNrrd", err[NRRD_STRLEN_LINE+NRRD_STRLEN_MED];
  int i, skipRet, ret, len;

  /* parse header lines */
  while (1) {
    io->pos = 0;
    len = airOneLine(file, io->line, NRRD_STRLEN_LINE);
    if (len > 1) {
      ret = _nrrdReadNrrdParseField(nrrd, io, AIR_TRUE);
      if (!ret) {
	sprintf(err, "%s: trouble parsing field in \"%s\"", me, io->line);
	biffAdd(NRRD, err); return 1;
      }
      /* the comment is the one field allowed more than once */
      if (ret != nrrdField_comment && io->seen[ret]) {
	sprintf(err, "%s: already set field %s", me, 
		nrrdEnumValToStr(nrrdEnumField, ret));
	biffAdd(NRRD, err); return 1;
      }
      if (_nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_TRUE)) {
	sprintf(err, "%s: trouble parsing %s info \"%s\"", me,
		nrrdEnumValToStr(nrrdEnumField, ret), io->line+io->pos);
	biffAdd(NRRD, err); return 1;
      }
      io->seen[ret] = AIR_TRUE;
    }
    else {
      /* len <= 1 */
      break;
    }
  }

  if (!len                        /* we're at EOF ... */
      && !io->seperateHeader) {   /* but there's supposed to be data here! */
    sprintf(err, "%s: hit end of header, but no \"%s\" given", me,
	    nrrdEnumValToStr(nrrdEnumField, nrrdField_data_file));
    biffAdd(NRRD, err); return 1;
  }
  
  if (!_nrrdValidHeader(nrrd, io)) {
    sprintf(err, "%s: %s", me, 
	    (len ? "finished reading header, but there were problems"
	     : "hit EOF before seeing a complete header"));
    biffAdd(NRRD, err); return 1;
  }
  
  /* we seemed to have read a valid header; now read the data */
  if (!io->seperateHeader) {
    io->dataFile = file;
  }
  if (2 <= nrrdStateVerboseIO) {
    fprintf(stderr, "(%s: reading %s data ", me, 
	    nrrdEnumValToStr(nrrdEnumEncoding, io->encoding));
    fflush(stderr);
  }

  for (i=1; i<=io->lineSkip; i++) {
    skipRet = airOneLine(io->dataFile, io->line, NRRD_STRLEN_LINE);
    if (!skipRet) {
      sprintf(err, "%s: hit EOF skipping line %d of %d", me, i, io->lineSkip);
      biffAdd(NRRD, err); return 1;
    }
  }
  for (i=1; i<=io->byteSkip; i++) {
    skipRet = fgetc(io->dataFile);
    if (EOF == skipRet) {
      sprintf(err, "%s: hit EOF skipping byte %d of %d", me, i, io->byteSkip);
      biffAdd(NRRD, err); return 1;
    }
  }

  if (_nrrdReadData[io->encoding](nrrd, io)) {
    if (2 <= nrrdStateVerboseIO) {
      fprintf(stderr, "error!\n");
    }
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  if (2 <= nrrdStateVerboseIO) {
    fprintf(stderr, "done)\n");
  }
  if (io->seperateHeader) {
    io->dataFile = airFclose(io->dataFile);
  }
  else {
    /* put things back the way we found them */
    io->dataFile = NULL;
  }
  
  return 0;
}

int
_nrrdReadPNM(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadPNM", err[NRRD_STRLEN_MED], *fs;
  int i, color, got, want, len, ret, val[5], sx, sy, max;
  
  nrrd->type = nrrdTypeUChar;
  switch(io->magic) {
  case nrrdMagicP2:
    color = AIR_FALSE;
    io->encoding = nrrdEncodingAscii;
    nrrd->dim = 2;
    break;
  case nrrdMagicP3:
    color = AIR_TRUE;
    io->encoding = nrrdEncodingAscii;
    nrrd->dim = 3;
    break;
  case nrrdMagicP5:
    color = AIR_FALSE;
    io->encoding = nrrdEncodingRaw;
    nrrd->dim = 2;
    break;
  case nrrdMagicP6:
    color = AIR_TRUE;
    io->encoding = nrrdEncodingRaw;
    nrrd->dim = 3;
    break;
  default:
    sprintf(err, "%s: sorry, magic %d not implemented", me, io->magic);
    biffAdd(NRRD, err); return 1;
    break;
  }
  val[0] = val[1] = val[2] = 0;
  got = 0;
  want = 3;
  while (got < want) {
    io->pos = 0;
    len = airOneLine(file, io->line, NRRD_STRLEN_LINE);
    if (!(0 < len)) {
      sprintf(err, "%s: line read failed, got %d of %d ints from header",
	      me, got, want);
      biffAdd(NRRD, err); return 1;
    }
    if ('#' == io->line[0]) {
      if (strncmp(io->line, NRRD_PNM_COMMENT, strlen(NRRD_PNM_COMMENT))) {
	/* this is a generic comment */
	ret = 0;
	goto plain;
      }
      /* else this PNM comment is trying to tell us something */
      io->pos = strlen(NRRD_PNM_COMMENT);
      io->pos += strspn(io->line + io->pos, _nrrdFieldSep);
      ret = _nrrdReadNrrdParseField(nrrd, io, AIR_FALSE);
      if (!ret) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: unparsable field \"%s\" --> plain comment)\n",
		  me, io->line);
	}
	goto plain;
      }
      if (nrrdField_comment == ret) {
	ret = 0;
	goto plain;
      }
      fs = nrrdEnumValToStr(nrrdEnumField, ret);
      if (!_nrrdFieldValidInPNM[ret]) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr, "(%s: field \"%s\" (not allowed in PNM) "
		  "--> plain comment)\n", me, fs);
	}
	ret = 0;
	goto plain;
      }
      if (!io->seen[ret] 
	  && _nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_FALSE)) {
	if (nrrdStateVerboseIO) {
	  fprintf(stderr,
		  "(%s: unparsable info for field \"%s\" --> plain comment)\n",
		  me, fs);
	}
	ret = 0;
	goto plain;
      }
      io->seen[ret] = AIR_TRUE;
    plain:
      if (!ret) {
	if (nrrdCommentAdd(nrrd, io->line+1)) {
	  sprintf(err, "%s: couldn't add comment", me);
	  biffAdd(NRRD, err); return 1;
	}
      }
      continue;
    }
    
    if (3 == sscanf(io->line, "%d%d%d", val+got+0, val+got+1, val+got+2)) {
      got += 3;
      continue;
    }
    if (2 == sscanf(io->line, "%d%d", val+got+0, val+got+1)) {
      got += 2;
      continue;
    }
    if (1 == sscanf(io->line, "%d", val+got+0)) {
      got += 1;
      continue;
    }

    /* else, we couldn't parse ANY numbers on this line, which is okay
       as long as the line contains nothing but white space */
    for (i=0; i<=strlen(io->line)-1 && isspace(io->line[i]); i++)
      ;
    if (i != strlen(io->line)) {
      sprintf(err, "%s: \"%s\" has no integers but isn't just whitespace", 
	      me, io->line);
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
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, 3, sx, sy);
  }
  else {
    nrrdAxesSet(nrrd, nrrdAxesInfoSize, sx, sy);
  }
  io->dataFile = file;
  if (_nrrdReadData[io->encoding](nrrd, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  
  return 0;
}

int
_nrrdReadTable(FILE *file, Nrrd *nrrd, nrrdIO *io) {
  char me[]="_nrrdReadTable", err[NRRD_STRLEN_MED], *fs;
  int line, len, ret, sx, sy;
  airArray *flArr, *alArr;
  float *fl, *al, oneFloat;

  /* we only get here with the first line already in io->line */
  line = 1;
  len = strlen(io->line);
  
  /* first, we get through comments */
  while (_NRRD_COMMENT_CHAR == io->line[0]) {
    io->pos = 1;
    io->pos += strspn(io->line + io->pos, _nrrdFieldSep);
    ret = _nrrdReadNrrdParseField(nrrd, io, AIR_FALSE);
    /* could we parse anything? */
    if (!ret) {
      /* being unable to parse a comment as a nrrd field is not 
	 any kind of error */
      goto plain;
    }
    if (nrrdField_comment == ret) {
      ret = 0;
      goto plain;
    }
    fs = nrrdEnumValToStr(nrrdEnumField, ret);
    if (!_nrrdFieldValidInTable[ret]) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: field \"%s\" (not allowed in table) "
		"--> plain comment)\n", me, fs);
      }
      ret = 0;
      goto plain;
    }
    if (!io->seen[ret]
	&& _nrrdReadNrrdParseInfo[ret](nrrd, io, AIR_FALSE)) {
      if (nrrdStateVerboseIO) {
	fprintf(stderr, "(%s: malformed field \"%s\" --> plain comment)\n",
		me, fs);
      }
      ret = 0;
      goto plain;
    }
    io->seen[ret] = AIR_TRUE;
  plain:
    if (!ret) {
      if (nrrdCommentAdd(nrrd, io->line + 1)) {
	sprintf(err, "%s: couldn't add comment", me);
	biffAdd(NRRD, err); return 1;
      }
    }
    len = airOneLine(file, io->line, NRRD_STRLEN_LINE);
    if (!len) {
      sprintf(err, "%s: hit EOF before any numbers parsed", me);
      biffAdd(NRRD, err); return 1;
    }
    line++;
  }

  /* we supposedly have a line of numbers, see how many there are */
  if (!airParseStrF(&oneFloat, io->line, _nrrdTableSep, 1)) {
    sprintf(err, "%s: couldn't parse any numbers on line %d", me, line);
    biffAdd(NRRD, err); return 1;
  }
  flArr = airArrayNew((void**)&fl, NULL, sizeof(float), _NRRD_TABLE_INCR);
  if (!flArr) {
    sprintf(err, "%s: couldn't create array for first line values", me);
    biffAdd(NRRD, err); return 1;
  }
  for (sx=1; 1; sx++) {
    /* there is obviously a limit to the number of numbers that can 
       be parsed from a single finite line of input text */
    if (airArraySetLen(flArr, sx)) {
      sprintf(err, "%s: couldn't alloc space for %d values", me, sx);
      biffAdd(NRRD, err); return 1;
    }
    if (sx > airParseStrF(fl, io->line, _nrrdTableSep, sx)) {
      /* We asked for sx ints and got less.  We know that we successfully
	 got one value, so we did succeed in parsing sx-1 values */
      sx--;
      break;
    }
  }
  flArr = airArrayNuke(flArr);
  
  /* now see how many more lines there are */
  alArr = airArrayNew((void**)&al, NULL, sx*sizeof(float), _NRRD_TABLE_INCR);
  if (!alArr) {
    sprintf(err, "%s: couldn't create data buffer", me);
    biffAdd(NRRD, err); return 1;
  }
  sy = 0;
  while (len) {
    if (-1 == airArrayIncrLen(alArr, 1)) {
      sprintf(err, "%s: couldn't create scanline of %d values", me, sx);
      biffAdd(NRRD, err); return 1;
    }
    ret = airParseStrF(al + sy*sx, io->line, _nrrdTableSep, sx);
    if (sx > ret) {
      sprintf(err, "%s: could only parse %d values (not %d) on line %d",
	      me, ret, sx, line);
      biffAdd(NRRD, err); return 1;
    }
    sy++;
    line++;
    len = airOneLine(file, io->line, NRRD_STRLEN_LINE);
  }

  nrrd->axis[0].size = sx;
  nrrd->axis[1].size = sy;
  if (nrrdAlloc(nrrd, sx*sy, nrrdTypeFloat, 2)) {
    sprintf(err, "%s: couldn't allocate table data", me);
    biffAdd(NRRD, err); return 1;
  }
  memcpy(nrrd->data, al, sx*sy*sizeof(float));
  
  alArr = airArrayNuke(alArr);
  
  return 0;
}

/*
******** nrrdRead()
**
** read in nrrd from a given file.  We don't yet know the format or
** anythign else about how the data is written.  The only use for the
** nrrdIO struct "_io" here is to contain the base directory in which
** we'll look for a seperate data file, if that's desired.  Currently
** no other communication to us is done via the nrrdIO, so _io can be
** passed as NULL (if no header-relative datafiles are desired), and a
** nrrdIO will be created as needed.
*/
int
nrrdRead(Nrrd *nrrd, FILE *file, nrrdIO *_io) {
  char err[NRRD_STRLEN_MED], me[] = "nrrdRead";
  int len;
  float oneFloat;
  nrrdIO *io;

  if (!(file && nrrd)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (_io) {
    io = _io;
  }
  else {
    io = nrrdIONew();
    if (!io) {
      sprintf(err, "%s: couldn't alloc I/O struct", me);
      biffAdd(NRRD, err); return 1;
    }
  }

  /* clear out anything in the given nrrd */
  nrrdInit(nrrd);

  len = airOneLine(file, io->line, NRRD_STRLEN_LINE);
  if (!len) {
    sprintf(err, "%s: immediately hit EOF", me);
    biffAdd(NRRD, err); return 1;
  }
  
  /* we have one line, see if there's magic, or comments, or numbers */
  io->magic = nrrdEnumStrToVal(nrrdEnumMagic, io->line);
  switch (io->magic) {
  case nrrdMagicOldNRRD:
  case nrrdMagicNRRD0001:
    io->format = nrrdFormatNRRD;
     if (_nrrdReadNrrd(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading NRRD", me);
      biffAdd(NRRD, err); return 1;
    }
    break;
  case nrrdMagicP2:
  case nrrdMagicP3:
  case nrrdMagicP5:
  case nrrdMagicP6:
    io->format = nrrdFormatPNM;
    if (_nrrdReadPNM(file, nrrd, io)) {
      sprintf(err, "%s: trouble reading PNM", me);
      biffAdd(NRRD, err); return 1;
    }
    break;
  default:
    /* see if line is a comment, which implies its a table */
    if (_NRRD_COMMENT_CHAR == io->line[0] 
	|| airParseStrF(&oneFloat, io->line, _nrrdTableSep, 1)) {
      io->format = nrrdFormatTable;
      if (_nrrdReadTable(file, nrrd, io)) {
	sprintf(err, "%s: trouble reading table", me);
	biffAdd(NRRD, err); return 1;
      }
    }
    else {
      /* there's no hope */
      sprintf(err, "%s: couldn't parse any format (NRRD, PNM, or table)", me);
      biffAdd(NRRD, err); return 1;
    }
    break;
  }

  if (!_io) {
    io = nrrdIONix(io);
  }
  else {
    /* reset the given nrrdIO so that it can be used again */
    nrrdIOReset(_io);
  }

  return 0;
}

/*
** _nrrdSplitName()
**
** slits a file name into a path and a base filename.  The directory
** seperator is assumed to be '/'.  The division between the path
** and the base is the last '/' in the file name.  The path is
** everything prior to this, and base is everythign after (so the
** base does NOT start wiht '/').  If there is not a '/' in the name,
** or if a '/' appears as the last character, then the path is set to
** ".", and the name is copied into base.
*/
int
_nrrdSplitName(char *path, char *base, char *name) {
  int i, ret;
  
  i = strrchr(name, '/') - name;
  /* we found a valid break if the last directory character
     is somewhere in the string except the last character */
  if (i>=0 && i<strlen(name)-1) {
    strcpy(path, name);
    path[i] = 0;
    strcpy(base, name + i + 1);
    /*
    printf("_nrrdSplitName: path = |%s|\n", path);
    printf("_nrrdSplitName: base = |%s|\n", base);
    */
    ret = 1;
  }
  else {
    /* if the name had no slash, its in the current directory, which
       means that we need to explicitly store "." as the header
       directory in case we have header-relative data. */
    strcpy(path, ".");
    strcpy(base, name);
    ret = 0;
  }
  return ret;
}

int
nrrdLoad(Nrrd *nrrd, char *filename) {
  char me[]="nrrdLoad", err[NRRD_STRLEN_MED];
  nrrdIO *io;
  FILE *file;

  if (!(nrrd && filename)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  io = nrrdIONew();
  if (!io) {
    sprintf(err, "%s: couldn't alloc I/O struct", me);
    biffAdd(NRRD, err); return 1;
  }

  /* we save the directory of the filename given to us so
     that if it turns out that its just a nhdr file, with
     a header-relative data file, then we will know how
     to find the data file */
  _nrrdSplitName(io->dir, io->base, filename);
  /* printf("!%s: |%s|%s|\n", me, io->dir, io->base); */

  if (!strcmp("-", filename)) {
    file = stdin;
  }
  else {
    file = fopen(filename, "rb");
    if (!file) {
      sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s", 
	      me, filename, strerror(errno));
      biffAdd(NRRD, err); return 1;
    }
  }

  /* YOOHOO */
  io->endian = airEndianBig;
  /* YOOHOO */

  if (nrrdRead(nrrd, file, io)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }

  if (file != stdin)
    fclose(file);
  io = nrrdIONix(io);
  return 0;
}

