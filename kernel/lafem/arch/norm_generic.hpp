#pragma once
#ifndef KERNEL_LAFEM_ARCH_NORM_GENERIC_HPP
#define KERNEL_LAFEM_ARCH_NORM_GENERIC_HPP 1

#ifndef KERNEL_LAFEM_ARCH_NORM_HPP
  #error "Do not include this implementation-only header file directly!"
#endif

#include <kernel/util/math.hpp>
#include <cmath>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::LAFEM::Arch;

template <typename DT_>
DT_ Norm2<Mem::Main, Algo::Generic>::value(const DT_ * const x, const Index size)
{
  DT_ r(0);
  for (Index i(0) ; i < size ; ++i)
  {
    r += x[i] * x[i];
  }

  return (DT_)Math::sqrt(r);
}

#endif // KERNEL_LAFEM_ARCH_NORM_GENERIC_HPP
