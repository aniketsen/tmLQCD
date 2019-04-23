/***********************************************************************
 * Copyright (C) 2002,2003,2004,2005,2006,2007,2008 Carsten Urbach
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
 ***********************************************************************/

#ifdef HAVE_CONFIG_H
# include<tmlqcd_config.h>
#endif
#include <stdlib.h>
#include <complex.h>
#ifdef TM_USE_OMP
# include <omp.h>
#endif
#include "su3.h"
#include "assign_mul_add_r.h"


#if ( defined SSE2 || defined SSE3 )
#include "sse.h"

/* k input , l output*/
void assign_mul_add_r(spinor * const R, const double c, const spinor * const S, const int N) {
#ifdef TM_USE_OMP
#pragma omp parallel
  {
#endif
  int ix;
  su3_vector *s,*r;
  __asm__ __volatile__ ("movsd %0, %%xmm7 \n\t"
			"unpcklpd %%xmm7, %%xmm7"
			:
			:
			"m" (c));
#ifndef TM_USE_OMP
  s=&S[0].s0;
  r=&R[0].s0;
#else
#pragma omp for
#endif
  for (ix=0;ix<4*N;ix++) {
#ifdef TM_USE_OMP
  s=&S[0].s0+ix;
  r=&R[0].s0+ix;
#endif
    _sse_load(*r);
    __asm__ __volatile__ ("mulpd %%xmm7, %%xmm0 \n\t"
			  "mulpd %%xmm7, %%xmm1 \n\t"
			  "mulpd %%xmm7, %%xmm2"
			  :
			  :);
    _sse_load_up(*s);
    __asm__ __volatile__ ("addpd %%xmm3, %%xmm0 \n\t"
			  "addpd %%xmm4, %%xmm1 \n\t"
			  "addpd %%xmm5, %%xmm2"
			  :
			  :);
    _sse_store(*r);
#ifndef TM_USE_OMP
    s++; r++;
#endif
  }

#ifdef TM_USE_OMP
  } /* OpenMP closing brace */
#endif  
}

#elif (defined BGQ && defined XLC)

void assign_mul_add_r(spinor * const R, const double c, const spinor * const S, const int N) {
#ifdef TM_USE_OMP
#pragma omp parallel
  {
#endif

  vector4double x0, x1, x2, x3, x4, x5, y0, y1, y2, y3, y4, y5;
  vector4double z0, z1, z2, z3, z4, z5, k;
  double *s, *r;
  double ALIGN _c;
  _c = c;
  __prefetch_by_load(S);
  __prefetch_by_load(R);

  k = vec_splats(_c);
  __alignx(32, s);
  __alignx(32, r);
  __alignx(32, S);
  __alignx(32, R);

#ifdef TM_USE_OMP
#pragma omp for
#else
#pragma unroll(4)
#endif
  for(int i = 0; i < N; i++) {
    s=(double*)((spinor *) S + i);
    r=(double*)((spinor *) R + i);
    __prefetch_by_load(S + i + 1);
    __prefetch_by_stream(1, R + i + 1);
    x0 = vec_ld(0, r);
    x1 = vec_ld(0, r+4);
    x2 = vec_ld(0, r+8);
    x3 = vec_ld(0, r+12);
    x4 = vec_ld(0, r+16);
    x5 = vec_ld(0, r+20);
    y0 = vec_ld(0, s);
    y1 = vec_ld(0, s+4);
    y2 = vec_ld(0, s+8);
    y3 = vec_ld(0, s+12);
    y4 = vec_ld(0, s+16);
    y5 = vec_ld(0, s+20);
    z0 = vec_madd(k, x0, y0);
    z1 = vec_madd(k, x1, y1);
    z2 = vec_madd(k, x2, y2);
    z3 = vec_madd(k, x3, y3);
    z4 = vec_madd(k, x4, y4);
    z5 = vec_madd(k, x5, y5);
    vec_st(z0, 0, r);
    vec_st(z1, 0, r+4);
    vec_st(z2, 0, r+8);
    vec_st(z3, 0, r+12);
    vec_st(z4, 0, r+16);
    vec_st(z5, 0, r+20);
  }
#ifdef TM_USE_OMP
  } /* OpenMP closing brace */
#endif  
  return;
}

#elif ((defined BGL) && (defined XLC))

#  include"bgl.h"

void assign_mul_add_r(spinor * const R, const double c, const spinor * const S, const int N) {
  int ix = 1;
  const double *s ALIGN;
  const double *sp ALIGN;
  double *r ALIGN;
  double *rp ALIGN;
  double _Complex x00, x01, x02, x03, x04, x05, x06, x07, 
    x08, x09, x10, x11;
  double _Complex y00, y01, y02, y03, y04, y05, y06, y07, 
    y08, y09, y10, y11;
  double _Complex a;

#pragma disjoint(*S, *R)
  a = __cmplx(c, c);
  __alignx(16, S);
  __alignx(16, R);
  s = (double*) S;
  r = (double*) R;
  rp = r + 24;
  sp = s + 24;
  _prefetch_spinor(rp);
  _prefetch_spinor(sp);
  x00 = __lfpd(r);    
  x01 = __lfpd(r+2);  
  x02 = __lfpd(r+4);  
  x03 = __lfpd(r+6);  
  x04 = __lfpd(r+8);  
  x05 = __lfpd(r+10); 
  x06 = __lfpd(r+12); 
  x07 = __lfpd(r+14); 
  x08 = __lfpd(r+16); 
  x09 = __lfpd(r+18); 
  x10 = __lfpd(r+20); 
  x11 = __lfpd(r+22); 
  y00 = __lfpd(s);   
  y01 = __lfpd(s+2); 
  y02 = __lfpd(s+4); 
  y03 = __lfpd(s+6); 
  y04 = __lfpd(s+8); 
  y05 = __lfpd(s+10);
  y06 = __lfpd(s+12);
  y07 = __lfpd(s+14);
  y08 = __lfpd(s+16);
  y09 = __lfpd(s+18);
  y10 = __lfpd(s+20);
  y11 = __lfpd(s+22);

  y00 = __fpmadd(y00, x00, a);
  y01 = __fpmadd(y01, x01, a);
  y02 = __fpmadd(y02, x02, a);
  y03 = __fpmadd(y03, x03, a);
  y04 = __fpmadd(y04, x04, a);
  y05 = __fpmadd(y05, x05, a);
  y06 = __fpmadd(y06, x06, a);
  y07 = __fpmadd(y07, x07, a);
  y08 = __fpmadd(y08, x08, a);
  y09 = __fpmadd(y09, x09, a);
  y10 = __fpmadd(y10, x10, a);
  y11 = __fpmadd(y11, x11, a);
  __stfpd(r, y00);
  __stfpd(r+2, y01);
  __stfpd(r+4, y02);
  __stfpd(r+6, y03);
  __stfpd(r+8, y04);
  __stfpd(r+10, y05);
  __stfpd(r+12, y06);
  __stfpd(r+14, y07);
  __stfpd(r+16, y08);
  __stfpd(r+18, y09);
  __stfpd(r+20, y10);
  __stfpd(r+22, y11);
  s = sp;
  r = rp;

#pragma unroll(12)
  for(ix = 1; ix < N-1; ix++) {
    rp += 24;
    sp += 24;
    _prefetch_spinor(rp);
    _prefetch_spinor(sp);
    x00 = __lfpd(r);    
    x01 = __lfpd(r+2);  
    x02 = __lfpd(r+4);  
    x03 = __lfpd(r+6);  
    x04 = __lfpd(r+8);  
    x05 = __lfpd(r+10); 
    x06 = __lfpd(r+12); 
    x07 = __lfpd(r+14); 
    x08 = __lfpd(r+16); 
    x09 = __lfpd(r+18); 
    x10 = __lfpd(r+20); 
    x11 = __lfpd(r+22); 
    y00 = __lfpd(s);   
    y01 = __lfpd(s+2); 
    y02 = __lfpd(s+4); 
    y03 = __lfpd(s+6); 
    y04 = __lfpd(s+8); 
    y05 = __lfpd(s+10);
    y06 = __lfpd(s+12);
    y07 = __lfpd(s+14);
    y08 = __lfpd(s+16);
    y09 = __lfpd(s+18);
    y10 = __lfpd(s+20);
    y11 = __lfpd(s+22);

    y00 = __fpmadd(y00, x00, a);
    y01 = __fpmadd(y01, x01, a);
    y02 = __fpmadd(y02, x02, a);
    y03 = __fpmadd(y03, x03, a);
    y04 = __fpmadd(y04, x04, a);
    y05 = __fpmadd(y05, x05, a);
    y06 = __fpmadd(y06, x06, a);
    y07 = __fpmadd(y07, x07, a);
    y08 = __fpmadd(y08, x08, a);
    y09 = __fpmadd(y09, x09, a);
    y10 = __fpmadd(y10, x10, a);
    y11 = __fpmadd(y11, x11, a);
    __stfpd(r, y00);
    __stfpd(r+2, y01);
    __stfpd(r+4, y02);
    __stfpd(r+6, y03);
    __stfpd(r+8, y04);
    __stfpd(r+10, y05);
    __stfpd(r+12, y06);
    __stfpd(r+14, y07);
    __stfpd(r+16, y08);
    __stfpd(r+18, y09);
    __stfpd(r+20, y10);
    __stfpd(r+22, y11);
    s = sp;
    r = rp;

  }
  x00 = __lfpd(r);    
  x01 = __lfpd(r+2);  
  x02 = __lfpd(r+4);  
  x03 = __lfpd(r+6);  
  x04 = __lfpd(r+8);  
  x05 = __lfpd(r+10); 
  x06 = __lfpd(r+12); 
  x07 = __lfpd(r+14); 
  x08 = __lfpd(r+16); 
  x09 = __lfpd(r+18); 
  x10 = __lfpd(r+20); 
  x11 = __lfpd(r+22); 
  y00 = __lfpd(s);   
  y01 = __lfpd(s+2); 
  y02 = __lfpd(s+4); 
  y03 = __lfpd(s+6); 
  y04 = __lfpd(s+8); 
  y05 = __lfpd(s+10);
  y06 = __lfpd(s+12);
  y07 = __lfpd(s+14);
  y08 = __lfpd(s+16);
  y09 = __lfpd(s+18);
  y10 = __lfpd(s+20);
  y11 = __lfpd(s+22);

  y00 = __fpmadd(y00, x00, a);
  y01 = __fpmadd(y01, x01, a);
  y02 = __fpmadd(y02, x02, a);
  y03 = __fpmadd(y03, x03, a);
  y04 = __fpmadd(y04, x04, a);
  y05 = __fpmadd(y05, x05, a);
  y06 = __fpmadd(y06, x06, a);
  y07 = __fpmadd(y07, x07, a);
  y08 = __fpmadd(y08, x08, a);
  y09 = __fpmadd(y09, x09, a);
  y10 = __fpmadd(y10, x10, a);
  y11 = __fpmadd(y11, x11, a);
  __stfpd(r, y00);
  __stfpd(r+2, y01);
  __stfpd(r+4, y02);
  __stfpd(r+6, y03);
  __stfpd(r+8, y04);
  __stfpd(r+10, y05);
  __stfpd(r+12, y06);
  __stfpd(r+14, y07);
  __stfpd(r+16, y08);
  __stfpd(r+18, y09);
  __stfpd(r+20, y10);
  __stfpd(r+22, y11);

  return;
}

#else

/* R inoutput , c,S input*/
/*   (*R) = c*(*R) + (*S)        c is a real constant   */

void assign_mul_add_r(spinor * const R, const double c, const spinor * const S, const int N)
{
#ifdef TM_USE_OMP
#pragma omp parallel
  {
#endif
  spinor *r;
  const spinor *s;
  
  /* Change due to even-odd preconditioning : VOLUME   to VOLUME/2 */   
#ifdef TM_USE_OMP
#pragma omp for
#endif
  for (int ix = 0; ix < N; ++ix)
  {
    r = R + ix;
    s = S + ix;
    
    r->s0.c0 = c * r->s0.c0 + s->s0.c0;
    r->s0.c1 = c * r->s0.c1 + s->s0.c1;
    r->s0.c2 = c * r->s0.c2 + s->s0.c2;    

    r->s1.c0 = c * r->s1.c0 + s->s1.c0;
    r->s1.c1 = c * r->s1.c1 + s->s1.c1;
    r->s1.c2 = c * r->s1.c2 + s->s1.c2;    

    r->s2.c0 = c * r->s2.c0 + s->s2.c0;
    r->s2.c1 = c * r->s2.c1 + s->s2.c1;
    r->s2.c2 = c * r->s2.c2 + s->s2.c2;    

    r->s3.c0 = c * r->s3.c0 + s->s3.c0;
    r->s3.c1 = c * r->s3.c1 + s->s3.c1;
    r->s3.c2 = c * r->s3.c2 + s->s3.c2;   
  }
#ifdef TM_USE_OMP
  } /* OpenMP closing brace */
#endif
}

#endif

#ifdef WITHLAPH
void assign_mul_add_r_su3vect(su3_vector * const R, const double c, su3_vector * const S, const int N)
{
#ifdef TM_USE_OMP
#pragma omp parallel
  {
#endif
  su3_vector *r,*s;

#ifdef TM_USE_OMP
#pragma omp for
#endif
  for (int ix = 0; ix < N; ++ix) 
  {
    r = R + ix;
    s = S + ix;
    r->c0 = c * r->c0 + s->c0;
    r->c1 = c * r->c1 + s->c1;
    r->c2 = c * r->c2 + s->c2;    
  }
#ifdef TM_USE_OMP
  } /* OpenMP closing brace */
#endif
}
#endif
