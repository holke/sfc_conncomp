#include "sfccc_piece.h"

static void
sfccc_piece_init_element (sfccc_piece_t * piece, sfccc_element_t * element,
                          t8_locidx_t index)
{
  int                 iface;

  /* TODO: For pyramids refined into tetrahedra the number of face neighbors differs from the
   * number of face neighbors of the parent. */
  element->num_neighbors = t8_eclass_num_faces[piece->eclass];
  element->face_neighbors = T8_ALLOC (int, element->num_neighbors);
  for (iface = 0; iface < element->num_neighbors; iface++) {
    /* Set the face neighbors to an invalid value */
    element->face_neighbors[iface] = -1;
  }
  /* At the beginning each element is its own representant */
  element->representant = index;
}

static void
sfccc_piece_init_elementarray (sfccc_piece_t * piece)
{
  t8_locidx_t         li;
  t8_locidx_t         num_elements;
  sfccc_element_t    *element;
  t8_element_t       *predecessor;
  t8_eclass_scheme_t *escheme;

  num_elements = piece->last_index - piece->first_index + 1;
  piece->elements = sc_array_new_size (sizeof (sfccc_element_t),
                                       num_elements);
  escheme = piece->scheme->eclass_schemes[piece->eclass];

  if (num_elements <= 0) {
    return;
  }
  /* Initilialize the first element */
  element = (sfccc_element_t *) t8_sc_array_index_locidx (piece->elements, 0);
  t8_element_new (escheme, 1, &element->t8element);
  t8_element_set_linear_id (escheme, element->t8element, piece->level,
                            piece->first_index);
  sfccc_piece_init_element (piece, element, 0);

  /* Fill the other elements as successors of the previous one */
  predecessor = element->t8element;
  for (li = 1; li < num_elements; li++) {
    element = t8_sc_array_index_locidx (piece->elements, li);
    t8_element_new (escheme, 1, &element->t8element);
    t8_element_successor (escheme, predecessor, element->t8element,
                          piece->level);
    /* initialize the rest of the element structure */
    sfccc_piece_init_element (piece, element, li);
  }
}

sfccc_piece_t      *
sfccc_piece_new (t8_scheme_t * scheme, t8_eclass_t eclass, uint64_t first,
                 uint64_t last, int8_t level)
{
  sfccc_piece_t      *piece;

  piece = T8_ALLOC (sfccc_piece_t, 1);

  piece->first_index = first;
  piece->last_index = last;
  piece->level = level;
  piece->eclass = eclass;
  piece->scheme = scheme;
  piece->num_conn_components = last - first + 1;
  sfccc_piece_init_elementarray (piece);

  return piece;
}

void
sfccc_piece_destroy (sfccc_piece_t * piece)
{
  t8_locidx_t         li;
  sfccc_element_t    *elem;

  /* Destroy all t8code elements */
  for (li = 0; li < piece->elements->elem_count; li++) {
    elem = (sfccc_element_t *) t8_sc_array_index_locidx (piece->elements, li);
    t8_element_destroy (piece->scheme->eclass_schemes[piece->eclass], 1,
                        &elem->t8element);
    T8_FREE (elem->face_neighbors);
  }
  /* Destroy the remaining structures */
  t8_scheme_unref (&piece->scheme);
  sc_array_destroy (piece->elements);
  T8_FREE (piece);
}
