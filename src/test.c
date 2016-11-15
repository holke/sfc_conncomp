#include <t8.h>
#include <t8_default.h>
#include "sfccc_piece.h"

void
test_sfccc ()
{
  sfccc_piece_t      *piece;

  piece = sfccc_piece_new (t8_scheme_new_default (), T8_ECLASS_TRIANGLE,
                           0, 4, 1);
  t8_debugf ("Created piece\n");
  sfccc_piece_destroy (piece);
  t8_debugf ("Destroyed piece\n");
}

int
main (int argc, char *argv[])
{
  int                 mpiret;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEBUG);

  test_sfccc ();

  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
}
