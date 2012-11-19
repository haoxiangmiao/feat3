#pragma once
#ifndef KERNEL_SPACE_DISCONTINUOUS_ELEMENT_HPP
#define KERNEL_SPACE_DISCONTINUOUS_ELEMENT_HPP 1

// includes, FEAST
#include <kernel/space/element_base.hpp>
#include <kernel/space/dof_assignment_common.hpp>
#include <kernel/space/dof_mapping_common.hpp>
#include <kernel/space/discontinuous/evaluator.hpp>

namespace FEAST
{
  namespace Space
  {
    /**
     * \brief Discontinuous Element namespace
     */
    namespace Discontinuous
    {
      /**
       * \brief Discontinuous Finite-Element space class template
       *
       * \tparam Trafo_
       * The transformation that is to be used by this space.
       *
       * \author Peter Zajac
       */
      template<
        typename Trafo_,
        typename Variant_ = Variant::StdPolyP<0> >
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
        /// variant of the element
        typedef Variant_ VariantTag;

        /** \copydoc ElementBase::TrafoConfig */
        template<typename SpaceConfig_>
        struct TrafoConfig :
          public Trafo::ConfigBase
        {
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

          /// space evaluation traits
          typedef StandardScalarEvalTraits<EvalPolicy, 1, DataType_> Traits;

        public:
          /// space evaluator type
          typedef Discontinuous::Evaluator<Element, Traits, TrafoEvaluator_, VariantTag> Type;
        };

        /** \copydoc ElementBase::DofMappingType */
        typedef DofMappingSingleEntity<Element, ShapeType::dimension> DofMappingType;

        /** \copydoc ElementBase::DofAssignment */
        template<int shape_dim_>
        class DofAssignment
        {
        public:
          /// Dof-Assignment type
          typedef DofAssignmentSingleEntity<Element, shape_dim_, ShapeType::dimension> Type;
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
          // number of DOFs = number of cells in the mesh
          return this->get_mesh().get_num_entities(ShapeType::dimension);
        }
      }; // class Element
    } // namespace Discontinuous
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_DISCONTINUOUS_ELEMENT_HPP
