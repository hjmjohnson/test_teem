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

#ifndef DEFT_POLYDATA_INCLUDED
#define DEFT_POLYDATA_INCLUDED

#include "Deft.h"
#include "Object.h"
#include "Gage.h"
#include "Cmap.h"
#include "Values.h"

namespace Deft {

/*
******** class PolyData
**
** this is a wrapper around a limnPolyData so that we can gageProbe
** at its vertices, colormap those values, and draw them
*/
class DEFT_EXPORT PolyData : public Object {
public:
  explicit PolyData(const limnPolyData *poly);     // don't own
  explicit PolyData(limnPolyData *poly, bool own);
  ~PolyData();
  const limnPolyData *lpld() const { return _lpld ? _lpld : _lpldOwn; }

  /*
  ** For now, probing with gage is a method of this class (as opposed
  ** to others) basically because the limnPolyData is our protected
  ** info, and the allocation of per-vertex values are also our
  ** business.  So, the std::vector of Values exactly mirrors the
  ** outer vector of the Deft::Gage std::vector<std::vector<int>> item,
  ** and the std::vector of ints inside the Value mirrors the
  ** inner vector of the Deft::Gage item
  */
  void probe(const Gage *gage);

  // for learning the query from whence _values came
  std::vector<std::vector<int> > query() const;

  /* 
  ** HEY: weird- the color(), alphaMask(), and RGBLut() methods are
  ** not governed by any internal state- that's a derived class's job ?
  */
  // set initial RGBA in *both* vertices and _nrgba
  void color(unsigned int valuesIdx, const Cmap *cmap);

  // set vertex alpha from _nrgba alpha via alpha mask
  void alphaMask(unsigned int valuesIdx, double thresh);

  // set vertex RGB from _nrgba with given per-component look-up table
  void RGBLut(unsigned char lut[256]);

  // set vertex RGB to this
  void RGBSet(unsigned char R, unsigned char G, unsigned char B);
  
  void drawImmediate();
  void boundsGet(float min[3], float max[3]) const;
protected:
  void _init();
  const limnPolyData *_lpld;      // cannot limnPolyDataNix()
  limnPolyData *_lpldOwn;         //   can  limnPolyDataNix()

  Nrrd *_nrgba; // initial RGBA cache

  std::vector<bool> _valid;
  std::vector<Values *> _values;
  void _valuesClear();
};

} /* namespace Deft */

#endif /* DEFT_POLYDATA_INCLUDED */
