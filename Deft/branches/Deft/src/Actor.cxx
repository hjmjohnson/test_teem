/*
  Deft: experiments in minimalist scientific visualization 
  Copyright (C) 2005  Gordon Kindlmann

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Deft.H"
#include "Actor.H"

/* 
** learned: don't be surprised that you're not seeing anything on
** screen when you leave the 4rth (W) coordinate of vertices 0 
**
** don't be surprised when geometry is all black, and you've left
** the color as (0,0,0)
**
** call glVertex* AFTER glNormal*, glColor*, etc.
**
** at least on my old powerbook, a display list of GL_POLYGON or
** GL_LINE_LOOP (one per face) was *NO* speed-up.  Display lists of
** GL_TRIANGLE_STRIP screamed!
*/

namespace Deft {

Actor::Actor(limnObject *lobj) {

  this->lobj = lobj;
  this->lpld = NULL;
  wireFrame = false;
  visible = true;
  useColor = true;
  useNormalize = true;
  useActorT = true;
  ELL_4M_IDENTITY_SET(actorT);
  compilingDisplayList = false;
  staticDrawCount = 0;
  displayList = 0;
}

Actor::Actor(limnPolyData *lpld) {

  this->lobj = NULL;
  this->lpld = lpld;
  wireFrame = false;
  visible = true;
  useColor = true;
  useNormalize = true;
  useActorT = true;
  ELL_4M_IDENTITY_SET(actorT);
  compilingDisplayList = false;
  staticDrawCount = 0;
  displayList = 0;
}

Actor::~Actor() {
  fprintf(stderr, "%s: nixing object @ %p/%p\n",
          "Actor::~Actor()", lobj, lpld);
  lobj = limnObjectNix(lobj);
  lpld = limnPolyDataNix(lpld);
}

void
Actor::changed() {
  staticDrawCount = 0;
}

const unsigned int DeftDisplayListDrawCount = 2;

void
Actor::draw() {
  char me[]="Actor::draw";
  limnFace *face;
  limnVertex *vertex;
  limnVrt *vrt;
  int glWhat;
  const GLenum glptFilled[LIMN_PRIMITIVE_MAX+1] = {
    GL_POINTS,         /* 0: limnPrimitiveUnknown */
    GL_TRIANGLES,      /* 1: limnPrimitiveTriangles */
    GL_TRIANGLE_STRIP, /* 2: limnPrimitiveTriangleStrip */
    GL_TRIANGLE_FAN,   /* 3: limnPrimitiveTriangleFan */
    GL_QUADS           /* 4: limnPrimitiveQuads */
  };
  const GLenum glptWireframe[LIMN_PRIMITIVE_MAX+1] = {
    GL_POINTS,         /* 0: limnPrimitiveUnknown */
    GL_LINE_LOOP,      /* 1: limnPrimitiveTriangles */
    GL_LINE_STRIP,     /* 2: limnPrimitiveTriangleStrip */
    GL_LINE_LOOP,      /* 3: limnPrimitiveTriangleFan */
    GL_LINE_LOOP       /* 4: limnPrimitiveQuads */
  };
  const GLenum *glpt;

  if (!( visible && (lobj || lpld) )) {
    /* there's nothing to render here */
    return;
  }

  if (!compilingDisplayList) {
    if (staticDrawCount == 0 && displayList) {
      // display list is stale, kill it
      fprintf(stderr, "display list is stale, kill it\n");
      glDeleteLists(displayList, 1);
      displayList = 0;
    } else if (staticDrawCount == DeftDisplayListDrawCount
               && !displayList) {
      this->compile();
    }
  }
  if (!compilingDisplayList && displayList) {
    glCallList(displayList);
    return;
  }

  if (compilingDisplayList) {
    fprintf(stderr, "\n%s: compiling display list %u ... ", me, displayList);
    fflush(stderr);
  }
  if (lobj) {
    vertex = lobj->vert;
    vrt = NULL;
  } else {
    vertex = NULL;
    vrt = lpld->vert;
  }
  if (useActorT) {
    float tmpMat[16];
    glPushMatrix();
    ELL_4M_TRANSPOSE(tmpMat, actorT);
    glMultMatrixf(tmpMat);
  }
  if (useNormalize) {
    glEnable(GL_NORMALIZE);
  }
  if (useColor) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  }
  if (!compilingDisplayList) {
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    if (useColor) {
      glEnableClientState(GL_COLOR_ARRAY);
    }
    if (lobj) {
      glVertexPointer(4, GL_FLOAT, sizeof(limnVertex), vertex[0].world);
      glNormalPointer(GL_FLOAT, sizeof(limnVertex), vertex[0].worldNormal);
      if (useColor) {
        glColorPointer(4, GL_FLOAT, sizeof(limnVertex), vertex[0].rgba);
      }
    } else {
      glVertexPointer(4, GL_FLOAT, sizeof(limnVrt), vrt[0].xyzw);
      glNormalPointer(GL_FLOAT, sizeof(limnVrt), vrt[0].norm);
      if (useColor) {
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(limnVrt), vrt[0].rgba);
      }
    }
  }
  if (lobj) {
    // we're drawing a limnObject
    glWhat = wireFrame ? GL_LINE_LOOP : GL_POLYGON;
    for (unsigned int faceIdx=0; faceIdx<lobj->faceNum; faceIdx++) {
      face = lobj->face + faceIdx;
      if (!compilingDisplayList) {
        glDrawElements(glWhat, face->sideNum, GL_UNSIGNED_INT, face->vertIdx);
      } else {
        glBegin(glWhat);
        for (unsigned int vii=0; vii<face->sideNum; vii++) {
          glNormal3fv(vertex[face->vertIdx[vii]].worldNormal);
          if (useColor) {
            glColor4fv(vertex[face->vertIdx[vii]].rgba);
          }
          glVertex4fv(vertex[face->vertIdx[vii]].world);
        }
        glEnd();
      }
    }
  } else {
    // we're drawing a limnPolyData
    unsigned int vertCnt, vertIdx = 0;
    glpt = wireFrame ? glptWireframe : glptFilled;
    if (!compilingDisplayList) {
      for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
        vertCnt = lpld->vcnt[primIdx];
        glWhat = glpt[lpld->type[primIdx]];
        glDrawElements(glWhat, vertCnt, GL_UNSIGNED_INT, lpld->indx + vertIdx);
        vertIdx += vertCnt;
      }
    } else {
      for (unsigned int primIdx=0; primIdx<lpld->primNum; primIdx++) {
        vertCnt = lpld->vcnt[primIdx];
        glWhat = glpt[lpld->type[primIdx]];
        glBegin(glWhat);
        for (unsigned int vii=0; vii<vertCnt; vii++) {
          glNormal3fv(vrt[(lpld->indx + vertIdx)[vii]].norm);
          if (useColor) {
            glColor4ubv(vrt[(lpld->indx + vertIdx)[vii]].rgba);
          }
          glVertex4fv(vrt[(lpld->indx + vertIdx)[vii]].xyzw);
        }
        glEnd();
        vertIdx += vertCnt;
      }
    }
  }
  if (!compilingDisplayList) {
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    if (useColor) {
      glDisableClientState(GL_COLOR_ARRAY);
    }
  }
  if (useColor) {
    glDisable(GL_COLOR_MATERIAL);
  }
  if (useNormalize) {
    glDisable(GL_NORMALIZE);
  }
  if (useActorT) {
    glPopMatrix();
  }
  if (compilingDisplayList) {
    glEndList();
    fprintf(stderr, "done\n\n");
  } else {
    staticDrawCount++;
  }
  return;
}

// HEY when you call this directly you're responsible for managing
// the display lists ...
unsigned int
Actor::compile() {

  displayList = glGenLists(1);
  glNewList(displayList, GL_COMPILE);
  compilingDisplayList = true;
  this->draw();  // used to be recursion
  compilingDisplayList = false;
  return displayList;
}

void
Actor::boundsGet(float min[3], float max[3]) {
  unsigned int vi, vertNum;
  double vertB[4], vertC[4];
  limnVertex *vertex;
  limnVrt *vrt;

  if (lobj || lpld) {
    if (lobj) {
      vertex = lobj->vert;
      vrt = NULL;
      vertNum = lobj->vertNum;
    } else {
      vertex = NULL;
      vrt = lpld->vert;
      vertNum = lpld->vertNum;
    }
    for (vi=0; vi<vertNum; vi++) {
      if (lobj) {
        ELL_4MV_MUL(vertB, actorT, vertex[vi].world);
      } else {
        ELL_4MV_MUL(vertB, actorT, vrt[vi].xyzw);
      }
      ELL_4V_HOMOG(vertC, vertB);
      if (!vi) {
        ELL_3V_COPY(min, vertC);
        ELL_3V_COPY(max, vertC);
      } else {
        ELL_3V_MIN(min, min, vertC);
        ELL_3V_MAX(max, max, vertC);
      }
    }
  } else {
    ELL_3V_SET(min, 0, 0, 0);
    ELL_3V_SET(max, 0, 0, 0);
  }
  return;
}

} /* namespace Deft */
