#pragma once
#ifndef KERNEL_SPACE_BOGNER_FOX_SCHMIT_ELEMENT_HPP
#define KERNEL_SPACE_BOGNER_FOX_SCHMIT_ELEMENT_HPP 1

// includes, FEAST
#include <kernel/space/element_base.hpp>
#include <kernel/space/dof_assignment_common.hpp>
#include <kernel/space/dof_mapping_common.hpp>
#include <kernel/space/bogner_fox_schmit/evaluator.hpp>
#include <kernel/space/bogner_fox_schmit/node_functional.hpp>

namespace FEAST
{
  namespace Space
  {
    /**
     * \brief Bogner-Fox-Schmit element namespace
     *
     * This namespace encapsulates all classes related to the implementation of the H2-conforming
     * Bogner-Fox-Schmit Finite Element spaces.
     */
    namespace BognerFoxSchmit
    {
      /**
       * \brief Bogner-Fox-Schmit element class template
       *
       * \attention
       * This element is only defined for Hypercube shape meshes!
       *
       * \attention
       * This element works only on affine equivalent meshes, i.e. on meshes where all cells are paralleloids.
       * If you use this element on non-affine equivalent meshes, the result will be garbage. This is not an
       * implementational issue, but a mathematical problem that can not be solved.
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
          /// number of dofs per vertex = 2^dimension
          num_vert_dofs = (1 << ShapeType::dimension),
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
            /// number of local dofs := (number of vertices per cell) * (2^dimension)
            num_loc_dofs = Shape::FaceTraits<ShapeType, 0>::count * num_vert_dofs
          };

          /// space evaluation traits
          typedef StandardScalarEvalTraits<EvalPolicy, num_loc_dofs, DataType_> Traits;

        public:
          /// space evaluator type
          typedef BognerFoxSchmit::Evaluator<Element, TrafoEvaluator_, Traits> Type;
        };

        /** \copydoc ElementBase::DofMappingType */
        typedef DofMappingSingleEntity<Element, ShapeType::dimension, num_vert_dofs> DofMappingType;

        /** \copydoc ElementBase::DofAssignment */
        template<
          int shape_dim_,
          typename DataType_ = Real>
        class DofAssignment
        {
        public:
          /// Dof-Assignment type
          typedef DofAssignmentSingleEntity<Element, shape_dim_, DataType_, 0, num_vert_dofs> Type;
        };

        /** \copydoc ElementBase::NodeFunctional */
        template<
          typename Functor_,
          int shape_dim_,
          typename DataType_ = Real>
        class NodeFunctional
        {
        public:
          /// node functional type
          typedef BognerFoxSchmit::NodeFunctional<Element, Functor_, ShapeType::dimension, shape_dim_, DataType_> Type;
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
          // number of DOFs = (number of vertices in the mesh) * (2^dimension)
          return this->get_mesh().get_num_entities(0) * Index(1 << ShapeType::dimension);
        }
      }; // class Element<...>
    } // namespace BognerFoxSchmit
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_BOGNER_FOX_SCHMIT_ELEMENT_HPP
