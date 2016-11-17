#include <t8.h>
#include <sc_options.h>
#include <t8_default.h>
#include "sfccc_piece.h"
#include "sfccc_searchgraph.h"

void
test_sfccc (int level, int dim)
{
  sfccc_piece_t      *piece;
  int                 start, end;
  int                *num_cc_counts;
  t8_eclass_t         eclass;

  T8_ASSERT (level > 0);
  if (dim == 2) {
    eclass = T8_ECLASS_TRIANGLE;
  }
  else if (dim == 3) {
    eclass = T8_ECLASS_TET;
  }
  else {
    SC_ABORT ("Wrong usage.\n");
  }

  /* Initialize array to count the multiplicity of number of components */
  /* We allocate one less than the possible maximum number of cc, since we
   * do not need a place in the array for zero connected components. */
  num_cc_counts = T8_ALLOC_ZERO (int, 2 * (level));

#if 1
  for (end = 1; end < 1 << (dim * level); end++) {
    for (start = 0; start < end; start++) {
#endif
      piece = sfccc_piece_new (t8_scheme_new_default (), eclass,
                               start, end, level);
      sfccc_compute_connected_components (piece);
      sfccc_piece_destroy (piece);
      t8_debugf ("%i conn components for [%i,%i]\n",
                 piece->num_conn_components, start, end);
      if (0 < piece->num_conn_components
          && piece->num_conn_components < 2 * level + 1) {
        num_cc_counts[piece->num_conn_components - 1]++;
      }
      else {
        SC_ABORTF
          ("Strange number of conn. components measured: %i for segment"
           " [%i,%i]\n", piece->num_conn_components, start, end);
      }
#if 1
    }
  }
#endif
  for (start = 0; start < 2 * level; start++) {
    printf ("%i\t%i\n", start + 1, num_cc_counts[start]);
  }
  T8_FREE (num_cc_counts);
}

int
main (int argc, char *argv[])
{
  int                 mpiret, level, dim;
  int                 parsed;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_DEBUG);

  /* Command line arguments */
  opt = sc_options_new (argv[0]);

  sc_options_add_int (opt, 'd', "dim", &dim, 2, "The dimension.");
  sc_options_add_int (opt, 'l', "level", &level, 0, "The refinement level.");

  parsed =
    sc_options_parse (t8_get_package_id (), SC_LP_ERROR, opt, argc, argv);

  if (parsed < 0 || dim < 2 || dim > 3 || level < 0) {
    /* Display help message */
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else {
    /* Run the program */
    test_sfccc (level, dim);
  }

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
}
