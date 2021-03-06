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

#ifndef DEFT_HYPER_STREAMLINE_INCLUDED
#define DEFT_HYPER_STREAMLINE_INCLUDED

#include "Deft.h"
#include "PolyProbe.h"

namespace Deft {

// HEY: this is the first time I may regret not using "Filter" in the
// name of things that really are filters, since now the thing that
// logically *is* a HyperStreamline has to use a different name
// 
// actually now that Deft::HyperStreamlineSingle got moved down to
// teem::tenFiberSingle, this objection is somewhat moot

class DEFT_EXPORT HyperStreamline : public PolyProbe {
public: 
  explicit HyperStreamline(const Volume *vol);
  ~HyperStreamline();

  bool useDwi() const;

  void verbose(int verb) { _tfx->verbose = verb; };
  int verbose() { return _tfx->verbose; };

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
  bool stopAnisoDo() const { 
    return (_tfx->stop & (1 << tenFiberStopAniso)) ? true : false; }

  void stopHalfLength(double maxHalfLength);
  void stopHalfLengthDo(bool doit);
  double stopHalfLength() const { return _tfx->maxHalfLen; }
  bool stopHalfLengthDo() const {
    return (_tfx->stop & (1 << tenFiberStopLength)) ? true : false; }

  void stopHalfStepNum(unsigned int steps);
  void stopHalfStepNumDo(bool doit);
  unsigned int stopHalfStepNum() const { return _tfx->maxNumSteps; }
  bool stopHalfStepNumDo() const {
    return (_tfx->stop & (1 << tenFiberStopNumSteps)) ? true : false; }

  void stopConfidence(double conf);
  void stopConfidenceDo(bool doit);
  double stopConfidence() const { return _tfx->confThresh; }
  bool stopConfidenceDo() const {
    return (_tfx->stop & (1 << tenFiberStopConfidence)) ? true : false; }

  void stopRadius(double conf);
  void stopRadiusDo(bool doit);
  double stopRadius() const { return _tfx->minRadius; }
  bool stopRadiusDo() const {
    return (_tfx->stop & (1 << tenFiberStopRadius)) ? true : false; }

  void stopFraction(double frac);
  void stopFractionDo(bool doit);
  double stopFraction() const { return _tfx->minFraction; }
  bool stopFractionDo() const {
    return (_tfx->stop & (1 << tenFiberStopFraction)) ? true : false; }

  void stopStubDo(bool doit);
  bool stopStubDo() const {
    return (_tfx->stop & (1 << tenFiberStopStub)) ? true : false; }

  void stopReset();

  // HEY: try again taking a std::vector<float[3]>: failed because
  // C++ can't do array assignment.  Is here any point in having
  // a whole class just to represent a seed point?
  void seedsSet(const Nrrd *nseeds);

  // overrides PolyProbe's
  void colorQuantity(int quantity);
  int colorQuantity();

  // overrides PolyProbe's
  void brightness(double brightness);
  double brightness() const { return _brightness; }

  void tubeDo(bool doit);
  bool tubeDo() const { return _tubeDo; }

  void stopColorDo(bool doit);
  bool stopColorDo() const { return _stopColorDo; }

  void tubeFacet(unsigned int facet);
  unsigned int tubeFacet() const { return _tubeFacet; }

  void tubeRadius(double radius);
  double tubeRadius() const { return _tubeRadius; }

  // volume to probe in for probeItem, probeMeasr
  void probeVol(const Nrrd *pnrrd, const gageKind *kind);

  // if non-zero, item to probe at fiber vertices */
  void probeItem(int item);
  int probeItem() const { return _probeItem; }

  // if non-zero, how to sort fibers based on probed values */
  void probeMeasr(int measr);
  int probeMeasr() const { return _probeMeasr; }

  // limit number displayed 
  void probeCap(unsigned int cap);
  unsigned int probeCap() const { return _probeCap; }

  // have to learn from another hsline instance, and not the UI,
  // or else the objects would depend on the UI
  // HEY: isn't there some other C++ idiomatic way of expressing this?
  void parmCopy(HyperStreamline *src);

  void update();

  unsigned int seedNum() const { return _seedNum; }
  unsigned int fiberNum() const { return _fibers->fiberNum; }
  unsigned int fiberVertexNum() const { return _fiberVertexNum; }
  double fiberAllocatedTime() const { return _fiberAllocatedTime; }
  double fiberGeometryTime() const { return _fiberGeometryTime; }
  double fiberColorTime() const { return _fiberColorTime; }
  double fiberStopColorTime() const { return _fiberStopColorTime; }
  double tubeAllocatedTime() const { return _tubeAllocatedTime; }
  double tubeGeometryTime() const { return _tubeGeometryTime; }
  double tubeColorTime() const { return _tubeColorTime; }
private:
  bool _flag[128], _tubeDo, _stopColorDo, _dwi;
  unsigned int _tubeFacet, _endFacet;
  const Volume *_volume;
  double _tubeRadius, _brightness,
    _fiberAllocatedTime,
    _fiberGeometryTime,
    _fiberColorTime,
    _fiberStopColorTime,
    _tubeAllocatedTime,
    _tubeGeometryTime,
    _tubeColorTime;
  limnPolyData *_lpldFibers, *_lpldTubes;
  const Nrrd *_probeNrrd;
  const gageKind *_probeKind;
  int _probeItem, _probeMeasr;
  unsigned int _probeCap;

  Nrrd *_nseed, *_nTubeVertMap;
  unsigned int _seedNum; /* # seed points or 0 to say "none" (and this 
                            semantics makes it not completely redundant 
                            with _nseeds->axis[1].size */

  tenFiberMulti *_fibers;
  unsigned int _fiberVertexNum; // total # vertices on non-trivial fibers

  tenFiberContext *_tfx;

  void updateFiberAllocated();
  void updateFiberContext();
  void updateProbe();
  void updateFiberGeometry();
  void updateFiberColor();
  void updateFiberStopColor();
  void updateTubeAllocated();
  void updateTubeGeometry();
  void updateTubeColor();
};

} /* namespace Deft */

#endif /* DEFT_XXX_INCLUDED */
