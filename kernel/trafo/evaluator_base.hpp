#pragma once
#ifndef KERNEL_TRAFO_EVALUATOR_BASE_HPP
#define KERNEL_TRAFO_EVALUATOR_BASE_HPP 1

// includes, FEAST
#include <kernel/trafo/eval_data.hpp>

namespace FEAST
{
  namespace Trafo
  {
    /// \cond internal
    namespace Intern
    {
      template<bool _enable>
      struct TrafoEvalHelper;
    } // namespace Intern
    /// \endcond

    /**
     * \brief Trafo Evaluator CRTP base-class template
     *
     * This class template is a CRTP base class used by various transformation evaluators to outsource
     * common wrapper code which is independent of the actual transformation in use.
     *
     * \tparam Evaluator_
     * The evaluator class that derives from this class template.
     *
     * \tparam EvalPolicy_
     * The evaluation policy of the trafo evaluator.
     *
     * \author Peter Zajac
     */
    template<
      typename Trafo_,
      typename Evaluator_,
      typename EvalPolicy_>
    class EvaluatorBase
    {
    public:
      /// trafo type
      typedef Trafo_ TrafoType;
      /// shape type
      typedef typename TrafoType::ShapeType ShapeType;

      /// evaluation policy
      typedef EvalPolicy_ EvalPolicy;

      /// evaluation traits; identical to eval policy
      typedef EvalPolicy_ EvalTraits;

      /// evaluation data type
      typedef typename EvalPolicy::DataType DataType;
      /// domain point type
      typedef typename EvalPolicy::DomainPointType DomainPointType;
      /// image point type
      typedef typename EvalPolicy::ImagePointType ImagePointType;
      /// jacobian matrix type
      typedef typename EvalPolicy::JacobianMatrixType JacobianMatrixType;
      /// jacobian inverse matrix type
      typedef typename EvalPolicy::JacobianInverseType JacobianInverseType;
      /// jacobian determinant type
      typedef typename EvalPolicy::JacobianDeterminantType JacobianDeterminantType;
      /// hessian tensor type
      typedef typename EvalPolicy::HessianTensorType HessianTensorType;
      /// hessian inverse tensor type
      typedef typename EvalPolicy::HessianInverseType HessianInverseType;

      /// domain dimension
      static constexpr int domain_dim = EvalPolicy::domain_dim;
      /// image dimension
      static constexpr int image_dim = EvalPolicy::image_dim;

      /// CellIterator typedef
      typedef Index CellIterator;

      // Note:
      // The following block serves as an element interface documentation and is therefore only
      // visible to doxygen. The actual functionality has to be supplied by the implementation.
#ifdef DOXYGEN
      /*
       * \brief Domain-Point capability
       *
       * This entry specifies whether the evaluator is capable of computing domain point coordinates.\n
       * This value is always non-zero.
       */
      static constexpr bool can_dom_point = ...;

      /**
       * \brief Image-Point capability
       *
       * This entry specifies whether the evaluator is capable of computing image point coordinates.\n
       * If this value is non-zero, the evaluator implements the #map_point member function.\n
       * See #map_point for details.
       */
      static constexpr bool can_img_point = ...;

      /**
       * \brief Jacobian-Matrix capability
       *
       * This entry specifies whether the evaluator is capable of computing jacobian matrices.\n
       * If this value is non-zero, the evaluator implements the #calc_jac_mat member function.\n
       * See #calc_jac_mat for details.
       */
      static constexpr bool can_jac_mat = ...;

      /**
       * \brief Jacobian-Inverse-Matrix capability
       *
       * This entry specifies whether the evaluator is capable of computing jacobian inverse matrices.\n
       * If this value is non-zero, the evaluator implements the #calc_jac_inv member function.\n
       * See #calc_jac_inv for details.
       */
      static constexpr bool can_jac_inv = ...;

      /**
       * \brief Jacobian-Determinant capability
       *
       * This entry specifies whether the evaluator is capable of computing jacobian determinants.\n
       * If this value is non-zero, the evaluator implements the #calc_jac_det member function.\n
       * See #calc_jac_det for details.
       */
      static constexpr bool can_jac_det = ...;

      /**
       * \brief Hessian-Tensor capability
       * This entry specifies whether the evaluator is capable of computing hessian tensors.\n
       * If this value is non-zero, the evaluator implements the #calc_hess_ten member function.\n
       * See #calc_hess_ten for details.
       */
      static constexpr bool can_hess_ten = ...;

      /**
       * \brief Hessian-Inverse-Tensor capability
       * This entry specifies whether the evaluator is capable of computing hessian inverse tensors.\n
       * If this value is non-zero, the evaluator implements the #calc_hess_inv member function.\n
       * See #calc_hess_inv for details.
       */
      static constexpr bool can_hess_inv = ...;
#endif // DOXYGEN

      /**
       * \brief Trafo configuration traits class template
       *
       * \tparam Cfg_
       * A Trafo configuration, see Trafo::ConfigBase for details.
       */
      template<typename Cfg_>
      struct ConfigTraits
      {
        /// evaluation data configuration
        struct EvalDataConfig :
          public Trafo::ConfigBase
        {
          /// specifies whether inverse hessian tensors are required
          static constexpr bool need_hess_inv = Cfg_::need_hess_inv;
          /// specifies whether hessian tensors are required
          static constexpr bool need_hess_ten = Cfg_::need_hess_ten || need_hess_inv;
          /// specifies whether jacobian determinants are required
          static constexpr bool need_jac_det = Cfg_::need_jac_det;
          /// specifies whether inverse jacobian matrices are required
          static constexpr bool need_jac_inv = Cfg_::need_jac_inv || need_hess_inv;
          /// specifies whether jacobian matrices are required
          static constexpr bool need_jac_mat = Cfg_::need_jac_mat || need_jac_det || need_jac_inv;
          /// specifies whetjer image points are required
          static constexpr bool need_img_point = Cfg_::need_img_point;
          /// we always store domain points
          static constexpr bool need_dom_point = true;
        };

        /// evaluation data typedef
        typedef Trafo::EvalData<EvalTraits, EvalDataConfig> EvalDataType;
      }; // struct ConfigTraits<...>

    protected:
      /// \cond internal
      Evaluator_& cast()
      {
        return static_cast<Evaluator_&>(*this);
      }

      const Evaluator_& cast() const
      {
        return static_cast<const Evaluator_&>(*this);
      }
      /// \endcond

      /// trafo reference
      const TrafoType& _trafo;

      /// currently active cell index
      Index _cell_index;

      /// constructor
      explicit EvaluatorBase(const TrafoType& trafo) :
        _trafo(trafo),
        _cell_index(~Index(0))
      {
      }

    public:
      /**
       * \brief Returns a reference to the trafo object.
       */
      const TrafoType& get_trafo() const
      {
        return _trafo;
      }

      /**
       * \brief Returns a CellIterator representing the index of the first cell.
       */
      CellIterator begin() const
      {
        return 0;
      }

      /**
       * \brief Returns a CellIterator representing the first index past the last cell.
       */
      CellIterator end() const
      {
        return get_num_cells();
      }

      /**
       * \brief Returns the number of cells in the mesh.
       *
       * \returns
       * The number of cells in the underlying mesh.
       */
      Index get_num_cells() const
      {
        return _trafo.get_mesh().get_num_entities(domain_dim);
      }

      /**
       * \brief Returns the index of the currently active cell.
       */
      Index get_cell_index() const
      {
        return _cell_index;
      }

      /**
       * \brief Prepares the evaluator for a given cell.
       *
       * \param[in] cell
       * The index of the cell for which the evaluator is to be prepared.
       */
      void prepare(const CellIterator& cell)
      {
        // store cell index
        _cell_index = cell;
      }

      /**
       * \brief Finishes the evaluator for the currently active cell.
       */
      void finish()
      {
        // reset cell index
        _cell_index = ~Index(0);
      }

      /**
       * \brief Trafo evaluation operator
       *
       * \param[out] trafo_data
       * A reference to the trafo data that is to be computed.
       *
       * \param[in] dom_point
       * A reference to the domain point on the reference cell in which the trafo is to be evaluated.
       */
      template<typename TrafoCfg_>
      void operator()(Trafo::EvalData<EvalTraits, TrafoCfg_>& trafo_data, const DomainPointType& dom_point) const
      {
        // typedef mumbo-jumbo
        typedef Trafo::EvalData<EvalTraits, TrafoCfg_> TrafoData;

        // store domain point
        Intern::TrafoEvalHelper<TrafoData::have_dom_point>::set_dom_point(trafo_data, dom_point);
        // map image point
        Intern::TrafoEvalHelper<TrafoData::have_img_point>::map_img_point(trafo_data, cast());
        // calculate jacobian matrix
        Intern::TrafoEvalHelper<TrafoData::have_jac_mat>::calc_jac_mat(trafo_data, cast());
        // calculate inverse jacobian matrix
        Intern::TrafoEvalHelper<TrafoData::have_jac_inv>::calc_jac_inv(trafo_data, cast());
        // calculate jacobian determinants
        Intern::TrafoEvalHelper<TrafoData::have_jac_det>::calc_jac_det(trafo_data, cast());
        // calculate hessian tensor
        Intern::TrafoEvalHelper<TrafoData::have_hess_ten>::calc_hess_ten(trafo_data, cast());
        // calculate inverse hessian tensor
        Intern::TrafoEvalHelper<TrafoData::have_hess_inv>::calc_hess_inv(trafo_data, cast());
      }

      // Note:
      // The following block serves as an element interface documentation and is therefore only
      // visible to doxygen. The actual functionality has to be supplied by the implementation.
#ifdef DOXYGEN
      /**
       * \brief Maps a domain point from the reference cell to the currently active cell.
       *
       * \param[out] img_point
       * A reference to the image point on the active cell that is to be computed.
       *
       * \param[in] dom_point
       * A reference to the domain point on the reference cell that is to be mapped.
       */
      void map_point(ImagePointType& img_point, const DomainPointType& dom_point) const;

      /**
       * \brief Computes the jacobian matrix for a given domain point.
       *
       * \param[out] jac_mat
       * A reference to the jacobian matrix that is to be computed.
       *
       * \param[in] dom_point
       * A reference to the domain point on the reference cell for which the jacobian matrix is to be computed.
       */
      void calc_jac_mat(JacobianMatrixType& jac_mat, const DomainPointType& dom_point) const;

      /**
       * \brief Computes the hessian tensor for a given domain point.
       *
       * \param[out] hess_ten
       * A reference to the hessian tensor that is to be computed.
       *
       * \param[in] dom_point
       * A reference to the domain point on the reference cell for which the hessian tensor is to be computed.
       */
      void calc_hess_ten(HessianTensorType& hess_ten, const DomainPointType& dom_point) const;
#endif // DOXYGEN
    }; // class EvaluatorBase<...>

    /// \cond internal
    namespace Intern
    {
      template<bool _enable>
      struct TrafoEvalHelper
      {
        template<typename TrafoData_, typename DomPoint_>
        static void set_dom_point(TrafoData_&, const DomPoint_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void map_img_point(TrafoData_&, const Evaluator_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_mat(TrafoData_&, const Evaluator_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_inv(TrafoData_&, const Evaluator_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_det(TrafoData_&, const Evaluator_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void calc_hess_ten(TrafoData_&, const Evaluator_&) {}

        template<typename TrafoData_, typename Evaluator_>
        static void calc_hess_inv(TrafoData_&, const Evaluator_&) {}
      };

      template<>
      struct TrafoEvalHelper<true>
      {
        template<typename TrafoData_, typename DomPoint_>
        static void set_dom_point(TrafoData_& trafo_data, const DomPoint_& dom_point)
        {
          trafo_data.dom_point = dom_point;
        }

        template<typename TrafoData_, typename Evaluator_>
        static void map_img_point(TrafoData_& trafo_data, const Evaluator_& evaluator)
        {
          static_assert(Evaluator_::can_img_point, "trafo evaluator can't compute image point coordinates");
          evaluator.map_point(trafo_data.img_point, trafo_data.dom_point);
        }

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_mat(TrafoData_& trafo_data, const Evaluator_& evaluator)
        {
          static_assert(Evaluator_::can_jac_mat, "trafo evaluator can't compute jacobian matrices");
          // let the evaluator compute the jacobian matrix
          evaluator.calc_jac_mat(trafo_data.jac_mat, trafo_data.dom_point);
        }

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_inv(TrafoData_& trafo_data, const Evaluator_&)
        {
          static_assert(Evaluator_::can_jac_inv, "trafo evaluator can't compute jacobian inverse matrices");
          // invert the jacobian matrix
          trafo_data.jac_inv.set_inverse(trafo_data.jac_mat);
        }

        template<typename TrafoData_, typename Evaluator_>
        static void calc_jac_det(TrafoData_& trafo_data, const Evaluator_&)
        {
          static_assert(Evaluator_::can_jac_det, "trafo evaluator can't compute jacobian determinants");
          // compute the volume of the jacobian matrix
          trafo_data.jac_det = trafo_data.jac_mat.vol();
        }

        template<typename TrafoData_, typename Evaluator_>
        static void calc_hess_ten(TrafoData_& trafo_data, const Evaluator_& evaluator)
        {
          static_assert(Evaluator_::can_hess_ten, "trafo evaluator can't compute hessian tensors");
          // let the evaluator compute the hessian tensor
          evaluator.calc_hess_ten(trafo_data.hess_ten, trafo_data.dom_point);
        }

        template<typename TrafoData_, typename Evaluator_>
        static void calc_hess_inv(TrafoData_& trafo_data, const Evaluator_&)
        {
          static_assert(Evaluator_::can_hess_inv, "trafo evaluator can't compute inverse hessian tensors");
          typedef typename TrafoData_::EvalTraits EvalTraits;
          typedef typename EvalTraits::DataType DataType;
          typename EvalTraits::HessianTensorType hess_jac;

          // compute inverse
          hess_jac.format();
          hess_jac.add_double_mat_mult(trafo_data.hess_ten, trafo_data.jac_inv, trafo_data.jac_inv);
          trafo_data.hess_inv.format();
          trafo_data.hess_inv.add_mat_tensor_mult(trafo_data.jac_inv, hess_jac, -DataType(1));
        }
      };
    } // namespace Intern
    /// \endcond
  } // namespace Trafo
} // namespace FEAST

#endif // KERNEL_TRAFO_EVALUATOR_BASE_HPP
