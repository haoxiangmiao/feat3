#pragma once
#ifndef KERNEL_SPACE_ARGYRIS_ELEMENT_HPP
#define KERNEL_SPACE_ARGYRIS_ELEMENT_HPP 1

// includes, FEAST
#include <kernel/space/element_base.hpp>
#include <kernel/space/dof_mapping_common.hpp>
#include <kernel/space/dof_assignment_common.hpp>
#include <kernel/space/argyris/dof_traits.hpp>
#include <kernel/space/argyris/evaluator.hpp>

namespace FEAST
{
  namespace Space
  {
    namespace Argyris
    {
      /**
       * \brief Argyris element class template
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

        /// dummy enum
        enum
        {
          num_vert_dofs = 6,
          num_edge_dofs = 1
        };

        /** \copydoc ElementBase::ElementCapabilities */
        enum ElementCapabilities
        {
          /// no node functionals available
          have_node_func = 0
        };


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
            /// number of local dofs
            num_loc_dofs = 21
          };

          /// space evaluation traits
          typedef StandardScalarEvalTraits<EvalPolicy, num_loc_dofs, DataType_> Traits;

        public:
          /// space evaluator type
          typedef Argyris::Evaluator<Element, TrafoEvaluator_, Traits> Type;
        };

        /** \copydoc ElementBase::DofMappingType */
        typedef DofMappingUniform<Element, Argyris::DofTraits, ShapeType> DofMappingType;

        /** \copydoc ElementBase::DofAssignment */
        template<
          int shape_dim_,
          typename DataType_ = Real>
        class DofAssignment
        {
        public:
          /// Dof-Assignment type
          typedef DofAssignmentUniform<Element, shape_dim_, DataType_, Argyris::DofTraits, ShapeType> Type;
        };

        /** \copydoc ElementBase::NodeFunctional */
        template<
          typename Functor_,
          int shape_dim_,
          typename DataType_ = Real>
        class NodeFunctional
        {
        public:
          /// no node functionals available
          typedef Nil Type;
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
          return
            // number of vertex dofs
            Index(num_vert_dofs) * this->get_mesh().get_num_entities(0) +
            // number of edge dofs
            Index(num_edge_dofs) * this->get_mesh().get_num_entities(1);
        }
      }; // class Element<...>
    } // namespace Argyris
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_ARGYRIS_ELEMENT_HPP
