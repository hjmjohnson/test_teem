/*
  teem: Gordon Kindlmann's research software
  Copyright (C) 2002, 2001, 2000, 1999, 1998 University of Utah

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


/*
** the end result of this is that the source file which includes
** this can be sure that TEEM_32BIT is set, and can be sure that
** it is set to either 0 or 1
*/

#if TEEM_32BIT == 1
#  /* its 1 */
#else
#  if TEEM_32BIT == 0
#    /* its 0 */
#  else
#    error TEEM_32BIT not set to 0 or 1, see architecture-specific .mk file or check compilation options
#  endif
#endif
