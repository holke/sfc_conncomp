#ifndef _SFCCC_PIECE_H
#define _SFCCC_PIECE_H

#include <t8_element.h>

typedef struct
{
  t8_element_t       *t8element;
  int                 num_neighbors;
  int                *face_neighbors;
  t8_locidx_t         representant;
  t8_locidx_t         count_in_cc;
} sfccc_element_t;

typedef struct
{
  sc_array_t         *elements;
  uint64_t            first_index;      /* 0 <= first_index <= #desc_on_level */
  uint64_t            last_index;
  int8_t              level;
  int                 dim;
  t8_eclass_t         eclass;
  t8_scheme_t        *scheme;
  t8_eclass_scheme_t *escheme;
  int                 num_conn_components;
} sfccc_piece_t;

sfccc_piece_t      *sfccc_piece_new (t8_scheme_t * scheme, t8_eclass_t eclass,
                                     uint64_t first, uint64_t last,
                                     int8_t level);

/** Allocate and initiliaze a new element */
sfccc_element_t   *sfccc_piece_new_element (sfccc_piece_t * piece);

/** Destroy an allocated element */
void               sfccc_piece_element_destroy (sfccc_piece_t *piece, sfccc_element_t *element);

/** Return the number of elements of a SFC piece */
t8_locidx_t sfccc_piece_num_elements (sfccc_piece_t * piece);

/** Check whether a given element lies within a SFC piece. If so, the array index
 *  of the element whithin piece is returned.
 *  -1 is returned otherwise.
 */
t8_locidx_t         sfccc_element_is_in_piece (sfccc_piece_t * piece,
                                               sfccc_element_t * element);

/** Return a pointer to a specific element */
sfccc_element_t * sfccc_piece_get_element (sfccc_piece_t * piece,
                                           t8_locidx_t elem_index);

/** Fills a specific neighbor entry of an element. Fills both sides */
void              sfccc_piece_element_fill_neighbor (sfccc_piece_t * piece, sfccc_element_t * elem,
                                                    int face);

/** Print basic information about a piece structure */
void                sfccc_piece_print (sfccc_piece_t *piece);

void                sfccc_piece_destroy (sfccc_piece_t * piece);

#endif
