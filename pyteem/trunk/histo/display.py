#!/usr/bin/env python

# display.py:
# Sam Quinan

# Import PySide classes
import sys, os
from PySide.QtCore import Qt, Slot, QSize, QPoint, QMutex
from PySide.QtGui import QApplication, QMainWindow, QImage, QPixmap, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem, QDialogButtonBox, QDockWidget, QLabel, QGroupBox, QFormLayout, QAction, QKeySequence, QFileDialog, QSplashScreen

# Import Nrrd Interface
import Nrrd as nrd
import teem
import ctypes
import numpy as np

def lerp(o,O,i,v,I):
    return (1.0*O-o)*(1.0*v-i)/(1.0*I-i) + 1.0*o


class NrrdHistoDisplay(QMainWindow):
    def __init__(self, nrrd, parent=None):
        super(NrrdHistoDisplay, self).__init__(parent)
        
        self.setWindowTitle("Nrrd Histogram Visualizer")
        
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
        self.localScene = QGraphicsScene()
        self.localGrphView.setScene(self.localScene)
        
        prelut = np.ones([256,4],dtype=np.uint8)
        prelut[:,0] = range(256)
        prelut[:,1] = range(256)
        prelut[:,2] = range(256)
        prelut[:,3] = 255
        self.lut = nrd.Nrrd()
        self.lut.fromNDArray(prelut)
        self.rng = teem.nrrdRangeNew(0, 255)
        
        self.num_bins = 300
        self.img_height = 300
        
        self.nrrd = nrrd
        self.nData = nrd.Nrrd()
        self.nImg = nrd.Nrrd()
        self.createHistogram(nrrd, self.num_bins)
        nHistoImg = self.createHistoImage(self.img_height)
        self.colormapGrayscale(nHistoImg)
        
        self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        self.pixmapItem.mouseMoveEvent = self.pixelClick
        
        # menu creation
        openAction = QAction("Open...", self)
        openAction.setShortcuts(QKeySequence.Open)
        openAction.setStatusTip("Open an existing file")
        openAction.triggered.connect(self.open)
        
        fileMenu = self.menuBar().addMenu("File")
        fileMenu.addAction(openAction)
        
        # info window creation
        groupBox = QGroupBox()
        self.binLabel = QLabel("n/a")
        self.valLabel  = QLabel("n/a")
        self.hitLabel  = QLabel("n/a")
        boxLayout = QFormLayout()
        boxLayout.addRow("Bin:", self.binLabel)
        boxLayout.addRow("Value:", self.valLabel)
        boxLayout.addRow("Hits:", self.hitLabel)
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
        super(NrrdHistoDisplay, self).resizeEvent(e)
        if (e.oldSize() != e.size()):
            if (self.num_bins != e.size().width()):
                self.num_bins = e.size().width()
                self.createHistogram(self.nrrd, self.num_bins)
            
            self.img_height = e.size().height()
            nHistoImg = self.createHistoImage(self.img_height)
            self.colormapGrayscale(nHistoImg)
    
            self.pixmapItem.setPixmap(self.getPixmap())
            rect = self.pixmapItem.boundingRect()
            self.localScene.setSceneRect(rect)
            self.adjust()
    
    def closeEvent(self, e):
        teem.nrrdRangeNix(self.rng)
        super(NrrdHistoDisplay, self).closeEvent(e)
    
    def open(self):
        (fileName, filter) = QFileDialog.getOpenFileName(self, "Open Image", os.getcwd(), "Text Files (*.nrrd)")
        if fileName == "":
            return
        
        nrrd = nrd.Nrrd()
        nrrd.load(fileName)
        
        self.nrrd = nrrd
        self.createHistogram(nrrd, self.num_bins)
        nHistoImg = self.createHistoImage(self.img_height)
        self.colormapGrayscale(nHistoImg)
                
        self.pixmapItem.setPixmap(self.getPixmap())
        rect = self.pixmapItem.boundingRect()
        self.localScene.setSceneRect(rect)
        self.adjust()
    
    def pixelClick(self, event):
        rect = self.pixmapItem.boundingRect()
        i_fast = int(event.pos().x())
        i_slow = int(event.pos().y())
        i_fast = (0 if i_fast < 0
                    else (self.num_bins-1 if i_fast > self.num_bins
                                          else i_fast))
        self.binLabel.setText("%i" % i_fast)
        self.valLabel.setText("%g" % lerp(self.nData._ctypesobj.contents.axis[0].min,
                                          self.nData._ctypesobj.contents.axis[0].max,
                                          0.0,  i_fast + 0.5, self.num_bins))
        self.hitLabel.setText("%d" % np.asarray(self.nData)[i_fast])

    def adjust(self):
        self.adjustSize()
        self.positionWidget(self.infoWidget)

    def positionWidget(self, widget):
        w_frame = self.infoWidget.geometry()
        frame = self.geometry()
        if (w_frame.intersects(frame)):
            widget.move((frame.x() + frame.width() + 10), w_frame.y())
        return
    
    def createHistogram(self, nin, num_bins):
        teem.nrrdHisto(self.nData._ctypesobj, nin._ctypesobj, None, None, num_bins, teem.nrrdTypeUInt)
        return
    
    def createHistoImage(self, height):
        nimg = nrd.Nrrd()
        teem.nrrdHistoDraw(nimg._ctypesobj, self.nData._ctypesobj, height, teem.AIR_TRUE, teem.AIR_NAN)
        return nimg
    
    def colormapGrayscale(self, nin):
        teem.nrrdApply1DLut(self.nImg._ctypesobj, nin._ctypesobj, self.rng, self.lut._ctypesobj,
                        teem.nrrdTypeUChar, teem.AIR_FALSE)
        return
    
    def getPixmap(self):
        nrrd = self.nImg
        localArray = nrd.ExtendedArray(nrrd)
        #localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_RGB888)
        localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_ARGB32)
        pixmap = QPixmap(localImage)
        return pixmap

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
    window = NrrdHistoDisplay(nrrd)
    window.show()
    window.positionWidget(window.infoWidget)
    splash.finish(window)
    
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
