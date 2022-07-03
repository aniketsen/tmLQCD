#!/bin/bash

module load gnu/8.4.0  blas/3.8.0--gnu--8.4.0  spectrum_mpi lapack/3.9.0--gnu--8.4.0 
module load cmake
module load cuda


#bash /m100_work/INF22_lqcd123_0/romiti/load_modules.sh

make -j64
