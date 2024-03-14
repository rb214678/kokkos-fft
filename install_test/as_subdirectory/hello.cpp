// SPDX-FileCopyrightText: (C) The Kokkos-FFT development team, see COPYRIGHT.md file
//
// SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception

#include <Kokkos_Core.hpp>
#include <Kokkos_Complex.hpp>
#include <Kokkos_Random.hpp>
#include <KokkosFFT.hpp>

using execution_space = Kokkos::DefaultExecutionSpace;
template <typename T>
using View2D = Kokkos::View<T**, execution_space>;

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    constexpr int n0 = 128, n1 = 128;
    const Kokkos::complex<double> I(1.0, 1.0);

    // 2D C2C FFT (Forward and Backward)
    View2D<Kokkos::complex<double> > xc2c("xc2c", n0, n1);
    View2D<Kokkos::complex<double> > xc2c_hat("xc2c_hat", n0, n1);
    View2D<Kokkos::complex<double> > xc2c_inv("xc2c_inv", n0, n1);

    Kokkos::Random_XorShift64_Pool<> random_pool(12345);
    execution_space exec;
    Kokkos::fill_random(exec, xc2c, random_pool, I);
    exec.fence();

    KokkosFFT::fft2(exec, xc2c, xc2c_hat);
    KokkosFFT::ifft2(exec, xc2c_hat, xc2c_inv);
    exec.fence();

    // 2D R2C FFT
    View2D<double> xr2c("xr2c", n0, n1);
    View2D<Kokkos::complex<double> > xr2c_hat("xr2c_hat", n0, n1 / 2 + 1);
    Kokkos::fill_random(exec, xr2c, random_pool, 1);
    exec.fence();

    KokkosFFT::rfft2(exec, xr2c, xr2c_hat);
    exec.fence();

    // 2D C2R FFT
    View2D<Kokkos::complex<double> > xc2r("xr2c_hat", n0, n1 / 2 + 1);
    View2D<double> xc2r_hat("xc2r", n0, n1);
    Kokkos::fill_random(exec, xc2r, random_pool, I);
    exec.fence();

    KokkosFFT::irfft2(exec, xc2r, xc2r_hat);
    exec.fence();
  }
  Kokkos::finalize();

  return 0;
}