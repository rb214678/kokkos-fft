#!/bin/bash

args=$#
WK_DIR=/tmp/kokkos_fft_as_subdirectory
PWD=/work
ROOT_DIR=$1
BUILD_TYPE=$2
C_COMPILER=$3
CXX_COMPILER=$4
BACKEND_FLAG=$5
TARGET_FLAG=$6

# Make work directory (Project directory)
mkdir -p ${WK_DIR} && cd ${WK_DIR}

# Copy CMakeLists.txt and hello.cpp
cp ${PWD}/install_test/as_subdirectory/* .

# Prepare Kokkos and KokkosFFT under tpls
mkdir tpls && cd tpls

# Prepare tpls/kokkos 
git clone https://github.com/kokkos/kokkos.git

# Copy KokkosFFT under tpls/kokkos-fft
cp -r ${PWD} .

# Try to build an example
# Build KokkosFFT code using installed KokkosFFT
cd ${WK_DIR}
mkdir ${EXAMPLE_BUILD_DIR} && cd ${EXAMPLE_BUILD_DIR}
cmake -DCMAKE_CXX_COMPILER=${CXX_COMPILER} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DKokkos_ENABLE_CUDA=ON \
      ${BACKEND_FLAG} \
      ${TARGET_FLAG}

cmake --build . -j 8

if [ $? -eq 0 ]; then
    echo "*** KokkosFFT as subdirectory test: build SUCCESSFUL ***"
else
    echo "*** KokkosFFT as subdirectory test: build FAILED ***"
    exit 1;
fi