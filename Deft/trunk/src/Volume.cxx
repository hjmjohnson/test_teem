/*
  Deft: experiments in minimalist scientific visualization 
  Copyright (C) 2006, 2005  Gordon Kindlmann

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Volume.h"

namespace Deft {

Volume::Volume(const gageKind *kind, const Nrrd *nrrd) {
  char me[]="Volume::Volume.1", *err;

  if (gageKindVolumeCheck(kind, nrrd)) {
    fprintf(stderr, "%s: trouble:\n%s", me, err = biffGetDone(GAGE));
    free(err); exit(1);
  }
  _kind = kind;
  _nrrd = nrrd;
  _nrrdOwn = NULL;
  fprintf(stderr, "!%s: okay\n", me);
}

Volume::Volume(const gageKind *kind, Nrrd *nrrd, bool own) {
  char me[]="Volume::Volume.2", *err;

  if (gageKindVolumeCheck(kind, nrrd)) {
    fprintf(stderr, "%s: trouble:\n%s", me, err = biffGetDone(GAGE));
    free(err); exit(1);
  }
  _kind = kind;
  if (own) {
    _nrrd = NULL;
    _nrrdOwn = nrrd;
  } else {
    _nrrd = nrrd;
    _nrrdOwn = NULL;
  }
}

Volume::~Volume() {

  if (_nrrdOwn) {
    nrrdNuke(_nrrdOwn);
  }
}

} /* namespace Deft */
