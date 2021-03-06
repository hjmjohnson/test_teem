#
# Teem: Tools to process and visualize scientific data and images
# Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
# Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
# Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# (LGPL) as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# The terms of redistributing and/or modifying this software also
# include exceptions to the LGPL that facilitate static linking.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library; if not, write to Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#

#### Library name
####
####
L := limn
####
####
####

# boilerplate: default targets and include tricks
TEEM_ROOT ?= ../..
TEEM_SRC ?= ..
ifeq (,$(DEF_TARGETS))
DEF_TARGETS = true
dev     : $(L)/dev
install : $(L)/install
clean   : $(L)/clean
clobber : $(L)/clobber
include ../GNUmakefile
endif
ifeq (,$($(L).SEEN))
$(L).SEEN := true

#### Describe library here
####
####
$(L).NEED = gage ell unrrdu nrrd biff hest air 
$(L).PUBLIC_HEADERS = limn.h
$(L).PRIVATE_HEADERS = privateLimn.h
$(L).OBJS = defaultsLimn.o qn.o light.o envmap.o cam.o methodsLimn.o \
        obj.o transform.o shapes.o renderLimn.o io.o hestLimn.o \
	splineMisc.o splineMethods.o splineEval.o enumsLimn.o \
	polydata.o polyshapes.o polymod.o polyfilter.o \
	lpuFlotsam.o lpu_about.o lpu_ccfind.o lpu_psel.o lpu_rast.o \
	lpu_verts.o lpu_meas.o lpu_sort.o
$(L).TESTS = test/map test/light test/tcam test/tps test/tspline test/tbc \
	test/tcamanim test/soid test/off2eps test/triimg test/plot test/intx \
	test/tio test/clip test/lpu test/tqn
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_SRC)/make/template.mk
endif
ifeq (,$(INCLUDED))
  include $(TEEM_SRC)/bin/GNUmakefile
endif
