#include <kernel/util/runtime.hpp>
#include <kernel/util/simple_arg_parser.hpp>
#include <kernel/util/time_stamp.hpp>
#include <kernel/util/statistics.hpp>
#include <kernel/trafo/standard/mapping.hpp>
#include <kernel/space/lagrange2/element.hpp>
#include <kernel/space/discontinuous/element.hpp>
#include <kernel/solver/legacy_preconditioners.hpp>
#include <kernel/assembly/unit_filter_assembler.hpp>
#include <kernel/assembly/error_computer.hpp>
#include <kernel/solver/basic_vcycle.hpp>
#include <kernel/solver/bicgstab.hpp>
#include <kernel/solver/fgmres.hpp>
#include <kernel/solver/pcr.hpp>
#include <kernel/solver/ilu_precond.hpp>
#include <kernel/solver/richardson.hpp>
#include <kernel/solver/scale_precond.hpp>
#include <kernel/solver/schwarz_precond.hpp>
#include <kernel/solver/schur_precond.hpp>
#include <kernel/solver/jacobi_precond.hpp>
#include <kernel/util/dist.hpp>
#include <kernel/solver/matrix_stock.hpp>

#include <control/domain/parti_domain_control.hpp>
#include <control/stokes_basic.hpp>
#include <control/statistics.hpp>
#include <control/solver_factory.hpp>

namespace StokesPoiseuille2D
{
  using namespace FEAT;

  template<typename T_>
  struct VeloFuncX
  {
    static T_ eval (T_, T_ y) {return y * (T_(1) - y);}
    static T_ der_x(T_, T_  ) {return T_(0);}
    static T_ der_y(T_, T_ y) {return T_(1) - T_(2)*y;}
  };

  template<typename T_>
  struct VeloFuncY
  {
    static T_ eval (T_, T_) {return T_(0);}
    static T_ der_x(T_, T_) {return T_(0);}
    static T_ der_y(T_, T_) {return T_(0);}
  };

  template<typename T_>
  struct PresFunc
  {
    static T_ eval (T_ x, T_) {return T_(2)*(T_(1) - x);}
  };

  template<
    typename SpaceVelo_,
    typename SpacePres_>
  class StokesUnitSquarePoiseuilleAssemblerLevel :
    public Control::StokesBasicAssemblerLevel<SpaceVelo_, SpacePres_>
  {
  public:
    typedef Control::StokesBasicAssemblerLevel<SpaceVelo_, SpacePres_> BaseClass;
    typedef typename SpaceVelo_::MeshType MeshType;

  public:
    explicit StokesUnitSquarePoiseuilleAssemblerLevel(typename BaseClass::DomainLevelType& dom_lvl) :
      BaseClass(dom_lvl)
    {
    }

    template<typename SystemLevel_>
    void assemble_velocity_filter(SystemLevel_& sys_level)
    {
      // get our global velocity filter
      typename SystemLevel_::GlobalVeloFilter& fil_glob_v = sys_level.filter_velo;

      // get our local velocity filter
      typename SystemLevel_::LocalVeloFilter& fil_loc_v = *fil_glob_v;

      // create unit-filter assembler
      Assembly::UnitFilterAssembler<MeshType> unit_asm;

      // loop over all boundary parts except for the right one, which is outflow
      std::deque<String> part_names;
      part_names.push_back("bnd:l");
      part_names.push_back("bnd:t");
      part_names.push_back("bnd:b");
      for(const auto& name : part_names)
      {
        // try to fetch the corresponding mesh part node
        auto* mesh_part_node = this->domain_level.get_mesh_node()->find_mesh_part_node(name);

        // found it?
        if (mesh_part_node == nullptr)
          throw InternalError("Mesh Part Node 'boundary' not found!");

        // let's see if we have that mesh part
        // if it is nullptr, then our patch is not adjacent to that boundary part
        auto* mesh_part = mesh_part_node->get_mesh();
        if (mesh_part != nullptr)
        {
          // add to boundary assembler
          unit_asm.add_mesh_part(*mesh_part);
        }
      }

      // our inflow BC function
      Analytic::StaticWrapperFunction<2, VeloFuncX> inflow_func;

      // finally, assemble the filters
      unit_asm.assemble(fil_loc_v.template at<0>(), this->space_velo, inflow_func);
      unit_asm.assemble(fil_loc_v.template at<1>(), this->space_velo);
    }

    template<typename SystemLevel_>
    void assemble_pressure_filter(SystemLevel_& /*sys_level*/)
    {
      // nothing to do
    }

    template<typename SystemLevel_>
    void assemble_system_filter(SystemLevel_& sys_level)
    {
      assemble_velocity_filter(sys_level);
      assemble_pressure_filter(sys_level);

      // clone into system filter
      (*sys_level.filter_sys).template at<0>() = (*sys_level.filter_velo).clone(LAFEM::CloneMode::Shallow);
      (*sys_level.filter_sys).template at<1>() = (*sys_level.filter_pres).clone(LAFEM::CloneMode::Shallow);
    }

    template<typename SystemLevel_>
    typename SystemLevel_::GlobalSystemVector assemble_rhs_vector(SystemLevel_& sys_level)
    {
      // create new vector, format and filter it
      typename SystemLevel_::GlobalSystemVector vec_rhs = sys_level.matrix_sys.create_vector_r();
      vec_rhs.format();
      sys_level.filter_sys.filter_rhs(vec_rhs);
      return vec_rhs;
    }

    template<typename SystemLevel_>
    typename SystemLevel_::GlobalSystemVector assemble_sol_vector(SystemLevel_& sys_level)
    {
      typename SystemLevel_::GlobalSystemVector vec_sol = sys_level.matrix_sys.create_vector_r();
      vec_sol.format();
      sys_level.filter_sys.filter_sol(vec_sol);
      return vec_sol;
    }

    template<typename SystemLevel_>
    void analyse_sol_vector(bool plot, SystemLevel_& sys_level, const typename SystemLevel_::GlobalSystemVector& vec_sol)
    {
      typedef typename SystemLevel_::DataType DataType;

      // define reference solution functions
      Analytic::StaticWrapperFunction<2, VeloFuncX, true, true> velo_x_func;
      Analytic::StaticWrapperFunction<2, VeloFuncY, true, true> velo_y_func;
      Analytic::StaticWrapperFunction<2, PresFunc> pres_func;

      // fetch our vector components
      const auto& vx = (*vec_sol).template at<0>().template at<0>();
      const auto& vy = (*vec_sol).template at<0>().template at<1>();
      const auto& vp = (*vec_sol).template at<1>();

      // compute local errors
      Assembly::ScalarErrorInfo<DataType> vxerr = Assembly::ScalarErrorComputer<1>::compute(
        vx, velo_x_func, this->space_velo, this->cubature);
      Assembly::ScalarErrorInfo<DataType> vyerr = Assembly::ScalarErrorComputer<1>::compute(
        vy, velo_y_func, this->space_velo, this->cubature);
      Assembly::ScalarErrorInfo<DataType> vperr = Assembly::ScalarErrorComputer<0>::compute(
        vp, pres_func, this->space_pres, this->cubature);

      // synhronise all local errors
      vxerr.norm_h0 = sys_level.gate_sys.norm2(vxerr.norm_h0);
      vyerr.norm_h0 = sys_level.gate_sys.norm2(vyerr.norm_h0);
      vxerr.norm_h1 = sys_level.gate_sys.norm2(vxerr.norm_h1);
      vyerr.norm_h1 = sys_level.gate_sys.norm2(vyerr.norm_h1);
      vperr.norm_h0 = sys_level.gate_sys.norm2(vperr.norm_h0);

      // compute field errors
      DataType vv_h0 = Math::sqrt(Math::sqr(vxerr.norm_h0) + Math::sqr(vyerr.norm_h0));
      DataType vv_h1 = Math::sqrt(Math::sqr(vxerr.norm_h1) + Math::sqr(vyerr.norm_h1));

      // print errors
      if (plot)
      {
        std::cout << "Velocity H0-Error: " << stringify_fp_sci(vv_h0, 12) << " [ ";
        std::cout << stringify_fp_sci(vxerr.norm_h0, 12) << " , " << stringify_fp_sci(vyerr.norm_h0, 12) << " ]" << std::endl;
        std::cout << "Velocity H1-Error: " << stringify_fp_sci(vv_h1, 12) << " [ ";
        std::cout << stringify_fp_sci(vxerr.norm_h1, 12) << " , " << stringify_fp_sci(vyerr.norm_h1, 12) << " ]" << std::endl;
        std::cout << "Pressure H0-Error: " << stringify_fp_sci(vperr.norm_h0, 12) << std::endl;
      }
    }
  }; // class StokesUnitSquarePoiseuilleAssemblerLevel<...>


  template<typename MeshType_>
  void run(const Dist::Comm& comm, SimpleArgParser& args, Control::Domain::DomainControl<MeshType_>& domain)
  {
    // define our mesh type
    typedef MeshType_ MeshType;
    typedef typename MeshType::ShapeType ShapeType;
    static constexpr int dim = ShapeType::dimension;

    // define our arch types
    typedef Mem::Main MemType;
    typedef double DataType;
    typedef Index IndexType;

    // define our domain type
    typedef Control::Domain::DomainControl<MeshType_> DomainControlType;

    // define our system level
    typedef Control::StokesUnitVeloNonePresSystemLevel<dim, MemType, DataType, IndexType> SystemLevelType;

    // define our trafo and FE spaces
    typedef Trafo::Standard::Mapping<MeshType> TrafoType;
    typedef Space::Lagrange2::Element<TrafoType> SpaceVeloType;
    typedef Space::Discontinuous::Element<TrafoType, Space::Discontinuous::Variant::StdPolyP<1>> SpacePresType;

    // define our assembler level
    typedef typename DomainControlType::LevelType DomainLevelType;
    typedef StokesUnitSquarePoiseuilleAssemblerLevel<SpaceVeloType, SpacePresType> AssemblerLevelType;

    // get our domain level and layer
    typedef typename DomainControlType::LayerType DomainLayerType;
    const DomainLayerType& layer = *domain.get_layers().back();
    const std::deque<DomainLevelType*>& domain_levels = domain.get_levels();

    std::deque<SystemLevelType*> system_levels;
    std::deque<AssemblerLevelType*> asm_levels;

    const Index num_levels = Index(domain_levels.size());

    //Lin-Solve phase related typedefs

    // create stokes and system levels
    for(Index i(0); i < num_levels; ++i)
    {
      asm_levels.push_back(new AssemblerLevelType(*domain_levels.at(i)));
      system_levels.push_back(new SystemLevelType());
    }

    /* ***************************************************************************************** */

    TimeStamp stamp_ass;

    comm.print("Creating gates...");

    for(Index i(0); i < num_levels; ++i)
    {
      asm_levels.at(i)->assemble_gates(layer, *system_levels.at(i));
    }

    /* ***************************************************************************************** */

    comm.print("Assembling system matrices...");

    for(Index i(0); i < num_levels; ++i)
    {
      asm_levels.at(i)->assemble_system_matrix(*system_levels.at(i));
    }

    // assemble Schur-matrix on finest level
    asm_levels.back()->assemble_schur_matrix(*system_levels.back());

    /* ***************************************************************************************** */

    comm.print("Assembling system filters...");

    for(Index i(0); i < num_levels; ++i)
    {
      asm_levels.at(i)->assemble_system_filter(*system_levels.at(i));
    }

    /* ***************************************************************************************** */

    comm.print("Assembling transfer operators...");

    for (Index i(1); i < num_levels; ++i)
    {
      asm_levels.at(i)->assemble_system_transfer(*system_levels.at(i), *asm_levels.at(i-1));
    }

    Statistics::toe_assembly = stamp_ass.elapsed_now();

    /* ***************************************************************************************** */

    // get our assembled vector type
    typedef typename SystemLevelType::GlobalSystemVector GlobalSystemVector;

    // fetch our finest levels
    DomainLevelType& the_domain_level = *domain_levels.back();
    SystemLevelType& the_system_level = *system_levels.back();
    AssemblerLevelType& the_asm_level = *asm_levels.back();

    // create our RHS and SOL vectors
    GlobalSystemVector vec_rhs = the_asm_level.assemble_rhs_vector(the_system_level);
    GlobalSystemVector vec_sol = the_asm_level.assemble_sol_vector(the_system_level);

    comm.print("Creating solver tree");
    ////////// MATRIX STOCK
    Solver::MatrixStock<typename SystemLevelType::GlobalMatrixBlockA, typename SystemLevelType::GlobalVeloFilter, typename SystemLevelType::GlobalVeloTransfer> matrix_stock_a;
    for (auto& system_level : system_levels)
    {
      matrix_stock_a.systems.push_back(system_level->matrix_a.clone(LAFEM::CloneMode::Shallow));
      matrix_stock_a.gates_row.push_back(&system_level->gate_velo);
      matrix_stock_a.gates_col.push_back(&system_level->gate_velo);
      matrix_stock_a.filters.push_back(system_level->filter_velo.clone(LAFEM::CloneMode::Shallow));
      matrix_stock_a.muxers.push_back(&system_level->coarse_muxer_velo);
      matrix_stock_a.transfers.push_back(system_level->transfer_velo.clone(LAFEM::CloneMode::Shallow));
    }

    Solver::MatrixStock<typename SystemLevelType::GlobalSchurMatrix, typename SystemLevelType::GlobalPresFilter, typename SystemLevelType::GlobalPresTransfer> matrix_stock_s;
    for (auto& system_level : system_levels)
    {
      matrix_stock_s.systems.push_back(system_level->matrix_s.clone(LAFEM::CloneMode::Shallow));
      matrix_stock_s.gates_row.push_back(&system_level->gate_pres);
      matrix_stock_s.gates_col.push_back(&system_level->gate_pres);
      matrix_stock_s.filters.push_back(system_level->filter_pres.clone(LAFEM::CloneMode::Shallow));
      matrix_stock_s.muxers.push_back(&system_level->coarse_muxer_pres);
      matrix_stock_s.transfers.push_back(system_level->transfer_pres.clone(LAFEM::CloneMode::Shallow));
    }


    /* ***************************************************************************************** */
    /* ***************************************************************************************** */
    /* ***************************************************************************************** */

    // our A/S block solvers
    String solver_ini_name;
    args.parse("solver-ini", solver_ini_name);
    PropertyMap property_map;
    property_map.parse(solver_ini_name, true);
    auto solver_a = Control::SolverFactory::create_scalar_solver(matrix_stock_a, &property_map, "linsolver_a");
    auto solver_s = Control::SolverFactory::create_scalar_solver(matrix_stock_s, &property_map, "linsolver_s");

    matrix_stock_a.hierarchy_init();
    matrix_stock_s.hierarchy_init();

    // create a global Schur-Complement preconditioner
    auto schur = Solver::new_schur_precond(
        the_system_level.matrix_a,
        the_system_level.matrix_b,
        the_system_level.matrix_d,
        the_system_level.filter_velo,
        the_system_level.filter_pres,
        solver_a,
        solver_s,
        Solver::SchurType::full
      );

    // create our solver
    auto solver = Solver::new_pcr(the_system_level.matrix_sys, the_system_level.filter_sys, schur);

    // enable plotting
    solver->set_plot(comm.rank() == 0);

    solver->set_max_iter(1000);

    // initialise
    solver->init();

    Statistics::reset();

    TimeStamp at;

    // solve
    Solver::solve(*solver, vec_sol, vec_rhs, the_system_level.matrix_sys, the_system_level.filter_sys);

    double solver_toe(at.elapsed_now());

    FEAT::Control::Statistics::report(solver_toe, args.check("statistics"), MeshType::ShapeType::dimension,
      system_levels, domain);

    // release solver
    solver->done();
    matrix_stock_a.hierarchy_done();
    matrix_stock_s.hierarchy_done();

    /* ***************************************************************************************** */
    /* ***************************************************************************************** */
    /* ***************************************************************************************** */

    if(args.check("no-err") < 0)
    {
      the_asm_level.analyse_sol_vector(comm.rank() == 0, the_system_level, vec_sol);
    }

    /* ***************************************************************************************** */
    /* ***************************************************************************************** */
    /* ***************************************************************************************** */
    //*
    if(args.check("vtk") >= 0)
    {
      // build VTK name
      String vtk_name = String("./stokes-poiseuille-2d");
      vtk_name += "-lvl" + stringify(the_domain_level.get_level_index());
      vtk_name += "-n" + stringify(comm.size());

      // write VTK file
      the_asm_level.write_vtk(vtk_name, *vec_sol, comm);
    }

    /* ***************************************************************************************** */
    /* ***************************************************************************************** */
    /* ***************************************************************************************** */

    if (args.check("test-iter") >= 0)
    {
      int num_iter = (int)solver->get_num_iter();
      int iter_target;
      args.parse("test-iter", iter_target);
      if (num_iter < iter_target - 1 || num_iter > iter_target + 1)
      {
        comm.print("FAILED");
        throw InternalError(__func__, __FILE__, __LINE__, "iter count deviation! " + stringify(num_iter) + " vs " + stringify(iter_target));
      }
    }

    // clean up
    while(!system_levels.empty())
    {
      delete system_levels.back();
      system_levels.pop_back();
    }

    while(!asm_levels.empty())
    {
      delete asm_levels.back();
      asm_levels.pop_back();
    }
  }

  void main(int argc, char* argv[])
  {
    // create world communicator
    Dist::Comm comm(Dist::Comm::world());

#ifdef FEAT_HAVE_MPI
    comm.print("NUM-PROCS: " + stringify(comm.size()));
#endif

    // create arg parser
    SimpleArgParser args(argc, argv);

    // check command line arguments
    args.support("level");
    args.support("no-err");
    args.support("vtk");
    args.support("statistics");
    args.support("mesh");
    args.support("parti-type");
    args.support("parti-name");
    args.support("parti-rank-elems");
    args.support("solver-ini");
    args.support("test-iter");

    // check for unsupported options
    auto unsupported = args.query_unsupported();
    if (!unsupported.empty())
    {
      // print all unsupported options to cerr
      for (auto it = unsupported.begin(); it != unsupported.end(); ++it)
        comm.print(std::cerr, "ERROR: unknown option '--" + (*it).second + "'");
      // abort
      FEAT::Runtime::abort();
    }

    // define our mesh type
    typedef Shape::Hypercube<2> ShapeType;
    typedef Geometry::ConformalMesh<ShapeType> MeshType;

    int lvl_max = 3;
    int lvl_min = 0;
    args.parse("level", lvl_max, lvl_min);

#ifndef DEBUG
    try
#endif
    {
      TimeStamp stamp1;
      if(args.check("mesh") < 1)
      {
        comm.print(std::cerr, "ERROR: Mandatory option --mesh is missing!");
        FEAT::Runtime::abort();
      }

      // query mesh filename list
      const std::deque<String>& mesh_filenames = args.query("mesh")->second;

      // create our domain control
      Control::Domain::PartiDomainControl<MeshType> domain(comm);

      // let the controller parse its arguments
      if(!domain.parse_args(args))
      {
        FEAT::Runtime::abort();
      }

      // read the base-mesh
      domain.read_mesh(mesh_filenames);

      TimeStamp stamp_partition;

      // try to create the partition
      domain.create_partition();

      Statistics::toe_partition = stamp_partition.elapsed_now();

      comm.print("Creating mesh hierarchy...");

      // create the level hierarchy
      domain.create_hierarchy(lvl_max, lvl_min);

      // plot our levels
      comm.print("LVL-MIN: " + stringify(domain.get_levels().front()->get_level_index()) + " [" + stringify(lvl_min) + "]");
      comm.print("LVL-MAX: " + stringify(domain.get_levels().back()->get_level_index()) + " [" + stringify(lvl_max) + "]");

      // run our application
      run<MeshType>(comm, args, domain);

      TimeStamp stamp2;

      // get times
      long long time1 = stamp2.elapsed_micros(stamp1);

      // accumulate times over all processes
      long long time2 = time1 * (long long)comm.size();

      // print time
      comm.print("Run-Time: " + stringify(TimeStamp::format_micros(time1, TimeFormat::m_s_m)) + " [" +
        stringify(TimeStamp::format_micros(time2, TimeFormat::m_s_m)) + "]");
    }
#ifndef DEBUG
    catch (const std::exception& exc)
    {
      std::cerr << "ERROR: unhandled exception: " << exc.what() << std::endl;
      FEAT::Runtime::abort();
    }
    catch (...)
    {
      std::cerr << "ERROR: unknown exception" << std::endl;
      FEAT::Runtime::abort();
    }
#endif // DEBUG
  }

} // namespace StokesPoiseuille2D

int main(int argc, char* argv[])
{
  FEAT::Runtime::initialise(argc, argv);
  StokesPoiseuille2D::main(argc, argv);
  return FEAT::Runtime::finalise();
}
