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

/*
** learned: something to be careful about when using fltk callbacks:
** for example:

  void
  ViewerUI::camResetButton_cb(fltk::Button *but, ViewerUI *ui) {
  
    _viewer->cameraReset();
    _viewer->redraw();
  }

Looks fine, but is no good because "_viewer" is a private member of 
ViewerUI, and camResetButton_cb is a static method.  But:

  void
  ViewerUI::camResetButton_cb(fltk::Button *but, ViewerUI *ui) {
  
    ui->_viewer->cameraReset();
    ui->_viewer->redraw();
  }

*/

#include "ViewerUI.h"

namespace Deft {

ViewerUI::ViewerUI(Viewer *vw) {
  const unsigned int W = 400, H = 200, lineH = 20;
  unsigned int incy, winy;

  _viewer = vw;
  _viewer->postDrawCallback((viewerCallback*)(postDraw_cb), this);

  winy = 0;
  _win = new fltk::Window(W, H, "Deft::Viewer");
  _win->begin();
  _win->resizable(_win);

  winy += 5;
  _camResetButton = new fltk::Button(8*W/12, winy, W/4, incy=lineH,
                                     "Camera Reset");
  _camResetButton->callback((fltk::Callback*)camResetButton_cb, this);

  winy += incy + 5;
  _fromXInput = new fltk::FloatInput(10+1*W/12, winy, W/6, incy=lineH, "From");
  _fromYInput = new fltk::FloatInput(10+3*W/12, winy, W/6, incy=lineH, "");
  _fromZInput = new fltk::FloatInput(10+5*W/12, winy, W/6, incy=lineH, "");
  _fovyInput = new fltk::FloatInput(10+9*W/12, winy, W/6, incy=lineH, "FOV");
  winy += incy;
  _atXInput = new fltk::FloatInput(10+1*W/12, winy, W/6, incy=lineH, "At");
  _atYInput = new fltk::FloatInput(10+3*W/12, winy, W/6, incy=lineH, "");
  _atZInput = new fltk::FloatInput(10+5*W/12, winy, W/6, incy=lineH, "");
  _neerInput = new fltk::FloatInput(10+9*W/12, winy, W/6, incy=lineH, "Near");
  winy += incy;
  _upXInput = new fltk::FloatInput(10+1*W/12, winy, W/6, incy=lineH, "Up");
  _upYInput = new fltk::FloatInput(10+3*W/12, winy, W/6, incy=lineH, "");
  _upZInput = new fltk::FloatInput(10+5*W/12, winy, W/6, incy=lineH, "");
  _faarInput = new fltk::FloatInput(10+9*W/12, winy, W/6, incy=lineH, "Far");

  winy += incy;

  _upFixButton = new fltk::CheckButton(10+1*W/12, winy, W/6, incy=lineH,
                                       "Fix Up");
  _upFixButton->callback((fltk::Callback*)upFix_cb, this);
  _upFixButton->value(_viewer->upFix());

  _orthographicButton = new fltk::CheckButton(10+4*W/12, winy, W/6, incy=lineH,
                                              "Orthographic");
  _orthographicButton->callback((fltk::Callback*)orthographic_cb, this);
  _orthographicButton->value(_viewer->orthographic());

  _fromXInput->value(_viewer->fromX());
  _fromYInput->value(_viewer->fromY());
  _fromZInput->value(_viewer->fromZ());
  _atXInput->value(_viewer->atX());
  _atYInput->value(_viewer->atY());
  _atZInput->value(_viewer->atZ());
  _upXInput->value(_viewer->upX());
  _upYInput->value(_viewer->upY());
  _upZInput->value(_viewer->upZ());
  _fovyInput->value(_viewer->fovy());
  _neerInput->value(_viewer->neer());
  _faarInput->value(_viewer->faar());

  _fromXInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _fromYInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _fromZInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _atXInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _atYInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _atZInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _upXInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _upYInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _upZInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _fovyInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _neerInput->callback((fltk::Callback*)(cameraInput_cb), this);
  _faarInput->callback((fltk::Callback*)(cameraInput_cb), this);

  winy += incy + 10;

  _widthInput = new fltk::IntInput(10+3*W/12, winy, W/6, incy=lineH,
                                   "Viewer Size");
  _heightInput = new fltk::IntInput(10+5*W/12, winy, W/6, incy=lineH, "");
  _widthInput->value(_viewer->w());
  _heightInput->value(_viewer->h());
  _widthInput->callback((fltk::Callback*)imageSizeInput_cb, this);
  _heightInput->callback((fltk::Callback*)imageSizeInput_cb, this);

  _screenDumpButton = new fltk::Button(8*W/12, winy, W/4, incy=lineH,
                                       "Image Save");
  _screenDumpButton->callback((fltk::Callback*)screenDumpButton_cb, this);

  // *_widthInput, *_heightInput;
  // static void imageSizeInput_cb(fltk::IntInput *in, ViewerUI *ui);

  winy += incy + 10;

  _framesPerSecondOutputRender = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                                  "Rendering frames/sec");
  _framesPerSecondOutputRender->box(fltk::NO_BOX);

  winy += incy + 2;
  _framesPerSecondOutputActual = new fltk::Output(W/2, winy, W/4, incy=lineH,
                                                  "Actual frames/sec");
  _framesPerSecondOutputActual->box(fltk::NO_BOX);

  _hestButton = new fltk::Button(17*W/20, winy, W/10, incy=lineH, "hest");
  _hestButton->callback((fltk::Callback*)_hest_cb, this);

  _win->end();
}

ViewerUI::~ViewerUI() {

  // nothing?
}

void
ViewerUI::show() {
  
  _win->show();
}

void
ViewerUI::camResetButton_cb(fltk::Button *, ViewerUI *ui) {

  ui->_viewer->cameraReset();
  ui->_viewer->redraw();
}

void
ViewerUI::screenDumpButton_cb(fltk::Button *, ViewerUI *ui) {

  ui->_viewer->screenDump();
}

void
ViewerUI::cameraInput_cb(fltk::FloatInput *, ViewerUI *ui) {

  ui->_viewer->fromX(ui->_fromXInput->fvalue());
  ui->_viewer->fromY(ui->_fromYInput->fvalue());
  ui->_viewer->fromZ(ui->_fromZInput->fvalue());
  ui->_viewer->atX(ui->_atXInput->fvalue());
  ui->_viewer->atY(ui->_atYInput->fvalue());
  ui->_viewer->atZ(ui->_atZInput->fvalue());
  ui->_viewer->upX(ui->_upXInput->fvalue());
  ui->_viewer->upY(ui->_upYInput->fvalue());
  ui->_viewer->upZ(ui->_upZInput->fvalue());
  ui->_viewer->fovy(ui->_fovyInput->fvalue());
  ui->_viewer->neer(ui->_neerInput->fvalue());
  ui->_viewer->faar(ui->_faarInput->fvalue());
  ui->_viewer->redraw();
}

void
ViewerUI::imageSizeInput_cb(fltk::IntInput *, ViewerUI *ui) {
  
  ui->_viewer->resize(ui->_widthInput->ivalue(),
                      ui->_heightInput->ivalue());
  ui->_viewer->redraw();
}

void
ViewerUI::upFix_cb(fltk::CheckButton *chbut, ViewerUI *ui) {
  ui->_viewer->upFix(chbut->value());
}

void
ViewerUI::orthographic_cb(fltk::CheckButton *chbut, ViewerUI *ui) {
  ui->_viewer->orthographic(chbut->value());
  ui->_viewer->redraw();
}

void
ViewerUI::postDraw_cb(Viewer *, ViewerUI *ui) {
  char buff[AIR_STRLEN_MED];

  ui->_fromXInput->value(ui->_viewer->fromX());
  ui->_fromYInput->value(ui->_viewer->fromY());
  ui->_fromZInput->value(ui->_viewer->fromZ());
  ui->_atXInput->value(ui->_viewer->atX());
  ui->_atYInput->value(ui->_viewer->atY());
  ui->_atZInput->value(ui->_viewer->atZ());
  ui->_upXInput->value(ui->_viewer->upX());
  ui->_upYInput->value(ui->_viewer->upY());
  ui->_upZInput->value(ui->_viewer->upZ());
  ui->_fovyInput->value(ui->_viewer->fovy());
  ui->_neerInput->value(ui->_viewer->neer());
  ui->_faarInput->value(ui->_viewer->faar());
  ui->_upFixButton->value(ui->_viewer->upFix());
  ui->_orthographicButton->value(ui->_viewer->orthographic());
  ui->_widthInput->value(ui->_viewer->w());
  ui->_heightInput->value(ui->_viewer->h());
  sprintf(buff, "%g", 1.0/(ui->_viewer->drawTime()));
  ui->_framesPerSecondOutputRender->value(buff);
  sprintf(buff, "%g", 1.0/(ui->_viewer->totalTime()));
  ui->_framesPerSecondOutputActual->value(buff);
}

void
ViewerUI::_hest_cb(fltk::Button *, ViewerUI *ui) {

  printf("-fr %g %g %g -at %g %g %g \\\n"
         "-up %g %g %g -rh %s-ar \\\n"
         "-dn %g -df %g -fv %g -is %d %d \\\n",
         ui->_viewer->fromX(),
         ui->_viewer->fromY(),
         ui->_viewer->fromZ(),
         ui->_viewer->atX(),
         ui->_viewer->atY(),
         ui->_viewer->atZ(),
         ui->_viewer->upX(),
         ui->_viewer->upY(),
         ui->_viewer->upZ(),
         ui->_viewer->orthographic() ? "-or " : "",
         ui->_viewer->neer(), ui->_viewer->faar(), ui->_viewer->fovy(),
         ui->_viewer->w(), ui->_viewer->h());
}

} /* namespace Deft */
