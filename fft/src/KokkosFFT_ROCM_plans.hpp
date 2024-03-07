// SPDX-License-Identifier: MIT

#ifndef KOKKOSFFT_ROCM_PLANS_HPP
#define KOKKOSFFT_ROCM_PLANS_HPP

#include <numeric>
#include <algorithm>
#include "KokkosFFT_ROCM_types.hpp"
#include "KokkosFFT_layouts.hpp"

namespace KokkosFFT {
namespace Impl {
// Helper to get input and output array type and direction from transform type
template <typename TransformType>
auto get_in_out_array_type(TransformType type,
                           [[maybe_unused]] Direction direction) {
  rocfft_array_type in_array_type, out_array_type;
  rocfft_transform_type fft_direction;

  if (type == FFTWTransformType::C2C || type == FFTWTransformType::Z2Z) {
    in_array_type  = rocfft_array_type_complex_interleaved;
    out_array_type = rocfft_array_type_complex_interleaved;
    fft_direction  = direction == Direction::forward
                         ? rocfft_transform_type_complex_forward
                         : rocfft_transform_type_complex_inverse;
  } else if (type == FFTWTransformType::R2C || type == FFTWTransformType::D2Z) {
    in_array_type  = rocfft_array_type_real;
    out_array_type = rocfft_array_type_hermitian_interleaved;
    fft_direction  = rocfft_transform_type_real_forward;
  } else if (type == FFTWTransformType::C2R || type == FFTWTransformType::Z2D) {
    in_array_type  = rocfft_array_type_hermitian_interleaved;
    out_array_type = rocfft_array_type_real;
    fft_direction  = rocfft_transform_type_real_inverse;
  }

  return std::tuple<rocfft_array_type, rocfft_array_type,
                    rocfft_transform_type>(
      {in_array_type, out_array_type, fft_direction});
};

template <typename ValueType>
rocfft_precision get_in_out_array_type() {
  return std::is_same_v<KokkosFFT::Impl::real_type_t<ValueType>, float>
             ? rocfft_precision_single
             : rocfft_precision_double;
}

// Helper to convert the integer type of vectors
template <typename InType, typename OutType>
auto convert_int_type_and_reverse(std::vector<InType>& in)
    -> std::vector<OutType> {
  std::vector<OutType> out(in.size());
  std::transform(
      in.begin(), in.end(), out.begin(),
      [](const InType v) -> OutType { return static_cast<OutType>(v); });

  std::reverse(out.begin(), out.end());
  return out;
}

// Helper to compute strides from extents
// (n0, n1, n2) -> (1, n0, n0*n1)
// (n0, n1) -> (1, n0)
// (n0) -> (1)
template <typename InType, typename OutType>
auto compute_strides(const std::vector<InType>& extents)
    -> std::vector<OutType> {
  std::vector<OutType> out = {1};
  auto reversed_extents    = extents;
  std::reverse(reversed_extents.begin(), reversed_extents.end());

  for (std::size_t i = 1; i < reversed_extents.size(); i++) {
    out.push_back(static_cast<OutType>(reversed_extents.at(i - 1)) *
                  out.at(i - 1));
  }

  return out;
}

// batched transform, over ND Views
template <typename ExecutionSpace, typename PlanType, typename InViewType,
          typename OutViewType, typename BufferViewType, typename InfoType,
          std::size_t fft_rank             = 1,
          std::enable_if_t<std::is_same_v<ExecutionSpace, Kokkos::HIP>,
                           std::nullptr_t> = nullptr>
auto _create(const ExecutionSpace& exec_space, std::unique_ptr<PlanType>& plan,
             const InViewType& in, const OutViewType& out,
             BufferViewType& buffer, InfoType& execution_info,
             [[maybe_unused]] Direction direction, axis_type<fft_rank> axes) {
  static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::_create: InViewType is not a Kokkos::View.");
  static_assert(Kokkos::is_view<InViewType>::value,
                "KokkosFFT::_create: OutViewType is not a Kokkos::View.");
  using in_value_type  = typename InViewType::non_const_value_type;
  using out_value_type = typename OutViewType::non_const_value_type;

  static_assert(
      InViewType::rank() >= fft_rank,
      "KokkosFFT::_create: Rank of View must be larger than Rank of FFT.");
  const int rank = fft_rank;

  constexpr auto type =
      KokkosFFT::Impl::transform_type<ExecutionSpace, in_value_type,
                                      out_value_type>::type();
  auto [in_extents, out_extents, fft_extents, howmany] =
      KokkosFFT::Impl::get_extents_batched(in, out, axes);
  int idist    = std::accumulate(in_extents.begin(), in_extents.end(), 1,
                              std::multiplies<>());
  int odist    = std::accumulate(out_extents.begin(), out_extents.end(), 1,
                              std::multiplies<>());
  int fft_size = std::accumulate(fft_extents.begin(), fft_extents.end(), 1,
                                 std::multiplies<>());

  auto* idata = reinterpret_cast<typename KokkosFFT::Impl::fft_data_type<
      ExecutionSpace, in_value_type>::type*>(in.data());
  auto* odata = reinterpret_cast<typename KokkosFFT::Impl::fft_data_type<
      ExecutionSpace, out_value_type>::type*>(out.data());

  // For the moment, considering the contiguous layout only
  // Create plan
  auto in_strides  = compute_strides<int, std::size_t>(in_extents);
  auto out_strides = compute_strides<int, std::size_t>(out_extents);
  auto _fft_extents =
      convert_int_type_and_reverse<int, std::size_t>(fft_extents);

  // Create the description
  rocfft_plan_description description;
  rocfft_status status = rocfft_plan_description_create(&description);
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_plan_description_create failed");

  auto [in_array_type, out_array_type, fft_direction] =
      get_in_out_array_type(type, direction);
  rocfft_precision precision = get_in_out_array_type<in_value_type>();

  status = rocfft_plan_description_set_data_layout(
      description,         // description handle
      in_array_type,       // input array type
      out_array_type,      // output array type
      nullptr,             // offsets to start of input data
      nullptr,             // offsets to start of output data
      in_strides.size(),   // input stride length
      in_strides.data(),   // input stride data
      idist,               // input batch distance
      out_strides.size(),  // output stride length
      out_strides.data(),  // output stride data
      odist);              // output batch distance
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_plan_description_set_data_layout failed");

  // Out-of-place transform
  const rocfft_result_placement place = rocfft_placement_notinplace;

  // Create a plan
  plan   = std::make_unique<PlanType>();
  status = rocfft_plan_create(&(*plan), place, fft_direction, precision,
                              _fft_extents.size(),  // Dimension
                              _fft_extents.data(),  // Lengths
                              howmany,              // Number of transforms
                              description           // Description
  );
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_plan_create failed");

  // Prepare workbuffer and set execution information
  status = rocfft_execution_info_create(&execution_info);
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_execution_info_create failed");

  // set stream
  // NOTE: The stream must be of type hipStream_t.
  // It is an error to pass the address of a hipStream_t object.
  hipStream_t stream = exec_space.hip_stream();
  status             = rocfft_execution_info_set_stream(execution_info, stream);
  if (status != rocfft_status_success)
    throw std::runtime_error("rocfft_execution_info_set_stream failed.");

  std::size_t workbuffersize = 0;
  status = rocfft_plan_get_work_buffer_size(*plan, &workbuffersize);
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_plan_get_work_buffer_size failed");

  if (workbuffersize > 0) {
    buffer = BufferViewType("work_buffer", workbuffersize);
    status = rocfft_execution_info_set_work_buffer(
        execution_info, (void*)buffer.data(), workbuffersize);
    if (status != rocfft_status_success)
      std::runtime_error("rocfft_execution_info_set_work_buffer failed");
  }

  status = rocfft_plan_description_destroy(description);
  if (status != rocfft_status_success)
    std::runtime_error("rocfft_plan_description_destroy failed");

  return fft_size;
}

template <typename ExecutionSpace, typename PlanType,
          std::enable_if_t<std::is_same_v<ExecutionSpace, Kokkos::HIP>,
                           std::nullptr_t> = nullptr>
void _destroy_plan(std::unique_ptr<PlanType>& plan) {
  rocfft_plan_destroy(*plan);
}

template <typename ExecutionSpace, typename InfoType,
          std::enable_if_t<std::is_same_v<ExecutionSpace, Kokkos::HIP>,
                           std::nullptr_t> = nullptr>
void _destroy_info(InfoType& execution_info) {
  rocfft_execution_info_destroy(execution_info);
}

}  // namespace Impl
}  // namespace KokkosFFT

#endif