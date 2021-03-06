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

#ifndef DEFT_TENSOR_GLYPH_INCLUDED
#define DEFT_TENSOR_GLYPH_INCLUDED

#include "Deft.H"
#include "Object.H"
#include "PolyData.H"

namespace Deft {

/*
** this is a good example of something that has to handle colormapping
** (assigning RGB to data attributes) internally- there is no
** per-vertex data value mapped to a per-vertex RGB (as with a
** vtkPolyDataMapper).  The RGB information is generated and stored in
** an internal format that communicates with the drawImmediate method,
** and is not externally set-able
*/

class DEFT_EXPORT TensorGlyph : public Object {
public: 
  explicit TensorGlyph();
  ~TensorGlyph();
  void volumeSet(const Nrrd *nten);
  void dataSet(unsigned int num,
               const float *tenData, unsigned int tenStride,
               const float *posData, unsigned int posStride,
               float measurementFrame[9]);
  void dynamic(bool dyn);
  bool dynamic();
  void anisoType(int aniso);
  int anisoType();
  void maskThresh(double maskThresh);
  double maskThresh();
  void clampEval(double clamp);
  double clampEval();
  void clampEvalUse(bool clampUse);
  bool clampEvalUse();
  void skipNegativeEigenvalues(bool skip);
  bool skipNegativeEigenvalues();
  void anisoThreshMin(double anisoThreshMin);
  double anisoThreshMin();
  void anisoThresh(double anisoThresh);
  double anisoThresh();
  void rgbParmSet(int aniso, unsigned int evec,
                  double maxSat, double isoGray,
                  double gamma, double modulate);
  // void cmapSet(Nrrd *rmap, int aniso, int anisoSat);
  void glyphType(int glyphType);
  int glyphType();
  void cleanEdge(bool cleanEdge);
  bool cleanEdge();
  void superquadSharpness(double sharpness);
  double superquadSharpness();
  void glyphResolution(unsigned int res);
  unsigned int glyphResolution();
  void barycentricResolution(unsigned int res);
  unsigned int barycentricResolution();
  void glyphScale(double scale);
  double glyphScale();

  void update();
  void drawImmediate();
  unsigned int glyphsDrawnNum();
  unsigned int glyphPositionsGet(Nrrd *npos);
  unsigned int polygonsPerGlyph();
  void boundsGet(float min[3], float max[3]) const;

private:
  // flags for managing state
  bool flag[DEFT_FLAG_NUM_MAX];

  // basic glyph parameters
  bool _dynamic,            // data is assumed dynamic; no sorting by aniso
    _cleanEdge,             // no vertex sharing on cylinder and cube glyphs
    _skipNegEval,           // don't show tensors w/ negative eigenvalues
    _clampEvalUse;          // do clamping of eigenvalues to this
  int _glyphType,           // from tenGlyphType* enum
    _anisoType,             // for culling, from tenAniso* enum
    _rgbAnisoType;          // for RGB coloring, from tenAniso* enum
  unsigned int _glyphRes,   // face resolution for all glyphs
    _baryRes,               // resolution of barycentric shape domain
    _rgbEvec;               // in {0,1,2}: which eigenvector used for coloring
  double _glyphScale,       // over-all glyph scaling
    _anisoThreshMin,        // estimated lower bound on anisoThresh
    _anisoThresh,           // culling based on anisotropy
    _maskThresh,            // culling based on confidence/mask value
    _clampEval,             // if _clampEvalUse, clamp eigenvalues to this
    _rgbMaxSat,             // maximal saturation
    _rgbIsoGray,            // gray value used for isotropic glyphs
    _rgbGamma,              // per-component gamma 
    _rgbModulate,           // how much to modulate color by anisotropy
    _superquadSharpness;    // sharpness ("gamma") of superquad formula

  // copies of, or info about, input data
  float _measrFrame[9];     // measurement frame (identity by default)
  gageShape *_gshape;       // set with input volume data
  const float *_tenData,    // given tensor data (NOT owned)
    *_posData,              // given position data (NOT owned)
    *_dataCache;            // pointer nDataCache (is owned)

  unsigned int _inDataNum,  // total number of input tensor values
    _inTenDataStride,       // stride of tenData
    _inPosDataStride,       // stride of posData
    _maxNum,                // modest bound on max number of glyphs
    _activeNum,             // number of glyphs meeting aniso,mask thresh
    _glyphsDrawnNum,        // number of glyphs recently drawn
    _polygonsPerGlyph;      // number of polygons per glyph

  Nrrd *_nAnisoCache,       // used to cache anisotropy values
    *_nDataCache,           // layout defined elsewhere
    *_nList;                // 2-D array of display lists

  // rendering stuff
  void nListEmpty();        // clear barycentric palette
  bool nListIsEmpty();      // barycentric palette is empty

  // for managing state and dependency graph 
  void anisoCacheUpdate();
  void maxNumUpdate();
  void dataAllocatedUpdate();
  void dataBasicUpdate();
  void dataSortedUpdate();
  void dataBaryIdxUpdate();
  void dataRGBUpdate();
  void glyphPaletteUpdate();
  void activeNumUpdate();
  void activeSetUpdate();
};

}

#endif /* DEFT_TENSOR_GLYPH_INCLUDED */
