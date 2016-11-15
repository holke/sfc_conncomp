#ifndef _SFCCC_PIECE_H
#define _SFCCC_PIECE_H

#include <t8_element.h>

typedef struct
{
  t8_element_t       *t8element;
  int                 num_neighbors;
  int                *face_neighbors;
  int                 representant;
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
  int                 num_conn_components;
} sfccc_piece_t;

sfccc_piece_t      *sfccc_piece_new (t8_scheme_t * scheme, t8_eclass_t eclass,
                                     uint64_t first, uint64_t last,
                                     int8_t level);

void                sfccc_piece_destroy (sfccc_piece_t * piece);

#endif
