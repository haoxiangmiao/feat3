#pragma once
#ifndef KERNEL_LAFEM_ARCH_MAX_ELEMENT_GENERIC_HPP
#define KERNEL_LAFEM_ARCH_MAX_ELEMENT_GENERIC_HPP 1

#ifndef KERNEL_LAFEM_ARCH_MAX_ELEMENT_HPP
#error "Do not include this implementation-only header file directly!"
#endif

#include <kernel/util/math.hpp>

namespace FEAT
{
  namespace LAFEM
  {
    namespace Arch
    {
      template <typename DT_>
      Index MaxElement<Mem::Main>::value_generic(const DT_ * const x, const Index size)
      {
        DT_ max(0);
        Index max_i(0);

        for (Index i(0) ; i < size ; ++i)
        {
          if (Math::abs(x[i]) > max)
          {
            max = Math::abs(x[i]);
            max_i = i;
          }
        }

        return max_i;
      }

    } // namespace Arch
  } // namespace LAFEM
} // namespace FEAT

#endif // KERNEL_LAFEM_ARCH_MAX_ELEMENT_GENERIC_HPP