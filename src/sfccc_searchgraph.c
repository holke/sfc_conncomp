#include "sfccc_searchgraph.h"


static void
sfccc_searchgraph_adapt_representant (sfccc_piece_t * piece,
                                      sfccc_element_t * elem1,
                                      sfccc_element_t * elem2)
{
  sfccc_element_t   *temp;
  int                iface;

  T8_ASSERT (piece != NULL && elem1 != NULL && elem2 != NULL);

  if (elem1->representant == elem2->representant) {
    return;
  }
  if (elem1->representant > elem2->representant) {
    /* We switch elem1 and elem2, such that elem1 has the smaller representant */
    temp = elem1;
    elem1 = elem2;
    elem2 = temp;
  }
  /* Change the representant of elem1 */
  t8_debugf ("Change rep of %i to %i\n", sfccc_element_is_in_piece (piece, elem1),
             elem2->representant);
  elem1->representant = elem2->representant;
  elem1->count_in_cc += elem2->count_in_cc;
#if 1
  for (iface = 0;iface < elem1->num_neighbors;iface++) {
    /* Iterate over all neighbors */
    if (elem1->face_neighbors[iface] >= 0) {
      temp = sfccc_piece_get_element (piece, elem1->face_neighbors[iface]);
      sfccc_searchgraph_adapt_representant (piece, elem1, temp);
    }
  }
#endif
}

/* Perform a depth first search on the connectivity graph
 * of a SFC piece to compute the number of connected components */
void sfccc_dfs (sfccc_piece_t * piece, sfccc_element_t * elem)
{
  sfccc_element_t        *neighbor, *neighbor_from_piece;
  int                     ineigh;
  t8_locidx_t             neighbor_index;

  t8_debugf ("into dfs from element %li\n",
             sfccc_element_is_in_piece (piece, elem));
  T8_ASSERT (piece != NULL);
  /* Initiliaze a new element to store the neighbor */
  neighbor = sfccc_piece_new_element (piece);
  /* Iterate over all neighbors and if they are in the piece of the SFC,
   * recursively continue the search */
  for (ineigh = 0; ineigh < elem->num_neighbors;ineigh++) {
      sfccc_piece_element_fill_neighbor (piece, elem, ineigh);
      neighbor_index = elem->face_neighbors[ineigh];
      /* We continue if the neighbor element is whithin the SFC curve
       * and if the current index is smaller than the neighbors index */
      if (neighbor_index >= 0){
        t8_debugf ("Check neighbor %i, index %i\n",
                   ineigh, neighbor_index);
        neighbor_from_piece = sfccc_piece_get_element (piece, neighbor_index);
        /* If necessary Change the representant of neighbors connectivity group to
         * the one if elements group. In this case, the search continues from neighbor on. */
        if (elem->representant != neighbor_from_piece->representant) {
          t8_debugf("Found new connection from %li to %li\n",
                    sfccc_element_is_in_piece (piece, elem),
                    neighbor_index);
          sfccc_searchgraph_adapt_representant (piece, elem, neighbor_from_piece);
          piece->num_conn_components--;
          sfccc_dfs (piece, neighbor_from_piece);
        }
      }
  }
  sfccc_piece_element_destroy (piece, neighbor);
  t8_debugf ("done dfs %li\n", sfccc_element_is_in_piece (piece, elem));
}

void sfccc_compute_connected_components (sfccc_piece_t *piece)
{
  t8_locidx_t   el_index, num_elements;
  sfccc_element_t *elem;

  T8_ASSERT (piece != NULL);
  num_elements = sfccc_piece_num_elements (piece);

  for (el_index = 0;el_index < num_elements;el_index++) {
    elem = sfccc_piece_get_element (piece, el_index);
    if (elem->representant == el_index && elem->count_in_cc == 1) {
      t8_debugf ("start new dfs branch with %li\n", (long) el_index);
      sfccc_dfs (piece, elem);
    }
  }
}
