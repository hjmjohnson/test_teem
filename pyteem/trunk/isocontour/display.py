#!/usr/bin/env python

# display.py:
# Sam Quinan

# Import PySide classes
import sys, os
from PySide.QtCore import Qt, Slot, QSize, QPoint, QMutex, QEvent, Signal, QLine 
from PySide.QtGui import QApplication, QMainWindow, QImage, QPixmap, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem, QDialogButtonBox, QDockWidget, QLabel, QGroupBox, QFormLayout, QAction, QKeySequence, QFileDialog, QSplashScreen, QTransform, QLineEdit, QDoubleValidator, QIntValidator, QPainter, QPen, QHBoxLayout, QWidget, QSlider, QCheckBox, QPushButton
from PySide.QtSvg import QSvgGenerator

# Import Nrrd Interface
import Nrrd as nrd
import teem
import ctypes
import numpy as np

init_aX = 1
init_aY = 1
init_scl = 100
init_iso = 32000
line_width = 2


def lerp(x0, y0, x1, y1, y):
    f = (float(y)-float(y0))/(float(y1)-float(y0))
    result = (1-f)*x0 + f*x1
    return result

def get_isocontour(nrrd, iso, aspectx, aspecty, scale):
    edges = [
             [],
             [(0,3), (3,2)],
             [(1,2), (3,2)],
             [(0,3), (1,2)],
             [(0,1), (1,2)],
             [(0,1), (0,3), (1,2), (3,2)],
             [(0,1), (3,2)],
             [(0,1), (0,3)],
             [(0,1), (0,3)],
             [(0,1), (3,2)],
             [(0,1), (1,2), (0,3), (3,2)],
             [(0,1), (1,2)],
             [(0,3), (1,2)],
             [(1,2), (3,2)],
             [(0,3), (3,2)],
             [],
             ]
    
    sx = nrrd._ctypesobj.contents.axis[0].size
    sy = nrrd._ctypesobj.contents.axis[1].size
    
    img_data = nrd.ExtendedArray(nrrd)
    
    paths = []
    
    for j in range(sx-1):
        for i in range(sy-1):
            corner = (img_data[i,j], img_data[i+1,j], img_data[i+1,j+1], img_data[i,j+1])
            corner2index = [(i,j), (i+1,j), (i+1, j+1), (i, j+1)]
            
            if (corner[0] >= iso):
                x = "1"
            else:
                x = "0"
            
            if (corner[1] >= iso):
                x += "1"
            else:
                x += "0"
            
            if (corner[2] >= iso):
                x += "1"
            else:
                x += "0"
            
            if (corner[3] >= iso):
                x += "1"
            else:
                x += "0"
            
            index = int(x, 2)
            
            e = edges[index]
            
            draw = np.zeros((len(e), 2), dtype=int)
            
            for x in range(len(e)):
                interp = lerp(0, corner[e[x][0]], 1, corner[e[x][1]], iso)
                
                low_indecies = corner2index[e[x][0]]
                if ((e[x][0] + e[x][1]) == 3): #implies adding to fast axis
                    augment = float(interp)*scale*aspectx 
                    slow = low_indecies[0]*scale*aspecty 
                    fast = low_indecies[1]*scale*aspectx + augment 
                else:# adding to slow axis 
                    augment = float(interp)*scale*aspecty 
                    slow = low_indecies[0]*scale*aspecty + augment 
                    fast = low_indecies[1]*scale*aspectx 
                
                draw[x][0] = fast + .5*scale*aspectx
                draw[x][1] = slow + .5*scale*aspecty
            
            if (len(e) == 2):
                paths.append(QLine(draw[0][0], draw[0][1], draw[1][0], draw[1][1]))
            elif (len(e) == 4):
                paths.append(QLine(draw[0][0], draw[0][1], draw[1][0], draw[1][1]))
                paths.append(QLine(draw[2][0], draw[2][1], draw[3][0], draw[3][1]))
    return paths


# Turns out this subclass isn't needed (I was accidentally connecting the same signal to the same slot multiple times), but I'll leave as an example:
#   class MyLineEdit (QLineEdit):
#    
#        focusOut = Signal()
#    
#        def focusOutEvent(self, event = None):
#            self.focusOut.emit()
#            super(MyLineEdit, self).focusOutEvent(event)
    

class NrrdDisplay(QMainWindow):
    def __init__(self, nrrd, parent=None):
        super(NrrdDisplay, self).__init__(parent)
        
        self.setWindowTitle("Isocontours")
        
        # setup for 16-bit greyscale to RGBA colormapping for image display
        prelut = np.ones([65536,4],dtype=np.uint8)
        prelut[:,0] = [ i/256 for i in range(65536)]
        prelut[:,1] = [ i/256 for i in range(65536)]
        prelut[:,2] = [ i/256 for i in range(65536)]
        prelut[:,3] = 255
        self.lut = nrd.Nrrd()
        self.lut.fromNDArray(prelut)
        self.rng = teem.nrrdRangeNew(0, 65536)
        
        # setup of QT display
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
        self.localScene = QGraphicsScene()
        self.localGrphView.setScene(self.localScene)

        self.nrrd = nrrd
        self.transform = QTransform()
        self.nImg = nrd.Nrrd()
        self.convertToGrayscale(self.nrrd, self.nImg)
        self.paths = get_isocontour(self.nrrd, init_iso, init_aX, init_aY, (init_scl/100.0))
        self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
        # menu creation
        openAction = QAction("Open...", self)
        openAction.setShortcuts(QKeySequence.Open)
        openAction.setStatusTip("Open an existing file")
        openAction.triggered.connect(self.open)
        
        genAction = QAction("Generate SVG", self)
        genAction.setStatusTip("Generate an SVG file of current isocountour")
        genAction.triggered.connect(self.generateSVG)
        
        
        fileMenu = self.menuBar().addMenu("File")
        fileMenu.addAction(openAction)
        fileMenu.addAction(genAction)
        
        # info window creation
        groupBox = QGroupBox()
        
        self.wSpaceLabel = QLabel("(n/a, n/a)")
        self.iSpaceLabel = QLabel("(n/a, n/a)")
        
        doubleVal = QDoubleValidator(0.1, 100.0, 3, self)
        intVal = QIntValidator(1, 10000, self)
        self.xAspectEdit = QLineEdit("%i" % init_aX)
        self.xAspectEdit.setValidator(doubleVal)
        self.xAspectEdit.editingFinished.connect(self.updateDisplay)
        self.yAspectEdit = QLineEdit("%i" % init_aY)
        self.yAspectEdit.setValidator(doubleVal)
        self.yAspectEdit.editingFinished.connect(self.updateDisplay)
        self.scaleEdit = QLineEdit("%i" % init_scl)
        self.scaleEdit.setValidator(intVal)
        self.scaleEdit.editingFinished.connect(self.updateDisplay)
        
        boxLayout = QFormLayout()
        boxLayout.addRow("World Space Coords:", self.wSpaceLabel)
        boxLayout.addRow("Image Space Coords:", self.iSpaceLabel)
        boxLayout.addRow("Aspect Ratio (width):", self.xAspectEdit)
        boxLayout.addRow("Aspect Ratio (height):", self.yAspectEdit)
        boxLayout.addRow("Scale (% orig. size):", self.scaleEdit)
        groupBox.setLayout(boxLayout)
        
        infoWidget = QDockWidget("Information", self)
        infoWidget.setFocusPolicy(Qt.ClickFocus)
        infoWidget.setAllowedAreas(Qt.NoDockWidgetArea)
        infoWidget.setFloating(True)
        infoWidget.setFeatures(QDockWidget.DockWidgetFloatable)
        infoWidget.setWidget(groupBox)
        self.addDockWidget(Qt.RightDockWidgetArea, infoWidget) #using Qt.NoDockWidetArea here spits out warning... 
        self.infoWidget = infoWidget
    
        # slider window creation -- note: size of slider and text input box automatically determined by 'sizeHint' property of widgets (see wiki for explanation) 
        hbox = QWidget()
        isoVal = QIntValidator(0, 65535, self)
        self.isoEdit = QLineEdit("%i" % init_iso)
        self.isoEdit.editingFinished.connect(self.isoUpdate)
        self.isoEdit.setValidator(isoVal)
        
        self.cbox = QCheckBox()
        self.cbox.setChecked(True)
        self.cbox.toggled.connect(self.checkChange)
        
        self.slider = QSlider(Qt.Horizontal)
        self.slider.setRange(0, 65535)
        self.slider.setSliderPosition(init_iso)
        self.slider.valueChanged.connect(self.isoChange)
        
        sliderLayout = QHBoxLayout()
        sliderLayout.addWidget(self.slider)
        sliderLayout.addWidget(self.isoEdit)
        sliderLayout.addWidget(self.cbox)
        hbox.setLayout(sliderLayout)
        
        sliderWidget = QDockWidget("Current Isovalue", self)
        sliderWidget.setFocusPolicy(Qt.ClickFocus)
        sliderWidget.setAllowedAreas(Qt.BottomDockWidgetArea)
        sliderWidget.setFloating(True)
        sliderWidget.setFeatures(QDockWidget.DockWidgetFloatable)
        sliderWidget.setWidget(hbox)
        self.addDockWidget(Qt.BottomDockWidgetArea, sliderWidget)
        self.sliderWidget = sliderWidget
    
    
    def sizeHint(self):
        s = self.pixmapItem.boundingRect().size().toSize()
        return s
    
    def resizeEvent(self, e):
        super(NrrdDisplay, self).resizeEvent(e)
    
    def updateDisplay(self):
        rx = float(self.xAspectEdit.text())
        ry = float(self.yAspectEdit.text())
        scale = float(self.scaleEdit.text())/100
        iso = int(self.isoEdit.text())
        self.transform = QTransform(rx*scale, 0, 0, 0, ry*scale, 0, 0, 0, 1)
        self.paths = get_isocontour(self.nrrd, iso, rx, ry, scale) #change isovalue when slider inserted
        self.pixmapItem.setPixmap(self.getPixmap())
        self.adjust()
        return
    
    def isoChange(self, val):
        self.isoEdit.setText("%i" % val)
        self.updateDisplay()
        return
    
    def isoUpdate(self):
        i = int(self.isoEdit.text())
        self.slider.setSliderPosition(i)
        self.updateDisplay()
        return
        
    def checkChange(self, bl):
        self.slider.setTracking(bl)
    
    def open(self):
        (fileName, filter) = QFileDialog.getOpenFileName(self, "Open Image", os.getcwd(), "Image Files (*.png);;Text Files (*.nrrd)")
        if fileName == "":
            return
        
        nrrd = nrd.Nrrd()
        nrrd.load(fileName)
        
        self.nrrd = nrrd
        self.transform = QTransform()
        self.convertToGrayscale(self.nrrd, self.nImg)
        
        self.xAspectEdit.setText("%i" % init_aX)
        self.yAspectEdit.setText("%i" % init_aY)
        self.scaleEdit.setText("%i" % init_scl)
        
        self.updateDisplay()
        self.pixmapItem.setPixmap(self.getPixmap())
                
        rect = self.pixmapItem.boundingRect()
        self.localScene.setSceneRect(rect)
        self.adjust()
    
    def pixelClick(self, event):
        i_fast = int(event.pos().x())
        i_slow = int(event.pos().y())
        self.wSpaceLabel.setText("(%i, %i)" % (i_fast, i_slow))
        # insert image space conversion code and label update here

    def adjust(self):
        self.adjustSize()
        self.localScene.setSceneRect(self.pixmapItem.boundingRect())
        self.positionWidgets()

    def positionWidgets(self):
        w_frame = self.infoWidget.geometry()
        frame = self.geometry()
        if (w_frame.intersects(frame)):
            self.infoWidget.move((frame.x() + frame.width() + 10), w_frame.y())
        
        w2_frame = self.sliderWidget.geometry()
        if (w2_frame.intersects(frame)):
            self.sliderWidget.move(w2_frame.x(), (frame.y() + frame.height() + 10))
            
        
        return

    def getPixmap(self):
        nrrd = self.nImg
        localArray = nrd.ExtendedArray(nrrd)
        localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_RGB32)
        pixmap = QPixmap(localImage.transformed(self.transform))
        
        painter = QPainter()
        painter.begin(pixmap)
        painter.setRenderHint(QPainter.Antialiasing, True)
        painter.setPen(QPen(Qt.green, line_width, Qt.SolidLine, Qt.FlatCap, Qt.RoundJoin))
        painter.drawLines(self.paths)
        painter.end()
        
        return pixmap
    
    def convertToGrayscale(self, nin, nout):
        teem.nrrdApply1DLut(nout._ctypesobj, nin._ctypesobj, self.rng, self.lut._ctypesobj, teem.nrrdTypeUChar, teem.AIR_FALSE)
        return
    
    def generateSVG(self):
        (fileName, filter) = QFileDialog.getSaveFileName(self, "Open Image", os.getcwd(), "Image Files (*.svg)")
        if fileName == "":
            return
        
        generator = QSvgGenerator()
        generator.setFileName(fileName)
        r = self.pixmapItem.boundingRect()
        generator.setSize(r.size().toSize())
        
        painter = QPainter()
        painter.begin(generator)
        painter.setRenderHint(QPainter.Antialiasing, True)
        painter.setPen(QPen(Qt.green, line_width, Qt.SolidLine, Qt.FlatCap, Qt.RoundJoin))
        painter.drawLines(self.paths)
        painter.end()
        


def main():
    app = QApplication(sys.argv)
    
    pixmap = QPixmap("splash.png")
    splash = QSplashScreen(pixmap)
    splash.show()
    
    (fileName, filter) = QFileDialog.getOpenFileName(None, "Open Image", os.getcwd(), "Image Files (*.png);;Text Files (*.nrrd)")
    if fileName == "":
        sys.exit()
    
    nrrd = nrd.Nrrd()
    nrrd.load(fileName)
    window = NrrdDisplay(nrrd)
    window.show()
    window.positionWidgets()
    splash.finish(window)
    
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()