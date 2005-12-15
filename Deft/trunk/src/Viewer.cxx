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

#include "Deft.h"
#include "Viewer.h"

namespace Deft {

Viewer::Viewer(Scene *scene, int w, int h, const char *l) : fltk::GlWindow(w,h,l) {
/* Viewer::Viewer(Scene *scene, int w, int h, const char *l) { */

  // HEY: add sanity check
  _scene = scene;
  verbose = 0;
  _camera = limnCameraNew();
  ELL_3V_SET(_camera->from, 2, 3, 4);
  ELL_3V_SET(_camera->at, 0, 0, 0);
  ELL_3V_SET(_camera->up, 0, 0, 1);
  // uRange and vRange will be set by limnCameraUpdate: aspect and fov exist
  _camera->aspect = (double)w/h;
  _camera->fov = 20;
  _camera->neer = -3;
  _camera->faar = 3;
  _camera->dist = 0;
  _camera->rightHanded = AIR_TRUE;  // always true for Deft!
  _camera->atRelative = AIR_TRUE;   // always true for Deft!
  limnCameraUpdate(_camera);      // shouldn't don't fail
  // can't do OpenGL calls yet (bus error)
  _mode = viewerModeUnknown;
  _newX = _newY = _oldX = _oldY = -1;
  _upFix = AIR_FALSE;
  _postDrawCallback = NULL;
  _postDrawCallbackData = NULL;

}

Viewer::~Viewer() {

  limnCameraNix(_camera);

}

void
Viewer::helpPrint() {

  fprintf(stderr, "------- key commands\n");
  fprintf(stderr, "v/V: increase/decrease verbosity\n");
  fprintf(stderr, "d: save current rendering to image file\n");
  fprintf(stderr, "o: toggle orthographic vs. perspective projection\n");
  fprintf(stderr, "u: toggle fixed pseudo-up-vector\n");
  fprintf(stderr, "U: pull up direction towards nearest coordinate axis\n");
  /* not implemented 
  fprintf(stderr, "r: toggle fixRight\n");
  */
  fprintf(stderr, "?: print this help info\n");

}

void
Viewer::camera(float frX, float frY, float frZ,
               float atX, float atY, float atZ,
               float upX, float upY, float upZ,
               float fovy, float neer, float faar) {

  char me[]="Deft::Viewer::camera", *err;

  ELL_3V_SET(_camera->from, frX, frY, frZ);
  ELL_3V_SET(_camera->at, atX, atY, atZ);
  ELL_3V_SET(_camera->up, upX, upY, upZ);
  _camera->aspect = (double)w()/h();
  _camera->fov = fovy;
  _camera->neer = neer;
  _camera->faar = faar;
  // is there a reason why this isn't method cameraUpdate()? 
  if (limnCameraUpdate(_camera)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: trouble with camera parameters:\n%s", me, err);
    free(err); return;
  }
  redraw();
  return;

}

float Viewer::fromX() { return static_cast<float>(_camera->from[0]); }
float Viewer::fromY() { return static_cast<float>(_camera->from[1]); }
float Viewer::fromZ() { return static_cast<float>(_camera->from[2]); }
void Viewer::fromX(float x) { _camera->from[0] = x; cameraUpdate(); }
void Viewer::fromY(float y) { _camera->from[1] = y; cameraUpdate(); }
void Viewer::fromZ(float z) { _camera->from[2] = z; cameraUpdate(); }
float Viewer::atX() { return static_cast<float>(_camera->at[0]); }
float Viewer::atY() { return static_cast<float>(_camera->at[1]); }
float Viewer::atZ() { return static_cast<float>(_camera->at[2]); }
void Viewer::atX(float x) { _camera->at[0] = x; cameraUpdate(); }
void Viewer::atY(float y) { _camera->at[1] = y; cameraUpdate(); }
void Viewer::atZ(float z) { _camera->at[2] = z; cameraUpdate(); }
float Viewer::upX() { return static_cast<float>(_camera->up[0]); }
float Viewer::upY() { return static_cast<float>(_camera->up[1]); }
float Viewer::upZ() { return static_cast<float>(_camera->up[2]); }
void Viewer::upX(float x) { _camera->up[0] = x; cameraUpdate(); }
void Viewer::upY(float y) { _camera->up[1] = y; cameraUpdate(); }
void Viewer::upZ(float z) { _camera->up[2] = z; cameraUpdate(); }
float Viewer::fovy() { return static_cast<float>(_camera->fov); }
float Viewer::neer() { return static_cast<float>(_camera->neer); }
float Viewer::faar() { return static_cast<float>(_camera->faar); }
void Viewer::fovy(float a) { _camera->fov = a; cameraUpdate(); }
void Viewer::neer(float n) { _camera->neer = n; cameraUpdate(); }
void Viewer::faar(float f) { _camera->faar = f; cameraUpdate(); }
bool Viewer::upFix() { return _upFix; }
void Viewer::upFix(bool fix) { _upFix = fix; }
bool Viewer::orthographic() { return _camera->orthographic ? true : false; }
void Viewer::orthographic(bool ortho) { 
  _camera->orthographic = ortho ? 1 : 0; 
  cameraUpdate(); 
}

void
Viewer::cameraReset() {

  double cmin, cmax, scl=8, fovY, fovZ;
  float min[3], max[3];

  _scene->boundsGet(min, max);
  ELL_3V_SCALE_ADD2(_camera->at, 0.5, min, 0.5, max);
  cmin = min[0];
  cmin = AIR_MIN(cmin, min[1]);
  cmin = AIR_MIN(cmin, min[2]);
  cmax = max[0];
  cmax = AIR_MAX(cmax, max[1]);
  cmax = AIR_MAX(cmax, max[2]);
  ELL_3V_SET(_camera->from, AIR_LERP(scl, cmin, cmax),
             _camera->at[1], _camera->at[2]);
  ELL_3V_SET(_camera->up, 0, 0, 1);
  _camera->aspect = (double)w()/h();
  fovY = 2*sqrt(3.0f)*180*atan2((max[1] - min[1])/2,
                             static_cast<float>(_camera->from[0]))/AIR_PI;
  fovZ = 2*sqrt(3.0f)*180*atan2((max[2] - min[2])/2,
                             static_cast<float>(_camera->from[0]))/AIR_PI;
  _camera->fov = AIR_MAX(fovY, fovZ);
  _camera->neer = -sqrt(3.0f)*1.001f*(cmax - cmin)/2.0f;
  _camera->faar = sqrt(3.0f)*1.001f*(cmax - cmin)/2.0f;
  cameraUpdate();
  return;

}

void
Viewer::cameraUpdate() {

  char me[]="Deft::Viewer::cameraUpdate", *err;
  
  if (limnCameraUpdate(_camera)) {
    err = biffGetDone(LIMN);
    fprintf(stderr, "%s: camera problem:\n%s", me, err);
    free(err); return;
  }
  if (!_upFix) {
    ELL_3V_SCALE(_camera->up, -1, _camera->V);
  }

  // update scene's (viewspace) lights
  _scene->lightUpdate(_camera);

  // set GL transforms
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  if (_camera->orthographic) {
    glOrtho(_camera->uRange[0], _camera->uRange[1],
            _camera->vRange[0], _camera->vRange[1],
            _camera->vspNeer, _camera->vspFaar);
  } else {
    gluPerspective(_camera->fov, _camera->aspect,
                   _camera->vspNeer, _camera->vspFaar);
  }
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(_camera->from[0], _camera->from[1], _camera->from[2],
            _camera->at[0], _camera->at[1], _camera->at[2],
            _camera->up[0], _camera->up[1], _camera->up[2]);
  return;

}

void
Viewer::reshape(int width, int height) {

  char me[]="Deft::Viewer::reshape";

  if (1 < verbose) {
    fprintf(stderr, "%s: reshape %d %d\n", me, width, height);
  }
  _camera->aspect = (double)width/height;
  glViewport(0, 0, (GLsizei)width, (GLsizei)height);
  /* many camera things didn't actually change; we use this opportunity 
     to set GL camera stuff for the *first* time when the window starts */
  cameraUpdate();
  redraw();
  return;

}

// is called by redraw()
void
Viewer::draw() {

  if (!valid()) {
    /* window resizes will trigger this */
    reshape(w(), h());
    valid(1);
  }
  _scene->draw();
  glFlush();
  if (_postDrawCallback) {
    _postDrawCallback(this, _postDrawCallbackData);
  }
  return;

}

void
Viewer::screenDump() {

  char me[]="Deft::Viewer::screenDump", *err, fname[AIR_STRLEN_SMALL];
  Nrrd *nimg, *nflip;
  airArray *mop;
  int test;
  FILE *file;

  /* allocate stuff */
  mop = airMopNew();
  nimg = nrrdNew();
  airMopAdd(mop, nimg, (airMopper)nrrdNuke, airMopAlways);
  nflip = nrrdNew();
  airMopAdd(mop, nflip, (airMopper)nrrdNuke, airMopAlways);
  if (nrrdAlloc_va(nflip, nrrdTypeUChar, 3,
                   AIR_CAST(size_t, 4),
                   AIR_CAST(size_t, w()),
                   AIR_CAST(size_t, h()))) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't allocate image:\n%s", me, err);
    airMopError(mop); return;
  }

  /* read data from GL buffer */
  glReadBuffer(GL_FRONT);
  glReadPixels(0, 0, w(), h(), GL_RGBA, GL_UNSIGNED_BYTE, nflip->data);

  /* look for unused image filename */
  for (test=0, file=NULL; AIR_TRUE; test++) {
    file = airFclose(file);
    sprintf(fname, "%06d.png", test);
    if (!(file = fopen(fname, "rb"))) {
      break;
    }
  }

  /* vertical flip and save */
  if (nrrdFlip(nimg, nflip, 2)
      || nrrdSave(fname, nimg, NULL)) {
    airMopAdd(mop, err = biffGetDone(NRRD), airFree, airMopAlways);
    fprintf(stderr, "%s: couldn't save image:\n%s", me, err);
    airMopError(mop); return;
  }
  fprintf(stderr, "%s: saved %s\n", me, fname);

  /* clean up */
  airMopOkay(mop);
  return;

}

void
Viewer::keyboard(char key, int x, int y) {

  char me[]="Deft::Viewer::keyboard";

  AIR_UNUSED(x);
  AIR_UNUSED(y);
  if (1 < verbose) {
    fprintf(stderr, "%s: key %c\n", me, key);
  }
  switch (key) {
  case 'v':
    verbose += 1;
    fprintf(stderr, "%s: verbose = %d\n", me, verbose);
    break;
  case 'V':
    verbose = AIR_MAX(0, verbose-1);
    fprintf(stderr, "%s: verbose = %d\n", me, verbose);
    break;
  case 'o':
    _camera->orthographic = !_camera->orthographic;
    cameraUpdate();
    redraw();
    break;
  case 'u':
    _upFix = !_upFix;
    // _fixRight = AIR_FALSE;
    cameraUpdate();
    redraw();
    break;
  case 'U':
    double tmp;
    _camera->up[0] = airSgnPow(_camera->up[0], 2);
    _camera->up[1] = airSgnPow(_camera->up[1], 2);
    _camera->up[2] = airSgnPow(_camera->up[2], 2);
    ELL_3V_NORM(_camera->up, _camera->up, tmp);
    cameraUpdate();
    redraw();
    break;
    /*
  case 'r':
    _fixRight = !_fixRight;
    fprintf(stderr, "%s: fixRight turned %s\n", me, 
            _fixRight ? "**ON**" : "OFF");
    _upFix = AIR_FALSE;
    cameraUpdate();
    break;
    */
  case 'd':
    screenDump();
    break;
  case '?':
    helpPrint();
  }
  return;

}

/*
** A diagram of how clicking in different parts of the window do
** different things with the camera.  "foo/bar" means that foo
** happens with button 1 (left click), and bar happens with button 3
** (right click)
**
**        +---------------------------------------+
**        | \          X  RotateV/TranslateV    / |
**        |   \  . . . . . . . . . . . . . .  /   |
**        |     :                           :     |
**        |     :                           :     |
**        |     :                           :     |
**        |     :                           :  X RotateU/
**        |     :     X RotateUV/           :    TranslateU
**        |     :       TranslateUV         :     |
**        |  X Fov                          :     |
**        |     :                           :     |
**        |     :                           :     |
**        |     :                           :     |
**        |. . . . . . . . . . . . . . . . . \    |
**        |  X  :      X  RotateN/TranslateN   \  |
**        +--|------------------------------------+
**           \__ Dolly/Depth
*/

void
Viewer::mouse(int button, int shift, int press, int x, int y) {

  char me[]="Deft::Viewer::mouse";
  double marg=0.8;

  AIR_UNUSED(shift);
  if (1 < verbose) {
    fprintf(stderr, "%s: button %d %s @ %d %d\n", me,
            button, press ? "down" : "up", x, y);
  }
  if (!press) {
    _mode = viewerModeUnknown;
  } else if (1 == button || 3 == button) {
    /* left or right click; figure out mode */
    double xf = AIR_AFFINE(0, x, w()-1, -1, 1);
    double yf = AIR_AFFINE(0, y, h()-1, -1, 1);
    if (AIR_IN_CL(-marg, xf, marg) && AIR_IN_CL(-marg, yf, marg)) {
      _mode = (1 == button
              ? viewerModeRotateUV
              : viewerModeTranslateUV);
    } else {
      /* we're in the wild and wacky fringe */
      if (AIR_IN_CL(-1.0, xf, -marg) && AIR_IN_CL(marg, yf, 1.0)) {
        _mode = (1 == button
                ? viewerModeDolly
                : viewerModeDepth);
      } else {
        if (yf > xf) {
          /* on left or bottom edge */
          if (AIR_IN_CL(-1.0, xf, -marg)) {
            _mode = viewerModeFov;
          } else {
            _mode = (1 == button
                    ? viewerModeRotateN
                    : viewerModeTranslateN);
          }
        } else {
          /* on right or top edge */
          if (yf < -xf) {
            _mode = (1 == button
                    ? viewerModeRotateV
                    : viewerModeTranslateV);
          } else {
            _mode = (1 == button
                    ? viewerModeRotateU
                    : viewerModeTranslateU);
          }
        }
      }
    }
  }
  if (verbose) {
    fprintf(stderr, "%s: mode to %s\n", me, airEnumStr(viewerMode, _mode));
  }
  redraw();
  return;

}

void
Viewer::motion(int button, int shift, int x, int y, int dx, int dy) {

  char me[]="Deft::Viewer::motion";
  double toEye[3], dist, tmp, tvec[3], oldDist, newDist;

  AIR_UNUSED(shift);
  if (1 < verbose) {
    fprintf(stderr, "%s: button %d motion @ %d %d (%d %d) (%s)\n", me,
            button, x, y, dx, dy, airEnumStr(viewerMode, _mode));
  }
  double angle = (atan2(x - w()/2.0f, y - h()/2.0f) 
                  - atan2(x+dx - w()/2.0f, y+dy - h()/2.0f));
  if (angle > AIR_PI) {
    angle -= 2*AIR_PI;
  } else if (angle < -AIR_PI) {
    angle += 2*AIR_PI;
  }
  double dxf = AIR_DELTA(0, dx, w()-1, -1, 1);
  double dyf = AIR_DELTA(0, dy, h()-1, -1, 1);
  double dxfsc = AIR_DELTA(0, dx, w()-1,
                           _camera->uRange[0], _camera->uRange[1]);
  double dyfsc = AIR_DELTA(0, dy, h()-1,
                           _camera->vRange[0], _camera->vRange[1]);
  switch(_mode) {
  case viewerModeFov:
    _camera->fov *= pow(3.0,-angle);
    _camera->fov = AIR_MIN(179, _camera->fov);
    cameraUpdate();
    break;
  case viewerModeRotateUV:
    ELL_3V_SUB(toEye, _camera->from, _camera->at);
    dist = ELL_3V_LEN(toEye);
    ELL_3V_SCALE_ADD3(_camera->from,
                      1, toEye,
                      -1.7*_camera->vspDist*dxf, _camera->U,
                      -1.7*_camera->vspDist*dyf, _camera->V);
    tmp = ELL_3V_LEN(_camera->from);
    ELL_3V_SCALE(_camera->from, dist/tmp, _camera->from);
    ELL_3V_INCR(_camera->from, _camera->at);
    cameraUpdate();
    break;
  case viewerModeRotateU:
    ELL_3V_SUB(toEye, _camera->from, _camera->at);
    dist = ELL_3V_LEN(toEye);
    ELL_3V_SCALE_ADD2(_camera->from,
                      1, toEye,
                      -1.7*_camera->vspDist*dyf, _camera->V);
    tmp = ELL_3V_LEN(_camera->from);
    ELL_3V_SCALE(_camera->from, dist/tmp, _camera->from);
    ELL_3V_INCR(_camera->from, _camera->at);
    cameraUpdate();
    break;
  case viewerModeRotateV:
    ELL_3V_SUB(toEye, _camera->from, _camera->at);
    /* this really needs some cleaning up */
    if (_upFix) {
      tmp = ELL_3V_DOT(toEye, _camera->up);
      ELL_3V_SCALE(tvec, tmp, _camera->up);
      ELL_3V_SUB(toEye, toEye, tvec);
      dist = ELL_3V_LEN(toEye);
      ELL_3V_ADD2(toEye, toEye, tvec);
    } else {
      dist = ELL_3V_LEN(toEye);
    }
    ELL_3V_SCALE_ADD2(_camera->from,
                      1, toEye,
                      -1.7*_camera->vspDist*dxf, _camera->U);
    if (_upFix) {
      tmp = ELL_3V_DOT(_camera->from, _camera->up);
      ELL_3V_SCALE(tvec, tmp, _camera->up);
      ELL_3V_SUB(_camera->from, _camera->from, tvec);
      tmp = ELL_3V_LEN(_camera->from);
      ELL_3V_SCALE(_camera->from, dist/tmp, _camera->from);
      ELL_3V_ADD2(_camera->from, _camera->from, tvec);
    } else {
      tmp = ELL_3V_LEN(_camera->from);
      ELL_3V_SCALE(_camera->from, dist/tmp, _camera->from);
    }
    ELL_3V_INCR(_camera->from, _camera->at);
    cameraUpdate();
    break;
  case viewerModeRotateN:
    if (!(_upFix /* || _fixRight */ )) {
      ELL_3V_SCALE_ADD2(_camera->up,
                        -cos(-angle), _camera->V,
                        sin(-angle), _camera->U);
      cameraUpdate();
    }
    break;
  case viewerModeDolly:
    ELL_3V_SUB(toEye, _camera->from, _camera->at);
    ELL_3V_NORM(toEye, toEye, oldDist);
    newDist = oldDist*pow(3.0,-angle);
    _camera->fov = 2*180*atan(oldDist*tan(AIR_PI*_camera->fov/(2*180))
                              /newDist)/AIR_PI;
    _camera->dist *= newDist/oldDist;
    ELL_3V_SCALE_ADD2(_camera->from, 1, _camera->at, newDist, toEye);
    cameraUpdate();
    break;
  case viewerModeDepth:
    _camera->neer *= pow(3.0,angle);
    _camera->faar *= pow(3.0,angle);
    cameraUpdate();
    break;
  case viewerModeTranslateUV:
    ELL_3V_SCALE_INCR2(_camera->from, -dxfsc, _camera->U, -dyfsc, _camera->V);
    ELL_3V_SCALE_INCR2(_camera->at, -dxfsc, _camera->U, -dyfsc, _camera->V);
    cameraUpdate();
    break;
  case viewerModeTranslateN:
    /* HEY: this should probably also be scaled by some notion
       of the over-all size of the objects in the scene */
    ELL_3V_SCALE_INCR(_camera->from, -AIR_ABS(dyfsc)*angle, _camera->N);
    ELL_3V_SCALE_INCR(_camera->at, -AIR_ABS(dyfsc)*angle, _camera->N);
    cameraUpdate();
    break;
  default:
    fprintf(stderr, "%s: viewer mode %s not handled!\n", me,
            airEnumStr(viewerMode, _mode));
    break;
  }
  redraw();

  return;
}

/* 
** for macs: emulates 3-button mouse same way as Apple's X11 server,
** while also mapping control-click to button 3 (right-click)
*/
int
Viewer::button() {

  int butt = fltk::event_button();
  int evstate = fltk::event_state();

  if (1 == butt) {
    if (evstate & fltk::ALT) {
      butt = 2;
    } else if ((evstate & fltk::CTRL) || (evstate & fltk::META)) {
      butt = 3;
    }
  }
  return butt;

  return 0;
}

int
Viewer::handle(int event)  {

  char me[]="Deft::Viewer::handle";
  int evstate = fltk::event_state();

  int done = fltk::GlWindow::handle(event);
  if (!done) {
    if (2 < verbose) {
      fprintf(stderr, "%s: event state = %d %d %d %d %d %d %d %d %d %d\n", me,
              !!(evstate & fltk::SHIFT),
              !!(evstate & fltk::CAPSLOCK),
              !!(evstate & fltk::CTRL),
              !!(evstate & fltk::ALT),
              !!(evstate & fltk::NUMLOCK),
              !!(evstate & fltk::META),
              !!(evstate & fltk::SCROLLLOCK),
              !!(evstate & fltk::BUTTON1),
              !!(evstate & fltk::BUTTON2),
              !!(evstate & fltk::BUTTON3));
    }
    switch(event) {
    case fltk::FOCUS:
      // yes, we want keyboard focus (new requirment from fltk2)
      done = 1;
      break;
    case fltk::PUSH:
    case fltk::RELEASE:
      _oldX = fltk::event_x();
      _oldY = fltk::event_y();
      mouse(button(), evstate & fltk::SHIFT,
            event != fltk::RELEASE,
            _oldX, _oldY);
      done = 1;
      break;
    case fltk::DRAG: /* case fltk::MOVE: */
      _newX = fltk::event_x();
      _newY = fltk::event_y();
      motion(button(), evstate & fltk::SHIFT, _oldX, _oldY,
             _newX - _oldX, _newY - _oldY);
      _oldX = _newX;
      _oldY = _newY;
      done = 1;
      break;
    case fltk::KEY: /* case fltk::KEYUP: */
      if (1 == fltk::event_length()) {
        keyboard(fltk::event_text()[0], fltk::event_x(), fltk::event_y());
        done = 1;
      }
      break;
    default:
      if (2 < verbose) {
        fprintf(stderr, "%s: leaving %s undone\n", me, 
                airEnumStr(fltkEvent, event));
      }
      break;
    }
  } else if (3 < verbose) {
    fprintf(stderr, "%s: %s already done\n", me, 
            airEnumStr(fltkEvent, event));
  }
  return done;
}

void
Viewer::postDrawCallback(viewerCallback *cb, void *data) {

  if (cb) {
    _postDrawCallback = cb;
    _postDrawCallbackData = data;
  } else {
    _postDrawCallback = NULL;
    _postDrawCallbackData = NULL;
  }

}

} /* namespace Deft */
