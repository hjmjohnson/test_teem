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

#ifndef DEFT_PLANE_INCLUDED
#define DEFT_PLANE_INCLUDED

#include "Deft.h"
#include "PolyProbe.h"

/* excessive, actually */
#define DEFT_PLANE_FLAG_NUM 80

namespace Deft {

class DEFT_EXPORT Plane : public PolyProbe {
public: 
  explicit Plane(unsigned int resU, unsigned int resV);
  ~Plane();

  void resolution(unsigned int resU, unsigned int resV);
  unsigned int resolutionU();
  unsigned int resolutionV();

  void origin(float X, float Y, float Z);
  void originSet(const float orig[3]);
  void originGet(float orig[3]);

  void edgeU(float X, float Y, float Z);
  void edgeUSet(const float vec[3]);
  void edgeUGet(float vec[3]);

  void edgeV(float X, float Y, float Z);
  void edgeVSet(const float vec[3]);
  void edgeVGet(float vec[3]);
  
  // returns false when update was a no-up
  bool update();

private:
  bool _flag[DEFT_PLANE_FLAG_NUM];
  unsigned int _resolutionU;
  unsigned int _resolutionV;
  float _origin[3], _edgeU[3], _edgeV[3];
};


} /* namespace Deft */

#endif /* DEFT_PLANE_INCLUDED */
