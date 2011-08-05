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

class NrrdHistoDisplay(QMainWindow):
    def __init__(self, nrrd, nmap, parent=None):
        super(NrrdHistoDisplay, self).__init__(parent)
        
        self.setWindowTitle("Transformed Nrrd Visualizer")
        
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
        self.localScene = QGraphicsScene()
        self.localGrphView.setScene(self.localScene)
        
        self.nrrd = nrrd
        self.nmap = nmap
        
        aclLen = 0
        self.nacl = nrd.Nrrd()
        teem.nrrd1DIrregAclGenerate(self.nacl._ctypesobj, self.nmap._ctypesobj, aclLen)
        
        mappedImg = nrd.Nrrd()
        teem.nrrdApply1DIrregMap(mappedImg._ctypesobj, self.nrrd._ctypesobj, None, self.nmap._ctypesobj, self.nacl._ctypesobj, self.nmap._ctypesobj.contents.type, teem.AIR_FALSE)
        
        self.mquantImage = nrd.Nrrd()
        range = teem.nrrdRangeNew(teem.AIR_NAN, teem.AIR_NAN)
        teem.nrrdRangeSafeSet(range, mappedImg._ctypesobj, teem.AIR_FALSE)
        teem.nrrdQuantize(self.mquantImage._ctypesobj, mappedImg._ctypesobj, range, ctypes.c_uint(8))
        self.mquantImage.save("test.nrrd")
        teem.nrrdRangeNix(range)
        
        
        
        #self.nImg = convertToGrayscale(self.nrrd)
        #self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        
        # menu creation
        openAction = QAction("Open...", self)
        openAction.setShortcuts(QKeySequence.Open)
        openAction.setStatusTip("Open an existing file")
        openAction.triggered.connect(self.open)
        
        fileMenu = self.menuBar().addMenu("File")
        fileMenu.addAction(openAction)
                
    
    #def sizeHint(self):
        #s = self.pixmapItem.boundingRect().size().toSize()
        #return s
    
    def resizeEvent(self, e):
        super(NrrdHistoDisplay, self).resizeEvent(e)
    
    def open(self):
        (fileName, filter) = QFileDialog.getOpenFileName(self, "Open Image", os.getcwd(), "Text Files (*.nrrd)")
        if fileName == "":
            return
        
        nrrd = nrd.Nrrd()
        nrrd.load(fileName)
        
        self.nrrd = nrrd
        self.nImg = convertToGrayscale(self.nrrd)
        
        self.pixmapItem = QGraphicsPixmapItem(self.getPixmap(), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
        self.pixmapItem.setPixmap(self.getPixmap())
        
        rect = self.pixmapItem.boundingRect()
        self.localScene.setSceneRect(rect)
        self.adjust()
    
    def pixelClick(self, event):
        rect = self.pixmapItem.boundingRect()
        i_fast = int(event.pos().x())
        i_slow = int(event.pos().y())
        self.binLabel.setText("%i" % i_fast)
    
        #val = (ctypes.c_void_p)()
        #index = (ctypes.c_size_t * 1)(i_fast)
        #print teem.nrrdSample_nva(val, self.nData._ctypesobj, index)
        #self.binLabel.setText("%i" % val)

    def adjust(self):
        self.adjustSize()
        #self.positionWidget(self.infoWidget)

    def positionWidget(self, widget):
        w_frame = self.infoWidget.geometry()
        frame = self.geometry()
        if (w_frame.intersects(frame)):
            widget.move((frame.x() + frame.width() + 10), w_frame.y())
        return

    def getPixmap(self):
        nrrd = self.nImg
        localArray = nrd.ExtendedArray(nrrd)
        #localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_RGB888)
        localImage = QImage(localArray.data, nrrd._ctypesobj.contents.axis[1].size, nrrd._ctypesobj.contents.axis[2].size, QImage.Format_RGB32)
        pixmap = QPixmap(localImage)
        return pixmap


def createHistogram(nin, num_bins):
    nhist = nrd.Nrrd()
    teem.nrrdHisto(nhist._ctypesobj, nin._ctypesobj, None, None, num_bins, teem.nrrdTypeUInt)
    return nhist

def createHistoImage(nhist, height):
    nimg = nrd.Nrrd()
    teem.nrrdHistoDraw(nimg._ctypesobj, nhist._ctypesobj, height, teem.AIR_TRUE, teem.AIR_NAN)
    return nimg

def convertToGrayscale(nin):
    #nin_list = [nin._ctypesobj, nin._ctypesobj, nin._ctypesobj]
    #nin_array = (ctypes.POINTER(teem.Nrrd) * 3)(*nin_list)
    #nout = nrd.Nrrd()
    #teem.nrrdJoin(nout._ctypesobj, nin_array, 3, 0, teem.AIR_TRUE)
    
    byte_grey = nrd.ExtendedArray(nin)
    (h, w) = byte_grey.shape
    rgba = np.zeros((h, w, 4), dtype=np.uint8)
    for y in range(h):
        for x in range(w):
            rgba[y][x][0:3] = byte_grey[y][x]
            rgba[y][x][3] = 255
    nout = nrd.Nrrd()
    nout.fromNDArray(rgba)
    return nout

def main():
    app = QApplication(sys.argv)
    
    pixmap = QPixmap("splash.png")
    splash = QSplashScreen(pixmap)
    splash.show()
    
    (fileName, filter) = QFileDialog.getOpenFileName(None, "Select Image", os.getcwd(), "Text Files (*.nrrd)")
    if fileName == "":
        sys.exit()
    
    nrrd = nrd.Nrrd()
    nrrd.load(fileName)
    
    (fileName, filter) = QFileDialog.getOpenFileName(None, "Select Color Map", os.getcwd(), "Text Files (*.txt)")
    
    nmap = nrd.Nrrd()
    nmap.load(fileName)
    
    window = NrrdHistoDisplay(nrrd, nmap)
    window.show()
    #window.positionWidget(window.infoWidget)
    splash.finish(window)
    
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()