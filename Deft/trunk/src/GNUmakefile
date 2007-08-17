
OBJS = Volume.o Object.o Group.o \
       Viewer.o ViewerUI.o \
       Slider.o \
       enums.o misc.o Scene.o \
       Contour.o \
       Plane.o Gage.o Values.o Cmap.o \
       StarGlyph.o TensorGlyph.o TensorGlyphUI.o \
       TriPlane.o TriPlaneUI.o \
       HyperStreamline.o HyperStreamlineUI.o \
       Anisocontour.o AnisocontourUI.o \
       Anisofeature.o AnisofeatureUI.o \
       SeedPoint.o SeedPointUI.o \
       PolyData.o PolyProbe.o

all: Deft_dwi 
# all: Deft_dti Deft_iso Deft_push Deft_sqd Deft_lmpd 
# all: Deft_dti   Deft_iso Deft_push 

Cmap.o: Values.o
PolyProbe.o: PolyData.o
Plane.o: PolyProbe.o
Scene.o StarGlyph.o TensorGlyph.o: Object.o
PolyData.o: Object.o Gage.o Values.o Cmap.o
TriPlane.o: Group.o Plane.o HyperStreamline.o
Values.o: Gage.o
TensorGlyphUI.o: Viewer.o
TensorGlyphUI.o: TensorGlyph.o
HyperStreamline.o: PolyData.o 
Viewer.o: Scene.o

%.o: %.cxx %.h Deft.h
	c++ -g -O3 -W -Wall `fltk-config --cxxflags` \
	  -I/Users/gk/teem/include -I/usr/include \
	  -c $< -o $@

%.o: %.cxx Deft.h
	c++ -g -O3 -W -Wall `fltk-config --cxxflags` \
	  -I/Users/gk/teem/include \
	  -c $< -o $@

Deft_dti: $(OBJS) main-dti.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-dti.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
          -L/Users/gk/teem/pre-src/levmar-2.1.3 \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

Deft_dwi: $(OBJS) main-dwi.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-dwi.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
          -L/Users/gk/teem/pre-src/levmar-2.1.3 \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

Deft_sqd: $(OBJS) main-sqd.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-sqd.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

Deft_iso: $(OBJS) main-iso.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-iso.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
          -L/Users/gk/teem/pre-src/levmar-2.1.3 \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

Deft_push: $(OBJS) main-push.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-push.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

Deft_lmpd: $(OBJS) main-lmpd.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-lmpd.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
          -L/Users/gk/teem/pre-src/levmar-2.1.3 \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

clean:
	rm -f $(OBJS) main-dti.o main-dwi.o \
          main-sqd.o Deft_dti Deft_sqd Deft_push
