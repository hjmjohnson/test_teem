#
# teem: Gordon Kindlmann's research software
# Copyright (C) 2003, 2002, 2001, 2000, 1999, 1998 University of Utah
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#

#### Library name
####
####
L := gage
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
$(L).NEED = ell nrrd biff air
$(L).PUBLIC_HEADERS = gage.h
$(L).PRIVATE_HEADERS = privateGage.h
$(L).OBJS = defaultsGage.o miscGage.o print.o filter.o update.o \
	scl.o sclfilter.o sclprint.o sclanswer.o \
	vecGage.o vecprint.o
$(L).OBJS = defaultsGage.o miscGage.o shape.o pvl.o ctx.o update.o filter.o \
	print.o scl.o sclanswer.o sclprint.o sclfilter.o \
	vecGage.o vecprint.o st.o
$(L).TESTS = 
####
####
####

# boilerplate: declare rules for this library
include $(TEEM_SRC)/make/template.mk
endif
ifeq (,$(INCLUDED))
  include $(TEEM_SRC)/bin/GNUmakefile
endif
