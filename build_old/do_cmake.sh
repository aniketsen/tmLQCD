#!/bin/bash
#cp /usr/lib/rpm/config.guess ..
#cp /usr/lib/rpm/config.sub ..
#cd ..
#autoconf
#cd build

#module load gnu/8.4.0
#module load blas/3.8.0--gnu--8.4.0
#module load spectrum_mpi/10.4.0--binary 
#module load lapack/3.9.0--gnu--8.4.0
#module load scalapack/2.1.0--spectrum_mpi--10.4.0--binary

#module load gnu/9.3.0  blas/3.8.0--gnu--9.3.0  spectrum_mpi lapack/3.9.0--gnu--9.3.0 
module load gnu/8.4.0  blas/3.8.0--gnu--8.4.0  spectrum_mpi lapack/3.9.0--gnu--8.4.0 
module load cmake cuda



qudadir=/m100_work/INF22_lqcd123_0/romiti/quda/build
export LD_LIBRARY_PAT=${qudadir}/lib:${LD_LIBRARY_PATH} \
#CC=mpiicc CXX=mpiicpc F77=ifort \
CC=mpicc CXX=mpicxx F77=gfortran \
LD=mpicxx \
CFLAGS="-O2 -fopenmp -std=c99 -I/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/include  -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib  -mtune=power9 " \
CXXFLAGS="-O2 -fopenmp -std=c++11 -I/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/include   -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib  -mtune=power9 " \
LDFLAGS="-fopenmp -L/cineca/prod/opt/libraries/blas/3.8.0/gnu--8.4.0/lib -lblas -L/cineca/prod/opt/libraries/lapack/3.9.0/gnu--8.4.0/lib/  -llapack  -L${CUDA_LIB} " \
FFLAGS="-g" \
../configure \
  --enable-halfspinor --enable-gaugecopy \
  --with-limedir=/m100_work/INF22_lqcd123_0/romiti/c-lime/install \
  --enable-mpi --with-mpidimension=4 --enable-omp \
  --disable-sse2 --disable-sse3 \
  --with-qudadir=${qudadir} \
  --enable-alignment=32  \
  --enable-quda_experimental \
  --with-cudadir=${CUDA_LIB}

#  --with-lemondir=/m100/home/userexternal/mgarofal/lemon/install \
#  --with-lapack="-Wl -liomp5 -lpthread -lm -ldl"\
#--with-lapack="-llapack -lblas"
#  --with-lapack="-llapack -lblas"\

