#ifndef __KOKKOSFFT_TRANSFORM_HPP__
#define __KOKKOSFFT_TRANSFORM_HPP__

#include <Kokkos_Core.hpp>
#include "KokkosFFT_default_types.hpp"
#include "KokkosFFT_utils.hpp"
#include "KokkosFFT_normalization.hpp"
#include "KokkosFFT_Plans.hpp"

#if defined(KOKKOS_ENABLE_CUDA)
  using default_device = Kokkos::Cuda;
  #include "KokkosFFT_Cuda_transform.hpp"
#elif defined(KOKKOS_ENABLE_HIP)
  using default_device = Kokkos::HIP;
  #include "KokkosFFT_HIP_transform.hpp"
#elif defined(KOKKOS_ENABLE_OPENMP)
  using default_device = Kokkos::OpenMP;
  #include "KokkosFFT_OpenMP_transform.hpp"
#elif defined(KOKKOS_ENABLE_THREADS)
  using default_device = Kokkos::Threads;
  #include "KokkosFFT_OpenMP_transform.hpp"
#else
  using default_device = Kokkos::Serial;
  #include "KokkosFFT_OpenMP_transform.hpp"
#endif

// 1D Transform
namespace KokkosFFT {
  template <typename PlanType, typename InViewType, typename OutViewType>
  void _fft(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    auto* idata = reinterpret_cast<typename fft_data_type<in_value_type>::type*>(in.data());
    auto* odata = reinterpret_cast<typename fft_data_type<out_value_type>::type*>(out.data());

    _exec(plan.plan(), idata, odata, KOKKOS_FFT_FORWARD);
    normalize(out, KOKKOS_FFT_FORWARD, norm, plan.fft_size());
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _ifft(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    auto* idata = reinterpret_cast<typename fft_data_type<in_value_type>::type*>(in.data());
    auto* odata = reinterpret_cast<typename fft_data_type<out_value_type>::type*>(out.data());

    _exec(plan.plan(), idata, odata, KOKKOS_FFT_BACKWARD);
    normalize(out, KOKKOS_FFT_BACKWARD, norm, plan.fft_size());
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _rfft(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(std::is_floating_point<in_value_type>::value,
                  "KokkosFFT::rfft: InViewType must be real");
    static_assert(is_complex<out_value_type>::value,
                  "KokkosFFT::rfft: OutViewType must be complex");

    _fft(plan, in, out, norm, axis);
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _irfft(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(is_complex<in_value_type>::value,
                  "KokkosFFT::irfft: InViewType must be complex");
    static_assert(std::is_floating_point<out_value_type>::value,
                  "KokkosFFT::irfft: OutViewType must be real");

    _ifft(plan, in, out, norm, axis);
  }

  template <typename InViewType, typename OutViewType>
  void fft(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: OutViewType is not a Kokkos::View.");

    Plan plan(in, out, KOKKOS_FFT_FORWARD, axis);
    _fft(plan, in, out, norm, axis);
  }

  template <typename InViewType, typename OutViewType>
  void ifft(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: OutViewType is not a Kokkos::View.");

    Plan plan(in, out, KOKKOS_FFT_BACKWARD, axis);
    _ifft(plan, in, out, norm, axis);
  }

  template <typename InViewType, typename OutViewType>
  void rfft(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(std::is_floating_point<in_value_type>::value,
                  "KokkosFFT::rfft: InViewType must be real");
    static_assert(is_complex<out_value_type>::value,
                  "KokkosFFT::rfft: OutViewType must be complex");

    Plan plan(in, out, KOKKOS_FFT_FORWARD, axis);
    _rfft(plan, in, out, norm, axis);
  }

  template <typename InViewType, typename OutViewType>
  void irfft(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, int axis=-1) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(is_complex<in_value_type>::value,
                  "KokkosFFT::irfft: InViewType must be complex");
    static_assert(std::is_floating_point<out_value_type>::value,
                  "KokkosFFT::irfft: OutViewType must be real");

    Plan plan(in, out, KOKKOS_FFT_BACKWARD, axis);
    _irfft(plan, in, out, norm, axis);
  }
};

namespace KokkosFFT {
  using axis2D_type = axis_type<2>;

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _fft2(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    auto* idata = reinterpret_cast<typename fft_data_type<in_value_type>::type*>(in.data());
    auto* odata = reinterpret_cast<typename fft_data_type<out_value_type>::type*>(out.data());

    _exec(plan.plan(), idata, odata, KOKKOS_FFT_FORWARD);
    normalize(out, KOKKOS_FFT_FORWARD, norm, plan.fft_size());
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _ifft2(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    auto* idata = reinterpret_cast<typename fft_data_type<in_value_type>::type*>(in.data());
    auto* odata = reinterpret_cast<typename fft_data_type<out_value_type>::type*>(out.data());

    _exec(plan.plan(), idata, odata, KOKKOS_FFT_BACKWARD);
    normalize(out, KOKKOS_FFT_BACKWARD, norm, plan.fft_size());
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _rfft2(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(std::is_floating_point<in_value_type>::value,
                  "KokkosFFT::rfft: InViewType must be real");
    static_assert(is_complex<out_value_type>::value,
                  "KokkosFFT::rfft: OutViewType must be complex");

    _fft2(plan, in, out, norm, axes);
  }

  template <typename PlanType, typename InViewType, typename OutViewType>
  void _irfft2(PlanType& plan, const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(is_complex<in_value_type>::value,
                  "KokkosFFT::irfft: InViewType must be complex");
    static_assert(std::is_floating_point<out_value_type>::value,
                  "KokkosFFT::irfft: OutViewType must be real");

    _ifft2(plan, in, out, norm, axes);
  }

  template <typename InViewType, typename OutViewType>
  void fft2(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::fft: OutViewType is not a Kokkos::View.");

    Plan plan(in, out, KOKKOS_FFT_FORWARD, axes);
    _fft2(plan, in, out, norm, axes);
  }

  template <typename InViewType, typename OutViewType>
  void ifft2(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::ifft: OutViewType is not a Kokkos::View.");

    Plan plan(in, out, KOKKOS_FFT_BACKWARD, axes);
    _ifft2(plan, in, out, norm, axes);
  }

  template <typename InViewType, typename OutViewType>
  void rfft2(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::rfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(std::is_floating_point<in_value_type>::value,
                  "KokkosFFT::rfft: InViewType must be real");
    static_assert(is_complex<out_value_type>::value,
                  "KokkosFFT::rfft: OutViewType must be complex");

    Plan plan(in, out, KOKKOS_FFT_FORWARD, axes);
    _rfft2(plan, in, out, norm, axes);
  }

  template <typename InViewType, typename OutViewType>
  void irfft2(const InViewType& in, const OutViewType& out, FFT_Normalization norm=FFT_Normalization::BACKWARD, axis2D_type axes={-2, -1}) {
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: InViewType is not a Kokkos::View.");
    static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::irfft: OutViewType is not a Kokkos::View.");

    using in_value_type = typename InViewType::non_const_value_type;
    using out_value_type = typename OutViewType::non_const_value_type;

    static_assert(is_complex<in_value_type>::value,
                  "KokkosFFT::irfft: InViewType must be complex");
    static_assert(std::is_floating_point<out_value_type>::value,
                  "KokkosFFT::irfft: OutViewType must be real");

    Plan plan(in, out, KOKKOS_FFT_BACKWARD, axes);
    _irfft2(plan, in, out, norm, axes);
  }
};

#endif