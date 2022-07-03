#!/bin/bash

module load gnu/8.4.0  blas/3.8.0--gnu--8.4.0  spectrum_mpi lapack/3.9.0--gnu--8.4.0 
module load cmake cuda

qudadir=/m100_work/INF22_lqcd123_0/romiti/quda/build

export LD_LIBRARY_PAT=${qudadir}/lib:${LD_LIBRARY_PATH} 

export CC=mpicc CXX=mpicxx F77=gfortran 

export LD=mpicxx

export CFLAGS="-O2 -fopenmp -std=c99 -I/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/include  -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib  -mtune=power9 " 

export CXXFLAGS="-O2 -fopenmp -std=c++11 -I/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/include   -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib  -mtune=power9 " 

export LDFLAGS="-fopenmp -L/cineca/prod/opt/libraries/blas/3.8.0/gnu--8.4.0/lib -lblas -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib/  -llapack  -L${CUDA_LIB} "

export FFLAGS="-g"

../configure \
  --enable-halfspinor --enable-gaugecopy \
  --with-limedir=/m100_work/INF22_lqcd123_0/romiti/c-lime/install \
  --enable-mpi --with-mpidimension=4 --enable-omp \
  --disable-sse2 --disable-sse3 \
  --with-qudadir=${qudadir} \
  --enable-alignment=32  \
  --enable-quda_experimental \
  --with-cudadir=${CUDA_LIB}

