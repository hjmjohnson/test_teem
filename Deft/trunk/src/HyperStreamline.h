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

#ifndef DEFT_HYPER_STREAMLINE_INCLUDED
#define DEFT_HYPER_STREAMLINE_INCLUDED

#include "Deft.h"
#include "PolyProbe.h"

namespace Deft {

// HEY: this is the first time I may regret not using "Filter" in the
// name of things that really are filters, since now the thing that
// logically *is* a HyperStreamline has to use a different name

// this is more a repository for per-fiber info than an interesting
// object with useful methods
class DEFT_EXPORT HyperStreamlineSingle {
public: 
  explicit HyperStreamlineSingle();
  ~HyperStreamlineSingle();

  Nrrd *nvert;
  unsigned int primIdx;
  double seedPos[3], halfLen[2];
  unsigned int seedIdx, stepNum[2];
  int whyStop[2], whyNowhere;
  /* maybe also have:
  ** aniso (mode,fa) at seed
  ** mean aniso (mode,fa) along vertices
  ** stdv of aniso (mode,fa) along vertices
  ** mean curvature magnitude
  ** mean torsion?
  */
};

class DEFT_EXPORT HyperStreamline : public PolyProbe {
public: 
  explicit HyperStreamline(const Nrrd *nten);
  ~HyperStreamline();

  void fiberType(int type);
  int fiberType() const { return _tfx->fiberType; }

  void integration(int intg);
  int integration() const { return _tfx->intg; }

  void step(double step);
  double step() const { return _tfx->stepSize; }
  
  void puncture(double punct);
  double puncture() const { return _tfx->wPunct; }

  void kernel(const NrrdKernelSpec *ksp);
  const NrrdKernelSpec *kernel() const { return _tfx->ksp; }

  void stopAniso(int aniso, double thresh);
  void stopAnisoDo(bool doit);
  int stopAnisoType() const { return _tfx->anisoStopType; }
  double stopAnisoThreshold() const { return _tfx->anisoThresh; }
  bool stopAnisoDo() const { return _tfx->stop 
                               & (1 << tenFiberStopAniso); }

  void stopHalfLength(double maxHalfLength);
  void stopHalfLengthDo(bool doit);
  double stopHalfLength() const { return _tfx->maxHalfLen; }
  bool stopHalfLengthDo() const { return _tfx->stop 
                                    & (1 << tenFiberStopLength); }

  void stopHalfStepNum(unsigned int steps);
  void stopHalfStepNumDo(bool doit);
  unsigned int stopHalfStepNum() const { return _tfx->maxNumSteps; }
  bool stopHalfStepNumDo() const { return _tfx->stop 
                                     & (1 << tenFiberStopNumSteps); }

  void stopConfidence(double conf);
  void stopConfidenceDo(bool doit);
  double stopConfidence() const { return _tfx->confThresh; }
  bool stopConfidenceDo() const { return  _tfx->stop 
                                    & (1 << tenFiberStopConfidence); }

  void stopReset();

  // HEY: try again taking a std::vector<float[3]>: failed because
  // C++ can't do array assignment.  Is here any point in having
  // a whole class just to represent a seed point?
  void seedsSet(const Nrrd *nseeds);

  // overrides PolyProbe's
  void colorQuantity(int quantity);
  int colorQuantity();

  void tubeDo(bool doit);
  void tubeFacet(unsigned int facet);
  void tubeRadius(double radius);
  bool tubeDo() const { return _tubeDo; }
  unsigned int tubeFacet() const { return _tubeFacet; }
  double tubeRadius() const { return _tubeRadius; }

  void update();
private:
  bool _flag[128], _tubeDo, _capColorDo;
  Nrrd *_nseeds;
  unsigned int _seedNum, _fiberVertexNum, _primNum, _tubeFacet;
  Volume *_volume;
  double _tubeRadius;
  limnPolyData *_lpldFibers, *_lpldTubes;

  // HEY: I tried making a vector of the class itself, and the
  // constructor was only called once!  Why?
  std::vector<HyperStreamlineSingle *>_fiber;

  tenFiberContext *_tfx;

  void updateFiberAllocated();
  void updateFiberContext();
  void updateFiberGeometry();
  void updateFiberColor();
  void updateFiberStopColor();
  void updateTubeAllocated();
  void updateTubeGeometry();
  void updateTubeColor();
};

} /* namespace Deft */

#endif /* DEFT_XXX_INCLUDED */
