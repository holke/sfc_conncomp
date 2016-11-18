#include "sfccc_piece.h"

void
sfccc_piece_element_fill_neighbor (sfccc_piece_t * piece,
                                   sfccc_element_t * elem, int face)
{
  sfccc_element_t    *neighbor;
  int                 neigh_face;
  t8_locidx_t         neighbor_index;

#if 0
  t8_debugf ("Neighbors of %llu across %i\n",
             (long long unsigned)
             t8_element_get_linear_id (piece->escheme, elem->t8element,
                                       piece->level),
             face);
#endif
  neighbor = sfccc_piece_new_element (piece);
  /* Compute the face neighbor across the given face */
  neigh_face = t8_element_face_neighbor (piece->escheme, elem->t8element,
                                         neighbor->t8element, face);
  /* Fill in -2 to indicate that we investigated this connection */
  neighbor_index = -2;
  if (t8_element_is_inside_root (piece->escheme, neighbor->t8element)) {
    /* We only need to continue if the new neighbor is inside the
     * root tree. We now check if it is a part of the SFC piece. */
    neighbor_index = sfccc_element_is_in_piece (piece, neighbor);
    t8_debugf("\tis inside %i\n", neighbor_index);
  }
  else {
    t8_debugf("\tis not inside\n");
  }
  /* Set the neighbor entry either to the index of the face neighbor
   * or to a negative value. */
  elem->face_neighbors[face] = neighbor_index;
  sfccc_piece_element_destroy (piece, neighbor);
  /* If the neighbor is part of the piece, set the approriate entry in its
   * neighbors array */
  if (neighbor_index >= 0) {
    neighbor = sfccc_piece_get_element (piece, neighbor_index);
    neighbor->face_neighbors[neigh_face] =
      sfccc_element_is_in_piece (piece, elem);
  }
}

#if 0
static void
sfccc_piece_element_fill_neighbors (sfccc_piece_t * piece)
{
  int                 iface;
  t8_locidx_t         li;
  sfccc_element_t    *elem;
  sfccc_element_t    *neighbor;
  t8_locidx_t         neighbor_index;

  T8_ASSERT (piece != NULL);
  neighbor = sfccc_piece_new_element (piece);

  for (li = 0; li < sfccc_piece_num_elements (piece); li++) {
    elem = sfccc_piece_get_element (piece, li);
    for (iface = 0; iface < elem->num_neighbors; iface++) {
      /* Compute the face neighbor across the given face */
      t8_element_face_neighbor (piece->escheme, elem->t8element,
                                neighbor->t8element, iface);
      neighbor_index = -1;
      if (t8_element_is_inside_root (piece->escheme, neighbor->t8element)) {
        /* We only need to continue if the new neighbor is inside the
         * root tree. We now check if it is a part of the SFC piece. */
        neighbor_index = sfccc_element_is_in_piece (piece, neighbor);
      }
      /* Set the neighbor entry either to the index of the face neighbor
       * or to a negative value. */
      elem->face_neighbors[iface] = neighbor_index;
    }
  }
  sfccc_piece_element_destroy (piece, neighbor);
}
#endif

static void
sfccc_piece_init_element (sfccc_piece_t * piece, sfccc_element_t * element,
                          t8_locidx_t index)
{
  int                 iface;

  /* TODO: For pyramids refined into tetrahedra the number of face neighbors differs from the
   * number of face neighbors of the parent. */
  element->num_neighbors = t8_eclass_num_faces[piece->eclass];
  element->face_neighbors = T8_ALLOC (int, element->num_neighbors);

  /* Fill the face neighbors of element with invalid value */
  for (iface = 0; iface < element->num_neighbors; iface++) {
    element->face_neighbors[iface] = -1;
  }

  /* At the beginning each element has an invalid representant */
  element->representant = -1;
  element->count_in_cc = 1;
}

sfccc_element_t    *
sfccc_piece_new_element (sfccc_piece_t * piece)
{
  sfccc_element_t    *elem;

  T8_ASSERT (piece != NULL);
  elem = T8_ALLOC_ZERO (sfccc_element_t, 1);

  t8_element_new (piece->escheme, 1, &elem->t8element);
  sfccc_piece_init_element (piece, elem, -1);
  return elem;
}

void
sfccc_piece_element_destroy (sfccc_piece_t * piece, sfccc_element_t * element)
{
  T8_ASSERT (element != NULL);

  T8_FREE (element->face_neighbors);
  if (element->t8element != NULL) {
    t8_element_destroy (piece->escheme, 1, &element->t8element);
  }
  T8_FREE (element);
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
  escheme = piece->escheme;

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
    predecessor = element->t8element;
  }
}

sfccc_piece_t      *
sfccc_piece_new (t8_scheme_t * scheme, t8_eclass_t eclass, uint64_t first,
                 uint64_t last, int8_t level)
{
  sfccc_piece_t      *piece;

  T8_ASSERT (0 <= first && first < t8_eclass_count_leaf (eclass, level));
  T8_ASSERT (0 <= last && last < t8_eclass_count_leaf (eclass, level));
  T8_ASSERT (0 <= level
             && level < t8_element_maxlevel (scheme->eclass_schemes[eclass]));
  /* Allocate memory */
  piece = T8_ALLOC (sfccc_piece_t, 1);

  /* Initialize struct */
  piece->first_index = first;
  piece->last_index = last;
  piece->level = level;
  piece->eclass = eclass;
  piece->scheme = scheme;
  t8_scheme_ref (scheme);
  piece->escheme = scheme->eclass_schemes[eclass];
  piece->num_conn_components = last - first + 1;
  /* Fill the array of elements */
  sfccc_piece_init_elementarray (piece);
#if 0
  /* Fill the face neighbor entries */
  sfccc_piece_element_fill_neighbors (piece);
#endif
  return piece;
}

sfccc_element_t    *
sfccc_piece_get_element (sfccc_piece_t * piece, t8_locidx_t elem_index)
{
  T8_ASSERT (piece != NULL);
  T8_ASSERT (piece->elements != NULL);
  return (sfccc_element_t *) t8_sc_array_index_locidx (piece->elements,
                                                       elem_index);
}

t8_locidx_t
sfccc_piece_num_elements (sfccc_piece_t * piece)
{
  T8_ASSERT (piece != NULL);
  T8_ASSERT (piece->elements != NULL);
  return piece->elements->elem_count;
}

int
sfccc_element_is_in_piece (sfccc_piece_t * piece, sfccc_element_t * element)
{
  uint64_t            element_id;

  T8_ASSERT (piece != NULL && element != NULL);
  /* Compute the linear id of the element */
  element_id = t8_element_get_linear_id (piece->escheme, element->t8element,
                                         piece->level);
  /* Check if the linear id lies within the bounds of the piece */
  if (piece->first_index <= element_id && element_id <= piece->last_index) {
    return element_id - piece->first_index;
  }
  return -2;
}

void
sfccc_piece_grow1 (sfccc_piece_t * piece)
{
  sfccc_element_t * new_elem;

  T8_ASSERT (piece != NULL);
  T8_ASSERT (piece->last_index <
             t8_eclass_count_leaf (piece->eclass, piece->level));

  /* The last index  grows by one */
  piece->last_index++;
  /* The added element possiblt adds a new connected component */
  piece->num_conn_components++;
  new_elem = (sfccc_element_t *) sc_array_push (piece->elements);
  /* Initialize the new element */
  sfccc_piece_init_element (piece, new_elem, piece->last_index
                            - piece->first_index);
  /* Construct new t8 element */
  t8_element_new (piece->escheme, 1, &new_elem->t8element);
  t8_element_set_linear_id (piece->escheme, new_elem->t8element,
                            piece->level, piece->last_index);
}

void
sfccc_piece_print (sfccc_piece_t * piece)
{
  t8_locidx_t         li;
  sfccc_element_t    *elem;

  t8_debugf ("Piece from %llu to %llu\n",
             (unsigned long long) piece->first_index,
             (unsigned long long) piece->last_index);
  t8_debugf ("This piece has %i connected components.\n",
             piece->num_conn_components);
  for (li = 0; li < sfccc_piece_num_elements (piece); li++) {
    elem = sfccc_piece_get_element (piece, li);
    t8_debugf ("Element %li:\n", (long) li);
    t8_debugf ("\trep = %li\n", (long) elem->representant);
    t8_debugf ("\tcount = %li\n", (long) elem->count_in_cc);
    t8_debugf ("\n");
  }
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
