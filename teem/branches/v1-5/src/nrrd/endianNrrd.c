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

void
_nrrdSwap16Endian(void *_data, size_t N) {
  short *data, s, fix;
  size_t I;
  
  if (_data) {
    data = (short *)_data;
    for (I=0; I<N; I++) {
      s = data[I];
      fix =  (s & 0x00FF);
      fix = ((s & 0xFF00) >> 0x08) | (fix << 0x08);
      data[I] = fix;
    }
  }
}

void
_nrrdSwap32Endian(void *_data, size_t N) {
  int *data, w, fix;
  size_t I;

  if (_data) {
    data = (int *)_data;
    for (I=0; I<N; I++) {
      w = data[I];
      fix =  (w & 0x000000FF);
      fix = ((w & 0x0000FF00) >> 0x08) | (fix << 0x08);
      fix = ((w & 0x00FF0000) >> 0x10) | (fix << 0x08);
      fix = ((w & 0xFF000000) >> 0x18) | (fix << 0x08);
      data[I] = fix;
    }
  }
}

void
_nrrdSwap64Endian(void *_data, size_t N) {
  airLLong *data, l, fix;
  size_t I;

  if (_data) {
    data = (airLLong *)_data;
    for (I=0; I<N; I++) {
      l = data[I];
      fix =  (l & 0x00000000000000FF);
      fix = ((l & 0x000000000000FF00) >> 0x08) | (fix << 0x08);
      fix = ((l & 0x0000000000FF0000) >> 0x10) | (fix << 0x08);
      fix = ((l & 0x00000000FF000000) >> 0x18) | (fix << 0x08);
#if defined(_WIN32)
      fix = ((l & 0x000000FF00000000i64) >> 0x20) | (fix << 0x08);
      fix = ((l & 0x0000FF0000000000i64) >> 0x28) | (fix << 0x08);
      fix = ((l & 0x00FF000000000000i64) >> 0x30) | (fix << 0x08);
      fix = ((l & 0xFF00000000000000i64) >> 0x38) | (fix << 0x08);
#else
      fix = ((l & 0x000000FF00000000LL) >> 0x20) | (fix << 0x08);
      fix = ((l & 0x0000FF0000000000LL) >> 0x28) | (fix << 0x08);
      fix = ((l & 0x00FF000000000000LL) >> 0x30) | (fix << 0x08);
      fix = ((l & 0xFF00000000000000LL) >> 0x38) | (fix << 0x08);
#endif
      data[I] = fix;
    }
  }
}

void
_nrrdNoopEndian(void *_data, size_t N) {
  
}

void
_nrrdBlockEndian(void *_data, size_t N) {
  char me[]="_nrrdBlockEndian";
  
  fprintf(stderr, "%s: WARNING: can't fix endiannes of nrrd type %s\n", me,
	  airEnumStr(nrrdType, nrrdTypeBlock));
}

void
(*_nrrdSwapEndian[])(void *, size_t) = {
  _nrrdNoopEndian,         /*  0: nobody knows! */
  _nrrdNoopEndian,         /*  1:   signed 1-byte integer */
  _nrrdNoopEndian,         /*  2: unsigned 1-byte integer */
  _nrrdSwap16Endian,       /*  3:   signed 2-byte integer */
  _nrrdSwap16Endian,       /*  4: unsigned 2-byte integer */
  _nrrdSwap32Endian,       /*  5:   signed 4-byte integer */
  _nrrdSwap32Endian,       /*  6: unsigned 4-byte integer */
  _nrrdSwap64Endian,       /*  7:   signed 8-byte integer */
  _nrrdSwap64Endian,       /*  8: unsigned 8-byte integer */
  _nrrdSwap32Endian,       /*  9:          4-byte floating point */
  _nrrdSwap64Endian,       /* 10:          8-byte floating point */
  _nrrdBlockEndian         /* 11: size user defined at run time */
};

void
nrrdSwapEndian(Nrrd *nrrd) {
  
  if (nrrd 
      && nrrd->data 
      && !airEnumValCheck(nrrdType, nrrd->type)) {
    _nrrdSwapEndian[nrrd->type](nrrd->data, nrrdElementNumber(nrrd));
  }
  return;
}
