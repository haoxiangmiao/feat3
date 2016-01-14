#include <test_system/test_system.hpp>
#include <kernel/assembly/grid_transfer.hpp>
#include <kernel/assembly/symbolic_assembler.hpp>
#include <kernel/assembly/common_operators.hpp>
#include <kernel/assembly/bilinear_operator_assembler.hpp>
#include <kernel/geometry/conformal_factories.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/vector_mirror.hpp>
#include <kernel/lafem/matrix_mirror.hpp>
#include <kernel/space/discontinuous/element.hpp>
#include <kernel/space/lagrange1/element.hpp>
#include <kernel/space/lagrange2/element.hpp>
#include <kernel/trafo/standard/mapping.hpp>
#include <kernel/util/math.hpp>

using namespace FEAST;
using namespace FEAST::TestSystem;

template<typename ShapeType, template<typename> class Element_, Index level_coarse_>
class GridTransferMassTest :
  public TestSystem::BaseTest
{
  typedef Mem::Main MemType;
  typedef double DataType;
  typedef Index IndexType;

  typedef LAFEM::DenseVector<MemType, DataType, IndexType> VectorType;
  typedef LAFEM::SparseMatrixCSR<MemType, DataType, IndexType> MatrixType;

  typedef LAFEM::VectorMirror<MemType, DataType, IndexType> VecMirType;
  typedef LAFEM::MatrixMirror<VecMirType> MatMirType;

  typedef Geometry::ConformalMesh<ShapeType> MeshType;

  typedef Trafo::Standard::Mapping<MeshType> TrafoType;
  typedef Element_<TrafoType> SpaceType;

public:
  explicit GridTransferMassTest() :
    TestSystem::BaseTest("GridTransferMassTest<" + ShapeType::name() + "," + SpaceType::name() + ">")
  {
  }

  virtual void run() const override
  {
    // create coarse mesh
    Geometry::RefinedUnitCubeFactory<MeshType> coarse_factory(level_coarse_);
    MeshType mesh_c(coarse_factory);

    // refine the mesh
    Geometry::StandardRefinery<MeshType> refine_factory(mesh_c);
    MeshType mesh_f(refine_factory);

    // compute eps
    const DataType eps = Math::pow(Math::eps<DataType>(), DataType(0.8));

    // create trafos
    TrafoType trafo_f(mesh_f);
    TrafoType trafo_c(mesh_c);

    // create spaces
    SpaceType space_f(trafo_f);
    SpaceType space_c(trafo_c);

    // create a cubature factory of appropriate degree
    Cubature::DynamicFactory cubature_factory("auto-degree:" + stringify(Math::sqr(SpaceType::local_degree+1)+2));

    // assemble fine/coarse mesh mass matrix
    MatrixType mass_f, mass_c;
    Assembly::SymbolicMatrixAssembler<>::assemble1(mass_f, space_f);
    Assembly::SymbolicMatrixAssembler<>::assemble1(mass_c, space_c);
    mass_f.format();
    mass_c.format();
    Assembly::Common::IdentityOperator operat;
    Assembly::BilinearOperatorAssembler::assemble_matrix1(mass_f, operat, space_f, cubature_factory);
    Assembly::BilinearOperatorAssembler::assemble_matrix1(mass_c, operat, space_c, cubature_factory);

    // assemble prolongation matrix
    MatrixType prol_matrix;
    VectorType weight_vector(space_f.get_num_dofs());
    Assembly::SymbolicMatrixAssembler<Assembly::Stencil::StandardRefinement>::assemble(prol_matrix, space_f, space_c);
    prol_matrix.format();
    weight_vector.format();
    Assembly::GridTransfer::assemble_prolongation(prol_matrix, weight_vector, space_f, space_c, cubature_factory);
    weight_vector.component_invert(weight_vector);
    prol_matrix.scale_rows(prol_matrix, weight_vector);

    // transpose to obtain restriction matrix
    MatrixType rest_matrix(prol_matrix.transpose());

    // build a matrix mirror using the prolongation and restriction matrices
    VecMirType vec_mirror(std::move(rest_matrix), std::move(prol_matrix));
    MatMirType mat_mirror(vec_mirror, vec_mirror);

    // finally, restrict the fine mesh mass matrix onto the coarse mesh and
    // subtract it from the coarse mesh mass matrix, i.e.
    // M_c <- M_c - R * M_f * P
    mat_mirror.gather_axpy(mass_c, mass_f, -DataType(1));

    // the resulting matrix should now be the null matrix
    DataType err = Math::sqr(mass_c.norm_frobenius());

    // and check for zero
    TEST_CHECK_EQUAL_WITHIN_EPS(err, DataType(0), eps);
  }
};

// Lagrange-1 element
GridTransferMassTest<Shape::Hypercube<1>, Space::Lagrange1::Element, 4> grid_transfer_mass_test_hy1_lagrange1;
GridTransferMassTest<Shape::Hypercube<2>, Space::Lagrange1::Element, 2> grid_transfer_mass_test_hy2_lagrange1;
GridTransferMassTest<Shape::Hypercube<3>, Space::Lagrange1::Element, 1> grid_transfer_mass_test_hy3_lagrange1;
GridTransferMassTest<Shape::Simplex<2>, Space::Lagrange1::Element, 2> grid_transfer_mass_test_sx2_lagrange1;
GridTransferMassTest<Shape::Simplex<3>, Space::Lagrange1::Element, 1> grid_transfer_mass_test_sx3_lagrange1;

// Lagrange-2 element
GridTransferMassTest<Shape::Hypercube<1>, Space::Lagrange2::Element, 4> grid_transfer_mass_test_hy1_lagrange2;
GridTransferMassTest<Shape::Hypercube<2>, Space::Lagrange2::Element, 2> grid_transfer_mass_test_hy2_lagrange2;
GridTransferMassTest<Shape::Hypercube<3>, Space::Lagrange2::Element, 1> grid_transfer_mass_test_hy3_lagrange2;
GridTransferMassTest<Shape::Simplex<2>, Space::Lagrange2::Element, 2> grid_transfer_mass_test_sx2_lagrange2;
GridTransferMassTest<Shape::Simplex<3>, Space::Lagrange2::Element, 1> grid_transfer_mass_test_sx3_lagrange2;
