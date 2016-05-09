#pragma once
#ifndef KERNEL_ASSEMBLY_TRACE_ASSEMBLER_HPP
#define KERNEL_ASSEMBLY_TRACE_ASSEMBLER_HPP 1

// includes, FEAST
#include <kernel/assembly/asm_traits.hpp>
#include <kernel/geometry/intern/face_index_mapping.hpp>
#include <kernel/geometry/intern/face_ref_trafo.hpp>
#include <kernel/geometry/intern/congruency_sampler.hpp>
#include <kernel/geometry/intern/congruency_trafo.hpp>
#include <kernel/geometry/intern/index_representative.hpp>

#include <vector>

namespace FEAST
{
  namespace Assembly
  {
    /// \cond internal
    namespace Intern
    {
      template<typename Outer_, typename Shape_, int cell_dim_>
      class CompIndexMap
      {
      protected:
        const Outer_& _outer;
        int _idx;

      public:
        explicit CompIndexMap(const Outer_& outer, int idx) : _outer(outer), _idx(idx) {}

        Index operator[](int i) const
        {
          return _outer[Geometry::Intern::FaceIndexMapping<Shape_, cell_dim_, 0>::map(_idx,i)];
        }
      }; // class CompIndexMap
    } // namespace Intern
    /// \endcond

    template<typename Trafo_>
    class TraceAssembler
    {
    public:
      typedef Trafo_ TrafoType;
      typedef typename TrafoType::MeshType MeshType;
      typedef typename TrafoType::ShapeType ShapeType;

      static constexpr int shape_dim = ShapeType::dimension;
      static constexpr int facet_dim = shape_dim-1;

      struct FacetTrafoConfig :
        public Trafo::ConfigBase
      {
        /// we need jacobian determinants for integration
        static constexpr bool need_jac_det = true;
      };

    protected:
      const TrafoType& _trafo;
      std::vector<int> _facet_mask, _cell_facet, _facet_ori;
      std::vector<Index> _cells, _facets;

    public:
      explicit TraceAssembler(const TrafoType& trafo) :
        _trafo(trafo),
        _facet_mask(trafo.get_mesh().get_num_entities(facet_dim), 0)
      {
      }

      void clear_facets()
      {
        _cell_facet.clear();
        _facet_ori.clear();
        _cells.clear();
        _facets.clear();
        for(auto& f : _facets)
          f = Index(0);
      }

      void add_facet(Index ifacet)
      {
        ASSERT_(ifacet < Index(_facets.size()));
        _facet_mask.at(ifacet) = 1;
      }

      void add_mesh_part(const Geometry::MeshPart<MeshType>& mesh_part)
      {
        const auto& trg = mesh_part.template get_target_set<facet_dim>();
        for(Index i(0); i < trg.get_num_entities(); ++i)
          _facet_mask.at(trg[i]) = 1;
      }

      void compile_facets()
      {
        _cell_facet.clear();
        _facet_ori.clear();
        _cells.clear();
        _facets.clear();

        // build elements-at-facet graph
        Adjacency::Graph elem_at_facet(Adjacency::rt_injectify_transpose,
          _trafo.get_mesh().template get_index_set<shape_dim, facet_dim>());

        // loop over all facets
        for(Index iface(0); iface < Index(_facet_mask.size()); ++iface)
        {
          // use this facet?
          if(_facet_mask[iface] == 0)
            continue;

          // ensure that this is a boundary facet
          if(elem_at_facet.degree(iface) != Index(1))
            throw InternalError(__func__, __FILE__, __LINE__, "facet is adjacent to more than 1 element");

          Index icell = *elem_at_facet.image_begin(iface);

          // try to compute local facet index and orientation
          int loc_face(0), face_ori(0);
          if(!find_local_facet(iface, icell, loc_face, face_ori))
            throw InternalError(__func__, __FILE__, __LINE__, "failed to find local facet");

          // alright, add this facet to our list
          _facets.push_back(iface);
          _cells.push_back(icell);
          _cell_facet.push_back(loc_face);
          _facet_ori.push_back(face_ori);
        }
      }

      template<
        typename Matrix_,
        typename Operator_,
        typename Space_,
        typename CubatureFactory_>
      void assemble_operator_matrix1(
        Matrix_& matrix,
        Operator_& operat,
        const Space_& space,
        const CubatureFactory_& cubature_factory,
        typename Matrix_::DataType alpha = typename Matrix_::DataType(1)) const
      {
        assemble_operator_matrix2(matrix, operat, space, space, cubature_factory, alpha);
      }

      template<
        typename Matrix_,
        typename Operator_,
        typename TestSpace_,
        typename TrialSpace_,
        typename CubatureFactory_>
      void assemble_operator_matrix2(
        Matrix_& matrix,
        Operator_& operat,
        const TestSpace_& test_space,
        const TrialSpace_& trial_space,
        const CubatureFactory_& cubature_factory,
        typename Matrix_::DataType alpha = typename Matrix_::DataType(1)) const
      {
        // matrix type
        typedef Matrix_ MatrixType;
        // functor type
        typedef Operator_ OperatorType;
        // test-space type
        typedef TestSpace_ TestSpaceType;
        // trial-space type
        typedef TrialSpace_ TrialSpaceType;

        // assembly traits
        typedef AsmTraits2<
          typename MatrixType::DataType,
          TestSpaceType,
          TrialSpaceType,
          typename OperatorType::TrafoConfig,
          typename OperatorType::TestConfig,
          typename OperatorType::TrialConfig> AsmTraits;

        typedef typename AsmTraits::DataType DataType;
        typedef typename AsmTraits::TrafoType TrafoType;

        // shape types
        typedef typename AsmTraits::ShapeType ShapeType;
        typedef typename Shape::FaceTraits<ShapeType, ShapeType::dimension-1>::ShapeType FacetType;

        // fetch the trafo
        const typename AsmTraits::TrafoType& trafo = test_space.get_trafo();

        // create a trafo evaluator
        typename AsmTraits::TrafoEvaluator trafo_eval(trafo);

        // create a trafo facet evaluator
        typedef typename TrafoType::template Evaluator<FacetType, DataType>::Type TrafoFacetEvaluator;
        TrafoFacetEvaluator trafo_facet_eval(trafo);

        typedef typename TrafoFacetEvaluator::template ConfigTraits<FacetTrafoConfig>::EvalDataType TrafoFacetEvalData;

        // create space evaluators
        typename AsmTraits::TestEvaluator test_eval(test_space);
        typename AsmTraits::TrialEvaluator trial_eval(trial_space);

        // create dof-mappings
        typename AsmTraits::TestDofMapping test_dof_mapping(test_space);
        typename AsmTraits::TrialDofMapping trial_dof_mapping(trial_space);

        // create a functor evaluator
        typename OperatorType::template Evaluator<AsmTraits> oper_eval(operat);

        // create trafo evaluation data
        typename AsmTraits::TrafoEvalData trafo_data;
        TrafoFacetEvalData trafo_facet_data;

        // create space evaluation data
        typename AsmTraits::TestEvalData test_data;
        typename AsmTraits::TrialEvalData trial_data;

        // create local matrix data
        typename AsmTraits::LocalMatrixType lmd;

        // create cubature rule
        typename Intern::CubatureTraits<TrafoFacetEvaluator>::RuleType cubature_rule(Cubature::ctor_factory, cubature_factory);

        // create matrix scatter-axpy
        typename MatrixType::ScatterAxpy scatter_axpy(matrix);

        // trafo matrices and vectors
        Tiny::Matrix<DataType, shape_dim, facet_dim> face_mat;
        Tiny::Matrix<DataType, facet_dim, facet_dim> ori_mat;
        Tiny::Vector<DataType, shape_dim> face_vec;
        Tiny::Vector<DataType, facet_dim> ori_vec;

        face_mat.format();
        ori_mat.format();
        face_vec.format();
        ori_vec.format();

        // loop over all cells of the mesh
        for(Index f(0); f < Index(_facets.size()); ++f)
        {
          // get facet index
          const Index face = _facets[f];
          const Index cell = _cells[f];

          // compute facet trafos
          Geometry::Intern::FaceRefTrafo<ShapeType, facet_dim>::compute(face_mat, face_vec, _cell_facet[f]);

          // compute orientation trafos
          Geometry::Intern::CongruencyTrafo<FacetType>::compute(ori_mat, ori_vec, _facet_ori[f]);

          // prepare trafo evaluators
          trafo_facet_eval.prepare(face);
          trafo_eval.prepare(cell);

          // prepare space evaluators
          test_eval.prepare(trafo_eval);
          trial_eval.prepare(trafo_eval);

          // prepare functor evaluator
          oper_eval.prepare(trafo_eval);

          // fetch number of local dofs
          int num_loc_test_dofs = test_eval.get_num_local_dofs();
          int num_loc_trial_dofs = trial_eval.get_num_local_dofs();

          // format local matrix
          lmd.format();

          // loop over all quadrature points and integrate
          for(int k(0); k < cubature_rule.get_num_points(); ++k)
          {
            // get cubature point
            auto cub_pt = cubature_rule.get_point(k);

            // transform to local facet
            auto cub_cf = (face_mat * ((ori_mat * cub_pt) + ori_vec)) + face_vec;

            // compute trafo data
            trafo_facet_eval(trafo_facet_data, cub_pt);
            trafo_eval(trafo_data, cub_cf);

            // compute basis function data
            test_eval(test_data, trafo_data);
            trial_eval(trial_data, trafo_data);

            // prepare bilinear operator
            oper_eval.set_point(trafo_data);

            // test function loop
            for(int i(0); i < num_loc_test_dofs; ++i)
            {
              // trial function loop
              for(int j(0); j < num_loc_trial_dofs; ++j)
              {
                // evaluate functor and integrate
                lmd(i,j) += trafo_facet_data.jac_det * cubature_rule.get_weight(k) *
                  oper_eval(trial_data.phi[j], test_data.phi[i]);
                // continue with next trial function
              }
              // continue with next test function
            }
            // continue with next cubature point
          }

          // finish functor evaluator
          oper_eval.finish();

          // finish evaluators
          trial_eval.finish();
          test_eval.finish();
          trafo_eval.finish();
          trafo_facet_eval.finish();

          // initialise dof-mappings
          test_dof_mapping.prepare(cell);
          trial_dof_mapping.prepare(cell);

          // incorporate local matrix
          scatter_axpy(lmd, test_dof_mapping, trial_dof_mapping, alpha);

          // finish dof mapping
          trial_dof_mapping.finish();
          test_dof_mapping.finish();

          // continue with next cell
        }

        // okay, that's it
      }

      template<
        typename Vector_,
        typename Functional_,
        typename CubatureFactory_,
        typename Space_>
      void assemble_functional_vector(
        Vector_& vector,
        const Functional_& functional,
        const Space_& space,
        const CubatureFactory_& cubature_factory,
        typename Vector_::DataType alpha = typename Vector_::DataType(1)) const
      {
        // vector type
        typedef Vector_ VectorType;
        // functor type
        typedef Functional_ FunctionalType;
        // space type
        typedef Space_ SpaceType;

        // assembly traits
        typedef AsmTraits1<
          typename VectorType::DataType,
          SpaceType,
          typename FunctionalType::TrafoConfig,
          typename FunctionalType::TestConfig> AsmTraits;

        typedef typename AsmTraits::DataType DataType;
        typedef typename AsmTraits::TrafoType TrafoType;

        // shape types
        typedef typename AsmTraits::ShapeType ShapeType;
        typedef typename Shape::FaceTraits<ShapeType, ShapeType::dimension-1>::ShapeType FacetType;

        // fetch the trafo
        const typename AsmTraits::TrafoType& trafo = space.get_trafo();

        // create a trafo evaluator
        typename AsmTraits::TrafoEvaluator trafo_eval(trafo);

        // create a trafo facet evaluator
        typedef typename TrafoType::template Evaluator<FacetType, DataType>::Type TrafoFacetEvaluator;
        TrafoFacetEvaluator trafo_facet_eval(trafo);

        typedef typename TrafoFacetEvaluator::template ConfigTraits<FacetTrafoConfig>::EvalDataType TrafoFacetEvalData;

        // create a space evaluator and evaluation data
        typename AsmTraits::TestEvaluator test_eval(space);

        // create a dof-mapping
        typename AsmTraits::DofMapping dof_mapping(space);

        // create a functor evaluator
        typename FunctionalType::template Evaluator<AsmTraits> func_eval(functional);

        // create trafo evaluation data
        typename AsmTraits::TrafoEvalData trafo_data;
        TrafoFacetEvalData trafo_facet_data;

        // create space evaluation data
        typename AsmTraits::TestEvalData test_data;

        // create local vector data
        typename AsmTraits::LocalVectorType lvad;

        // create cubature rule
        typename Intern::CubatureTraits<TrafoFacetEvaluator>::RuleType cubature_rule(Cubature::ctor_factory, cubature_factory);

        // create matrix scatter-axpy
        typename VectorType::ScatterAxpy scatter_axpy(vector);

        // trafo matrices and vectors
        Tiny::Matrix<DataType, shape_dim, facet_dim> face_mat;
        Tiny::Matrix<DataType, facet_dim, facet_dim> ori_mat;
        Tiny::Vector<DataType, shape_dim> face_vec;
        Tiny::Vector<DataType, facet_dim> ori_vec;

        face_mat.format();
        ori_mat.format();
        face_vec.format();
        ori_vec.format();

        // loop over all cells of the mesh
        for(Index f(0); f < Index(_facets.size()); ++f)
        {
          // get facet index
          const Index face = _facets[f];
          const Index cell = _cells[f];

          // compute facet trafos
          Geometry::Intern::FaceRefTrafo<ShapeType, facet_dim>::compute(face_mat, face_vec, _cell_facet[f]);

          // compute orientation trafos
          Geometry::Intern::CongruencyTrafo<FacetType>::compute(ori_mat, ori_vec, _facet_ori[f]);

          // prepare trafo evaluators
          trafo_facet_eval.prepare(face);
          trafo_eval.prepare(cell);

          // prepare test-space evaluator
          test_eval.prepare(trafo_eval);

          // prepare functor evaluator
          func_eval.prepare(trafo_eval);

          // fetch number of local dofs
          int num_loc_dofs = test_eval.get_num_local_dofs();

          // format local matrix
          lvad.format();

          // loop over all quadrature points and integrate
          for(int k(0); k < cubature_rule.get_num_points(); ++k)
          {
            // get cubature point
            auto cub_pt = cubature_rule.get_point(k);

            // transform to local facet
            auto cub_cf = (face_mat * ((ori_mat * cub_pt) + ori_vec)) + face_vec;

            // compute trafo data
            trafo_facet_eval(trafo_facet_data, cub_pt);
            trafo_eval(trafo_data, cub_cf);

            // compute test basis function data
            test_eval(test_data, trafo_data);

            // prepare functional
            func_eval.set_point(trafo_data);

            // test function loop
            for(int i(0); i < num_loc_dofs; ++i)
            {
              // evaluate functor and integrate
              lvad(i) += trafo_facet_data.jac_det * cubature_rule.get_weight(k) * func_eval(test_data.phi[i]);
              // continue with next trial function
            }
            // continue with next test function
          }

          // finish functor evaluator
          func_eval.finish();

          // finish evaluators
          test_eval.finish();
          trafo_eval.finish();
          trafo_facet_eval.finish();

          // initialise dof-mapping
          dof_mapping.prepare(cell);

          // incorporate local matrix
          scatter_axpy(lvad, dof_mapping, alpha);

          // finish dof-mapping
          dof_mapping.finish();

          // continue with next cell
        }
      }

    protected:
      bool find_local_facet(Index face, Index cell, int& facet, int& ori)
      {
        typedef typename Shape::FaceTraits<ShapeType, facet_dim>::ShapeType FacetType;
        static constexpr int num_facets = Shape::FaceTraits<ShapeType, shape_dim-1>::count;
        static constexpr int num_vaf = Shape::FaceTraits<FacetType, 0>::count;

        typedef Geometry::Intern::IndexRepresentative<FacetType> FacetRepr;

        const auto& vert_at_elem = _trafo.get_mesh().template get_index_set<shape_dim, 0>();
        const auto& vert_at_face = _trafo.get_mesh().template get_index_set<facet_dim, 0>();
        //const auto& face_at_elem = _trafo.get_mesh().template get_index_set<shape_dim, facet_dim>();

        Index face_verts[num_vaf];
        FacetRepr::compute(face_verts, vert_at_face[face]);

        for(int li(0); li < num_facets; ++li)
        {
          Intern::CompIndexMap<decltype(vert_at_elem[0]), ShapeType, shape_dim-1> cim(vert_at_elem[cell], li);
          Index lf_verts[num_vaf];
          FacetRepr::compute(lf_verts, cim);
          bool match = true;
          for(int k(0); k < num_vaf; ++k)
          {
            if(lf_verts[k] != face_verts[k])
            {
              match = false;
              break;
            }
          }
          if(match)
          {
            ori = Geometry::Intern::CongruencySampler<FacetType>::compare(vert_at_face[face], cim);
            facet = li;
            return true;
          }
        }
        return false;
      }
    }; // class TraceAssembler<...>
  } // namespace Assembly
} // namespace FEAST

#endif // KERNEL_ASSEMBLY_TRACE_ASSEMBLER_HPP