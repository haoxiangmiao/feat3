#pragma once
#ifndef KERNEL_SPACE_NODE_FUNCTIONAL_BASE_HPP
#define KERNEL_SPACE_NODE_FUNCTIONAL_BASE_HPP 1

// includes, FEAST
#include <kernel/space/base.hpp>

namespace FEAST
{
  namespace Space
  {
    /**
     * \brief Node-functional base class template
     *
     * This class acts as a base-class and interface documentation for Finite-Element Node-Functional implementations.
     *
     * \tparam Space_
     * The finite-element space that this node-functional is used by.
     *
     * \tparam Function_
     * The type of the function that is to be evaluated by the node functionals.\n
     * Has to implement the Assembly::AnalyticFunction interface.
     *
     * \author Peter Zajac
     */
    template<
      typename Space_,
      typename Function_,
      typename DataType_>
    class NodeFunctionalBase
    {
    public:
      /// space typedef
      typedef Space_ SpaceType;
      /// functor type
      typedef Function_ FunctionType;
      /// data type
      typedef DataType_ DataType;

    protected:
      /// space reference
      const SpaceType& _space;
      /// functor reference
      const FunctionType& _function;
      /// currently active cell index
      Index _cell_index;

      /// protected constructor
      explicit NodeFunctionalBase(const SpaceType& space, const FunctionType& function) :
        _space(space),
        _function(function),
        _cell_index(~Index(0))
      {
      }

    public:
      /**
       * \brief Prepares the node-functional for a given cell.
       *
       * \param[in] cell_index
       * The index of the cell that is to be used by the node-functional.
       */
      void prepare(Index cell_index)
      {
        _cell_index = cell_index;
      }

      /**
       * \brief Releases the node-functional from the current cell.
       */
      void finish()
      {
        _cell_index = ~Index(0);
      }

      /**
       * \brief Returns the maximum number of assigned dofs.
       */
      Index get_max_assigned_dofs() const
      {
        return 1;
      }

#ifdef DOXYGEN
      /**
       * \brief Returns the number of assigned dofs on the current cell.
       */
      Index get_num_assigned_dofs() const;

      /**
       * \brief Evaluation operator.
       *
       * This operator evaluates an assigned dof on the current cell.
       *
       * \param[in] assign_idx
       * The index of the assigned dofs that is to be evaluated.
       *
       * \returns
       * The value of the node functional applied onto the functor.
       */
      DataType_ operator()(Index assign_idx) const;
#endif // DOXYGEN
    }; // class NodeFunctionalBase<...>

    /**
     * \brief Null-Node-Functional class template
     *
     * This class implements the node-functional interface for an empty node functional set.
     *
     * \author Peter Zajac
     */
    template<
      typename Space_,
      typename Function_,
      typename DataType_>
    class NodeFunctionalNull :
      public NodeFunctionalBase<Space_, Function_, DataType_>
    {
    private:
      /// base-class typedef
      typedef NodeFunctionalBase<Space_, Function_, DataType_> BaseClass;

    public:
      /// constructor
      explicit NodeFunctionalNull(const Space_& space, const Function_& function) :
        BaseClass(space, function)
      {
      }

      /** \copydoc NodeFunctionalBase::get_max_assigned_dofs() */
      Index get_max_assigned_dofs() const
      {
        return 0;
      }

      /** \copydoc NodeFunctionalBase::get_num_assigned_dofs() */
      Index get_num_assigned_dofs() const
      {
        return 0;
      }

      /** \copydoc NodeFunctionalBase::operator()() */
      DataType_ operator()(Index) const
      {
        throw InternalError("invalid call of NodeFunctionalNull::operator()()");
      }
    }; // class NodeFunctionalNull<...>
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_NODE_FUNCTIONAL_BASE_HPP
