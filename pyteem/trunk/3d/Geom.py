
### pyDeft: Python/OpenGL/Teem, version 0.0.7

import math

def affine(i,v,I,o,O):
  return (1.0*O-o)*(1.0*v-i)/(1.0*I-i) + 1.0*o

def lerp(w,a,b):
  return (1.0-w)*a + 1.0*w*b

# based on http://blog.kernphase.com/?p=331
class Vec3:
  def __init__(self, data):
    self.data = [data[0],data[1],data[2]]
  def __repr__(self):
    return repr(self.data)  
  def __add__(self, other):
    data = [self.data[0] + other.data[0],
            self.data[1] + other.data[1],
            self.data[2] + other.data[2]]
    return Vec3(data)  
  def __iadd__(self, other):
    return Vec3.__add__(self, other)
  def __sub__(self, other):
    data = [self.data[0] - other.data[0],
            self.data[1] - other.data[1],
            self.data[2] - other.data[2]]
    return Vec3(data)  
  def __isub__(self, other):
    return Vec3.__sub__(self, other)
  def __neg__(self):
    return Vec3([-self.data[0], -self.data[1], -self.data[2]])
  def __mul__(self, scl):
    # scalar multiplication "vec * scl", in that order
    return Vec3([self.data[0]*scl, self.data[1]*scl, self.data[2]*scl])
  def __rmul__(self, scl):
    return Vec3.__mul__(self, scl)
  def __div__(self, scl):
    # division by scalar "vec/scl", in that order
    return Vec3([self.data[0]/scl, self.data[1]/scl, self.data[2]/scl])
  def cross(self, other):
    data = [self.data[1]*other.data[2] - self.data[2]*other.data[1],
            self.data[2]*other.data[0] - self.data[0]*other.data[2],
            self.data[0]*other.data[1] - self.data[1]*other.data[0]]
    return Vec3(data)  
  def dot(self, other):
    return (self.data[0]*other.data[0] \
          + self.data[1]*other.data[1] \
          + self.data[2]*other.data[2])
  def min(self, other):
    data = [min(self.data[0], other.data[0]),
            min(self.data[1], other.data[1]),
            min(self.data[2], other.data[2])]
    return Vec3(data)  
  def max(self, other):
    data = [max(self.data[0], other.data[0]),
            max(self.data[1], other.data[1]),
            max(self.data[2], other.data[2])]
    return Vec3(data)  
  def __getitem__(self, idx):
    return self.data[idx]
  def len(self):
    sum = self.data[0]*self.data[0] \
        + self.data[1]*self.data[1] \
        + self.data[2]*self.data[2]
    return math.sqrt(sum)
  def length(self):
    return Vec3.len(self)
  def __str__(self):
    return '[%g,%g,%g]' % (self.data[0],self.data[1],self.data[2])

