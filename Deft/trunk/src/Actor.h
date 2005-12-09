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

#ifndef DEFT_ACTOR_INCLUDED
#define DEFT_ACTOR_INCLUDED

#include "Deft.h"

namespace Deft {

// NOTE: currently, the Actor *owns* the limnObject and limnPolyData
class DEFT_EXPORT Actor {
public:
  explicit Actor(limnObject *lobj);
  explicit Actor(limnPolyData *lpld);
  ~Actor();

  // draw this actor
  void draw();
  unsigned int compile();

  // find bounding box for this actor
  void boundsGet(float min[3], float max[3]);

  // set new object
  // void objectSet(limnObject *obj);
  // void trisurfSet(limnPolyData *pld);

  limnObject *lobj;
  limnPolyData *lpld;
  bool wireFrame, visible, useColor, useNormalize, useActorT;
  double actorT[16];
  void changed();
private:
  bool compilingDisplayList;
  unsigned int staticDrawCount;
  unsigned int displayList;
};

}

#endif /* DEFT_ACTOR_INCLUDED */

