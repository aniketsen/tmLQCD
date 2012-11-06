/***********************************************************************
 *
 * Copyright (C) 2001 Martin Hasenbusch
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
 * File observables.c
 *
 *
 * The externally accessible functions are
 *
 *   double measure_gauge_action(void)
 *     Returns the value of the action
 ************************************************************************/

#ifdef HAVE_CONFIG_H
# include<config.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#ifdef OMP
# include <omp.h>
#endif
#include "su3.h"
#include "su3adj.h"
#include "sse.h"
#include "geometry_eo.h"
#include "global.h"
#include <io/params.h>
#include "measure_gauge_action.h"
#include <buffers/gauge.h>

double measure_gauge_action(gauge_field_t const gf) {
  static double res;
#ifdef MPI
  double ALIGN mres;
#endif

#ifdef OMP
#pragma omp parallel
  {
  int thread_num = omp_get_thread_num();
#endif

  int ix,ix1,ix2,mu1,mu2;
  su3 ALIGN pr1,pr2; 
  const su3 *v,*w;
  double ALIGN ac,ks,kc,tr,ts,tt;

  if(g_update_gauge_energy) {
    kc=0.0; ks=0.0;
#ifdef OMP
#pragma omp for
#endif
    for (ix=0;ix<VOLUME;ix++){
      for (mu1=0;mu1<3;mu1++){ 
	ix1=g_iup[ix][mu1];
	for (mu2=mu1+1;mu2<4;mu2++){ 
	  ix2=g_iup[ix][mu2];
	  v=&gf[ix][mu1];
	  w=&gf[ix1][mu2];
	  _su3_times_su3(pr1,*v,*w);
	  v=&gf[ix][mu2];
	  w=&gf[ix2][mu1];
	  _su3_times_su3(pr2,*v,*w);
	  _trace_su3_times_su3d(ac,pr1,pr2);
	  tr=ac+kc;
	  ts=tr+ks;
	  tt=ts-ks;
	  ks=ts;
	  kc=tr-tt;
	}
      }
    }
    kc=(kc+ks)/3.0;
#ifdef OMP
    g_omp_acc_re[thread_num] = kc;
#else
    res = kc;
#endif
  }

#ifdef OMP
  } /* OpenMP parallel closing brace */

  if(g_update_gauge_energy) {
    res = 0.0;
    for(int i=0; i < omp_num_threads; ++i)
      res += g_omp_acc_re[i];
#else
  if(g_update_gauge_energy) {
#endif
#ifdef MPI
    MPI_Allreduce(&res, &mres, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    res = mres;
#endif
    GaugeInfo.plaquetteEnergy = res;
    g_update_gauge_energy = 0;
  }
  return res;
}
