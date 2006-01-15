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

#ifndef DEFT_VALUES_INCLUDED
#define DEFT_VALUES_INCLUDED

#include "Deft.h"
#include "Gage.h"

namespace Deft {

class DEFT_EXPORT Values {
public: 
  explicit Values(size_t size, const gageKind *kind, std::vector<int> item);
  ~Values();

  // "size": the number of values stored here
  // "length": the length of vector values; # components per value
  // Does the "size" vs. "length" distinction make any sense? 
  // No, it does not.
  size_t size() const { return _size1; }
  size_t length() const { return _size0; }

  // TERRIBLE: exposing the underlying data for writing (or
  // possibly reading) values, which is really very much 
  // not "const" in spirit...
  // 0 is returned when there is no actual underlying nrrd
  double *data() const { 
    return static_cast<double*>(_nvalues ? _nvalues->data : 0);
  }

  // exposing things for read-only
  const Nrrd *nrrd() const { return _nvalues; }
  const gageKind *kind() const { return _kind; }
  const std::vector<int> item() const { return _item; }

  void save();
private:
  size_t _size0, _size1;  // sizes of _nvalues
  const gageKind *_kind;
  std::vector<int> _item;

  // the storage of the values themselves
  Nrrd *_nvalues;
};

} /* namespace Deft */

#endif /* DEFT_VALUES_INCLUDED */
