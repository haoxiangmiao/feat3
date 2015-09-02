#pragma once
#ifndef KERNEL_SOLVER_SCHWARZ_PRECOND_HPP
#define KERNEL_SOLVER_SCHWARZ_PRECOND_HPP 1

#include <kernel/solver/base.hpp>
#include <kernel/global/vector.hpp>
#include <kernel/global/filter.hpp>

namespace FEAST
{
  namespace Solver
  {
    /**
     * \brief Schwarz preconditioner class template declaration.
     *
     * \note This class is only specialised for Global::Vector types.
     * There exists no generic implementation.
     *
     * \tparam Vector_
     * The type of the vector. Must be an instance of Global::Vector.
     *
     * \tparam Filter_
     * The type of the filter. Must be an instance of Global::Filter.
     */
    template<typename Vector_, typename Filter_>
    class SchwarzPrecond;

    /**
     * \brief Schwarz preconditioner specialisation for Global::Vector
     *
     * This class template implements an additive Schwarz preconditioner,
     * i.e. this class implements a global preconditioner by synchronously
     * adding (and averaging) a set of local solutions.
     *
     * \tparam LocalVector_
     * The type of the local vector nested in a global vector.
     */
    template<typename LocalVector_, typename LocalFilter_>
    class SchwarzPrecond<Global::Vector<LocalVector_>, Global::Filter<LocalFilter_>> :
      public SolverBase<Global::Vector<LocalVector_>>
    {
    public:
      /// our local vector type
      typedef LocalVector_ LocalVectorType;
      /// our global vector type
      typedef Global::Vector<LocalVector_> GlobalVectorType;
      /// our global filter type
      typedef Global::Filter<LocalFilter_> GlobalFilterType;

      /// the local solver interface
      typedef SolverBase<LocalVector_> LocalSolverType;

    protected:
      /// our local solver object
      std::shared_ptr<LocalSolverType> _local_solver;
      /// our global filter
      const GlobalFilterType& _filter;

    public:
      /**
       * \brief Constructor
       *
       * \param[in] local_solver
       * The local solver that is to be used by the Schwarz preconditioner.
       */
      explicit SchwarzPrecond(std::shared_ptr<LocalSolverType> local_solver, const GlobalFilterType& filter) :
        _local_solver(local_solver),
        _filter(filter)
      {
      }

      virtual void init_symbolic() override
      {
        _local_solver->init_symbolic();
      }

      virtual void init_numeric() override
      {
        _local_solver->init_numeric();
      }

      virtual void done_numeric() override
      {
        _local_solver->done_numeric();
      }

      virtual void done_symbolic() override
      {
        _local_solver->done_symbolic();
      }

      virtual Status apply(GlobalVectorType& vec_cor, const GlobalVectorType& vec_def) override
      {
        // apply local solver
        Status status = _local_solver->apply(*vec_cor, *vec_def);
        if(!status_success(status))
          return status;

        // synchronise
        vec_cor.sync_1();

        // apply filter
        _filter.filter_cor(vec_cor);

        // okay
        return status;
      }
    }; // class SchwarzPrecond<...>

    /**
     * \brief Creates a new SchwarzPrecond solver object
     *
     * \param[in] local_solver
     * The local solver object.
     *
     * \param[in] filter
     * The global system filter.
     *
     * \returns
     * A shared pointer to a new SchwarzPrecond object.
     */
    template<typename LocalFilter_>
    inline std::shared_ptr<SchwarzPrecond<Global::Vector<typename LocalFilter_::VectorType>, Global::Filter<LocalFilter_>>> new_schwarz_precond(
      std::shared_ptr<SolverBase<typename LocalFilter_::VectorType>> local_solver,
      Global::Filter<LocalFilter_>& filter)
    {
      return std::make_shared<SchwarzPrecond<Global::Vector<typename LocalFilter_::VectorType>, Global::Filter<LocalFilter_>>>
        (local_solver, filter);
    }
  } // namespace Solver
} // namespace FEAST

#endif // KERNEL_SOLVER_SCHWARZ_PRECOND_HPP