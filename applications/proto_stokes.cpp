/**
\brief FEAST prototypic Stokes 2D solver (TM)

This file contains a simple prototypic 2D Stokes Poiseuille-Flow solver for the unit square domain.

For the sake of fancyness, the linear solver in use is a geometric multigrid with a
Pressure-Schur-Complement-SOR smoother ;)

\attention This application is just a "proof-of-concept" and is not indented as a starting point
for further application development.

\author Peter Zajac
**/

// FEAST includes
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/cell_sub_set.hpp>
#include <kernel/geometry/conformal_factories.hpp>
#include <kernel/geometry/export_vtk.hpp>
#include <kernel/trafo/standard/mapping.hpp>
#include <kernel/space/discontinuous/element.hpp>
#include <kernel/space/rannacher_turek/element.hpp>
#include <kernel/assembly/symbolic_assembler.hpp>
#include <kernel/assembly/common_operators.hpp>
#include <kernel/assembly/common_functions.hpp>
#include <kernel/assembly/dirichlet_assembler.hpp>
#include <kernel/assembly/bilinear_operator_assembler.hpp>
#include <kernel/assembly/discrete_projector.hpp>
#include <kernel/assembly/grid_transfer.hpp>
#include <kernel/assembly/error_computer.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/lafem/transposition.hpp>

using namespace FEAST;

// basic typedefs
typedef double DataType;
typedef Mem::Main MemType;
typedef Algo::Generic AlgoType;

// we're working on quads
typedef Shape::Quadrilateral ShapeType;

// geometry typedefs
typedef Geometry::ConformalMesh<ShapeType> MeshType;
typedef Geometry::CellSubSet<ShapeType> CellSetType;
typedef Geometry::Factory<MeshType> MeshFactoryType;
typedef Geometry::Factory<CellSetType> CellFactoryType;

// our standard trafo
typedef Trafo::Standard::Mapping<MeshType> TrafoType;

// the velocity and pressure elements
typedef Space::RannacherTurek::Element<TrafoType> SpaceVeloType;
typedef Space::Discontinuous::Element<TrafoType> SpacePresType;

// matrix, vector and filter typedefs
typedef LAFEM::SparseMatrixCSR<MemType, DataType> MatrixType;
typedef LAFEM::DenseVector<MemType, DataType> VectorType;
typedef LAFEM::UnitFilter<MemType, DataType> FilterType;

// a simple SOR preconditioner
void SOR(int num_iter, const MatrixType& a, VectorType& x, const VectorType& b, DataType omega = DataType(1))
{
  Index num_rows(a.rows());
  const Index* row_ptr(a.row_ptr());
  const Index* col_idx(a.col_ind());
  const DataType* av(a.val());
  DataType* xv(x.elements());
  const DataType* bv(b.elements());
  DataType aii, di;

  for(int iter(0); iter < num_iter; ++iter)
  {
    for(Index i(0); i < num_rows; ++i)
    {
      aii = DataType(0);
      di = bv[i];
      for(Index j(row_ptr[i]); j < row_ptr[i + 1]; ++j)
      {
        if(col_idx[j] == i)
          aii = av[j];
        di -= av[j] * xv[col_idx[j]];
      }
      xv[i] += (omega * di) / aii;
    }
  }
}

// analytic X-velocity function
template<typename T_>
class SolX
{
public:
  static T_ eval(T_, T_ y)
  {
    return y * (T_(1) - y);
  }

  static T_ der_x(T_, T_)
  {
    return T_(0);
  }

  static T_ der_y(T_, T_ y)
  {
    return T_(1) - T_(2) * y;
  }
};

// analytic Y-velocity function
template<typename T_>
class SolY
{
public:
  static T_ eval(T_, T_)
  {
    return T_(0);
  }

  static T_ der_x(T_, T_)
  {
    return T_(0);
  }

  static T_ der_y(T_, T_)
  {
    return T_(0);
  }
};

// analytic pressure function
template<typename T_>
class SolP
{
public:
  static T_ eval(T_ x, T_)
  {
    return T_(2) * (T_(1) - x);
  }
};

typedef Assembly::StaticWrapperFunction<SolX, true, true> FuncSolX;
typedef Assembly::StaticWrapperFunction<SolY, true, true> FuncSolY;
typedef Assembly::StaticWrapperFunction<SolP, true, false> FuncSolP;

// a cell factory for the coarse mesh: contains all boundary edges except for the ones
// with a X-coordinate of 1.
class MyCellSetFactory :
  public Geometry::Factory<CellSetType>
{
public:
  virtual Index get_num_entities(int dim)
  {
    return Index(dim == 0 ? 4 : (dim == 1 ? 3 : 0));
  }

  virtual void fill_target_sets(CellSetType::TargetSetHolderType& target_set_holder)
  {
    Geometry::TargetSet& vt = target_set_holder.get_target_set<0>();
    vt[0] = 0;
    vt[1] = 1;
    vt[2] = 2;
    vt[3] = 3;
    Geometry::TargetSet& et = target_set_holder.get_target_set<1>();
    et[0] = 0;
    et[1] = 1;
    et[2] = 2;
  }
}; // class Factory<CellSubSet<...>>

// a bilinear operator for the pressure gradient matrices
template<int der_>
class PressureGradientOperator :
  public Assembly::BilinearOperator
{
public:
  /// test space configuration
  struct TestConfig :
    public Space::ConfigBase
  {
    /// test-space requirement enumeration
    enum
    {
      /// this functor requires test-function values
      need_grad = 1
    };
  };

  /// trial space configuration
  struct TrialConfig :
    public Space::ConfigBase
  {
    /// trial-space requirement enumeration
    enum
    {
      /// this functor requires trial-function gradients
      need_value = 1
    };
  };

  template<typename AsmTraits_>
  class Evaluator :
    public Assembly::BilinearOperator::Evaluator<AsmTraits_>
  {
  public:
    /// the data type to be used
    typedef typename AsmTraits_::DataType DataType;
    /// the assembler's trafo data type
    typedef typename AsmTraits_::TrafoData TrafoData;
    /// the assembler's test-function data type
    typedef typename AsmTraits_::TestBasisData TestBasisData;
    /// the assembler's trial-function data type
    typedef typename AsmTraits_::TrialBasisData TrialBasisData;

  public:
    explicit Evaluator(const PressureGradientOperator&)
    {
    }

    /** \copydoc BilinearOperator::Evaluator::operator() */
    DataType operator()(const TrafoData&, const TrialBasisData& phi, const TestBasisData& psi)
    {
      return -phi.value * psi.grad[der_];
    }
  };
}; // class PressureGradientOperator

// A class containing all data for the discretised Stokes equation on a particular mesh level.
class StokesLevel
{
public:
  // the mesh for this level
  MeshType _mesh;
  // the cellset describing the boundary on this level
  CellSetType _cell_set;
  // a standard trafo object
  TrafoType _trafo;
  // the velocity space object
  SpaceVeloType _space_v;
  // the pressure space object
  SpacePresType _space_p;

  // velocity laplace matrix A
  MatrixType _matrix_a;
  // pressure gradient matrices B1, B2
  MatrixType _matrix_b1;
  MatrixType _matrix_b2;
  // velocity divergence matrices D1, D2
  MatrixType _matrix_d1;
  MatrixType _matrix_d2;
  // pressure mass matrix, needed for the Schur-complement preconditioner
  MatrixType _matrix_m;

  // prolongation/restriction matrices
  MatrixType _prol_v;
  MatrixType _prol_p;
  MatrixType _rest_v;
  MatrixType _rest_p;

  // boundary condition filters
  FilterType _filter_x;
  FilterType _filter_y;

  // rhs vectors
  VectorType _vec_rhs_x;
  VectorType _vec_rhs_y;
  VectorType _vec_rhs_p;

  // solution vectors
  VectorType _vec_sol_x;
  VectorType _vec_sol_y;
  VectorType _vec_sol_p;

  // defect vectors
  VectorType _vec_def_x;
  VectorType _vec_def_y;
  VectorType _vec_def_p;

public:
  explicit StokesLevel(MeshFactoryType& mesh_factory, CellFactoryType& cell_factory) :
    _mesh(mesh_factory),
    _cell_set(cell_factory),
    _trafo(_mesh),
    _space_v(_trafo),
    _space_p(_trafo),
    _vec_rhs_x(_space_v.get_num_dofs()),
    _vec_rhs_y(_space_v.get_num_dofs()),
    _vec_rhs_p(_space_p.get_num_dofs()),
    _vec_sol_x(_space_v.get_num_dofs()),
    _vec_sol_y(_space_v.get_num_dofs()),
    _vec_sol_p(_space_p.get_num_dofs()),
    _vec_def_x(_space_v.get_num_dofs()),
    _vec_def_y(_space_v.get_num_dofs()),
    _vec_def_p(_space_p.get_num_dofs())
  {
  }

  // refines this level, i.e. creates and returns the next finer stokes level
  StokesLevel* refine() const
  {
    Geometry::StandardRefinery<MeshType> mesh_factory(_mesh);
    Geometry::StandardRefinery<CellSetType, MeshType> cell_factory(_cell_set, _mesh);
    return new StokesLevel(mesh_factory, cell_factory);
  }

  // assembles all basic matrices
  void assemble_matrices()
  {
    // assemble matrix structures
    Assembly::SymbolicMatrixAssembler<>::assemble1(_matrix_a, _space_v);
    Assembly::SymbolicMatrixAssembler<>::assemble2(_matrix_b1, _space_v, _space_p);
    _matrix_b2 = _matrix_b1.clone();
    Assembly::SymbolicMatrixAssembler<>::assemble1(_matrix_m, _space_p);

    // create cubature factories
    Cubature::DynamicFactory cubature_factory_velo("gauss-legendre:3");
    Cubature::DynamicFactory cubature_factory_pres("gauss-legendre:2");

    // clear all matrices
    _matrix_a.clear();
    _matrix_b1.clear();
    _matrix_b2.clear();
    _matrix_m.clear();

    // assemble velocity laplace matrix A
    Assembly::Common::LaplaceOperator laplace;
    Assembly::BilinearOperatorAssembler::assemble_matrix1(_matrix_a, laplace, _space_v, cubature_factory_velo);

    // assemble pressure mass matrix
    Assembly::Common::IdentityOperator identity;
    Assembly::BilinearOperatorAssembler::assemble_matrix1(_matrix_m, identity, _space_p, cubature_factory_pres, -DataType(1));

    // assemble pressure gradient matrices B1 and B2
    PressureGradientOperator<0> gradient_x;
    PressureGradientOperator<1> gradient_y;
    Assembly::BilinearOperatorAssembler::assemble_matrix2(_matrix_b1, gradient_x, _space_v, _space_p, cubature_factory_velo);
    Assembly::BilinearOperatorAssembler::assemble_matrix2(_matrix_b2, gradient_y, _space_v, _space_p, cubature_factory_velo);

    // build velocity divergence matrices D1 and D2 by transposing B1 and B2
    _matrix_d1 = LAFEM::Transposition<AlgoType>::value(_matrix_b1);
    _matrix_d2 = LAFEM::Transposition<AlgoType>::value(_matrix_b2);
  }

  // assembles prolongation and restriction matrices
  void assemble_prolrest(StokesLevel& coarse)
  {
    // assemble matrix structures
    Assembly::SymbolicMatrixAssembler<Assembly::Stencil::StandardRefinement>::assemble(_prol_v, _space_v, coarse._space_v);
    Assembly::SymbolicMatrixAssembler<Assembly::Stencil::StandardRefinement>::assemble(_prol_p, _space_p, coarse._space_p);
    _prol_v.clear();
    _prol_p.clear();

    // create cubature factories
    Cubature::DynamicFactory cubature_factory_velo("gauss-legendre:3");
    Cubature::DynamicFactory cubature_factory_pres("gauss-legendre:1");

    // assemble prolongation matrices
    Assembly::GridTransfer::assemble_prolongation(_prol_v, _space_v, coarse._space_v, cubature_factory_velo);
    Assembly::GridTransfer::assemble_prolongation(_prol_p, _space_p, coarse._space_p, cubature_factory_pres);

    // transpose to obtain restriction matrices
    _rest_v = LAFEM::Transposition<AlgoType>::value(_prol_v);
    _rest_p = LAFEM::Transposition<AlgoType>::value(_prol_p);
  }

  // assembles the boundary conditions
  void assemble_bc()
  {
    // create two Dirichlet BC assemblers
    Assembly::DirichletAssembler<SpaceVeloType> dirichlet_x(_space_v);
    Assembly::DirichletAssembler<SpaceVeloType> dirichlet_y(_space_v);

    // add our boundary cell sets
    dirichlet_x.add_cell_set(_cell_set);
    dirichlet_y.add_cell_set(_cell_set);

    // assemble X-velocity BC values
    FuncSolX sol_x;
    _filter_x = dirichlet_x.assemble<MemType, DataType>(sol_x);

    // assemble Y-velocity BC values
    _filter_y = dirichlet_y.assemble<MemType, DataType>();

    // filter matrices
    _filter_x.filter_mat<AlgoType>(_matrix_a);

    // filter (off-diagonal) pressure gradient matrices
    _filter_x.filter_offdiag_row_mat<AlgoType>(_matrix_b1);
    _filter_y.filter_offdiag_row_mat<AlgoType>(_matrix_b2);
  }

  // filters the rhs vectors
  void filter_rhs(VectorType& rhs_x, VectorType& rhs_y)
  {
    _filter_x.filter_rhs<AlgoType>(rhs_x);
    _filter_y.filter_rhs<AlgoType>(rhs_y);
  }

  // filter the solution vectors
  void filter_sol(VectorType& sol_x, VectorType& sol_y)
  {
    _filter_x.filter_sol<AlgoType>(sol_x);
    _filter_y.filter_sol<AlgoType>(sol_y);
  }

  // computes the current velocity defect vectors
  void calc_defect_u(VectorType& def_x, VectorType& def_y,
    const VectorType& rhs_x, const VectorType& rhs_y,
    const VectorType& sol_x, const VectorType& sol_y, const VectorType& sol_p) const
  {
    // dx = bx - A*ux - B1*p
    _matrix_a.apply<AlgoType>(def_x, sol_x, rhs_x, -DataType(1));
    _matrix_b1.apply<AlgoType>(def_x, sol_p, def_x, -DataType(1));

    // dy = by - A*uy - B2*p
    _matrix_a.apply<AlgoType>(def_y, sol_y, rhs_y, -DataType(1));
    _matrix_b2.apply<AlgoType>(def_y, sol_p, def_y, -DataType(1));

    // filter defect vectors
    _filter_x.filter_def<AlgoType>(def_x);
    _filter_y.filter_def<AlgoType>(def_y);
  }

  // computes the current pressure/divergence defect vector
  void calc_defect_p(VectorType& def_p, const VectorType& rhs_p,
    const VectorType& sol_x, const VectorType& sol_y, const VectorType& /*sol_p*/) const
  {
    // dp = bp - D1*ux - D2*uy
    _matrix_d1.apply<AlgoType>(def_p, sol_x, rhs_p, -DataType(1));
    _matrix_d2.apply<AlgoType>(def_p, sol_y, def_p, -DataType(1));
  }

  // computes the current system defect and returns its norm
  DataType calc_defect(
    const VectorType& sol_x, const VectorType& sol_y, const VectorType& sol_p,
    const VectorType& rhs_x, const VectorType& rhs_y, const VectorType& rhs_p)
  {
    // compute defect vectors
    calc_defect_u(_vec_rhs_x, _vec_rhs_y,  rhs_x, rhs_y, sol_x, sol_y, sol_p);
    calc_defect_p(_vec_rhs_p, rhs_p, sol_x, sol_y, sol_p);

    // clear local solution
    _vec_sol_x.clear();
    _vec_sol_y.clear();
    _vec_sol_p.clear();

    // compute defect norm
    DataType dx = _vec_rhs_x.norm2<AlgoType>();
    DataType dy = _vec_rhs_y.norm2<AlgoType>();
    DataType dp = _vec_rhs_p.norm2<AlgoType>();
    return Math::sqrt(dx*dx + dy*dy + dp*dp);
  }

  // updates the solution vectors
  void update_solution(VectorType& sol_x, VectorType& sol_y, VectorType& sol_p)
  {
    // update solution vector
    _filter_x.filter_cor<AlgoType>(_vec_sol_x);
    _filter_y.filter_cor<AlgoType>(_vec_sol_y);
    sol_x.axpy<AlgoType>(_vec_sol_x, sol_x);
    sol_y.axpy<AlgoType>(_vec_sol_y, sol_y);
    sol_p.axpy<AlgoType>(_vec_sol_p, sol_p);
  }

  // applies the Schur-complement-SOR smoother
  void smooth(int nsteps, int na, int ns, DataType wa = DataType(1), DataType ws = DataType(1))
  {
    _vec_def_p.clear();
    for(int step(0); step < nsteps; ++step)
    {
      // dx_k = bx - B1*p_{k-1}
      _matrix_b1.apply<AlgoType>(_vec_def_x, _vec_sol_p, _vec_rhs_x, -DataType(1));
      SOR(na, _matrix_a, _vec_sol_x, _vec_def_x, wa);

      // dy_k = by - B2*p_{k-1}
      _matrix_b2.apply<AlgoType>(_vec_def_y, _vec_sol_p, _vec_rhs_y, -DataType(1));
      SOR(na, _matrix_a, _vec_sol_y, _vec_def_y, wa);

      // dp_k = dp_{k-1} + bp - D1*ux_k - D2*uy_k
      _vec_def_p.axpy<AlgoType>(_vec_rhs_p, _vec_def_p);
      _matrix_d1.apply<AlgoType>(_vec_def_p, _vec_sol_x, _vec_def_p, -DataType(1));
      _matrix_d2.apply<AlgoType>(_vec_def_p, _vec_sol_y, _vec_def_p, -DataType(1));
      SOR(ns, _matrix_m, _vec_sol_p, _vec_def_p, ws);
    }
  }

  // prolongates the solution of the coarse level onto this level
  void prolongate(StokesLevel& coarse)
  {
    // prolongate
    _prol_v.apply<AlgoType>(_vec_def_x, coarse._vec_sol_x);
    _prol_v.apply<AlgoType>(_vec_def_y, coarse._vec_sol_y);
    _prol_p.apply<AlgoType>(_vec_def_p, coarse._vec_sol_p);
    // filter
    _filter_x.filter_cor<AlgoType>(_vec_def_x);
    _filter_y.filter_cor<AlgoType>(_vec_def_y);
    // correct
    _vec_sol_x.axpy<AlgoType>(_vec_def_x, _vec_sol_x);
    _vec_sol_y.axpy<AlgoType>(_vec_def_y, _vec_sol_y);
    _vec_sol_p.axpy<AlgoType>(_vec_def_p, _vec_sol_p);
  }

  // restricts the defect of this level onto the coarse level
  void restrict(StokesLevel& coarse)
  {
    // compute defect
    _matrix_a.apply<AlgoType>(_vec_def_x, _vec_sol_x, _vec_rhs_x, -DataType(1));
    _matrix_b1.apply<AlgoType>(_vec_def_x, _vec_sol_p, _vec_def_x, -DataType(1));
    _matrix_a.apply<AlgoType>(_vec_def_y, _vec_sol_y, _vec_rhs_y, -DataType(1));
    _matrix_b1.apply<AlgoType>(_vec_def_y, _vec_sol_p, _vec_def_y, -DataType(1));
    _matrix_d1.apply<AlgoType>(_vec_def_p, _vec_sol_x, _vec_rhs_p, -DataType(1));
    _matrix_d2.apply<AlgoType>(_vec_def_p, _vec_sol_y, _vec_rhs_p, -DataType(1));

    // restrict
    _rest_v.apply<AlgoType>(coarse._vec_rhs_x, _vec_def_x);
    _rest_v.apply<AlgoType>(coarse._vec_rhs_y, _vec_def_y);
    _rest_p.apply<AlgoType>(coarse._vec_rhs_p, _vec_def_p);

    // filter
    coarse._filter_x.filter_def<AlgoType>(coarse._vec_rhs_x);
    coarse._filter_y.filter_def<AlgoType>(coarse._vec_rhs_y);

    // clear
    coarse._vec_sol_x.clear();
    coarse._vec_sol_y.clear();
    coarse._vec_sol_p.clear();
  }
};

// assembles the coarse Stokes level
StokesLevel* build_coarse_level(std::size_t lvl)
{
  MeshType* mesh = nullptr;
  CellSetType* cell = nullptr;

  MeshFactoryType* mesh_factory = new Geometry::UnitCubeFactory<MeshType>();
  CellFactoryType* cell_factory = new MyCellSetFactory();
  for(std::size_t i(0); i < lvl; ++i)
  {
    MeshType* mesh2 = mesh;
    CellSetType* cell2 = cell;
    mesh = new MeshType(*mesh_factory);
    cell = new CellSetType(*cell_factory);
    delete cell_factory;
    delete mesh_factory;
    if(cell2 != nullptr)
      delete cell2;
    if(mesh2 != nullptr)
      delete mesh2;
    mesh_factory = new Geometry::StandardRefinery<MeshType>(*mesh);
    cell_factory = new Geometry::StandardRefinery<CellSetType, MeshType>(*cell, *mesh);
  }

  StokesLevel* level = new StokesLevel(*mesh_factory, *cell_factory);

  delete cell_factory;
  delete mesh_factory;
  if(cell != nullptr)
    delete cell;
  if(mesh != nullptr)
    delete mesh;

  return level;
}

// computes the L2/H1 errors of the discrete solution
void calc_errors(const StokesLevel& level, const VectorType& vec_ux, const VectorType& vec_uy, const VectorType& vec_p)
{
  // define solution functors
  FuncSolX func_sol_x;
  FuncSolY func_sol_y;
  FuncSolP func_sol_p;

  // define cubature factory
  Cubature::DynamicFactory cubature_factory("gauss-legendre:4");

  // compute velocity L2-errors
  DataType l2_ux = Assembly::ScalarErrorComputerL2::compute(vec_ux, func_sol_x, level._space_v, cubature_factory);
  DataType l2_uy = Assembly::ScalarErrorComputerL2::compute(vec_uy, func_sol_y, level._space_v, cubature_factory);
  DataType l2_u = Math::sqrt(Math::sqr(l2_ux) + Math::sqr(l2_uy));

  // compute velocity H1-errors
  DataType h1_ux = Assembly::ScalarErrorComputerH1::compute(vec_ux, func_sol_x, level._space_v, cubature_factory);
  DataType h1_uy = Assembly::ScalarErrorComputerH1::compute(vec_uy, func_sol_y, level._space_v, cubature_factory);
  DataType h1_u = Math::sqrt(Math::sqr(h1_ux) + Math::sqr(h1_uy));

  // compute pressure L2-error
  DataType l2_p = Assembly::ScalarErrorComputerL2::compute(vec_p, func_sol_p, level._space_p, cubature_factory);

  // print errors
  std::cout << std::endl;
  std::cout << "u L2-Errors: " << scientify(l2_u) << " ( " << scientify(l2_ux) << " , " << scientify(l2_uy) << " )" << std::endl;
  std::cout << "u H1-Errors: " << scientify(h1_u) << " ( " << scientify(h1_ux) << " , " << scientify(h1_uy) << " )" << std::endl;
  std::cout << "p L2-Error : " << scientify(l2_p) << std::endl;
}

// writes the discrete solution to a VTK file
void write_vtk(String vtk_name, const StokesLevel& level,
  const VectorType& vec_ux, const VectorType& vec_uy, const VectorType& vec_p)
{
  // write VTK
  std::cout << std::endl << "Writing VTK file '" << vtk_name << "'..." << std::endl;
  Geometry::ExportVTK<MeshType> writer(level._mesh);
  VectorType ux, uy, p;
  Cubature::DynamicFactory cub_factory("barycentre");
  Assembly::DiscreteVertexProjector::project(ux, vec_ux, level._space_v);
  Assembly::DiscreteVertexProjector::project(uy, vec_uy, level._space_v);
  Assembly::DiscreteCellProjector::project(p, vec_p, level._space_p, cub_factory);
  writer.add_scalar_vertex("sol_x", ux.elements());
  writer.add_scalar_vertex("sol_y", uy.elements());
  writer.add_scalar_cell("sol_p", p.elements());
  writer.write(vtk_name);
}

int main(int /*argc*/, char** /*argv*/)
{
  std::size_t lvl_min = 1;
  std::size_t lvl_max = 5;

  // allocate levels
  typedef std::vector<StokesLevel*> Levels;
  Levels levels;
  std::cout << "Allocating Level " << lvl_min << "..." << std::endl;
  levels.push_back(build_coarse_level(lvl_min));
  for(std::size_t i(lvl_min+1); i <= lvl_max; ++i)
  {
    std::cout << "Allocating Level " << i << "..." << std::endl;
    levels.push_back(levels.back()->refine());
  }

  // assemble grid transfer
  for(std::size_t i(lvl_min+1); i <= lvl_max; ++i)
  {
    std::cout << "Assembling Grid Transfer for Level " << (i-1) << " -> " << i << "..." << std::endl;
    levels.at(i - lvl_min)->assemble_prolrest(*levels.at(i - lvl_min - 1));
  }

  // assemble matrices
  for(std::size_t i(lvl_min); i <= lvl_max; ++i)
  {
    std::cout << "Assembling Matrices on Level " << i << "..." << std::endl;
    levels.at(i - lvl_min)->assemble_matrices();
  }

  // assemble BCs
  for(std::size_t i(lvl_min); i <= lvl_max; ++i)
  {
    std::cout << "Assembling Boundary Conditions on Level " << i << "..." << std::endl;
    levels.at(i - lvl_min)->assemble_bc();
  }

  // allocate the rhs and solution vectors on finest level
  std::cout << "Assembling RHS and initial solution vector..." << std::endl;
  VectorType vec_rhs_x(levels.back()->_space_v.get_num_dofs(), DataType(0));
  VectorType vec_rhs_y(levels.back()->_space_v.get_num_dofs(), DataType(0));
  VectorType vec_rhs_p(levels.back()->_space_p.get_num_dofs(), DataType(0));
  VectorType vec_sol_x(levels.back()->_space_v.get_num_dofs(), DataType(0));
  VectorType vec_sol_y(levels.back()->_space_v.get_num_dofs(), DataType(0));
  VectorType vec_sol_p(levels.back()->_space_p.get_num_dofs(), DataType(0));

  // filter the rhs and solution vectors
  levels.back()->filter_rhs(vec_rhs_x, vec_rhs_y);
  levels.back()->filter_sol(vec_sol_x, vec_sol_y);

  // compute initial defect
  DataType def0 = levels.back()->calc_defect(vec_sol_x, vec_sol_y, vec_sol_p, vec_rhs_x, vec_rhs_y, vec_rhs_p);
  std::cout << std::endl << "Iteration 0 | Defect: " << scientify(def0) << std::endl;

  // start iteration
  for(int i(0); i < 100; ++i)
  {
    // restriction loop
    for(std::size_t lvl(lvl_max); lvl > lvl_min; --lvl)
    {
      // pre-smooth
      levels.at(lvl - lvl_min)->smooth(16, 2, 1, 1.0, 1.0);

      // restrict
      levels.at(lvl - lvl_min)->restrict(*levels.at(lvl - lvl_min - 1));
    }

    // coarse-grid solve
    levels.front()->smooth(500, 2, 1, 1.0, 1.0);

    // prolongation loop
    for(std::size_t lvl(lvl_min+1); lvl <= lvl_max; ++lvl)
    {
      // prolongate
      levels.at(lvl - lvl_min)->prolongate(*levels.at(lvl - lvl_min - 1));

      // post-smooth
     //levels.at(lvl - lvl_min)->smooth(1, 1, 1, 1.0, 1.0);
    }

    // update solution
    levels.back()->update_solution(vec_sol_x, vec_sol_y, vec_sol_p);

    // compute new defect
    DataType def = levels.back()->calc_defect(vec_sol_x, vec_sol_y, vec_sol_p, vec_rhs_x, vec_rhs_y, vec_rhs_p);
    std::cout << "Iteration " << (i+1) << " | Defect: " << scientify(def) << std::endl;
    if((def / def0) <= 1E-8)
      break;
  }

  // compute errors against reference solution
  calc_errors(*levels.back(), vec_sol_x, vec_sol_y, vec_sol_p);

  // write vtk
  //write_vtk("./proto_stokes.vtk", *levels.back(), vec_sol_x, vec_sol_y, vec_sol_p);

  // clear levels
  std::cout << std::endl << "Cleaning up..." << std::endl;
  while(!levels.empty())
  {
    delete levels.back();
    levels.pop_back();
  }

  // okay
  return 0;
}
