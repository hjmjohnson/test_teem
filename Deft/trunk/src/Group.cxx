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

#include "Group.h"

namespace Deft {

Group::Group(unsigned int num) {
  
  object = AIR_CAST(Object **, calloc(num, sizeof(Object *)));
  _objectNum = num;
}

unsigned int Group::objectNum() const { return _objectNum; }

Group::~Group() {
  
  free(object);
}

void
Group::draw() {
  char me[]="Group::draw";

  double time0 = airTime();
  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    object[objIdx]->draw();
    GLenum errCode;
    while ((errCode = glGetError()) != GL_NO_ERROR) {
      fprintf(stderr, "\n%s: !!! post object (%p) %u OpenGL Error: %s\n\n", me,
              object[objIdx], objIdx, gluErrorString(errCode));
    }
  }
  _drawTime = airTime() - time0;
}

double Group::drawTime() const { return _drawTime; }

void
Group::boundsGet(float gmin[3], float gmax[3]) const {
  char me[]="Group::boundsGet";
  float min[3], max[3];
  bool starting = true;

  ELL_3V_SET(gmin, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(gmax, AIR_NAN, AIR_NAN, AIR_NAN);
  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    object[objIdx]->boundsGet(min, max);
    if (!( ELL_3V_EXISTS(min) && ELL_3V_EXISTS(max) )) {
      continue;
    }

    fprintf(stderr, "%s: obj[%u]  min=(%g,%g,%g)  max=(%g,%g,%g)\n", me,
            objIdx, min[0], min[1], min[2], max[0], max[1], max[2]);

    if (starting) {
      ELL_3V_COPY(gmin, min);
      ELL_3V_COPY(gmax, max);
      starting = false;
    } else {
      ELL_3V_MIN(gmin, gmin, min);
      ELL_3V_MAX(gmax, gmax, max);
    }
  }
  fprintf(stderr, "%s: final  min=(%g,%g,%g)  max=(%g,%g,%g)\n", me,
          min[0], min[1], min[2], max[0], max[1], max[2]);

}

void Group::visible(bool vis) { 

  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    object[objIdx]->visible(vis);
  }
}

bool Group::visible() const { 
  bool ret = false;

  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    ret |= object[objIdx]->visible();
  }
  return ret;
}

void
Group::wireframe(bool wf) {
  // char me[]="Object::wireframe";

  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    object[objIdx]->wireframe(wf);
  }
}

bool
Group::wireframe() const {
  bool ret = true;

  for (unsigned int objIdx=0; objIdx<_objectNum; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    ret &= object[objIdx]->wireframe();
  }
  return ret;
}

} /* namespace Deft */
