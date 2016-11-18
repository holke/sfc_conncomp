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
