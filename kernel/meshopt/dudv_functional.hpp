#pragma once
#ifndef KERNEL_MESHOPT_DUDV_FUNCTIONAL_HPP
#define KERNEL_MESHOPT_DUDV_FUNCTIONAL_HPP 1

#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/assembly/bilinear_operator_assembler.hpp> // for BilinearOperatorAssembler
#include <kernel/assembly/common_operators.hpp>            // for DuDvOperator
#include <kernel/assembly/symbolic_assembler.hpp>          // for SymbolicMatrixAssembler
#include <kernel/assembly/slip_filter_assembler.hpp>
#include <kernel/assembly/unit_filter_assembler.hpp>
#include <kernel/cubature/dynamic_factory.hpp>             // for DynamicFactory
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/lafem/dense_vector_blocked.hpp>
#include <kernel/lafem/filter_chain.hpp>
#include <kernel/lafem/filter_sequence.hpp>
#include <kernel/lafem/sparse_matrix_bcsr.hpp>
#include <kernel/lafem/slip_filter.hpp>
#include <kernel/lafem/unit_filter_blocked.hpp>
#include <kernel/meshopt/mesh_quality_functional.hpp>
#include <kernel/space/lagrange1/element.hpp>
#include <kernel/util/comm_base.hpp>

namespace FEAT
{
  namespace Meshopt
  {
    /**
     * \brief Mesh optimiser based on minimisation of harmonic energy
     *
     * \tparam Mem_
     * Memory architecture for the solver (not the mesh)
     *
     * \tparam DT_
     * Data type for the solver (not the mesh)
     *
     * \tparam IT_
     * Index type for the solver (not the mesh)
     *
     * \tparam TrafoType_
     * Type of the underlying transformation.
     *
     * \author Jordi Paul
     *
     */
    template
    <
      typename Mem_, typename DT_, typename IT_, typename TrafoType_,
      template<typename, typename, typename, int, int> class MatrixType_ = LAFEM::SparseMatrixBCSR
    >
    class DuDvFunctional:
      public MeshQualityFunctional<typename TrafoType_::MeshType>
    {
      public:
        /// Memory architecture
        typedef Mem_ MemType;
        /// Our datatype
        typedef DT_ DataType;
        /// Our index type
        typedef IT_ IndexType;
        /// Type for the transformation
        typedef TrafoType_ TrafoType;
        /// The mesh the transformation is defined on
        typedef typename TrafoType::MeshType MeshType;

        /// Type for the system matrix
        typedef MatrixType_<Mem_, DT_, IT_, MeshType::world_dim, MeshType::world_dim> MatrixType;
        /// Blockheight of the system matrix
        static constexpr int BlockHeight = MatrixType::BlockHeight;
        /// Blockwidth of the system matrix
        static constexpr int BlockWidth = MatrixType::BlockWidth;

        /// Our base class
        typedef MeshQualityFunctional<MeshType> BaseClass;
        /// Type for exchanging information between state variable and mesh
        typedef typename BaseClass::CoordsBufferType CoordsBufferType;

        /// The precision of the mesh coordinates
        typedef typename MeshType::CoordType CoordType;

        //template<typename A, typename B, typename C>
        //using MatrixTemplate = MatrixType_<A, B, C, MeshType::world_dim, MeshType::world_dim>;

        /// Type for vectors from the dual space
        typedef typename MatrixType::VectorTypeL VectorTypeL;
        /// Type for vectors from the primal space
        typedef typename MatrixType::VectorTypeR VectorTypeR;
        /// Type for i.e. cell vectors
        typedef LAFEM::DenseVector<Mem_, DT_, IT_> ScalarVectorType;

        /// Filter for Dirichlet boundary conditions
        typedef LAFEM::UnitFilterBlocked<Mem_, DT_, IT_, MeshType::world_dim> DirichletFilterType;
        /// Sequence of Dirichlet filters for several different boundary parts
        typedef LAFEM::FilterSequence<DirichletFilterType> DirichletFilterSequence;
        /// Filter for slip boundary conditions
        typedef LAFEM::SlipFilter<Mem_, DT_, IT_, MeshType::world_dim> SlipFilterType;
        /// Sequence of Slip filters for several different boundary parts
        typedef LAFEM::FilterSequence<SlipFilterType> SlipFilterSequence;
        /// Combined filter
        typedef LAFEM::FilterChain<SlipFilterSequence, DirichletFilterSequence> FilterType;

        /// Finite Element space for the transformation
        typedef typename Intern::TrafoFE<TrafoType>::Space TrafoSpace;
        /// Maximum polynomial degree
        // 2 * (degree of trafo) for both trial and test spaces, +1 for safety reasons
        // This could be decreased by the degree of the operator, i.e. 2 for Du:Dv
        static constexpr int _local_degree = 4*TrafoSpace::local_degree + 1;

        /// The system matrix
        MatrixType sys_matrix;

      protected:
        /// The transformation defining the physical mesh
        TrafoType* _trafo;
        /// The FE space for the transformation, needed for filtering
        TrafoSpace* _trafo_space;
        /// Vector saving the cell sizes on the reference mesh
        ScalarVectorType _cell_sizes;
        /// Assembler for Dirichlet boundary conditions
        std::map<String, std::shared_ptr<Assembly::UnitFilterAssembler<MeshType>>>* _dirichlet_asm;
        /// Assembler for slip boundary conditions
        std::map<String, std::shared_ptr<Assembly::SlipFilterAssembler<MeshType>>>* _slip_asm;
        /// Cubature factory, for P1/Q1 transformations in 2d degree 5 is enough
        Cubature::DynamicFactory _cubature_factory;

      public:
        /**
         * \brief Constructor
         *
         * \param[in] rmn_
         * The RootMeshNode representing the tree of root mesh, all of its MeshParts and Charts
         *
         * \param[in] dirichlet_list_
         * List of boundary identifiers for enforcing Dirichlet boundary conditions, can be empty
         *
         * \param[in] slip_list_
         * List of boundary identifiers for enforcing slip boundary conditions, can be empty
         *
         */
        explicit DuDvFunctional(
          Geometry::RootMeshNode<MeshType>* rmn_,
          TrafoSpace* trafo_space_,
          std::map<String, std::shared_ptr<Assembly::UnitFilterAssembler<MeshType>>>* dirichlet_asm_,
          std::map<String, std::shared_ptr<Assembly::SlipFilterAssembler<MeshType>>>* slip_asm_) :
          BaseClass(rmn_),
          sys_matrix(),
          _trafo(&(trafo_space_->get_trafo())),
          _trafo_space(trafo_space_),
          _cell_sizes(rmn_->get_mesh()->get_num_entities(MeshType::shape_dim)),
          _dirichlet_asm(dirichlet_asm_),
          _slip_asm(slip_asm_),
          _cubature_factory("auto-degree:"+stringify(int(_local_degree)))
          {
          }

        explicit DuDvFunctional() :
          BaseClass(),
          sys_matrix(),
          _trafo(nullptr),
          _trafo_space(nullptr),
          _cell_sizes(),
          _dirichlet_asm(nullptr),
          _slip_asm(nullptr),
          _cubature_factory("auto-degree:"+stringify(int(_local_degree)))
          {
          }

        explicit DuDvFunctional(DuDvFunctional&& other) :
          sys_matrix(std::move(other.sys_matrix)),
          _trafo(other._trafo),
          _trafo_space(other._trafo_space),
          _cell_sizes(std::move(other._cell_sizes)),
          _dirichlet_asm(other._dirichlet_asm),
          _slip_asm(other._slip_asm),
          _cubature_factory(other._cubature_factory)
          {
            if(this != &other)
            {
              other._trafo = nullptr;
              other._trafo_space = nullptr;
              other._dirichlet_asm = nullptr;
              other._slip_asm = nullptr;
            }
          }

        /// Explicitly delete copy constructor
        DuDvFunctional(const DuDvFunctional&) = delete;

        /// \brief Virtual destructor
        virtual ~DuDvFunctional()
        {
          _trafo = nullptr;
          _trafo_space = nullptr;
          _dirichlet_asm = nullptr;
          _slip_asm = nullptr;
          _cell_sizes.clear();
        }

        /**
         * \brief Computes a quality indicator concerning the cell sizes
         *
         * For the DuDv functional, the reference mesh is optimal, so we want to keep the cells at the same size.
         *
         * So in an optimal mesh,
         * \f[
         *   \forall K \in \mathcal{T}_h: \frac{|K_{new}|}{|K_{ref}|} = 1,
         * \f]
         * so we compute the Euclidean norm of the vector \f$(v)_i = \frac{1}{N}(1 -  \frac{|K_i|}{|K_{i,ref}|)} \f$.
         * This is scaled by the number of cells so it is independent of the refinement level. Not sure if the
         * scaling part is sensible, though.
         *
         * \returns The relative cell size quality indicator.
         *
         */
        virtual CoordType compute_cell_size_quality() const override
        {
          CoordType my_vol(0);
          CoordType my_quality(0);

          for(Index cell(0); cell < this->get_mesh()->get_num_entities(MeshType::shape_dim); ++cell)
          {
            my_vol = this->_trafo->template compute_vol<typename MeshType::ShapeType, CoordType>(cell);
            my_quality += Math::sqr(CoordType(1) - my_vol/_cell_sizes(cell));
          }

          Index ncells(this->get_mesh()->get_num_entities(MeshType::shape_dim));

#ifdef FEAT_HAVE_MPI
          Index ncells_snd(ncells);
          Util::Comm::allreduce(&ncells_snd, Index(1), &ncells, MPI_SUM);
          CoordType my_quality_snd(my_quality);
          Util::Comm::allreduce(&my_quality_snd, Index(1), &my_quality, MPI_SUM);
#endif
          my_quality = Math::sqrt(my_quality);

          return Math::abs(CoordType(1) - my_quality/CoordType(ncells));

        }

        /**
         * \brief Performs one-time initialisations
         *
         * This is not done in the constructor for the case that the system matrix gets overwritten by a derived
         * class, so the unused system matrix of THIS class is not assembled symbolically
         */
        virtual void init() override
        {
          XASSERT(_trafo != nullptr);
          XASSERT(_trafo_space != nullptr);
          XASSERT(_dirichlet_asm != nullptr);
          XASSERT(_slip_asm != nullptr);
          Assembly::SymbolicAssembler::assemble_matrix_std1(sys_matrix, *_trafo_space);
        }

        /// \copydoc BaseClass::name()
        virtual String name() const override
        {
          return "DuDvFunctional<"+MeshType::name()+">";
        }


        virtual void assemble_system_matrix()
        {
          XASSERT(_trafo_space != nullptr);

          sys_matrix.format();

          Assembly::Common::DuDvOperatorBlocked<MeshType::world_dim> my_operator;

          Assembly::BilinearOperatorAssembler::assemble_block_matrix1(
            sys_matrix,           // the matrix that receives the assembled operator
            my_operator, // the operator that is to be assembled
            *_trafo_space,            // the finite element space in use
            _cubature_factory  // the cubature factory to be used for integration
            );
        }

        /**
         * \brief Prepares the functional for evaluation.
         *
         * Needs to be called whenever any data like the mesh, the levelset function etc. changed.
         *
         **/
        // \todo override
        virtual void prepare(VectorTypeR& vec_state, FilterType& filter)
        {
          XASSERT(_dirichlet_asm != nullptr);
          XASSERT(_slip_asm != nullptr);

          for(Index cell(0); cell < this->get_mesh()->get_num_entities(MeshType::shape_dim); ++cell)
          {
            _cell_sizes(cell, _trafo->template compute_vol<typename MeshType::ShapeType>(cell));
          }

          auto& dirichlet_filters = filter.template at<1>();

          for(auto& it : dirichlet_filters)
          {
            const auto& assembler = _dirichlet_asm->find(it.first);
            if(assembler == _dirichlet_asm->end())
              throw InternalError(__func__,__FILE__,__LINE__,
              "Could not find dirichlet assembler for filter with key "+it.first);

            assembler->second->assemble(it.second, *_trafo_space, vec_state);
          }

          // The slip filter contains the outer unit normal, so reassemble it
          auto& slip_filters = filter.template at<0>();

          for(auto& it : slip_filters)
          {
            const auto& assembler = _slip_asm->find(it.first);
            if(assembler == _slip_asm->end())
              throw InternalError(__func__,__FILE__,__LINE__,
              "Could not find slip filter assembler for filter with key "+it.first);

            assembler->second->assemble(it.second, *_trafo_space);
          }

          for(const auto& it:slip_filters)
            this->_mesh_node->adapt_by_name(it.first);

        }

        /**
         * \brief Checks if the functional is empty (= the null functional
         *
         * \returns True if the number of DoFs is zero.)
         */
        bool empty() const
        {
          return sys_matrix.empty();
        }

        /**
         * \brief Creates an L-vector for the functional's gradient
         */
        VectorTypeL create_vector_l() const
        {
          return sys_matrix.create_vector_l();
        }

        /**
         * \brief Creates an R-vector for the functional and its gradient
         */
        VectorTypeR create_vector_r() const
        {
          return sys_matrix.create_vector_r();
        }

        /**
         * \brief Returns the number of columns
         *
         * \returns The number of columns.
         */
        Index columns() const
        {
          return sys_matrix.columns();
        }

        /**
         * \brief Returns the number of rows
         *
         * \returns The number of rows.
         */
        Index rows() const
        {
          return sys_matrix.rows();
        }

        /// \copydoc MatrixType::apply()
        void apply(VectorTypeL& r, const VectorTypeR& x) const
        {
          sys_matrix.apply(r, x);
        }

        /// \copydoc MatrixType::apply()
        void apply(VectorTypeL& r, const VectorTypeR& x, const VectorTypeL& y, const DataType alpha = DataType(1)) const
        {
          // copy y to r
          r.copy(y);
          sys_matrix.apply(r, x, r, alpha);
        }

        /// \copydoc MatrixType::extract_diag()
        void extract_diag(VectorTypeL& diag) const
        {
          sys_matrix.extract_diag(diag);
        }

        /// \copydoc MatrixType::format()
        void format(DataType value = DataType(0))
        {
          sys_matrix.format(value);
        }

        //virtual void prepare(VectorTypeR& vec_state)
        //{
        //  if(_filter != nullptr)
        //    _dirichlet_asm.assemble(_filter->template at<1>(), _trafo_space, vec_state);

        //  // Adapt all slip boundaries
        //  //for(auto& it : _slip_list)
        //  //  this->_mesh_node->adapt_by_name(it);

        //  // Assemble homogeneous slip boundary conditions, as the outer normal could have changed
        //  //_slip_asm.assemble(_filter.template at<0>(), _trafo_space);

        //  //sys_matrix.format();

        //  //Assembly::Common::DuDvOperatorBlocked<MeshType::world_dim> my_operator;

        //  //Assembly::BilinearOperatorAssembler::assemble_block_matrix1(
        //  //  sys_matrix,           // the matrix that receives the assembled operator
        //  //  my_operator, // the operator that is to be assembled
        //  //  _trafo_space,            // the finite element space in use
        //  //  _cubature_factory  // the cubature factory to be used for integration
        //  //  );

        //  return;
        //}

    }; // class DuDvFunctional

    extern template class DuDvFunctional
    <
      Mem::Main, double, Index,
      Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Simplex<2>, 2, 2, double>>,
      LAFEM::SparseMatrixBCSR
    >;

    extern template class DuDvFunctional
    <
      Mem::Main, double, Index,
      Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<2>, 2, 2, double>>,
      LAFEM::SparseMatrixBCSR
    >;

  } // namespace Meshopt
} // namespace FEAT
#endif // KERNEL_MESHOPT_DUDV_FUNCTIONAL_HPP
