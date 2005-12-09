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

#ifndef DEFT_GROUP_INCLUDED
#define DEFT_GROUP_INCLUDED

#include "Deft.H"
#include "Object.H"

namespace Deft {

/*
** the idea is to have the thinnest possible wrapper around a
** list of Objects; e.g. this doesn't even have its own
** visible or wireframe flags
*/
class DEFT_EXPORT Group {
public: 
  explicit Group(unsigned int objNum);     // WRONG
  virtual ~Group();
  
  Object **object;   // array of objNum Object pointers, WRONG
  unsigned int objectNum() const;

  void draw();
  void boundsGet(float min[3], float max[3]) const;

  double drawTime() const;

  // Setting and getting various booleans
  void visible(bool);
  bool visible() const;
  void wireframe(bool);
  bool wireframe() const;

private:
  double _drawTime;
  unsigned int _objectNum;  // WRONG
};

} /* namespace Deft */

#endif /* DEFT_GROUP_INCLUDED */
