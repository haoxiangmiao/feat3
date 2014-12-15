#pragma once
#ifndef KERNEL_SPACE_LAGRANGE2_ELEMENT_HPP
#define KERNEL_SPACE_LAGRANGE2_ELEMENT_HPP 1

// includes, FEAST
#include <kernel/space/element_base.hpp>
#include <kernel/space/dof_assignment_base.hpp>
#include <kernel/space/dof_mapping_common.hpp>
#include <kernel/space/lagrange2/dof_traits.hpp>
#include <kernel/space/lagrange2/evaluator.hpp>
#include <kernel/space/lagrange2/node_functional.hpp>

namespace FEAST
{
  namespace Space
  {
    /**
     * \brief Lagrange-2 Element namespace
     *
     * This namespace encapsulates all classes related to the implementation of the standard third-order
     * H1-conforming Finite Element spaces widely known as P2 and Q2, resp.
     */
    namespace Lagrange2
    {
      /**
       * \brief Standard Lagrange-2 Finite-Element space class template
       *
       * \tparam Trafo_
       * The transformation that is to be used by this space.
       *
       * \author Peter Zajac
       */
      template<typename Trafo_>
      class Element :
        public ElementBase<Trafo_>
      {
      public:
        /// base-class typedef
        typedef ElementBase<Trafo_> BaseClass;
        /// transformation type
        typedef Trafo_ TrafoType;
        /// mesh type
        typedef typename TrafoType::MeshType MeshType;
        /// shape type
        typedef typename TrafoType::ShapeType ShapeType;

        /** \copydoc ElementBase::ElementCapabilities */
        enum ElementCapabilities
        {
          /// node functionals available
          have_node_func = 1
        };

        /** \copydoc ElementBase::local_degree */
        static constexpr int local_degree = 2;

        /** \copydoc ElementBase::DofMappingType */
        typedef DofMappingUniform<Element, Lagrange2::DofTraits, ShapeType> DofMappingType;

        /** \copydoc ElementBase::Evaluator */
        template<
          typename TrafoEvaluator_,
          typename DataType_ = typename TrafoEvaluator_::DataType>
        class Evaluator
        {
        private:
          /// evaluation policy
          typedef typename TrafoEvaluator_::EvalPolicy EvalPolicy;

          /// dummy enum
          enum
          {
            /// number of local dofs := number of vertices per cell
            num_loc_dofs = DofMappingType::dof_count
          };

          /// space evaluation traits
          typedef StandardScalarEvalTraits<EvalPolicy, num_loc_dofs, DataType_> Traits;

        public:
          /// space evaluator type
          typedef Lagrange2::Evaluator<Element, TrafoEvaluator_, Traits> Type;
        };

        /** \copydoc ElementBase::DofAssignment */
        template<
          int shape_dim_,
          typename DataType_ = Real>
        class DofAssignment
        {
        public:
          /// Dof-Assignment type
          typedef DofAssignmentUniform<Element, shape_dim_, DataType_, DofTraits, ShapeType> Type;
        };

        /** \copydoc ElementBase::NodeFunctional */
        template<
          typename Function_,
          int shape_dim_,
          typename DataType_ = Real>
        class NodeFunctional
        {
          typedef typename Shape::FaceTraits<ShapeType, shape_dim_>::ShapeType FaceType;
        public:
          /// node functional type
          typedef Lagrange2::NodeFunctional<Element, Function_, FaceType, DataType_> Type;
        };

      public:
        /**
         * \brief Constructor
         *
         * \param[in] trafo
         * A reference to the transformation which is to be used by this space.
         */
        explicit Element(TrafoType& trafo)
          : BaseClass(trafo)
        {
        }

        /// virtual destructor
        virtual ~Element()
        {
        }

        /** \copydoc ElementBase::get_num_dofs() */
        Index get_num_dofs() const
        {
          DofMappingType dof_map(*this);
          return dof_map.get_num_global_dofs();
        }

        /** \copydoc ElementBase::name() */
        static String name()
        {
          return "Lagrange2";
        }
      }; // class Element
    } // namespace Lagrange2
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_LAGRANGE2_ELEMENT_HPP