/*
  This file is part of the sfccc collection.

  Copyright (C) 2016 the developers

  The sfccc collection is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  The sfccc collection is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with the sfccc collection; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.
*/


#ifndef _SFCCC_SEARCHGRAPH_H
#define _SFCCC_SEARCHGRAPH_H

#include "sfccc_piece.h"

/** Compute the number of connected components of a SFC piece */
void sfccc_compute_connected_components (sfccc_piece_t *piece);

/** If the connected components were previously computed and only the last
 * element was added afterwards, we can performe a quick count of the
 * connected components.
 * \see sfccc_piece_grow1
 */
void sfccc_compute_connected_components_fromlast (sfccc_piece_t * piece);

#endif
