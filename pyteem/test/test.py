#!/usr/bin/env python

# test.py: display nrrd image
# Sam Quinan

# Import PySide classes
import sys
from PySide.QtCore import *
from PySide.QtGui import *

# Import PIL and numpy for getting array data
#import Image, numpy as np
import Nrrd as nrd
import numpy as np


# Create a Qt application 
app = QApplication(sys.argv)

# Create a Label
imageLabel = QLabel()
# Get Image
nrrd = nrd.Nrrd()
nrrd.load("Disney.nrrd")
arr = nrrd.ExtendedArray(nrrd) 
img = QImage(arr.data, 400, 400, QImage.Format_RGB888)
# Display Image on Label
imageLabel.setPixmap(QPixmap.fromImage(img))
imageLabel.adjustSize()
imageLabel.show()

# Enter Qt application main loop
sys.exit(app.exec_())