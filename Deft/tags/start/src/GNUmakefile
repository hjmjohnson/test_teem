
OBJS = Volume.o Object.o Group.o \
       enums.o misc.o Viewer.o ViewerUI.o Scene.o \
       Slider.o Actor.o Contour.o GlyphOld.o \
       HyperStreamline.o HyperStreamlineUI.o \
       TensorGlyph.o TensorGlyphUI.o \
       Plane.o Gage.o Values.o Cmap.o \
       PolyData.o PolyProbe.o \
       TriPlane.o TriPlaneUI.o

all: deft deft-sqd

Cmap.o: Values.o
PolyProbe.o: PolyData.o
Plane.o: PolyProbe.o
Scene.o: Actor.o
Scene.o: Object.o
PolyData.o: Object.o Gage.o Values.o Cmap.o
Values.o: Gage.o
TensorGlyph.o: PolyData.o
TensorGlyphUI.o: Viewer.o
TensorGlyphUI.o: TensorGlyph.o
HyperStreamline.o: PolyData.o 
Viewer.o: Scene.o

%.o: %.cxx %.H Deft.H
	c++ -g -O3 -W -Wall `fltk-config --cxxflags` \
	  -I/Users/gk/teem/include -I/usr/include \
	  -c $< -o $@

%.o: %.cxx Deft.H
	c++ -g -O3 -W -Wall `fltk-config --cxxflags` \
	  -I/Users/gk/teem/include \
	  -c $< -o $@

NOT-OBJS = Viewer.o ViewerUI.o Volume.o

../deft: $(OBJS) main-dti.o
	c++ -g -O3 -W -Wall -o $@  $(OBJS) main-dti.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

../Deft-sqd: $(OBJS) main-sqd.o
	c++ -O3 -W -Wall -o $@  $(OBJS) main-sqd.o \
	  -L/sw/lib -L/usr/local/lib -L/Users/gk/teem/darwin.32/lib \
	  -lfltk -lfltk_gl \
	  -framework Carbon -lGLEW -framework OpenGL -framework AGL \
          -lteem -lpng -lz -lm -lobjc
	/Developer/Tools/Rez -t APPL -o $@  /usr/local/src/fltk/FL/mac.r

clean:
	rm -f $(OBJS) main-dti.o main-sqd.o ../deft ../deft-sqd
