#pragma once
#ifndef KERNEL_SOLVER_RICHARDSON_HPP
#define KERNEL_SOLVER_RICHARDSON_HPP 1

// includes, FEAST
#include <kernel/solver/iterative.hpp>

namespace FEAST
{
  namespace Solver
  {
    /**
     * \brief (Preconditioned) Richardson solver implementation
     *
     * This class implements a simple preconditioned Richardson iteration solver.
     *
     * \tparam Matrix_
     * The matrix class to be used by the solver.
     *
     * \tparam Filter_
     * The filter class to be used by the solver.
     *
     * \author Peter Zajac
     */
    template<typename Matrix_, typename Filter_>
    class Richardson :
      public PreconditionedIterativeSolver<Matrix_, Filter_>
    {
    public:
      typedef Matrix_ MatrixType;
      typedef Filter_ FilterType;
      typedef typename MatrixType::VectorTypeR VectorType;
      typedef typename MatrixType::DataType DataType;
      typedef PreconditionedIterativeSolver<MatrixType, FilterType> BaseClass;

      typedef SolverBase<VectorType> PrecondType;

    protected:
      /// damping parameter
      DataType _omega;
      /// defect vector
      VectorType _vec_def;
      /// correction vector
      VectorType _vec_cor;

    public:
      /**
       * \brief Constructor
       *
       * \param[in] matrix
       * A reference to the system matrix.
       *
       * \param[in] filter
       * A reference to the system filter.
       *
       * \param[in] precond
       * A pointer to the preconditioner. May be \c nullptr.
       *
       * \param[in] omega
       * The damping parameter for the solver.
       */
      explicit Richardson(const MatrixType& matrix, const FilterType& filter,
        std::shared_ptr<PrecondType> precond = nullptr, DataType omega = DataType(1)) :
        BaseClass("Richardson", matrix, filter, precond),
        _omega(omega)
      {
      }

      virtual String name() const override
      {
        return "Richardson";
      }

      virtual void init_symbolic() override
      {
        BaseClass::init_symbolic();
        // create two temporary vectors
        _vec_def = this->_system_matrix.create_vector_r();
        _vec_cor = this->_system_matrix.create_vector_r();
      }

      virtual void done_symbolic() override
      {
        this->_vec_cor.clear();
        this->_vec_def.clear();
        BaseClass::done_symbolic();
      }

      virtual Status apply(VectorType& vec_cor, const VectorType& vec_def) override
      {
        // save defect
        this->_vec_def.copy(vec_def);
        //this->_system_filter.filter_def(this->_vec_def);

        // clear solution vector
        vec_cor.format();

        // apply
        return _apply_intern(vec_cor, vec_def);
      }

      virtual Status correct(VectorType& vec_sol, const VectorType& vec_rhs) override
      {
        // compute defect
        this->_system_matrix.apply(this->_vec_def, vec_sol, vec_rhs, -DataType(1));
        this->_system_filter.filter_def(this->_vec_def);

        // apply
        return _apply_intern(vec_sol, vec_rhs);
      }

    protected:
      virtual Status _apply_intern(VectorType& vec_sol, const VectorType& vec_rhs)
      {
        VectorType& vec_def(this->_vec_def);
        VectorType& vec_cor(this->_vec_cor);
        const MatrixType& matrix(this->_system_matrix);
        const FilterType& filter(this->_system_filter);

        // compute initial defect
        Status status = this->_set_initial_defect(vec_def, vec_sol);

        // start iterating
        while(status == Status::progress)
        {
          // apply preconditioner
          if(!this->_apply_precond(vec_cor, vec_def))
            return Status::aborted;
          //filter.filter_cor(vec_cor);

          // update solution vector
          vec_sol.axpy(vec_cor, vec_sol, this->_omega);

          // compute new defect vector
          matrix.apply(vec_def, vec_sol, vec_rhs, -DataType(1));
          filter.filter_def(vec_def);

          // compute new defect norm
          status = this->_set_new_defect(vec_def, vec_sol);
        }

        // return our status
        return status;
      }
    }; // class Richardson<...>

    /**
     * \brief Creates a new Richardson solver object
     *
     * \param[in] matrix
     * The system matrix.
     *
     * \param[in] filter
     * The system filter.
     *
     * \param[in] precond
     * The preconditioner. May be \c nullptr.
     *
     * \param[in] omega
     * The damping parameter for the solver.
     *
     * \returns
     * A shared pointer to a new Richardson object.
     */
    template<typename Matrix_, typename Filter_>
    inline std::shared_ptr<Richardson<Matrix_, Filter_>> new_richardson(
      const Matrix_& matrix, const Filter_& filter,
      std::shared_ptr<SolverBase<typename Matrix_::VectorTypeL>> precond = nullptr,
      typename Matrix_::DataType omega = typename Matrix_::DataType(1))
    {
      return std::make_shared<Richardson<Matrix_, Filter_>>(matrix, filter, precond, omega);
    }
  } // namespace Solver
} // namespace FEAST

#endif // KERNEL_SOLVER_RICHARDSON_HPP