#!/usr/bin/env python

# display.py:
# Sam Quinan

# Import PySide classes
import sys, os
from PySide.QtCore import Qt, Slot, QSize, QPoint
from PySide.QtGui import QApplication, QMainWindow, QImage, QPixmap, QGraphicsView, QGraphicsScene, QGraphicsPixmapItem, QDialogButtonBox, QDockWidget, QLabel, QGroupBox, QFormLayout, QAction, QKeySequence, QFileDialog

# Import Nrrd Interface
import Nrrd as nrd
import Image, numpy as np


class NrrdDisplay(QMainWindow):
    def __init__(self, nrrd, fmt, parent=None):
        super(NrrdDisplay, self).__init__(parent)
        
        self.setWindowTitle("Nrrd Display")
        
        openAction = QAction("Open...", self)
        openAction.setShortcuts(QKeySequence.Open)
        openAction.setStatusTip("Open an existing file")
        openAction.triggered.connect(self.open)
        
        fileMenu = self.menuBar().addMenu("File")
        fileMenu.addAction(openAction)
        
        
        self.localGrphView = QGraphicsView()
        self.setCentralWidget(self.localGrphView)
                
        #buttonBox = QDialogButtonBox(Qt.Vertical)
        #buttonA = buttonBox.addButton("Image A", QDialogButtonBox.ActionRole)
        #buttonB = buttonBox.addButton("Image B", QDialogButtonBox.ActionRole)
        #
        #buttonA.clicked.connect(self.clickA)
        #buttonB.clicked.connect(self.clickB)
        #
        #dockWidget = QDockWidget("Select Image:", self)
        #dockWidget.setAllowedAreas(Qt.LeftDockWidgetArea | Qt.RightDockWidgetArea)
        #dockWidget.setFloating(False)
        #dockWidget.setFeatures(QDockWidget.DockWidgetMovable)
        #dockWidget.setWidget(buttonBox)
        #self.addDockWidget(Qt.RightDockWidgetArea, dockWidget)
        
        groupBox = QGroupBox()
        self.pixelCoordLabel = QLabel("(n/a, n/a)")
        self.worldCoordLabel = QLabel("(n/a, n/a)")
        boxLayout = QFormLayout()
        boxLayout.addRow("Pixel Coords:", self.pixelCoordLabel)
        boxLayout.addRow("World Coords:", self.worldCoordLabel)
        groupBox.setLayout(boxLayout)
        
        infoWidget = QDockWidget("Information", self)
        infoWidget.setAllowedAreas(Qt.NoDockWidgetArea)
        infoWidget.setFloating(True)
        infoWidget.setFeatures(QDockWidget.DockWidgetFloatable)
        infoWidget.setWidget(groupBox)
        self.addDockWidget(Qt.RightDockWidgetArea, infoWidget) #using Qt.NoDockWidetArea here spits out warning... 
        self.infoWidget = infoWidget
        
        infoWidget.topLevelChanged.connect(self.adjust)
        
        self.setCorner(Qt.TopLeftCorner, Qt.LeftDockWidgetArea)
        self.setCorner(Qt.BottomLeftCorner, Qt.LeftDockWidgetArea)
        self.setCorner(Qt.TopRightCorner, Qt.RightDockWidgetArea)
        self.setCorner(Qt.BottomRightCorner, Qt.RightDockWidgetArea)
        
        self.fmt = fmt
        
        self.localScene = QGraphicsScene()
        self.localGrphView.setScene(self.localScene)
        
        self.localNrrd = nrrd
        self.localArray = nrd.ExtendedArray(self.localNrrd)
        self.localImage = QImage(self.localArray.data, self.localNrrd._ctypesobj.contents.axis[1].size, self.localNrrd._ctypesobj.contents.axis[2].size, fmt)
        self.pixmapItem = QGraphicsPixmapItem(QPixmap(self.localImage), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
    def sizeHint(self):
        s = self.pixmapItem.boundingRect().size().toSize()
        return s
    
    def open(self):
        (fileName, filter) = QFileDialog.getOpenFileName(self, "Open Image", os.getcwd(), "Image Files (*.png)")
        self.localNrrd.load(fileName)
        self.localArray = nrd.ExtendedArray(self.localNrrd)
        self.localImage = QImage(self.localArray.data, self.localNrrd._ctypesobj.contents.axis[1].size, self.localNrrd._ctypesobj.contents.axis[2].size, self.fmt)
        
        self.localScene.removeItem(self.pixmapItem)
        self.pixmapItem = QGraphicsPixmapItem(QPixmap(self.localImage), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
        
        rect = self.pixmapItem.boundingRect()
        self.localScene.setSceneRect(rect)
        self.adjust()
    
    def pixelClick(self, event):
        rect = self.pixmapItem.boundingRect()
        
        i_fast = int(round(self.localNrrd._ctypesobj.contents.axis[1].size * (event.pos().x() - rect.left()) / rect.width()))
        i_slow = int(round(self.localNrrd._ctypesobj.contents.axis[2].size * (event.pos().y() - rect.top()) / rect.height()))
        
        self.pixelCoordLabel.setText("(%i, %i)" % (i_fast, i_slow))

    def doNothing(self, event):
        pass
    
    def clickA(self):
        self.localNrrd.load("a.png")
        self.localArray = nrd.ExtendedArray(self.localNrrd)
        self.localImage = QImage(self.localArray.data, self.localNrrd._ctypesobj.contents.axis[1].size, self.localNrrd._ctypesobj.contents.axis[2].size, self.fmt)
        
        self.localScene.removeItem(self.pixmapItem)
        self.pixmapItem = QGraphicsPixmapItem(QPixmap(self.localImage), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick

    def clickB(self):
        self.localNrrd.load("b.png")
        self.localArray = nrd.ExtendedArray(self.localNrrd)
        self.localImage = QImage(self.localArray.data, self.localNrrd._ctypesobj.contents.axis[1].size, self.localNrrd._ctypesobj.contents.axis[2].size, self.fmt)
        
        self.localScene.removeItem(self.pixmapItem)
        self.localScene.addItem(self.pixmapItem)
        self.pixmapItem = QGraphicsPixmapItem(QPixmap(self.localImage), None, self.localScene)
        self.pixmapItem.mousePressEvent = self.pixelClick
    
    def adjust(self):
        self.adjustSize()
        self.positionWidget(self.infoWidget)

    def positionWidget(self, widget):
        w_frame = self.infoWidget.geometry()
        frame = self.geometry()
        if (w_frame.intersects(frame)):
            widget.move((frame.x() + frame.width() + 10), w_frame.y())
        return
        
        
        
        
def main():
    app = QApplication(sys.argv)
    
    nrrd = nrd.Nrrd()
    nrrd.load("c.png")
    frame = NrrdDisplay(nrrd, QImage.Format_RGB888)
    frame.show()
    frame.positionWidget(frame.infoWidget)
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()