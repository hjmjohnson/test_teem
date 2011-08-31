#!/usr/bin/env python

# display.py:
# Sam Quinan

# Import PySide classes
import sys, os
from PySide.QtCore import Qt, Slot, QSize, QPoint, QMutex
from PySide.QtGui import QApplication, QMainWindow, QImage, QPixmap, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem, QDialogButtonBox, QDockWidget, QLabel, QGroupBox, QFormLayout, QAction, QKeySequence, QFileDialog, QSplashScreen, QTransform

# Import Nrrd Interface
import Nrrd as nrd
import teem
import ctypes
import numpy as np

def lerp(o,O,i,v,I):
    return (1.0*O-o)*(1.0*v-i)/(1.0*I-i) + 1.0*o

class NrrdDisplay(QMainWindow):
    def __init__(self, nrrd, parent=None):
        super(NrrdDisplay, self).__init__(parent)
        
        self.setWindowTitle("Oriented Display")
        
        # setup for 8-bit greyscale to RGBA colormapping for image display
        prelut = np.ones([256,4],dtype=np.uint8)
        prelut[:,0] = range(256)
        prelut[:,1] = range(256)
        prelut[:,2] = range(256)
        prelut[:,3] = 255
        self.lut = nrd.Nrrd()
        self.lut.fromNDArray(prelut)
        self.rng = teem.nrrdRangeNew(0, 255)
        
        # setup of QT display
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
        self.localScene = QGraphicsScene()
        self.localGrphView.setScene(self.localScene)

        self.nrrd = nrrd
        self.transform = getOrientationTransform(self.nrrd)
        self.nImg = nrd.Nrrd()
        self.convertToGrayscale(self.nrrd, self.nImg)
        self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
        # menu creation
        openAction = QAction("Open...", self)
        openAction.setShortcuts(QKeySequence.Open)
        openAction.setStatusTip("Open an existing file")
        openAction.triggered.connect(self.open)
        
        fileMenu = self.menuBar().addMenu("File")
        fileMenu.addAction(openAction)
        
        # info window creation
        groupBox = QGroupBox()
        self.wSpaceLabel = QLabel("n/a")
        self.iSpaceLabel  = QLabel("n/a")
        boxLayout = QFormLayout()
        boxLayout.addRow("World Space Coords:", self.wSpaceLabel)
        boxLayout.addRow("Image Space Coords:", self.iSpaceLabel)
        groupBox.setLayout(boxLayout)
        
        infoWidget = QDockWidget("Information", self)
        infoWidget.setAllowedAreas(Qt.NoDockWidgetArea)
        infoWidget.setFloating(True)
        infoWidget.setFeatures(QDockWidget.DockWidgetFloatable)
        infoWidget.setWidget(groupBox)
        self.addDockWidget(Qt.RightDockWidgetArea, infoWidget) #using Qt.NoDockWidetArea here spits out warning... 
        self.infoWidget = infoWidget
        
    
    def sizeHint(self):
        s = self.pixmapItem.boundingRect().size().toSize()
        return s
    
    def resizeEvent(self, e):
        super(NrrdDisplay, self).resizeEvent(e)
    
    def open(self):
        (fileName, filter) = QFileDialog.getOpenFileName(self, "Open Image", os.getcwd(), "Text Files (*.nrrd)")
        if fileName == "":
            return
        
        nrrd = nrd.Nrrd()
        nrrd.load(fileName)
        
        self.nrrd = nrrd
        self.transform = getOrientationTransform(self.nrrd)
        self.convertToGrayscale(self.nrrd, self.nImg)
        
        self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
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
        self.positionWidget(self.infoWidget)

    def positionWidget(self, widget):
        w_frame = self.infoWidget.geometry()
        frame = self.geometry()
        if (w_frame.intersects(frame)):
            widget.move((frame.x() + frame.width() + 10), w_frame.y())
        return

    def getPixmap(self):
        nrrd = self.nImg
        localArray = nrd.ExtendedArray(nrrd)
        localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_RGB32)
        pixmap = QPixmap(localImage.transformed(self.transform))
        return pixmap

    def convertToGrayscale(self, nin, nout):
        teem.nrrdApply1DLut(nout._ctypesobj, nin._ctypesobj, self.rng, self.lut._ctypesobj, teem.nrrdTypeUChar, teem.AIR_FALSE)
        return

def getOrientationTransform(nin):
    # note this function is only designed to work for 8-bit greyscale images -- no good way to generalize to RGB/RGBA/CMYK/etc. without making major assumptions about axis positioning based on # of dimensions
    m00 = nin._ctypesobj.contents.axis[0].spaceDirection[0]
    m01 = nin._ctypesobj.contents.axis[1].spaceDirection[0]
    m10 = nin._ctypesobj.contents.axis[0].spaceDirection[1]
    m11 = nin._ctypesobj.contents.axis[1].spaceDirection[1]
    # reminder: b/c this is being used to create a new image for display, with it's own image space, we don't have to worry about translation as defined by spaceOrigins... spaceOrigins however will need to be accounted for when attempting to determine the image space coordinate from a user click...
    m02 = 0.0
    m12 = 0.0
    m20 = 0.0
    m21 = 0.0
    m22 = 1.0
        
    return QTransform(m00, m01, m02, m10, m11, m12, m20, m21, m22)

def main():
    app = QApplication(sys.argv)
    
    pixmap = QPixmap("splash.png")
    splash = QSplashScreen(pixmap)
    splash.show()
    
    (fileName, filter) = QFileDialog.getOpenFileName(None, "Open Image", os.getcwd(), "Text Files (*.nrrd)")
    if fileName == "":
        sys.exit()
    
    nrrd = nrd.Nrrd()
    nrrd.load(fileName)
    window = NrrdDisplay(nrrd)
    window.show()
    window.positionWidget(window.infoWidget)
    splash.finish(window)
    
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()