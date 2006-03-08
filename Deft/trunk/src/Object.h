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

/*
** learned: DO NOT forget the ";" at the end of a class declaration, or
** else you'll get totally cryptic: "expected unqualified-id before ..."
** compile errors...
*/

#ifndef DEFT_OBJECT_INCLUDED
#define DEFT_OBJECT_INCLUDED

#include "Deft.h"

namespace Deft {

extern const unsigned int displayListDrawCount;

class DEFT_EXPORT Object;

typedef void (Callback)(Object *, void *);

/*
******** class Object
**
** the superclass for all things that are drawn to the screen
*/
class DEFT_EXPORT Object {
public:
  explicit Object();
  virtual ~Object();

  // the draw() method handles display lists
  void draw();
  virtual void drawImmediate()=0;

  // for getting vertices out of things
  virtual unsigned int verticesGet(Nrrd *npos)=0;

  // For learning the axis-aligned bounding box
  virtual void boundsGet(float min[3], float max[3]) const =0;

  // To indicate that any cached displayList should be nixed.
  // It is the Object user's job to indicate when its contents have changed...
  void changed();

  // For when we're merely a vessel for compiling a display last; 
  // the display list ID is returned, and we do not "own" it
  unsigned int compileDisplayList();

  // Setting and getting various booleans
  void wireframe(bool);
  bool wireframe() const;
  void twoSided(bool);
  bool twoSided() const;
  void visible(bool);
  bool visible() const;
  void colorUse(bool);
  bool colorUse() const;
  void normalsUse(bool);
  bool normalsUse() const;
  void normalizeUse(bool);
  bool normalizeUse() const;
  void compilingUse(bool);
  bool compilingUse() const;
  void lightingUse(bool);
  bool lightingUse() const;
  void transformUse(bool);
  bool transformUse() const;

  // Setting and getting the transform
  void transformSet(const float mat[16]);
  void transformGet(float mat[16]) const;

  double drawTime() const { return _drawTime; }

  // callbacks
  void postDrawCallback(Callback *cb, void *data);
protected:
  // for compiling display list when we're speeding up drawing;
  // we do own the resulting display list
  void compile();
  bool _compiling;
  unsigned int _staticDrawCount;
  unsigned int _displayList;

  // internal booleans
  bool _wireframe, _twoSided, _visible, _colorUse,
    _normalsUse, _normalizeUse,
    _compilingUse, _lightingUse, _transformUse;

  // transform applied to whole object
  float _transform[16];

  // time elapsed during drawing
  double _drawTime;

  // callbacks
  Callback *_postDrawCallback;
  void *_postDrawCallbackData;
};

}

#endif /* DEFT_OBJECT_INCLUDED */
