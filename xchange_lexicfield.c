/* $Id$ */

/**********************************************************
 * 
 * exchange routines for lexicographic spinor fields
 *  (not even/odd)
 *
 * Author: Carsten Urbach 
 *
 **********************************************************/

#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#ifdef MPI
# include <mpi.h>
#endif

#include "global.h"
#if (defined XLC && defined BGL)
#  include "bgl.h"
#endif
#include "mpi_init.h"
#include "su3.h"
#include "xchange_lexicfield.h"

#if (defined _NON_BLOCKING)

/* this version uses non-blocking MPI calls */

void xchange_lexicfield(spinor * const l) {

  MPI_Request requests[16];
  MPI_Status status[16];
#  ifdef PARALLELT
  int reqcount = 4;
#  elif defined PARALLELXT
  int reqcount = 8;
#  elif defined PARALLELXYT
  int reqcount = 12;
#  elif defined PARALLELXYZT
  int x0=0, x1=0, x2=0, ix=0;
  int reqcount = 16;
#  endif

#ifdef _KOJAK_INST
#pragma pomp inst begin(xchange_lexicfield)
#endif
#  if (defined BGL && defined XLC)
  __alignx(16, l);
#  endif

#  ifdef MPI


  /* send the data to the neighbour on the left */
  /* recieve the data from the neighbour on the right */
  MPI_Isend((void*)l, 1, lfield_time_slice_cont, g_nb_t_dn, 5081, g_cart_grid, &requests[0]);
  MPI_Irecv((void*)(l+VOLUME), 1, lfield_time_slice_cont, g_nb_t_up, 5081, g_cart_grid, &requests[1]);
#    if (defined PARALLELXT || defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the left in x direction */
  /* recieve the data from the neighbour on the right in x direction */
  MPI_Isend((void*)l, 1, lfield_x_slice_gath, g_nb_x_dn, 5091, g_cart_grid,  &requests[4]);
  MPI_Irecv((void*)(l+(T+2)*LX*LY*LZ), 1, lfield_x_slice_cont, g_nb_x_up, 5091, g_cart_grid, &requests[5]);
#    endif
  
#    if (defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the left in y direction */
  /* recieve the data from the neighbour on the right in y direction */
  MPI_Isend((void*)l, 1, lfield_y_slice_gath, g_nb_y_dn, 5101, g_cart_grid, &requests[8]);
  MPI_Irecv((void*)(l + VOLUME + 2*LZ*(LX*LY + T*LY)), 1, lfield_y_slice_cont, g_nb_y_up, 5101, g_cart_grid, &requests[9]);
#    endif
  
#    if (defined PARALLELXYZT)
  
  /* send the data to the neighbour on the left in z direction */
  /* recieve the data from the neighbour on the right in z direction */
  MPI_Isend((void*)l, 1, lfield_z_slice_gath, g_nb_z_dn, 5503, g_cart_grid, &requests[12]); 
  MPI_Irecv((void*)(l+VOLUME + 2*LZ*(LX*LY + T*LY) + 2*LZ*T*LX), 1, lfield_z_slice_cont, g_nb_z_up, 5503, g_cart_grid, &requests[13]); 
#    endif
  /* send the data to the neighbour on the right */
  /* recieve the data from the neighbour on the left */
  MPI_Isend((void*)(l+(T-1)*LX*LY*LZ), 1, lfield_time_slice_cont, g_nb_t_up, 5082, g_cart_grid, &requests[2]);
  MPI_Irecv((void*)(l+(T+1)*LX*LY*LZ), 1, lfield_time_slice_cont, g_nb_t_dn, 5082, g_cart_grid, &requests[3]);
  
#    if (defined PARALLELXT || defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the right in x direction */
  /* recieve the data from the neighbour on the left in x direction */  
  MPI_Isend((void*)(l+(LX-1)*LY*LZ), 1, lfield_x_slice_gath, g_nb_x_up, 5092, g_cart_grid, &requests[6]);
  MPI_Irecv((void*)(l+((T+2)*LX*LY*LZ + T*LY*LZ)), 1, lfield_x_slice_cont, g_nb_x_dn, 5092, g_cart_grid, &requests[7]);
#    endif
  
#    if (defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the right in y direction */
  /* recieve the data from the neighbour on the left in y direction */  
  MPI_Isend((void*)(l+(LY-1)*LZ), 1, lfield_y_slice_gath, g_nb_y_up, 5102, g_cart_grid, &requests[10]);
  MPI_Irecv((void*)(l+VOLUME + 2*LZ*(LX*LY + T*LY) + T*LX*LZ), 1, lfield_y_slice_cont, g_nb_y_dn, 5102, g_cart_grid, &requests[11]);
#    endif
  
#    if defined PARALLELXYZT
  
  /* send the data to the neighbour on the right in y direction */
  /* recieve the data from the neighbour on the left in y direction */  
  MPI_Isend((void*)(l+LZ-1), 1, lfield_z_slice_gath, g_nb_z_up, 5504, g_cart_grid, &requests[14]); 
  MPI_Irecv((void*)(l+VOLUME + 2*LZ*(LX*LY + T*LY) + 2*T*LX*LZ + T*LX*LY), 1, lfield_z_slice_cont, g_nb_z_dn, 5504, g_cart_grid, &requests[15]); 
#    endif
  
  MPI_Waitall(reqcount, requests, status);

#  endif
  return;
#ifdef _KOJAK_INST
#pragma pomp inst end(xchange_lexicfield)
#endif
}
#else


/* Here comes the naive version */  
/* Using MPI_Sendrecv */
/* exchanges the field  l */
void xchange_field(spinor * const l) {
  
#  ifdef PARALLELXYZT
  int x0=0, x1=0, x2=0, ix=0;
#  endif
#ifdef _KOJAK_INST
#pragma pomp inst begin(xchange_lexicfield)
#endif

#  ifdef MPI
    
  MPI_Status status;
  /* send the data to the neighbour on the left */
  /* recieve the data from the neighbour on the right */
  MPI_Sendrecv((void*)l,                1, lfield_time_slice_cont, g_nb_t_dn, 5081,
	       (void*)(l+T*LX*LY*LZ), 1, lfield_time_slice_cont, g_nb_t_up, 5081,
	       g_cart_grid, &status);
    
  /* send the data to the neighbour on the right */
  /* recieve the data from the neighbour on the left */
  MPI_Sendrecv((void*)(l+(T-1)*LX*LY*LZ), 1, lfield_time_slice_cont, g_nb_t_up, 5082,
	       (void*)(l+(T+1)*LX*LY*LZ), 1, lfield_time_slice_cont, g_nb_t_dn, 5082,
	       g_cart_grid, &status);
    
#    if (defined PARALLELXT || defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the left in x direction */
  /* recieve the data from the neighbour on the right in x direction */
  MPI_Sendrecv((void*)l,                    1, lfield_x_slice_gath, g_nb_x_dn, 5091, 
	       (void*)(l+(T+2)*LX*LY*LZ), 1, lfield_x_slice_cont, g_nb_x_up, 5091,
	       g_cart_grid, &status);
    
  /* send the data to the neighbour on the right in x direction */
  /* recieve the data from the neighbour on the left in x direction */  
  MPI_Sendrecv((void*)(l+(LX-1)*LY*LZ),               1, lfield_x_slice_gath, g_nb_x_up, 5092, 
	       (void*)(l+((T+2)*LX*LY*LZ + T*LY*LZ)), 1, lfield_x_slice_cont, g_nb_x_dn, 5092,
	       g_cart_grid, &status);
    
#    endif
    
#    if (defined PARALLELXYT || defined PARALLELXYZT)
  /* send the data to the neighbour on the left in y direction */
  /* recieve the data from the neighbour on the right in y direction */
  MPI_Sendrecv((void*)l,                                1, lfield_y_slice_gath, g_nb_y_dn, 5101, 
	       (void*)(l+((T+2)*LX*LY*LZ + 2*T*LY*LZ)), 1, lfield_y_slice_cont, g_nb_y_up, 5101,
	       g_cart_grid, &status);
    
  /* send the data to the neighbour on the right in y direction */
  /* recieve the data from the neighbour on the left in y direction */  
  MPI_Sendrecv((void*)(l+(LY-1)*LZ/2),                            1, lfield_y_slice_gath, g_nb_y_up, 5102, 
	       (void*)(l+((T+2)*LX*LY*LZ + 2*T*LY*LZ + T*LX*LZ)), 1, lfield_y_slice_cont, g_nb_y_dn, 5102,
	       g_cart_grid, &status);
    
#    endif
    
#    if (defined PARALLELXYZT)
  /* send the data to the neighbour on the left in z direction */
  /* recieve the data from the neighbour on the right in z direction */
  MPI_Sendrecv((void*)l, 
	       1, lfield_z_slice_gath, g_nb_z_dn, 5503,  
	       (void*)(l + VOLUME + 2*LZ*(LX*LY + T*LY) + 2*LZ*T*LX),  
	       1, lfield_z_slice_cont, g_nb_z_up, 5503, 
	       g_cart_grid, &status); 
    
  /* send the data to the neighbour on the right in y direction */
  /* recieve the data from the neighbour on the left in y direction */  
  MPI_Sendrecv((void*)(l+LZ-1),  
	       1, lfield_z_slice_gath, g_nb_z_up, 5504, 
	       (void*)(l+(VOLUME + 2*LX*LY*LZ + 2*T*LY*LZ + 2*T*LX*LZ + T*LX*LY)),  
	       1, lfield_z_slice_cont, g_nb_z_dn, 5504, 
	       g_cart_grid, &status); 
    
#    endif
#  endif
  return;
#ifdef _KOJAK_INST
#pragma pomp inst end(xchange_lexicfield)
#endif
}


#endif
