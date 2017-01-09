#include <kernel/util/runtime.hpp>
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/mesh_node.hpp>
#include <kernel/geometry/export_vtk.hpp>
#include <kernel/trafo/standard/mapping.hpp>
#include <kernel/space/lagrange1/element.hpp>
#include <kernel/space/lagrange2/element.hpp>
#include <kernel/analytic/static_wrapper.hpp>
#include <kernel/assembly/interpolator.hpp>
#include <kernel/assembly/common_functionals.hpp>
#include <kernel/assembly/linear_functional_assembler.hpp>

#include <control/domain/unit_cube_domain_control.hpp>
#include <control/scalar_basic.hpp>

using namespace FEAT;

namespace HierarchTransferTestApp
{
  template<typename T_>
  struct MyTestFunc
  {
    static T_ eval (T_ x, T_ y)
    {
      //return x+y;
      return Math::sqr(x - 0.5) - Math::sqr(y - 0.5);
    }
  };

  typedef Mem::Main MemType;
  typedef Real DataType;
  typedef Index IndexType;

  typedef Shape::Quadrilateral ShapeType;
  typedef Geometry::ConformalMesh<ShapeType> MeshType;
  typedef Trafo::Standard::Mapping<MeshType> TrafoType;
  //typedef Space::Lagrange1::Element<TrafoType> SpaceType;
  typedef Space::Lagrange2::Element<TrafoType> SpaceType;

  typedef Control::Domain::SimpleDomainLevel<MeshType, TrafoType, SpaceType> DomainLevelType;
  typedef Control::Domain::HierarchUnitCubeDomainControl<DomainLevelType> DomainControlType;

  typedef Control::ScalarBasicSystemLevel<> SystemLevelType;

  typedef typename SystemLevelType::GlobalSystemVector GlobalVectorType;

  void test_rest(DomainControlType& domain, std::deque<std::shared_ptr<SystemLevelType>>& system)
  {
    const Dist::Comm& comm = domain.comm();

    comm.print(">>>>> RESTRICTION-TEST <<<<<");

    Analytic::StaticWrapperFunction<2, MyTestFunc> test_func;
    Assembly::Common::ForceFunctional<decltype(test_func)> force(test_func);
    Cubature::DynamicFactory cubature("auto-degree:5");

    // assemble global vector on finest level
    GlobalVectorType vec_fine(&system.front()->gate_sys, domain.front()->space.get_num_dofs());
    vec_fine.format();
    Assembly::LinearFunctionalAssembler::assemble_vector(vec_fine.local(), force, domain.front()->space, cubature);
    vec_fine.sync_0();

    String msg;
    DataType derr = DataType(0);

    for(std::size_t i(0); i < system.size(); ++i)
    {
      // no more system levels?
      if((i+1) >= system.size())
      {
        // do we have another virtual domain level?
        if((i+1) < domain.size_virtual())
        {
          // send restriction to parent
          XASSERT(domain.back().is_child());
          system.back()->transfer_sys.rest_send(vec_fine);
        }
        // exit loop
        break;
      }

      auto& sys_lvl_f = *system.at(i);
      auto& sys_lvl_c = *system.at(i+1);
      auto& dom_lvl_c = *domain.at(i+1);

      // create coarse vectors
      GlobalVectorType vec_crs(&sys_lvl_c.gate_sys, dom_lvl_c.space.get_num_dofs());
      GlobalVectorType vec_rst(&sys_lvl_c.gate_sys, dom_lvl_c.space.get_num_dofs());

      // restrict fine vector
      sys_lvl_f.transfer_sys.rest(vec_fine, vec_rst);

      /*
      {
        const auto& vdl = domain.at(i+1);
        String name = "rest." + stringify(vdl->get_level_index());
        Geometry::ExportVTK<MeshType> vtk(vdl->get_mesh());
        vtk.add_vertex_scalar("test", vec_rst.local().elements());
        vtk.write(name, vdl.layer().comm());
      }
      */

      // assemble coarse vector
      vec_crs.format();
      Assembly::LinearFunctionalAssembler::assemble_vector(vec_crs.local(), force, dom_lvl_c.space, cubature);
      vec_crs.sync_0();

      // compute difference norm
      GlobalVectorType vec_err = vec_crs.clone();
      vec_err.axpy(vec_rst, vec_crs, -DataType(1));
      DataType de = vec_err.norm2();
      derr += de;
      msg += " | " + stringify_fp_sci(de, 3);

      // replace fine by coarse
      vec_fine = std::move(vec_crs);
    }

    comm.barrier();
    comm.allprint(msg);

    // compute final error
    derr /= DataType(system.size());
    DataType derr_total;
    comm.allreduce(&derr, &derr_total, std::size_t(1), Dist::op_max);

    comm.print(String("\nTotal Error: ") + stringify_fp_sci(derr_total) + "\n");

    if(derr_total < Math::pow(Math::eps<DataType>(), DataType(0.9)))
      comm.print("RESTRICTION-TEST: PASSED");
    else
      comm.print("RESTRICTION-TEST: FAILED");
  }

  void test_prol(DomainControlType& domain, std::deque<std::shared_ptr<SystemLevelType>>& system)
  {
    const Dist::Comm& comm = domain.comm();

    comm.print(">>>>> PROLONGATION-TEST <<<<<");

    Analytic::StaticWrapperFunction<2, MyTestFunc> test_func;

    String msg;
    DataType derr = DataType(0);

    for(std::size_t i(0); i < system.size(); ++i)
    {
      auto& sys_lvl_f = *system.at(i);
      auto& dom_lvl_f = *domain.at(i);

      GlobalVectorType vec_fine(&sys_lvl_f.gate_sys, dom_lvl_f.space.get_num_dofs());
      GlobalVectorType vec_prol(&sys_lvl_f.gate_sys, dom_lvl_f.space.get_num_dofs());

      Assembly::Interpolator::project(vec_fine.local(), test_func, dom_lvl_f.space);

      // parent?
      if((i+1) < system.size())
      {
        auto& sys_lvl_c = *system.at(i+1);
        auto& dom_lvl_c = *domain.at(i+1);
        GlobalVectorType vec_crs(&sys_lvl_c.gate_sys, dom_lvl_c.space.get_num_dofs());
        Assembly::Interpolator::project(vec_crs.local(), test_func, dom_lvl_c.space);

        sys_lvl_f.transfer_sys.prol(vec_prol, vec_crs);
      }
      else if((i+1) < domain.size_virtual())
      {
        sys_lvl_f.transfer_sys.prol_recv(vec_prol);
      }
      else
        break;

      /*
      {
        const auto& vdl = domain.at(i);
        String name = "prol." + stringify(vdl->get_level_index());
        Geometry::ExportVTK<MeshType> vtk(vdl->get_mesh());
        vtk.add_vertex_scalar("test", vec_prol.local().elements());
        vtk.write(name, vdl.layer().comm());
      }
      */

      // compute difference norm
      GlobalVectorType vec_err = vec_fine.clone();
      vec_err.axpy(vec_prol, vec_fine, -DataType(1));
      DataType de = vec_err.norm2();
      derr += de;
      msg += " | " + stringify_fp_sci(de, 3);
    }

    comm.barrier();
    comm.allprint(msg);

    // compute final error
    derr /= DataType(system.size());
    DataType derr_total;
    comm.allreduce(&derr, &derr_total, std::size_t(1), Dist::op_max);

    comm.print(String("\nTotal Error: ") + stringify_fp_sci(derr_total) + "\n");

    if(derr_total < Math::pow(Math::eps<DataType>(), DataType(0.8)))
      comm.print("PROLONGATION-TEST: PASSED");
    else
      comm.print("PROLONGATION-TEST: FAILED");
  }

  void run()
  {
    Dist::Comm comm = Dist::Comm::world();

    const int nprocs = comm.size();

    std::deque<int> lvls;
    lvls.push_back(0);
    comm.print("Levels:");
    for(int l(1), k(1); k <= nprocs; ++l, k *= 4)
    {
      lvls.push_front(2*l);
      comm.print(
        stringify(k).pad_front(3) + ": " +
        stringify(lvls.at(std::size_t(1))).pad_front(2) + " > " +
        stringify(lvls.front()).pad_front(2));
    }

    DomainControlType domain(comm, lvls);

    comm.print("\nLayers:");
    domain.dump_layers();
    comm.print("\nLayer-Levels:");
    domain.dump_layer_levels();
    comm.print("\nVirtual Levels:");
    domain.dump_virt_levels();

    Cubature::DynamicFactory cubature("auto-degree:5");

    std::deque<std::shared_ptr<SystemLevelType>> system;

    for(std::size_t i(0); i < domain.size_physical(); ++i)
    {
      system.push_back(std::make_shared<SystemLevelType>());
      system.at(i)->assemble_gate(domain.at(i));
      if((i+1) < domain.size_virtual())
      {
        system.at(i)->assemble_coarse_muxer(domain.at(i+1));
        system.at(i)->assemble_transfer(domain.at(i), domain.at(i+1), cubature);
      }
    }

    comm.print(String(60, '='));

    test_rest(domain, system);

    comm.print(String(60, '='));

    test_prol(domain, system);

    comm.print(String(60, '='));
  }
} // namespace HierarchTransferTestApp

int main(int argc, char** argv)
{
  Runtime::initialise(argc, argv);
  HierarchTransferTestApp::run();
  return Runtime::finalise();
}