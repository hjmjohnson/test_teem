ADD_LIBRARY(ten
  aniso.c
  bimod.c
  bvec.c
  chan.c
  defaultsTen.c
  enumsTen.c
  epireg.c
  estimate.c
  fiber.c
  fiberMethods.c
  glyph.c
  grads.c
  miscTen.c
  mod.c
  tenGage.c
  tenDwiGage.c
  tensor.c
  tendAnhist.c
  tendAnplot.c
  tendAnscale.c
  tendAnvol.c
  tendBfit.c
  tendBmat.c
  tendEllipse.c
  tendEpireg.c
  tendEstim.c
  tendEval.c
  tendEvaladd.c
  tendEvalclamp.c
  tendEvalpow.c
  tendEvec.c
  tendEvecrgb.c
  tendEvq.c
  tendExpand.c
  tendFiber.c
  tendFlotsam.c
  tendGlyph.c
  tendGrads.c
  tendHelix.c
  tendMake.c
  tendNorm.c
  tendPoint.c
  tendSatin.c
  tendShrink.c
  tendSim.c
  tendSlice.c
  tendSten.c
  tendUnmf.c
)

TARGET_LINK_LIBRARIES(ten echo limn gage dye unrrdu ell nrrd biff air)
