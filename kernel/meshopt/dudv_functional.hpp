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
          _dirichlet_asm(nullptr),
          _slip_asm(nullptr),
          _cubature_factory("auto-degree:"+stringify(int(_local_degree)))
          {
          }

        explicit DuDvFunctional(DuDvFunctional&& other) :
          sys_matrix(std::move(other.sys_matrix)),
          _trafo(other._trafo),
          _trafo_space(other._trafo_space),
          _dirichlet_asm(other._dirichlet_asm),
          _slip_asm(other._slip_asm),
          _cubature_factory(other.cubature_factory)
          {
            if(this != &other)
            {
              other._trafo = nullptr;
              other._trafo_space = nullptr;
              other._dirichlet_asm = nullptr;
              other._slip_asm = nullptr;
            }
          }

        DuDvFunctional(const DuDvFunctional&) = delete;

        /// \brief Virtual destructor
        virtual ~DuDvFunctional()
        {
          _trafo = nullptr;
          _trafo_space = nullptr;
          _dirichlet_asm = nullptr;
          _slip_asm = nullptr;
        }

        /**
         * \brief Performs one-time initialisations
         *
         * This is not done in the constructor for the case that the system matrix gets overwritten by a derived
         * class, so the unused system matrix of THIS class is not assembled symbolically
         */
        virtual void init() override
        {
          ASSERT_(_trafo != nullptr);
          ASSERT_(_trafo_space != nullptr);
          ASSERT_(_dirichlet_asm != nullptr);
          ASSERT_(_slip_asm != nullptr);
          Assembly::SymbolicAssembler::assemble_matrix_std1(sys_matrix, *_trafo_space);
        }

        /**
         * \brief The class name
         *
         * \returns String with the class name
         */
        static String name()
        {
          return "DuDvFunctional<"+MeshType::name()+">";
        }


        virtual void assemble_system_matrix()
        {
          ASSERT_(_trafo_space != nullptr);

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
          auto& dirichlet_filters = filter.template at<1>();

          ASSERT_(_dirichlet_asm != nullptr);
          ASSERT_(_slip_asm != nullptr);

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
        const Index& columns() const
        {
          return sys_matrix.columns();
        }

        /**
         * \brief Returns the number of rows
         *
         * \returns The number of rows.
         */
        const Index& rows() const
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

  } // namespace Meshopt
} // namespace FEAT
#endif // KERNEL_MESHOPT_DUDV_FUNCTIONAL_HPP
