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

#ifndef DEFT_VOLUME_INCLUDED
#define DEFT_VOLUME_INCLUDED

#include "Deft.H"

namespace Deft {

// maybe time-varying volumes can be handled here...
class DEFT_EXPORT Volume {
public: 
  explicit Volume(const gageKind *kind, const Nrrd *nrrd); // don't own
  explicit Volume(const gageKind *kind, Nrrd *nrrd, bool own);

  inline const gageKind *kind() const { return _kind; }
  inline const Nrrd *nrrd() const { return _nrrd; }
  
  inline bool own() const { return (_nrrdOwn ? true : false); }

  ~Volume();
private:
  const gageKind *_kind; // never owned
  const Nrrd *_nrrd;     // cannot nrrdNuke()
  Nrrd *_nrrdOwn;        //   can  nrrdNuke()
};

} /* namespace Deft */

#endif /* DEFT_VOLUME_INCLUDED */
