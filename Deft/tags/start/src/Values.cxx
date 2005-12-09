/*
  Deft: experiments in minimalist scientific visualization 
  Copyright (C) 2005  Gordon Kindlmann

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

#include "Values.H"

namespace Deft {

Values::Values(size_t size1, const gageKind *kind, std::vector<int> item) {
  char me[]="Values::Values", labelBuff[AIR_STRLEN_MED], *err;

  _size1 = size1;
  _kind = kind;
  _item = item;
  _size0 = 0;
  sprintf(labelBuff, "gage(%s:", kind->name);
  for (unsigned int ii=0; ii<item.size(); ii++) {
    _size0 += gageKindAnswerLength(kind, item[ii]);
    if (ii) {
      strcat(labelBuff, ",");
    }
    strcat(labelBuff, airEnumStr(kind->enm, item[ii]));
  }
  strcat(labelBuff, ")");
  
  // fprintf(stderr, "%s: labelBuff = |%s|\n", me, labelBuff);
  if  (_size0) {
    _nvalues = nrrdNew();
    if (nrrdMaybeAlloc_va(_nvalues, nrrdTypeDouble, 2, _size0, _size1)) {
      fprintf(stderr, "%s: trouble:\n%s", me, err=biffGetDone(NRRD));
      exit(1);
    }
    _nvalues->axis[0].label = airStrdup(labelBuff);
  } else {
    _nvalues = NULL;
  }
  // fprintf(stderr, "%s: data at %p\n", me, _nvalues->data);
  return;
}

Values::~Values() {

  if (_nvalues) {
    nrrdNuke(_nvalues);
  }
}

void
Values::save() {
  char fname[AIR_STRLEN_MED];

  if (_nvalues) {
    strcpy(fname, "values");
    for (unsigned int ii=0; ii<_item.size(); ii++) {
      strcat(fname, "-");
      strcat(fname, airEnumStr(_kind->enm, _item[ii]));
    }
    strcat(fname, ".nrrd");
    nrrdSave(fname, _nvalues, NULL);
  }
}

} /* namespace Deft */
