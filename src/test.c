#include <t8.h>
#include <t8_default.h>
#include "sfccc_piece.h"
#include "sfccc_searchgraph.h"

void
test_sfccc (int level, int dim)
{
  sfccc_piece_t      *piece;
  int                 start, end;

  T8_ASSERT (dim == 2);
  T8_ASSERT (level > 0);
  for (end = 1; end < 1 << (dim * level);end++) {
    for (start = 0; start < end; start++) {
      piece = sfccc_piece_new (t8_scheme_new_default (), T8_ECLASS_TRIANGLE,
                               start, end, level);
      sfccc_compute_connected_components (piece);
      sfccc_piece_destroy (piece);
      t8_debugf ("%i conn components for [%i,%i]\n", piece->num_conn_components,
                 start, end);
    }
  }
}

int
main (int argc, char *argv[])
{
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEBUG);

  test_sfccc (5, 2);

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
}
