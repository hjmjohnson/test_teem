/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998 University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "limn.h"

/*
** "16simple": 16 bit representation based on 
** <http://www.gamedev.net/reference/articles/article1252.asp>
**
** info: [z sign] [y sign] [x sign] [y component] [x component]
** bits:    1        1        1          7             6
**
** despite appearances, this is isotropic in X and Y
*/

void
_limnQN16simple_QNtoV_f(float *vec, int qn) {
  int xi, yi;
  float x, y, z, n;
  
  xi = qn & 0x3F;
  yi = (qn >> 6) & 0x7F;
  if (xi + yi >= 127) {
    xi = 127 - xi;
    yi = 127 - yi;
  }
  x = xi/126.0;
  y = yi/126.0;
  z = 1.0 - x - y;
  x = (qn & 0x2000) ? -x : x;
  y = (qn & 0x4000) ? -y : y;
  z = (qn & 0x8000) ? -z : z;
  n = 1.0/sqrt(x*x + y*y + z*z);
  vec[0] = x*n;
  vec[1] = y*n;
  vec[2] = z*n;
}

int
_limnQN16simple_VtoQN_f(float *vec) {
  float x, y, z, L, w;
  int sgn = 0;
  int xi, yi;

  x = vec[0];
  y = vec[1];
  z = vec[2];
  if (x < 0) {
    sgn |= 0x2000;
    x = -x;
  }
  if (y < 0) {
    sgn |= 0x4000;
    y = -y;
  }
  if (z < 0) {
    sgn |= 0x8000;
    z = -z;
  }
  L = x + y + z;
  if (!L) {
    return 0;
  }
  w = 126.0/L;
  xi = x*w;
  yi = y*w;
  if (xi >= 64) {
    xi = 127 - xi;
    yi = 127 - yi;
  }
  return sgn | (yi << 6) | xi;
}

/* ----------------------------------------------------------------  */

void
_limnQN16border1_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  float u, v, x, y, z, n;
  
  ui = qn & 0xFF;
  vi = qn >> 8;
  u = AIR_AFFINE(0.5, ui, 254.5, -0.5, 0.5);
  v = AIR_AFFINE(0.5, vi, 254.5, -0.5, 0.5);
  x =  u + v;
  y =  u - v;
  z = 1 - AIR_ABS(x) - AIR_ABS(y);
  z *= (((ui ^ vi) & 0x01) << 1) - 1;
  n = 1.0/sqrt(x*x + y*y + z*z);
  vec[0] = x*n; 
  vec[1] = y*n; 
  vec[2] = z*n;
}

int
_limnQN16border1_VtoQN_f(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  char me[]="limnQNVto16PB1";
  
  x = vec[0];
  y = vec[1];
  z = vec[2];
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z);
  if (!L) {
    return 0;
  }
  x /= L;
  y /= L;
  u = x + y;
  v = x - y;
  AIR_INDEX(-1, u, 1, 254, ui); ui++;
  AIR_INDEX(-1, v, 1, 254, vi); vi++;
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/128.0) {
    ui -= (((ui >> 7) & 0x01) << 1) - 1;
  } 
  else if (zi && z < -1.0/128.0) {
    vi -= (((vi >> 7) & 0x01) << 1) - 1;
  }
  zi = (ui ^ vi) & 0x01;
  if (!zi && z > 1.0/127.0) {
    printf("%s: panic01\n", me);
  } 
  else if (zi && z < -1.0/127.0) {
    printf("%s: panic02\n", me);
  }
  return (vi << 8) | ui;
}

/* ----------------------------------------------------------------  */

  /*
    x =  [  1.0   1.0 ] u
    y    [  1.0  -1.0 ] v
  */

  /*
    u =  [  0.5   0.5 ] x
    v    [  0.5  -0.5 ] y
  */

  /* xor of low bits == 0 --> z<0 ; xor == 1 --> z>0 */

/* May 11 2003 GK (visually) verified that except at equator seam,
   the error due to quantization is unbiased */

#define _EVEN_QtoV(HNB, vec, qn) \
  ui = (qn) & ((1<<HNB)-1); \
  vi = ((qn) >> HNB) & ((1<<HNB)-1); \
  u = AIR_AFFINE(0, ui, ((1<<HNB)-1), -0.5, 0.5); \
  v = AIR_AFFINE(0, vi, ((1<<HNB)-1), -0.5, 0.5); \
  x =  u + v; \
  y =  u - v; \
  z = 1 - AIR_ABS(x) - AIR_ABS(y); \
  /* would this be better served by a branch? */ \
  z *= (((ui ^ vi) & 0x01) << 1) - 1; \
  n = 1.0/sqrt(x*x + y*y + z*z); \
  (vec)[0] = x*n; \
  (vec)[1] = y*n; \
  (vec)[2] = z*n

#define _EVEN_VtoQ(HNB, vec) \
  x = (vec)[0]; \
  y = (vec)[1]; \
  z = (vec)[2]; \
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z); \
  if (!L) { \
    return 0; \
  } \
  x /= L; \
  y /= L; \
  if (z > 0) { \
    AIR_INDEX(-1.0, x, 1.0, ((1<<HNB)-1), xi); \
    AIR_INDEX(-1.0 - 1.0/((1<<HNB)-1), y, 1.0 + 1.0/((1<<HNB)-1), \
              (1<<HNB), yi); \
    ui = xi + yi - ((1<<(HNB-1))-1); \
    vi = xi - yi + (1<<(HNB-1)); \
  } \
  else { \
    AIR_INDEX(-1.0 - 1.0/((1<<HNB)-1), x, 1.0 + 1.0/((1<<HNB)-1), \
              (1<<HNB), xi); \
    AIR_INDEX(-1, y, 1, ((1<<HNB)-1), yi); \
    ui = xi + yi - ((1<<(HNB-1))-1); \
    vi = xi - yi + ((1<<(HNB-1))-1); \
  } \
  return (vi << HNB) | ui

#define _16_QtoV(vec, qn) \
  ui = (qn) & 0xFF; \
  vi = ((qn) >> 8) & 0xFF; \
  u = AIR_AFFINE(0, ui, 255, -0.5, 0.5); \
  v = AIR_AFFINE(0, vi, 255, -0.5, 0.5); \
  x =  u + v; \
  y =  u - v; \
  z = 1 - AIR_ABS(x) - AIR_ABS(y); \
  /* would this be better served by a branch? */ \
  z *= (((ui ^ vi) & 0x01) << 1) - 1; \
  n = 1.0/sqrt(x*x + y*y + z*z); \
  (vec)[0] = x*n; \
  (vec)[1] = y*n; \
  (vec)[2] = z*n

#define _16_VtoQ(vec) \
  x = (vec)[0]; \
  y = (vec)[1]; \
  z = (vec)[2]; \
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z); \
  if (!L) { \
    return 0; \
  } \
  x /= L; \
  y /= L; \
  if (z > 0) { \
    AIR_INDEX(-1.0, x, 1.0, 255, xi); \
    AIR_INDEX(-1.0 - 1.0/255, y, 1.0 + 1.0/255, 256, yi); \
    ui = xi + yi - 127; \
    vi = xi - yi + 128; \
  } \
  else { \
    AIR_INDEX(-1.0 - 1.0/255, x, 1.0 + 1.0/255, 256, xi); \
    AIR_INDEX(-1, y, 1, 255, yi); \
    ui = xi + yi - 127; \
    vi = xi - yi + 127; \
  }

void
_limnQN16checker_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(8, vec, qn);
}

void
_limnQN16checker_QNtoV_d(double *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(8, vec, qn);
}

int
_limnQN16checker_VtoQN_f(float *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(8, vec);
}

int
_limnQN16checker_VtoQN_d(double *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;

  _EVEN_VtoQ(8, vec);
}

/* ----------------------------------------------------------------  */

/* 15 bit --> HNB == 7 */

#define _ODD_QtoV(HNB, vec, qn) \
  ui = qn & ((1<<HNB)-1); \
  vi = (qn >> HNB) & ((1<<HNB)-1); \
  zi = (qn >> (2*HNB)) & 0x01; \
  u = AIR_AFFINE(-0.5, ui, ((1<<HNB)-1)+0.5, -0.5, 0.5); \
  v = AIR_AFFINE(-0.5, vi, ((1<<HNB)-1)+0.5, -0.5, 0.5); \
  x =  u + v; \
  y =  u - v; \
  z = 1 - AIR_ABS(x) - AIR_ABS(y); \
  z *= (zi << 1) - 1; \
  n = 1.0/sqrt(x*x + y*y + z*z); \
  vec[0] = x*n; \
  vec[1] = y*n; \
  vec[2] = z*n

#define _ODD_VtoQ(HNB, vec) \
  x = vec[0]; \
  y = vec[1]; \
  z = vec[2]; \
  L = AIR_ABS(x) + AIR_ABS(y) + AIR_ABS(z); \
  if (!L) { \
    return 0; \
  } \
  x /= L; \
  y /= L; \
  u = x + y; \
  v = x - y; \
  AIR_INDEX(-1, u, 1, (1<<HNB), ui); \
  AIR_INDEX(-1, v, 1, (1<<HNB), vi); \
  zi = (z > 0); \
  return (zi << (2*HNB)) | (vi << HNB) | ui


void
_limnQN15checker_QNtoV_f(float *vec, int qn) {
  int ui, vi, zi;
  float u, v, x, y, z, n;
  
  _ODD_QtoV(7, vec, qn);
}

void
_limnQN15checker_QNtoV_d(double *vec, int qn) {
  int ui, vi, zi;
  double u, v, x, y, z, n;
  
  _ODD_QtoV(7, vec, qn);
}

int
_limnQN15checker_VtoQN_f(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(7, vec);
}

int
_limnQN15checker_VtoQN_d(double *vec) {
  double L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(7, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN14checker_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(7, vec, qn);
}

void
_limnQN14checker_QNtoV_d(double *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(7, vec, qn);
}

int
_limnQN14checker_VtoQN_f(float *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;

  _EVEN_VtoQ(7, vec);
}

int
_limnQN14checker_VtoQN_d(double *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(7, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN13checker_QNtoV_f(float *vec, int qn) {
  int ui, vi, zi;
  float u, v, x, y, z, n;
  
  _ODD_QtoV(6, vec, qn);
}

void
_limnQN13checker_QNtoV_d(double *vec, int qn) {
  int ui, vi, zi;
  double u, v, x, y, z, n;
  
  _ODD_QtoV(6, vec, qn);
}

int
_limnQN13checker_VtoQN_f(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(6, vec);
}

int
_limnQN13checker_VtoQN_d(double *vec) {
  double L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(6, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN12checker_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(6, vec, qn);
}

void
_limnQN12checker_QNtoV_d(double *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(6, vec, qn);
}

int
_limnQN12checker_VtoQN_f(float *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(6, vec);
}

int
_limnQN12checker_VtoQN_d(double *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(6, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN11checker_QNtoV_f(float *vec, int qn) {
  int ui, vi, zi;
  float u, v, x, y, z, n;
  
  _ODD_QtoV(5, vec, qn);
}

void
_limnQN11checker_QNtoV_d(double *vec, int qn) {
  int ui, vi, zi;
  double u, v, x, y, z, n;
  
  _ODD_QtoV(5, vec, qn);
}

int
_limnQN11checker_VtoQN_f(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(5, vec);
}

int
_limnQN11checker_VtoQN_d(double *vec) {
  double L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(5, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN10checker_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(5, vec, qn);
}

void
_limnQN10checker_QNtoV_d(double *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(5, vec, qn);
}

int
_limnQN10checker_VtoQN_f(float *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(5, vec);
}

int
_limnQN10checker_VtoQN_d(double *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(5, vec);
}

/* ----------------------------------------------------------- */
void
_limnQN9checker_QNtoV_f(float *vec, int qn) {
  int ui, vi, zi;
  float u, v, x, y, z, n;
  
  _ODD_QtoV(4, vec, qn);
}

void
_limnQN9checker_QNtoV_d(double *vec, int qn) {
  int ui, vi, zi;
  double u, v, x, y, z, n;
  
  _ODD_QtoV(4, vec, qn);
}

int
_limnQN9checker_VtoQN_f(float *vec) {
  float L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(4, vec);
}

int
_limnQN9checker_VtoQN_d(double *vec) {
  double L, u, v, x, y, z;
  int ui, vi, zi;
  
  _ODD_VtoQ(4, vec);
}

/* ----------------------------------------------------------- */

void
_limnQN8checker_QNtoV_f(float *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(4, vec, qn);
}

void
_limnQN8checker_QNtoV_d(double *vec, int qn) {
  int ui, vi;
  double u, v, x, y, z, n;

  _EVEN_QtoV(4, vec, qn);
}

int
_limnQN8checker_VtoQN_f(float *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(4, vec);
}

int
_limnQN8checker_VtoQN_d(double *vec) {
  double L, x, y, z;
  int xi, yi, ui, vi;
  
  _EVEN_VtoQ(4, vec);
}

/* ----------------------------------------------------------- */

void (*
limnQNtoV_f[LIMN_QN_MAX+1])(float *, int) = {
  NULL,
  _limnQN16simple_QNtoV_f,
  _limnQN16border1_QNtoV_f,
  _limnQN16checker_QNtoV_f,
  _limnQN15checker_QNtoV_f,
  _limnQN14checker_QNtoV_f,
  _limnQN13checker_QNtoV_f,
  _limnQN12checker_QNtoV_f,
  _limnQN11checker_QNtoV_f,
  _limnQN10checker_QNtoV_f,
  _limnQN9checker_QNtoV_f,
  _limnQN8checker_QNtoV_f
};
  
void (*
limnQNtoV_d[LIMN_QN_MAX+1])(double *, int) = {
  NULL,
  NULL,
  NULL,
  _limnQN16checker_QNtoV_d,
  _limnQN15checker_QNtoV_d,
  _limnQN14checker_QNtoV_d,
  _limnQN13checker_QNtoV_d,
  _limnQN12checker_QNtoV_d,
  _limnQN11checker_QNtoV_d,
  _limnQN10checker_QNtoV_d,
  _limnQN9checker_QNtoV_d,
  _limnQN8checker_QNtoV_d
};
  
int (*
limnVtoQN_f[LIMN_QN_MAX+1])(float *vec) = {
  NULL,
  _limnQN16simple_VtoQN_f,
  _limnQN16border1_VtoQN_f,
  _limnQN16checker_VtoQN_f,
  _limnQN15checker_VtoQN_f,
  _limnQN14checker_VtoQN_f,
  _limnQN13checker_VtoQN_f,
  _limnQN12checker_VtoQN_f,
  _limnQN11checker_VtoQN_f,
  _limnQN10checker_VtoQN_f,
  _limnQN9checker_VtoQN_f,
  _limnQN8checker_VtoQN_f
};

int (*
limnVtoQN_d[LIMN_QN_MAX+1])(double *vec) = {
  NULL,
  NULL,
  NULL,
  _limnQN16checker_VtoQN_d,
  _limnQN15checker_VtoQN_d,
  _limnQN14checker_VtoQN_d,
  _limnQN13checker_VtoQN_d,
  _limnQN12checker_VtoQN_d,
  _limnQN11checker_VtoQN_d,
  _limnQN10checker_VtoQN_d,
  _limnQN9checker_VtoQN_d,
  _limnQN8checker_VtoQN_d
};

int
limnQNBins[LIMN_QN_MAX+1] = {
  -1,
  (1 << 16),
  (1 << 16),
  (1 << 16),
  (1 << 15),
  (1 << 14),
  (1 << 13),
  (1 << 12),
  (1 << 11),
  (1 << 10),
  (1 << 9),
  (1 << 8)
};

