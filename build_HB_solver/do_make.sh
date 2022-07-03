#!/bin/bash

module load gnu/8.4.0  blas/3.8.0--gnu--8.4.0  spectrum_mpi lapack/3.9.0--gnu--8.4.0 
module load cmake
module load cuda

make -j64
