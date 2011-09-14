#!/usr/bin/env python

# teapotDemo.py:
# Sam Quinan - University of Chicago
# 3D Teapot Display with Mouse-Informed Camera Movement

import sys
import math
from Geom import *
from PySide import QtCore, QtGui, QtOpenGL

try:
    from OpenGL import GL
    from OpenGL import constants as CONST
    from OpenGL import GLU
    from OpenGL import GLUT
except ImportError:
    app = QtGui.QApplication(sys.argv)
    QtGui.QMessageBox.critical(None, "OpenGL hellogl",
                            "PyOpenGL must be installed to run this example.",
                            QtGui.QMessageBox.Ok | QtGui.QMessageBox.Default,
                            QtGui.QMessageBox.NoButton)
    sys.exit(1)

# helper functions
def affine(i,v,I,o,O):
    return (float(O)-float(o))*(float(v)-i)/(float(I)-float(i)) + float(o)

def delta(i,x,I,o,O):
    return (float(O)-float(o))*float(x)/(float(I)-float(i))

def inside(min, val, max):
    return min <= val and val < max

# wrapper display class (could also use QMainWindow)
class Window(QtGui.QWidget):
    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)

        self.glWidget = GLWidget()
        
        #hBoxLayout will line up all sub-widgets horizontaly
        mainLayout = QtGui.QHBoxLayout()
        mainLayout.addWidget(self.glWidget)
        self.setLayout(mainLayout)
        
        self.setWindowTitle(self.tr("3D Camera Demo"))
    
    # inherited function handles keyboard input (keyboard input response happens in main window, not in the widget like the mouse-click response)
    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_U:
            self.glWidget.fixUp = not self.glWidget.fixUp
        else:
            super(Window, self).keyPressEvent(event)
    
# QGLWidget subclass
class GLWidget(QtOpenGL.QGLWidget):
    def __init__(self, parent=None):
        QtOpenGL.QGLWidget.__init__(self, parent)
        
        self.cameraPos = Vec3([0.0,0.0,4.0])
        self.lookAt = Vec3([0.0,0.0,0.0])
        self.upVec = Vec3([0.0,1.0,0.0])
        self.fov = 45
        self.near = -2.5
        self.far = 2.5
        
        self.aspect = float(self.width())/float(self.height())
        
        self.objectTransform = None
        
        self.fixUp = False
        self.mode = '' 
        
        self.lastPos = QtCore.QPoint()
    
    # see sizeHints() section on wiki
    def minimumSizeHint(self):
        return QtCore.QSize(50, 50)
    
    def sizeHint(self):
        return QtCore.QSize(400, 400)
    
    def _update(self):
        self.upVec /= self.upVec.len()
        self.nn = self.lookAt - self.cameraPos
        eyeDist = self.nn.len()
        self.vspNear = eyeDist + self.near
        self.vspDist = eyeDist
        self.vspFar = eyeDist + self.far
        self.vMin = -math.tan(self.fov*math.pi/360.0)*(self.vspDist);
        self.vMax = -self.vMin
        self.uMin = self.vMin * self.aspect
        self.uMax = -self.uMin
        self.nn /= eyeDist
        self.uu = self.nn.cross(self.upVec)
        self.uu /= self.uu.len()
        self.vv = self.nn.cross(self.uu)
        if not self.fixUp:
            self.upVec = -self.vv
        return None
    
    def cameraUpdate(self):
        self._update()
        # camera
        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glLoadIdentity()
        GLU.gluLookAt(self.cameraPos[0], self.cameraPos[1],  self.cameraPos[2],
                      self.lookAt[0],    self.lookAt[1],     self.lookAt[2],
                      self.upVec[0],     self.upVec[1],      self.upVec[2])
    
    # main openGL code goes in the next 3 functions
    
    # same as openGL initialize
    def initializeGL(self):
        
        # background
        GL.glClearColor (0.0, 0.0, 0.0, 0.0)
        
        # fixed pipeline shade model
        GL.glShadeModel(GL.GL_SMOOTH)
        
        # camera
        self.cameraUpdate()
        
        # setup lighting
        light0_pos = CONST.GLfloat_4(1.0, 1.0, 3.0, 1.0)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_POSITION, light0_pos)
        
        diffuse = CONST.GLfloat_4(0.9, 0.9, 0.9, 1.0)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_DIFFUSE, diffuse)
        
        ambient = CONST.GLfloat_4(0.1, 0.1, 0.1, 1.0)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_AMBIENT, ambient)
        
        specular = CONST.GLfloat_4(1.0, 1.0, 1.0, 1.0)
        GL.glLightfv(GL.GL_LIGHT0, GL.GL_SPECULAR, ambient)
        
        GL.glEnable(GL.GL_LIGHTING)
        GL.glEnable(GL.GL_LIGHT0)
    
        # enable depth test and back-face culling
        GL.glEnable(GL.GL_DEPTH_TEST)
        GL.glEnable(GL.GL_CULL_FACE)
    
    # same as openGL draw
    def paintGL(self):
        
        GL.glClear(GL.GL_COLOR_BUFFER_BIT | GL.GL_DEPTH_BUFFER_BIT)
        
        GL.glMatrixMode(GL.GL_MODELVIEW)
        GL.glPushMatrix()
        
        # special note: OpenGL's default glFrontFace state assumes that front facing polygons (for the purpose of face culling) have vertices that wind counter clockwise when projected into window space. pyOpenGl's teapot is rendered with its front facing polygon vertices winding clockwise.
        GL.glFrontFace(GL.GL_CW)
        GL.glColor3f(9.0,0.1,0.1)
        GLUT.glutSolidTeapot(0.7)
        GL.glFrontFace(GL.GL_CCW)
        
        GL.glPopMatrix()

    # same as openGL window re-size code
    def resizeGL(self, width, height):
        
        self.aspect = float(width)/float(height)
        
        GL.glViewport(0, 0, width, height)
        self.updateProjection()
     
    def updateProjection (self):
        GL.glMatrixMode(GL.GL_PROJECTION)
        GL.glLoadIdentity()
        GLU.gluPerspective(self.fov, self.aspect, self.vspNear, self.vspFar)
    
    # sublcassing mouse event response
    def mousePressEvent(self, event):
        self.lastPos = QtCore.QPoint(event.pos())
        xf = affine(0, self.lastPos.x(), self.width(), -1, 1)
        yf = affine(0, self.lastPos.y(), self.height(), -1, 1)
        
        # handle button press, as follows:
        #        +---------------------------------------+
        #        | \          X  rotateV/translateU    / |
        #        |   \  . . . . . . . . . . . . . .  /   |
        #        |     :                           :     |
        #        |     :                           :     |
        #        |     :                           :     |
        #        |     :                           :  X rotateU/
        #        |     :     X rotateUV/           :    translateV
        #        |     :       translateUV         :     |
        #        |  X  :                           :     |
        #        |  fieldOfView/ClippingDepth      :     |
        #        |     :                           :     |
        #        |     :                           :     |
        #        |    / . . . . . . . . . . . . . . \    |
        #        |  /      X  rotateN/*other*        \   |
        #        +---------------------------------------+
        #
        #      *other*:
        #        |    / . . . . . . .+. . . . . . . \    |
        #        |  /   perspective  | translateN    \   |
        #        +-------------------+-------------------+
        
        marg = 0.8
        
        # determine right click vs left click
        if event.buttons() & QtCore.Qt.LeftButton:
            if event.modifiers() & QtCore.Qt.MetaModifier: #control pressed - standard mac modifier for right click (keyboard modifiers for clicks handled differently from other keyboard inputs)
                button = 2
            else:
                button = 0
        elif event.buttons() & QtCore.Qt.RightButton:
            button = 2
        
        if inside(-marg, xf, marg) and inside(-marg, yf, marg):
            # in the center of the window
            self.mode = ['rotateUV','','translateUV'][button]
        else:
            # in the margins of the window
            if yf < xf:
                if yf < -xf:
                    # top edge: rotate *around* V, or *along* U
                    self.mode = ['rotateV','','translateU'][button]
                else:
                    # right edge
                    self.mode = ['rotateU','','translateV'][button]
            elif yf < -xf:
                # left edge
                self.mode = ['fieldOfView','','clippingDepth'][button]
            else:
                if 0 == button:
                    self.mode = 'rotateN'
                elif xf < 0:
                    self.mode = 'perspective'
                else:
                    self.mode = 'translateN'
        #print "Viewer mode:", self.mode
        self.updateGL()
        
    def mouseMoveEvent(self, event):
        x = event.x()
        oldX = self.lastPos.x()
        y = event.y()
        oldY = self.lastPos.y()
            
        rspd = 1.7 # rotation speed
        ww = self.width()
        hh = self.height()
        dx = rspd*delta(0, x - oldX, ww-1, -1, 1)
        dy = rspd*delta(0, y - oldY, hh-1, -1, 1)
        du = delta(0, x - oldX, ww-1, self.uMin, self.uMax)
        dv = delta(0, y - oldY, hh-1, self.vMin, self.vMax)
            
        angle0 = math.atan2(oldX - ww/2.0, oldY - hh/2.0)
        angle1 = math.atan2(x         - ww/2.0, y         - hh/2.0)
        angle = angle0 - angle1
        if angle > math.pi:
            angle -= 2*math.pi
        elif angle < -math.pi:
            angle += 2*math.pi
    
        if 'rotateUV' == self.mode:
            toEye = self.cameraPos - self.lookAt
            newToEye = toEye - self.vspDist*dx*self.uu - self.vspDist*dy*self.vv
            self.cameraPos = self.lookAt + (toEye.len()/newToEye.len())*newToEye
        elif 'rotateU' == self.mode:
            toEye = self.cameraPos - self.lookAt
            newToEye = toEye - self.vspDist*dy*self.vv
            newToEye *= toEye.len()/newToEye.len()
            self.cameraPos = self.lookAt + newToEye
        elif 'rotateV' == self.mode:
            toEye = self.cameraPos - self.lookAt
            if self.fixUp:
                toEyeUp = toEye.dot(self.upVec)*self.upVec
                toEyeSub = toEye - toEyeUp
                newToEyeSub = toEyeSub - self.vspDist*dx*self.uu
                newToEyeSub -= newToEyeSub.dot(self.upVec)*self.upVec
                newToEyeSub *= toEyeSub.len()/newToEyeSub.len()
                newToEye = newToEyeSub + toEyeUp
            else:
                newToEye = toEye - self.vspDist*dx*self.uu
            newToEye *= toEye.len()/newToEye.len()
            self.cameraPos = self.lookAt + newToEye
        elif 'rotateN' == self.mode:
            if self.fixUp:
                print "Can\'t rotate in-plane with fixUp on"
            else:
                self.upVec = -math.cos(angle)*self.vv - math.sin(angle)*self.uu
        elif 'translateUV' == self.mode:
            offset = du*self.uu + dv*self.vv
            self.cameraPos -= offset
            self.lookAt -= offset
        elif 'translateU' == self.mode:
            self.cameraPos -= du*self.uu
            self.lookAt -= du*self.uu
        elif 'translateV' == self.mode:
            self.cameraPos -= dv*self.vv
            self.lookAt -= dv*self.vv
        elif 'translateN' == self.mode:
            self.cameraPos += 0.2*angle*self.vspDist*self.nn
            self.lookAt += 0.2*angle*self.vspDist*self.nn
        elif 'fieldOfView' == self.mode:
            self.fov *= math.pow(2,-angle);
            self.fov = min(179, self.fov);
        elif 'clippingDepth' == self.mode:
            self.near *= pow(2,angle);
            self.near = -min(self.vspDist*0.999, -self.near)
            self.far *= pow(2,angle);
        elif 'perspective' == self.mode:
            toEye = self.cameraPos - self.lookAt
            oldDist = toEye.len()
            toEye /= oldDist
            newDist = oldDist*pow(2,-angle)
            self.fov = 360*math.atan2(oldDist*math.tan(math.pi*self.fov/360), newDist)/math.pi
            self.cameraPos = self.lookAt + newDist*toEye

        self.cameraUpdate()
        self.updateProjection()   
        self.lastPos = QtCore.QPoint(event.pos())
        self.updateGL()
    
    def mouseReleaseEvent(self, event):
        self.mode = ''
        self.updateGL()

if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec_())
