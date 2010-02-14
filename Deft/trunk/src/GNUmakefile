
# TEEM = /Users/gk/teem-svn/teem
TEEM = /Users/gk/teem

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

all:  Deft_pull Deft_dti Deft_push Deft_sqd puller Deft_iso Deft_gsqd Deft_lmpd Deft_dwi
# 
# Deft_iso Deft_dti Deft_dwi
# Deft_lmpd Deft_dti
# all: Deft_pull Deft_dti Deft_iso Deft_push Deft_sqd Deft_lmpd 
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
	c++ -g -O3 -W -Wall `fltk2-config --cxxflags` \
	  -I$(TEEM)/include -I/usr/include \
	  -c $< -o $@

%.o: %.cxx Deft.h
	c++ -g -O3 -W -Wall `fltk2-config --cxxflags` \
	  -I$(TEEM)/include -I/usr/include \
	  -c $< -o $@

# handled specially because of DEFT trickery
main-pull.o: main-pull.c Deft.h
	c++ -g -O3 -W -Wall -DDEFT `fltk2-config --cxxflags` \
	  -DTEEM_BUILD_EXPERIMENTAL_LIBS \
	  -I$(TEEM)/include \
	  -c $< -o $@

Deft_dti: $(OBJS) main-dti.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-dti.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_dwi: $(OBJS) main-dwi.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-dwi.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_fig: $(OBJS) main-fig.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-fig.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

main-sqd.o: main-sqd.cxx
	c++ -g -O3 -W -Wall \
	  -I$(TEEM)/include -I/usr/include \
	  -c $< -o $@
Deft_sqd: main-sqd.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-sqd.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_hact: $(OBJS) main-hact.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-hact.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_gsqd: $(OBJS) main-gsqd.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-gsqd.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_iso: $(OBJS) main-iso.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-iso.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_pull: $(OBJS) main-pull.o $(TEEM)/arch/darwin.32/lib/libteem.a
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-pull.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

# -L/Users/gk/teem-install/lib 
puller: main-pull.c $(TEEM)/arch/darwin.32/lib/libteem.a
	cc -g -O3 -W -Wall -o puller  main-pull.c \
	  -I$(TEEM)/include -I/usr/include \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -lteem -lpng -lz -lbz2 -lm

Deft_push: $(OBJS) main-push.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-push.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

Deft_lmpd: $(OBJS) main-lmpd.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-lmpd.o \
	  -L/sw/lib -L/usr/local/lib -L$(TEEM)/arch/darwin.32/lib \
          -L/Users/gk/teem-pre-src/levmar-2.3 \
          `fltk2-config --use-gl --ldflags` \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -llevmar -lz -lbz2 -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk-2.0/fltk/mac.r

clean:
	rm -f $(OBJS) *.o Deft_* puller
	rm -rf puller.dSYM
