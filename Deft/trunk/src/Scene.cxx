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
** learned: no huge speed difference between 
**  glShadeModel(GL_SMOOTH); and 
**  glShadeModel(GL_FLAT);
*/

#include "Deft.h"
#include "Scene.h"

namespace Deft {

Scene::Scene() {
  int arrayIncr = 10;

  ELL_3V_SET(_bgColor, 0, 0, 0);
  lit = limnLightNew();
  limnLightAmbientSet(lit, 1.0, 1.0, 1.0);
  limnLightSet(lit, 0,                /* which */
               AIR_TRUE,              /* in viewspace */
               0.95f, 0.95f, 0.95f,   /* r g b */
               -1.0f, -1.0f, -3.5f);  /* x y z */
  _lastTime = _totalTime = _drawTime = 0;
  _fog = false;
  objectArr = airArrayNew((void**)(&object), NULL,
                          sizeof(Object*), arrayIncr);
  groupArr = airArrayNew((void**)(&group), NULL,
                         sizeof(Group*), arrayIncr);
}

Scene::~Scene() {
  lit = limnLightNix(lit);
  airArrayNuke(objectArr);
}

void
Scene::bgColor(float R, float G, float B) {
  ELL_3V_SET(_bgColor, R, G, B);
  return;
}

void
Scene::ambLight(float R, float G, float B) {
  limnLightAmbientSet(lit, R, G, B);
  return;
}

void
Scene::lightUpdate(limnCamera *cam) {
  char me[]="Scene::lightUpdate", *err;

  if (limnLightUpdate(lit, cam)) {
    err = biffGetDone(LIMN);
    sprintf(err, "%s: trouble:\n%s", me, err);
    free(err); return;
  }
  return;
}

void
Scene::fog(bool f) {
  _fog = f;
  return;
}

int
Scene::objectAdd(Object *newObject) {
  char me[]="Scene::objectAdd";
  unsigned int objIdx;
  
  for (objIdx=0; objIdx<objectArr->len; objIdx++) {
    if (object[objIdx] == newObject) {
      fprintf(stderr, "%s: already have object in slot %d\n", me, objIdx);
      return -1;
    }
  }

  for (objIdx=0; objIdx<objectArr->len; objIdx++) {
    if (!object[objIdx]) {
      /* this object slot is unused */
      break;
    }
  }
  if (objIdx >= objectArr->len) {
    objIdx = airArrayLenIncr(objectArr, 1);
  }
  object[objIdx] = newObject;

  return objIdx; 
}

int
Scene::groupAdd(Group *newGroup) {
  char me[]="Scene::groupAdd";
  unsigned int grpIdx;
  
  for (grpIdx=0; grpIdx<groupArr->len; grpIdx++) {
    if (group[grpIdx] == newGroup) {
      fprintf(stderr, "%s: already have group in slot %d\n", me, grpIdx);
      return -1;
    }
  }

  for (grpIdx=0; grpIdx<groupArr->len; grpIdx++) {
    if (!group[grpIdx]) {
      /* this group slot is unused */
      break;
    }
  }
  if (grpIdx >= groupArr->len) {
    grpIdx = airArrayLenIncr(groupArr, 1);
  }
  group[grpIdx] = newGroup;

  return grpIdx; 
}

Object *
Scene::objectRemove(int objIdx) {
  Object *ret;
  
  ret = object[objIdx];
  object[objIdx] = NULL;
  return ret;
}

Group *
Scene::groupRemove(int grpIdx) {
  Group *ret;
  
  ret = group[grpIdx];
  group[grpIdx] = NULL;
  return ret;
}

void
Scene::draw() {
  char me[]="Deft::Scene::draw";
  unsigned int lightIdx;
  GLenum errCode;

  double time0 = airTime();

  glEnable(GL_DEPTH_TEST);

  // very crude test showed this wasn't a huge performance hit
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.5);

  // clear screen
  // HEY: glClearColor only needs to be called once
  glClearColor(bgColorR(), bgColorG(), bgColorB(), 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // set lights
  // this is weird: when using glColorMaterial to set material's
  // intrinsic color, we only get to set RGB-- we don't get to set
  // the equivalent of k_a and k_d separately.  So, those parameters
  // have to be built into the lights...
  glShadeModel(GL_SMOOTH);
  GLfloat tmpv[4] = {0,0,0,1};
  ELL_3V_SCALE(tmpv, 0.1f, lit->amb);
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, tmpv);
  for (lightIdx=0; lightIdx<8; lightIdx++) {
    if (lit->on[lightIdx]) {
      glEnable(GL_LIGHT0 + lightIdx);
      ELL_4V_COPY(tmpv, lit->dir[lightIdx]);
      // ELL_4V_SCALE(tmpv, -1, tmpv);
      glLightfv(GL_LIGHT0 + lightIdx, GL_POSITION, tmpv);
      ELL_3V_SET(tmpv, 0, 0, 0);
      glLightfv(GL_LIGHT0 + lightIdx, GL_AMBIENT, tmpv);
      /* ELL_3V_SCALE(tmpv, 0.8, lit->col[lightIdx]); */
      ELL_3V_SCALE(tmpv, 1.0f, lit->col[lightIdx]);
      glLightfv(GL_LIGHT0 + lightIdx, GL_DIFFUSE, tmpv);
      glLightfv(GL_LIGHT0 + lightIdx, GL_SPECULAR, lit->col[lightIdx]);
    } else {
      glDisable(GL_LIGHT0 + lightIdx);
    }
  }

  // set materials (until scene objects have their own materials)
  /* GLfloat mat_zero[] = {0.0f, 0.0f, 0.0f, 0.0f}; */
  /* YOOHOO changed for van gogh 
  GLfloat mat_ambient[] = {0.1f, 0.1f, 0.1f, 1.0f};
  // may be over-ridden with glColorMaterial 
  GLfloat mat_diffuse[] = {0.7f, 0.7f, 0.7f, 1.0f};
  GLfloat mat_specular[] = {0.3f, 0.3f, 0.3f, 1.0f};
  */
  GLfloat mat_ambient[] = {0.7f, 0.7f, 0.7f, 1.0f};
  // may be over-ridden with glColorMaterial 
  GLfloat mat_diffuse[] = {0.3f, 0.3f, 0.3f, 1.0f};
  GLfloat mat_specular[] = {0.0f, 0.0f, 0.0f, 1.0f};

  GLfloat mat_shininess[] = {70.0f};
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, mat_ambient);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, mat_diffuse);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);
  /*
  glMaterialfv(GL_BACK, GL_AMBIENT, mat_zero);
  glMaterialfv(GL_BACK, GL_DIFFUSE, mat_zero);
  glMaterialfv(GL_BACK, GL_SPECULAR, mat_zero);
  glMaterialfv(GL_BACK, GL_SHININESS, mat_zero);
  */

  for (unsigned int objIdx=0; objIdx<objectArr->len; objIdx++) {
    object[objIdx]->draw();
    // glFinish(); glFlush(); glFinish(); glFlush();
    // while ((errCode = glGetError()) != GL_NO_ERROR) {
    //   fprintf(stderr, "\n%s: !!! post object %u OpenGL Error: %s\n\n", me,
    //           objIdx, gluErrorString(errCode));
    // }
  }
  glDisable(GL_LIGHTING);
  for (unsigned int grpIdx=0; grpIdx<groupArr->len; grpIdx++) {
    group[grpIdx]->draw();
    // glFinish(); glFlush(); glFinish(); glFlush();
    // while ((errCode = glGetError()) != GL_NO_ERROR) {
    //   fprintf(stderr, "\n%s: !!! post group %u/%u OpenGL Error: %s\n\n", me,
    //           grpIdx, groupArr->len, gluErrorString(errCode));
    // }
  }
  double time1 = airTime();
  _drawTime = time1 - time0;
  _totalTime = time1 - _lastTime;
  _lastTime = time1;
  glFlush();
  while ((errCode = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "\n%s: !!! OpenGL Error: %s\n\n", me,
            gluErrorString(errCode));
  }
  /* does this do anything?
  glutSwapBuffers();
  */
}

void
Scene::boundsGet(float gmin[3], float gmax[3]) const {
  char me[]="Scene::boundsGet";
  float min[3], max[3];
  bool starting = true;

  ELL_3V_SET(gmin, AIR_NAN, AIR_NAN, AIR_NAN);
  ELL_3V_SET(gmax, AIR_NAN, AIR_NAN, AIR_NAN);
  for (unsigned int objIdx=0; objIdx<objectArr->len; objIdx++) {
    if (!object[objIdx]) {
      continue;
    }
    object[objIdx]->boundsGet(min, max);
    fprintf(stderr, "%s: object[%u]: (%g,%g,%g) (%g,%g,%g)\n", me, objIdx,
            min[0], min[1], min[2], max[0], max[1], max[2]);
    if (!( ELL_3V_EXISTS(min) && ELL_3V_EXISTS(max) )) {
      continue;
    }
    if (starting) {
      ELL_3V_COPY(gmin, min);
      ELL_3V_COPY(gmax, max);
      starting = false;
    } else {
      ELL_3V_MIN(gmin, gmin, min);
      ELL_3V_MAX(gmax, gmax, max);
    }
  }
  for (unsigned int grpIdx=0; grpIdx<groupArr->len; grpIdx++) {
    if (!group[grpIdx]) {
      continue;
    }
    group[grpIdx]->boundsGet(min, max);
    fprintf(stderr, "%s: group[%u]: (%g,%g,%g) (%g,%g,%g)\n", me, grpIdx,
            min[0], min[1], min[2], max[0], max[1], max[2]);
    if (!( ELL_3V_EXISTS(min) && ELL_3V_EXISTS(max) )) {
      continue;
    }
    if (starting) {
      ELL_3V_COPY(gmin, min);
      ELL_3V_COPY(gmax, max);
      starting = false;
    } else {
      ELL_3V_MIN(gmin, gmin, min);
      ELL_3V_MAX(gmax, gmax, max);
    }
  }

  fprintf(stderr, "%s: obj->boundsGet: (%g,%g,%g) (%g,%g,%g)\n", me, 
          min[0], min[1], min[2], max[0], max[1], max[2]);

  return;
}

} /* namespace Deft */
