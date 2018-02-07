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

#include <t8.h>
#include <sc_options.h>
#include <t8_default.h>
#include <t8_eclass.h>
#include "sfccc_piece.h"
#include "sfccc_searchgraph.h"

void
compute_all_simplex_cc (int level, int dim, int do_cubical, sc_MPI_Comm comm)
{
  sfccc_piece_t      *piece;
  t8_locidx_t         start, len, proc_start;
  int                *num_cc_counts, *num_cc_counts_all = NULL;
  int               (*num_cc_counts_binned)[32];
  int               (*num_cc_counts_binned_all)[32] = NULL;
  int                 bin_sizes[32] = {0}, bin_sizes_all[32] = {0};
  double             *len_per_count, *len_per_count_all = NULL;
  t8_eclass_t         eclass;
  t8_locidx_t         last_element;
  t8_scheme_t        *scheme;
  int                 mpisize, mpirank, mpiret;

  T8_ASSERT (level >= 0);

  /* For load-balancing the process p will compute all segments
   * with first element index congruent 0 modulo p.
   * This is only a heuristic and still imbalanced. */
  mpiret = sc_MPI_Comm_rank (comm, &mpirank);
  SC_CHECK_MPI (mpiret);
  mpiret = sc_MPI_Comm_size (comm, &mpisize);
  SC_CHECK_MPI (mpiret);
  proc_start = mpirank;

  if (dim == 2) {
    if (do_cubical) {
      eclass = T8_ECLASS_QUAD;
    }
    else {
      eclass = T8_ECLASS_TRIANGLE;
    }
  }
  else if (dim == 3) {
    if (do_cubical) {
      eclass = T8_ECLASS_HEX;
    }
    else {
      eclass = T8_ECLASS_TET;
    }
  }
  else {
    SC_ABORT ("Wrong usage.\n");
  }

  /* Initialize array to count the multiplicity of number of components
   * and the average length of a segment per num cc. */
  /* We allocate one less than the possible maximum number of cc, since we
   * do not need a place in the array for zero connected components. */
  num_cc_counts = T8_ALLOC_ZERO (int, 2 * (level));
  len_per_count = T8_ALLOC_ZERO (double, 2 * (level));
  num_cc_counts_binned = (int (*)[32]) calloc (2 * level, 32 * sizeof(int));

  scheme = t8_scheme_new_default ();
  last_element = t8_eclass_count_leaf (eclass, level);
#if 1
  for (start = proc_start; start < last_element; start += mpisize) {
    if (start % 1000) {
      t8_debugf ("Starting run %li/%li\n", (long) start,
                 (long) last_element - 1);
    }
    len = start;
    piece = sfccc_piece_new (scheme, eclass,
                             start, len, level);
    sfccc_compute_connected_components (piece);
    for (len = 1; len < last_element - start + 1; len++) {
#endif
      sfccc_compute_connected_components_fromlast (piece);
      t8_global_productionf ("%i conn components for [%li,%li]\n",
                 piece->num_conn_components, piece->first_index, piece->last_index);
      if (0 < piece->num_conn_components
          && piece->num_conn_components < 2 * level + 1) {
        num_cc_counts[piece->num_conn_components - 1]++;
        num_cc_counts_binned[piece->num_conn_components - 1][SC_LOG2_32(len)]++;
        bin_sizes[SC_LOG2_32(len)]++;
        len_per_count[piece->num_conn_components - 1] += len;
      }
      else {
        SC_ABORTF
          ("Strange number of conn. components measured: %i for segment"
           " [%i,%i]\n", piece->num_conn_components, start, len);
      }
      if (len < last_element - start) {
        sfccc_piece_grow1 (piece);
      }
#if 1
    }
    sfccc_piece_destroy (piece);
  }
#endif
  /* Print the number of connected components */
  if (mpisize > 1) {
    /* If more than one process is running, we need to communicate
     * the local counts to process 0 */
    if (mpirank == 0) {
      num_cc_counts_all = T8_ALLOC (int, 2 * level);
      len_per_count_all = T8_ALLOC (double, 2 * level);
      num_cc_counts_binned_all = (int (*)[32]) calloc (2 * level, 32 * sizeof(int));
    }
    mpiret = sc_MPI_Reduce (num_cc_counts, num_cc_counts_all, 2*level,
                            sc_MPI_INT, sc_MPI_SUM, 0, comm);
    mpiret = sc_MPI_Reduce (len_per_count, len_per_count_all, 2*level,
                            sc_MPI_DOUBLE, sc_MPI_SUM, 0, comm);
    mpiret = sc_MPI_Reduce (num_cc_counts_binned, num_cc_counts_binned_all, 32 * 2*level,
                            sc_MPI_INT, sc_MPI_SUM, 0, comm);
    mpiret = sc_MPI_Reduce (bin_sizes, bin_sizes_all, 32, sc_MPI_INT, sc_MPI_SUM, 0, comm);
    SC_CHECK_MPI (mpiret);
  }
  else {
    num_cc_counts_all = num_cc_counts;
    len_per_count_all = len_per_count;
    num_cc_counts_binned_all = num_cc_counts_binned;
    memcpy (bin_sizes_all, bin_sizes, sizeof(int) * 32);
  }
  if (mpirank == 0) {
    int bin;
    printf ("For %s, level %i\n", t8_eclass_to_string[eclass], level);
    printf ("Num cc.\tcount\tAvg. length\n");
    for (start = 0; start < 2 * level; start++) {
      if (num_cc_counts_all[start] != 0) {
        len_per_count_all[start] /= num_cc_counts_all[start];
      }
      printf ("%i\t%i", start + 1, num_cc_counts_all[start]);
      printf ("\t%f\n", len_per_count_all[start]);
    }
    printf ("\nBinned counts:\n2^ ");
    for (bin = 0; bin < 32; bin++) {
      if (!bin_sizes_all[bin]) continue;
      printf("  [%2d,%2d)",bin,bin+1);
    }
    printf("\n");
    for (start = 0; start < 2 * level; start++) {
      if (num_cc_counts_all[start] != 0) {
        len_per_count_all[start] /= num_cc_counts_all[start];
      }
      printf ("%2d:", start + 1);
      for (bin = 0; bin < 32; bin++) {
        if (!bin_sizes_all[bin]) continue;
        printf(" %8d",num_cc_counts_binned_all[start][bin]);
      }
      printf("\n   ");
      for (bin = 0; bin < 32; bin++) {
        if (!bin_sizes_all[bin]) continue;
        printf(" %4.2e",bin_sizes_all[bin] ? ((double) num_cc_counts_binned_all[start][bin] / (double) bin_sizes_all[bin]) : 0);
      }
      printf("\n\n");
    }
  }
  t8_scheme_unref (&scheme);
  free (num_cc_counts_binned);
  T8_FREE (num_cc_counts);
  T8_FREE (len_per_count);
  if (mpisize > 1 && mpirank == 0) {
    T8_FREE (num_cc_counts_all);
    free (num_cc_counts_binned_all);
    T8_FREE (len_per_count_all);
  }
}

int
main (int argc, char *argv[])
{
  int                 mpiret, level, dim;
  int                 cube;
  int                 parsed;
  sc_options_t       *opt;

  mpiret = sc_MPI_Init (&argc, &argv);
  SC_CHECK_MPI (mpiret);

  sc_init (sc_MPI_COMM_WORLD, 1, 1, NULL, SC_LP_ESSENTIAL);
  t8_init (SC_LP_ESSENTIAL);

  /* Command line arguments */
  opt = sc_options_new (argv[0]);

  sc_options_add_int (opt, 'd', "dim", &dim, 2, "The dimension.");
  sc_options_add_int (opt, 'l', "level", &level, 0, "The refinement level.");
  sc_options_add_switch (opt, 'c', "cubical", &cube,
                         "Switch to cubical Morton.");

  parsed =
    sc_options_parse (t8_get_package_id (), SC_LP_ERROR, opt, argc, argv);

  if (parsed < 0 || dim < 2 || dim > 3 || level < 0) {
    /* Display help message */
    sc_options_print_usage (t8_get_package_id (), SC_LP_ERROR, opt, NULL);
  }
  else {
    /* Run the program */
    compute_all_simplex_cc (level, dim, cube, sc_MPI_COMM_WORLD);
  }

  sc_options_destroy (opt);
  sc_finalize ();

  mpiret = sc_MPI_Finalize ();
  SC_CHECK_MPI (mpiret);
}
