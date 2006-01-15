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

#ifndef DEFT_SCENE_INCLUDED
#define DEFT_SCENE_INCLUDED

#include "Deft.h"
#include "Actor.h"
#include "Object.h"
#include "Group.h"

namespace Deft {

class DEFT_EXPORT Scene {
public:
  // constructor/destructor
  explicit Scene();
  ~Scene();

  // set/get background color
  void bgColor(float R, float G, float B);
  float bgColorR();
  float bgColorG();
  float bgColorB();

  // set/get ambient light
  void ambLight(float R, float G, float B);
  float ambLightR();
  float ambLightG();
  float ambLightB();
  // update (viewspace) lights
  void lightUpdate(limnCamera *cam);

  // set objects in scene
  int actorAdd(Actor *actor);
  Actor *actorRemove(int actorIdx);

  int objectAdd(Object *obj);
  Object *objectRemove(int objectIdx);

  int groupAdd(Group *obj);
  Group *groupRemove(int groupIdx);

  // draw the scene
  void draw();
  double drawTime() const { return _drawTime; }
  double totalTime() const { return _totalTime; }

  // find bounding box for everything in scene
  void boundsGet(float min[3], float max[3]) const;

private:
  Actor **actor;
  airArray *actorArr;
  Object **object;
  airArray *objectArr;
  Group **group;
  airArray *groupArr;
  float _bgColor[3];
  double _lastTime, _totalTime, _drawTime;
  limnLight *lit;
};

}

#endif /* DEFT_SCENE_INCLUDED */
