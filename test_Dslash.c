/***********************************************************************
 *
 * Copyright (C) 2015 Mario Schroeck
 *               2016 Peter Labus
 *               2017 Peter Labus, Martin Ueding, Bartosz Kostrzewa
 *
 * This file is part of tmLQCD.
 *
 * tmLQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tmLQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tmLQCD.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef TM_USE_QPHIX
#include <qphix/qphix_config.h>
#endif
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if (defined BGL && !defined BGP)
#include <rts.h>
#endif
#ifdef TM_USE_MPI
#include <mpi.h>
#ifdef HAVE_LIBLEMON
#include <io/gauge.h>
#include <io/params.h>
#endif
#endif
#ifdef TM_USE_OMP
#include <omp.h>
#include "init/init_openmp.h"
#endif
#ifdef QPHIX_QMP_COMMS
#include <qmp.h>
#endif
#include "boundary.h"
#include "gamma.h"
#include "geometry_eo.h"
#include "gettime.h"
#include "global.h"
#include "init/init.h"
#include "operator.h"
#include "operator/D_psi.h"
#include "operator/Hopping_Matrix.h"
#include "operator/Hopping_Matrix_nocom.h"
#include "operator/clovertm_operators.h"
#include "operator/tm_operators.h"
#include "ranlxd.h"
#include "read_input.h"
#include "solver/cg_her.h"
#include "start.h"
#include "su3.h"
#include "su3adj.h"
#include "test/check_geometry.h"
#include "xchange/xchange.h"
#include "update_backward_gauge.h"
#include "invert_clover_eo.h"
#include "invert_eo.h"
#include "linalg/assign_add_mul_r.h"
#include "linalg/convert_eo_to_lexic.h"
#include "linalg/square_norm.h"
#include "mpi_init.h"
#include "operator/clover_leaf.h"
#include "operator/clovertm_operators.h"
#include "prepare_source.h"
#include "qphix_interface.h"
#include "init/init.h"
#include "linalg/diff_and_square_norm.h"

int check_xchange();

// Full Dslash for twised mass
void _M_full(spinor *const Even_new, spinor *const Odd_new, spinor *const Even, spinor *const Odd) {
  /* Even sites */
  Hopping_Matrix(EO, g_spinor_field[8], Odd);
  assign_mul_one_pm_imu(Even_new, Even, 1., VOLUME / 2);
  assign_add_mul_r(Even_new, g_spinor_field[8], -1., VOLUME / 2);

  /* Odd sites */
  Hopping_Matrix(OE, g_spinor_field[8], Even);
  assign_mul_one_pm_imu(Odd_new, Odd, 1., VOLUME / 2);
  assign_add_mul_r(Odd_new, g_spinor_field[8], -1., VOLUME / 2);
}

// Full Dslash for twised mass and clover
void _Msw_full(spinor *const Even_new, spinor *const Odd_new, spinor *const Even,
               spinor *const Odd) {
  /* Even sites */
  Hopping_Matrix(EO, g_spinor_field[8], Odd);
  assign_mul_one_sw_pm_imu(EE, Even_new, Even, +g_mu);
  assign_add_mul_r(Even_new, g_spinor_field[8], -1., VOLUME / 2);

  /* Odd sites */
  Hopping_Matrix(OE, g_spinor_field[8], Even);
  assign_mul_one_sw_pm_imu(OO, Odd_new, Odd, +g_mu);
  assign_add_mul_r(Odd_new, g_spinor_field[8], -1., VOLUME / 2);
}

int main(int argc, char *argv[]) {
  int j;
#ifdef HAVE_LIBLEMON
  paramsXlfInfo *xlfInfo;
#endif
  int status = 0;

  static double t1, t2;

  DUM_DERI = 10;
  DUM_MATRIX = DUM_DERI + 8;
  NO_OF_SPINORFIELDS = DUM_MATRIX + 4;

  /* Set the input file */
  char input_filename[500];
  snprintf(input_filename, 500, "test_Dslash.input");

  init_parallel_and_read_input(argc, argv, input_filename);
  tmlqcd_mpi_init(argc, argv);

#ifdef _GAUGE_COPY
  init_gauge_field(VOLUMEPLUSRAND + g_dbw2rand, 1);
#else
  init_gauge_field(VOLUMEPLUSRAND + g_dbw2rand, 0);
#endif

  init_geometry_indices(VOLUMEPLUSRAND + g_dbw2rand);
  j = init_spinor_field(VOLUMEPLUSRAND, NO_OF_SPINORFIELDS);
  if (j != 0) {
    fprintf(stderr, "Not enough memory for spinor fields! Aborting...\n");
    exit(0);
  }

  if (g_proc_id == 0) {
    fprintf(stdout, "# The number of processes is %d \n", g_nproc);
    printf("# The lattice size is %d x %d x %d x %d\n", (int)(T * g_nproc_t), (int)(LX * g_nproc_x),
           (int)(LY * g_nproc_y), (int)(g_nproc_z * LZ));
    printf("# The local lattice size is %d x %d x %d x %d\n", (int)(T), (int)(LX), (int)(LY),
           (int)LZ);
    if (even_odd_flag) {
      printf("# testing the even/odd preconditioned Dirac operator\n");
    } else {
      printf("# testing the standard Dirac operator\n");
    }
    fflush(stdout);
  }

  /* define the geometry */
  geometry();
  /* define the boundary conditions for the fermion fields */
  boundary(g_kappa);

  // check BC
  if (g_proc_id == 0) {
    printf("\nphase_0 = %f + I*%f\n", creal(phase_0), cimag(phase_0));
    printf("phase_1 = %f + I*%f\n", creal(phase_1), cimag(phase_1));
    printf("phase_2 = %f + I*%f\n", creal(phase_2), cimag(phase_2));
    printf("phase_3 = %f + I*%f\n\n", creal(phase_3), cimag(phase_3));
  }
#ifdef _USE_HALFSPINOR
  j = init_dirac_halfspinor();
  if (j != 0) {
    fprintf(stderr, "Not enough memory for halfspinor fields! Aborting...\n");
    exit(0);
  }
  j = init_dirac_halfspinor32();
  if (j != 0) {
    fprintf(stderr, "Not enough memory for 32-Bit halfspinor fields! Aborting...\n");
    exit(0);
  }
#if (defined _PERSISTENT)
  init_xchange_halffield();
#endif
#endif

  status = check_geometry();
  if (status != 0) {
    fprintf(stderr, "Checking if geometry failed. Unable to proceed.\nAborting....\n");
    exit(1);
  }

  start_ranlux(1, 123456);
  random_gauge_field(1, g_gauge_field);
  // unit_g_gauge_field(); // unit 3x3 colour matrices
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c00 = 1.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c01 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c02 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c10 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c11 = 1.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c12 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c20 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c21 = 0.0;
// g_gauge_field[ g_ipt[0][0][0][1] ][0].c22 = 1.0;

#ifdef TM_USE_MPI
  /*For parallelization: exchange the gaugefield */
  xchange_gauge(g_gauge_field);
#endif

  g_update_gauge_copy = 1;
  update_backward_gauge(g_gauge_field);

  // Init a lexicographic spinor with uniform random source
  zero_spinor_field(g_spinor_field[0], VOLUME);
  random_spinor_field_lexic(g_spinor_field[0], 1, RN_UNIF);

  // Coordinates are T, X, Y, Z
  // g_spinor_field[0][ g_ipt[0][0][0][0] ].s0.c0 = 1.0; // even point source
  // g_spinor_field[0][ g_ipt[0][0][0][1] ].s0.c0 = 1.0; // odd point source

  /************************** tmLQCD D_psi **************************/

  if (g_proc_id == 0) {
    printf("\n\n");
    printf("# -------------------------------------------- #\n\n");
    printf("# Dslash 1 (tmLQCD):\n");
    printf("# ====================\n\n");
  }
  // print L2-norm of source:
  double squarenorm;
  squarenorm = square_norm(g_spinor_field[0], VOLUME, 1);
  if (g_proc_id == 0) {
    printf("  ||source||^2 = %e\n", squarenorm);
    fflush(stdout);
  }
#ifdef TM_USE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  // Split the full spinor in an even and an odd part
  convert_lexic_to_eo(/*even */ g_spinor_field[1],
                      /*odd */ g_spinor_field[2], /*full */
                      g_spinor_field[0]);

  // Apply a dslash on both odd and even parts
  t1 = gettime();
  Hopping_Matrix(OE, /*odd */ g_spinor_field[3], g_spinor_field[1]);
  Hopping_Matrix(EO, /*even */ g_spinor_field[4], g_spinor_field[2]);
  t2 = gettime();

  // Recombine even and odd spinors to a full spinor
  zero_spinor_field(g_spinor_field[1], VOLUME);
  convert_eo_to_lexic(/*full */ g_spinor_field[1],
                      /*even */ g_spinor_field[4], /*odd */
                      g_spinor_field[3]);

  // print L2-norm of result:
  squarenorm = square_norm(g_spinor_field[1], VOLUME, 1);
  if (g_proc_id == 0) {
    printf("  ||result_1||^2 = %.16e\n", squarenorm);
    printf("  Time for MV mult: %e\n", t2 - t1);
    fflush(stdout);
  }

  /************************** D_psi_qphix **************************/

  if (g_proc_id == 0) {
    printf("\n");
    printf("# Dslash 2 (QPhiX):\n");
    printf("# ====================\n\n");
  }
  // print L2-norm of source:
  squarenorm = square_norm(g_spinor_field[0], VOLUME, 1);
  if (g_proc_id == 0) {
    printf("  ||source||^2 = %e\n", squarenorm);
    fflush(stdout);
  }
#ifdef TM_USE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  zero_spinor_field(g_spinor_field[2], VOLUME);

  t1 = gettime();
  D_psi_qphix(g_spinor_field[2], g_spinor_field[0]);
  t2 = gettime();

  // print L2-norm of result:
  squarenorm = square_norm(g_spinor_field[2], VOLUME, 1);
  if (g_proc_id == 0) {
    printf("  ||result_2||^2 = %.16e\n", squarenorm);
    printf("  Time for MV mult: %e\n", t2 - t1);
    fflush(stdout);
  }

  /************************** DEBUG PRINT OUTS **************************/

  // printf("\n INPUT SPINOR:\n");
  // double* show_in = (double*) g_spinor_field[0];
  // for(int i=0; i<24*VOLUME; ++i) {
  //      if(show_in[i] != 0.) {
  // int j = i/24;
  //              printf("%d %d %d %d : %2f\n", g_coord[j][0],
  //              g_coord[j][1],g_coord[j][2],g_coord[j][3],show_in[i]);
  //      }
  // }
  // printf("\n");

  // printf("\n OUTPUT TMLQCD vs QPHIX SPINOR (tmlQCD format):\n");
  // double* show_out       = (double*) &(g_spinor_field[1][0]);
  // double* show_out_qphix = (double*) &(g_spinor_field[2][0]);
  // printf("%d %d %d %d : \t\t", T, LX, LY, LZ);
  // printf("%d %d %d %d : \n", T, LX, LY, LZ);
  // for(int i=0; i<24*VOLUME; ++i) {
  //      if( fabs(show_out_qphix[i]) > DBL_EPSILON || fabs(show_out[i]) >
  //      DBL_EPSILON) {
  // int j = i/24;
  //              printf("%d %d %d %d : %2g\t\t", g_coord[j][0],
  //              g_coord[j][1],g_coord[j][2],g_coord[j][3],show_out[i]);
  //              printf("%d %d %d %d : %2g\n", g_coord[j][0],
  //              g_coord[j][1],g_coord[j][2],g_coord[j][3],show_out_qphix[i]);
  //      }
  // }
  // printf("\n");

  /************************** finished: get difference
   * **************************/

  if (g_proc_id == 0) {
    printf("\n");
    printf("# Comparison tmLQCD vs QPhiX:\n");
    printf("# ===========================\n\n");
  }

  // print L2-norm of result1 - result2:
  squarenorm = diff_and_square_norm(g_spinor_field[1], g_spinor_field[2], VOLUME);
  if (g_proc_id == 0) {
    printf("  ||result_1 - result_2||^2 = %e\n\n", squarenorm);
    fflush(stdout);
  }
// ---------------

#ifdef TM_USE_OMP
  free_omp_accumulators();
#endif
  free_gauge_field();
  free_geometry_indices();
  free_spinor_field();
  free_moment_field();
#ifdef TM_USE_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
#endif
  return (0);
}
