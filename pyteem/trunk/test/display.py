#!/usr/bin/env python

# display.py:
# Sam Quinan

# Import PySide classes
import sys
#from PySide.QtCore import *
from PySide.QtGui import QApplication, QMainWindow, QImage, QPixmap, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem

# Import Nrrd Interface
import Nrrd as nrd


class NrrdDisplay(QMainWindow):
    def __init__(self, nrrd, fmt, parent=None):
        super(NrrdDisplay, self).__init__(parent)
        
        self.setWindowTitle("Nrrd Display")
        
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
        
        self.localNrrd = nrrd
        self.localArray = nrd.ExtendedArray(self.localNrrd)
        self.localImage = QImage(self.localArray.data, self.localNrrd._ctypesobj.contents.axis[2].size, self.localNrrd._ctypesobj.contents.axis[1].size, fmt)
        
        self.localScene = QGraphicsScene()
        self.pixmapItem = QGraphicsPixmapItem(QPixmap(self.localImage), None, self.localScene)
        self.localGrphView.setScene(self.localScene)
        
        self.pixmapItem.mousePressEvent = self.pixelClick
    
    def pixelClick(self, event):
        print event.pos()
        
        
def main():
    app = QApplication(sys.argv)
    
    nrrd = nrd.Nrrd()
    nrrd.load("Disney.nrrd")
    
    frame = NrrdDisplay(nrrd, QImage.Format_RGB888)
    frame.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()