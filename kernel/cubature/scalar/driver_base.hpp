#pragma once
#ifndef KERNEL_CUBATURE_SCALAR_DRIVER_BASE_HPP
#define KERNEL_CUBATURE_SCALAR_DRIVER_BASE_HPP 1

// includes, FEAT
#include <kernel/cubature/scalar/rule.hpp>

namespace FEAT
{
  namespace Cubature
  {
    namespace Scalar
    {
      /**
       * \brief Scalar cubature driver base class.
       *
       * \author Peter Zajac
       */
      class DriverBase
      {
      public:
        /// by default, tensorise the cubature forumula
        static constexpr bool tensorise = true;

        /**
         * \brief Applies an alias-functor.
         */
        template<typename Functor_>
        static void alias(Functor_&)
        {
          // do nothing
        }
      }; // class DriverBase
    } // namespace Scalar
  } // namespace Cubature
} // namespace FEAT

#endif // KERNEL_CUBATURE_SCALAR_DRIVER_BASE_HPP
