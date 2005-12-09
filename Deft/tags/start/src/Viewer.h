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

#ifndef DEFT_VIEWER_INCLUDED
#define DEFT_VIEWER_INCLUDED

#include "Deft.H"
#include "Scene.H"

namespace Deft {

class DEFT_EXPORT Viewer;
typedef void (viewerCallback)(Viewer *, void *);

class DEFT_EXPORT Viewer : public fltk::GlWindow {
/* class DEFT_EXPORT Viewer { */
public:
  // explicit Viewer(Scene *scene, int x, int y, int w, int h, const char *l=0);
  explicit Viewer(Scene *scene, int w, int h, const char *l=0);
  ~Viewer();
  int verbose;                 // verbosity


  void camera(float frX, float frY, float frZ,
              float atX, float atY, float atZ,
              float upX, float upY, float upZ,
              float fovy, float neer, float faar);

  // void redraw() {};

  float fromX();
  float fromY();
  float fromZ();
  void fromX(float x);
  void fromY(float y);
  void fromZ(float z);
  float atX();
  float atY();
  float atZ();
  void atX(float x);
  void atY(float y);
  void atZ(float z);
  float upX();
  float upY();
  float upZ();
  void upX(float x);
  void upY(float y);
  void upZ(float z);
  float fovy();
  float neer();
  float faar();
  void fovy(float a);
  void neer(float n);
  void faar(float f);
  void upFix(bool);
  bool upFix();
  void orthographic(bool);
  bool orthographic();

  void helpPrint();
  void reshape(int w, int h);  // glut-like
  void draw();
  float drawTime() const { return _scene->drawTime(); }
  float totalTime() const { return _scene->totalTime(); }
  void screenDump();
  int handle(int event);       // dispatches to keyboard, mouse, and motion
  void keyboard(char key, int x, int y);  // glut-like
  void mouse(int button, int shift, int press, int x, int y);
  void motion(int button, int shift, int x, int y, int dx, int dy);
  // resets camera to see all of current scene
  void cameraReset();

  // callbacks
  void postDrawCallback(viewerCallback *cb, void *data);
private:
  Scene *_scene;             // we do NOT own
  limnCamera *_camera;       // the camera we update, we DO own
  bool _upFix;
  int _mode;                 // from DeftViewerMode* enum
  int _newX, _newY;          // current cursor coords
  int _oldX, _oldY;          // last cursor coords
  int button();              // which button was just pressed
  void cameraUpdate();

  // callbacks
  viewerCallback *_postDrawCallback;
  void *_postDrawCallbackData;
};

}

#endif /* DEFT_VIEWER_INCLUDED */
