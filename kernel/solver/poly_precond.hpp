#pragma once
#ifndef KERNEL_SOLVER_POLY_PRECOND_HPP
#define KERNEL_SOLVER_POLY_PRECOND_HPP 1

// includes, FEAST
#include <kernel/solver/precon_wrapper.hpp>

namespace FEAST
{
  namespace Solver
  {
    /// \todo reimplement this
    template<typename Matrix_, typename Filter_>
    using PolyPrecond = Solver::PreconWrapper<Matrix_, Filter_, LAFEM::PolynomialPreconditioner>;

    /**
     * \brief Creates a new PolyPrecond solver object
     *
     * \param[in] matrix
     * The system matrix.
     *
     * \param[in] filter
     * The system filter.
     *
     * \todo adjust arguments to reimplemented class
     *
     * \returns
     * A shared pointer to a new PolyPrecond object.
     */
    template<typename Matrix_, typename Filter_, typename... Args_>
    inline std::shared_ptr<PolyPrecond<Matrix_, Filter_>> new_poly_precond(
      const Matrix_& matrix, const Filter_& filter, Args_&&... args)
    {
      return std::make_shared<PolyPrecond<Matrix_, Filter_>>
        (filter, matrix, std::forward<Args_>(args)...);
    }
  } // namespace Solver
} // namespace FEAST

#endif // KERNEL_SOLVER_POLY_PRECOND_HPP
