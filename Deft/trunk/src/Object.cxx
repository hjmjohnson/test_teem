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

#include "Deft.h"
#include "Object.h"

/* learned: this doesn't make things to faster
**   glEnable(GL_CULL_FACE);
**   glCullFace(GL_FRONT);
** it actually made things a tiny bit slower
*/

namespace Deft {

const unsigned int displayListDrawCount = 8;

Object::Object() {
  // char me[]="Object::Object";

  _staticDrawCount = 0;
  _compiling = false;
  _displayList = 0;
  _wireframe = false;
  _twoSided = false;
  _visible = true;
  _colorUse = true;
  _normalsUse = true;
  _normalizeUse = true;
  _compilingUse = true;
  _lightingUse = true;
  _transformUse = true;
  ELL_4M_IDENTITY_SET(_transform);
  _postDrawCallback = NULL;
  _postDrawCallbackData = NULL;
}

Object::~Object() {

  // virtual destructors are called after the subclass's desctructor, yes?
  if (_displayList) {
    glDeleteLists(_displayList, 1);
    _displayList = 0;
  }
}

void
Object::changed() {

  _staticDrawCount = 0;
}

void
Object::draw() {
  char me[]="Object::draw";
  double time0;

  // fprintf(stderr, "!%s(%p): hello, visible = %s\n", me, this,
  //         _visible ? "true" : "false");
  if (_visible) {
    time0 = airTime();
    if (_staticDrawCount == 0 && _displayList) {
      // display list is stale, kill it
      // fprintf(stderr, "%s: nixing stale display list %u\n", me,
      // _displayList);
      glDeleteLists(_displayList, 1);
      _displayList = 0;
    } else if (_staticDrawCount >= displayListDrawCount + airRandInt(10)
               && _compilingUse
               && !_displayList) {
      this->compile();
    }
    if (_displayList) {
      glCallList(_displayList);
    } else {
      this->drawImmediate();
      _staticDrawCount++;
    }
    _drawTime = airTime() - time0;
    if (_postDrawCallback) {
      _postDrawCallback(this, _postDrawCallbackData);
    }
  } else {
    _drawTime = 0;
  }
  // GLenum errCode;
  // glFinish(); glFlush(); glFinish(); glFlush();
  // while ((errCode = glGetError()) != GL_NO_ERROR) {
  //   fprintf(stderr, "\n%s: !!! post object OpenGL Error: %s\n\n", me,
  //           gluErrorString(errCode));
  // }
  // fprintf(stderr, "!%s(%p): bye, visible = %s\n", me, this,
  //         _visible ? "true" : "false");
}

unsigned int
Object::compileDisplayList() {
  char me[]="Object::compileDisplayList";
  GLenum errCode;

  unsigned int dlid = glGenLists(1);
  if (dlid) {
    // fprintf(stderr, "%s: compiling %u ... ", me, dlid); fflush(stderr);
    glNewList(dlid, GL_COMPILE);
    _compiling = true;
    this->drawImmediate();
    _compiling = false;
    glEndList();
    // fprintf(stderr, "done\n");
    while ((errCode = glGetError()) != GL_NO_ERROR) {
      fprintf(stderr, "\n%s: !!! OpenGL Error: %s\n\n", me,
              gluErrorString(errCode));
    }
  }
  // else perhaps gl drawing context not ready or something?
  return dlid;
}

void
Object::compile() {
  
  _displayList = compileDisplayList();
}

void
Object::wireframe(bool wf) {
  // char me[]="Object::wireframe";

  if (_wireframe != wf) {
    _wireframe = wf;
    this->changed();
  }
}

void
Object::flatShading(bool fs) {

  if (_flatShading != fs) {
    _flatShading = fs;
    this->changed();
  }
}

void
Object::twoSided(bool ts) {
  // char me[]="Object::twoSided";

  if (_twoSided != ts) {
    _twoSided = ts;
    this->changed();
  }
}

void
Object::visible(bool vis) { 
  _visible = vis;
}

void
Object::colorUse(bool col) { 
  if (_colorUse != col) {
    _colorUse = col;
    this->changed();
  }
}

void
Object::normalsUse(bool col) { 
  if (_normalsUse != col) {
    _normalsUse = col;
    this->changed();
  }
}

void
Object::normalizeUse(bool norm) {
  if (_normalizeUse != norm) {
    _normalizeUse = norm;
    this->changed();
  }
}

void
Object::compilingUse(bool comp) {
  if (_compilingUse != comp) {
    _compilingUse = comp;
    // changed?
  }
}

void
Object::lightingUse(bool shad) {
  if (_lightingUse != shad) {
    _lightingUse = shad;
    this->changed();
  }
}

void
Object::transformUse(bool tran) {
  if (_transformUse != tran) {
    _transformUse = tran;
    this->changed();
  }
}

void
Object::transformSet(const float mat[16]) {
  if (mat) {
    ELL_4M_COPY(_transform, mat);
    this->changed();
  } else {
    ELL_4M_IDENTITY_SET(_transform);
    this->changed();
  }
}

void
Object::transformGet(float mat[16]) const {
  if (mat) {
    ELL_4M_COPY(mat, _transform);
  }
}

void
Object::postDrawCallback(Callback *cb, void *data) {

  if (cb) {
    _postDrawCallback = cb;
    _postDrawCallbackData = data;
  } else {
    _postDrawCallback = NULL;
    _postDrawCallbackData = NULL;
  }  
}

}
