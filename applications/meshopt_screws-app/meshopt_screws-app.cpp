#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>

#include <kernel/geometry/conformal_factories.hpp>
#include <kernel/geometry/export_vtk.hpp>
#include <kernel/geometry/mesh_atlas.hpp>
#include <kernel/geometry/mesh_file_reader.hpp>
#include <kernel/geometry/mesh_quality_heuristic.hpp>
#include <kernel/geometry/mesh_extruder.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/util/mpi_cout.hpp>
#include <kernel/util/runtime.hpp>
#include <kernel/util/simple_arg_parser.hpp>

#include <control/domain/partitioner_domain_control.hpp>
#include <control/meshopt/meshopt_control.hpp>
#include <control/meshopt/meshopt_control_factory.hpp>

using namespace FEAT;

static void display_help();
static void read_test_mode_application_config(std::stringstream&);
static void read_test_mode_meshopt_config(std::stringstream&);
static void read_test_mode_solver_config(std::stringstream&);
static void read_test_mode_mesh(std::stringstream&);
static void read_test_mode_chart(std::stringstream&);

template<typename Mesh_>
struct MeshExtrudeHelper
{
  typedef Mesh_ MeshType;
  typedef Mesh_ ExtrudedMeshType;
  typedef typename MeshType::CoordType CoordType;

  Geometry::RootMeshNode<MeshType>* extruded_mesh_node;

  explicit MeshExtrudeHelper(Geometry::RootMeshNode<MeshType>* DOXY(rmn), Index DOXY(slices), CoordType DOXY(z_min), CoordType DOXY(z_max), const String& DOXY(z_min_part_name), const String& DOXY(z_max_part_name)) :
    extruded_mesh_node(nullptr)
    {
    }

  ~MeshExtrudeHelper()
  {
  }

  void extrude_vertex_set(const typename MeshType::VertexSetType& DOXY(vtx))
  {
  }

};

template<typename Coord_>
struct MeshExtrudeHelper<Geometry::ConformalMesh<Shape::Hypercube<2>,2,2,Coord_>>
{
  typedef Geometry::ConformalMesh<Shape::Hypercube<2>,2,2,Coord_> MeshType;
  typedef Coord_ CoordType;
  typedef Geometry::ConformalMesh<Shape::Hypercube<3>,3,3,Coord_> ExtrudedMeshType;
  typedef Geometry::MeshAtlas<ExtrudedMeshType> ExtrudedAtlasType;

  Geometry::MeshExtruder<MeshType> mesh_extruder;
  ExtrudedAtlasType* extruded_atlas;
  Geometry::RootMeshNode<ExtrudedMeshType>* extruded_mesh_node;

  explicit MeshExtrudeHelper(Geometry::RootMeshNode<MeshType>* rmn, Index slices, CoordType z_min, CoordType z_max, const String& z_min_part_name, const String& z_max_part_name) :
    mesh_extruder(slices, z_min, z_max, z_min_part_name, z_max_part_name),
    extruded_atlas(new ExtrudedAtlasType),
    extruded_mesh_node(new Geometry::RootMeshNode<ExtrudedMeshType>(nullptr, extruded_atlas))
  {
    mesh_extruder.extrude_atlas(*extruded_atlas, *(rmn->get_atlas()));
    mesh_extruder.extrude_root_node(*extruded_mesh_node, *rmn, extruded_atlas);
  }

  ~MeshExtrudeHelper()
  {
    if(extruded_atlas != nullptr)
      delete extruded_atlas;
    if(extruded_mesh_node != nullptr)
      delete extruded_mesh_node;
  }

  void extrude_vertex_set(const typename MeshType::VertexSetType& vtx)
  {
    mesh_extruder.extrude_vertex_set(extruded_mesh_node->get_mesh()->get_vertex_set(), vtx);
  }
};

template<typename Mem_, typename DT_, typename IT_, typename Mesh_>
struct MeshoptScrewsApp
{
  /// The memory architecture. Although this looks freely chosable, it has to be Mem::Main for now because all the
  /// Hyperelasticity functionals are implemented for Mem::Main only
  typedef Mem_ MemType;
  /// The floating point type
  typedef DT_ DataType;
  /// The index type
  typedef IT_ IndexType;
  /// The type of mesh to use
  typedef Mesh_ MeshType;
  /// The shape type of the mesh's cells
  typedef typename Mesh_::ShapeType ShapeType;

  /// The only transformation available is the standard P1 or Q1 transformation
  typedef Trafo::Standard::Mapping<Mesh_> TrafoType;

  /// This is how far the inner screw's centre deviates from the outer screw's
  static constexpr DataType excentricity_inner = DataType(0.2833);
  /// Type for points in the mesh
  typedef Tiny::Vector<DataType, MeshType::world_dim> ImgPointType;

#ifdef FEAT_HAVE_PARMETIS
  // If we have ParMETIS, we can use it for partitioning
  typedef Control::Domain::PartitionerDomainControl<
    Foundation::PExecutorParmetis<Foundation::ParmetisModePartKway>, Mesh_> DomCtrl;
#else
#ifdef FEAT_HAVE_MPI
  // Otherwise we have to use the fallback partitioner
  typedef Control::Domain::PartitionerDomainControl<Foundation::PExecutorFallback<DT_, IT_>, Mesh_> DomCtrl;
#else
  // If we are in serial mode, there is no partitioning
  typedef Control::Domain::PartitionerDomainControl<Foundation::PExecutorNONE<DT_, IT_>, Mesh_> DomCtrl;
#endif // FEAT_HAVE_MPI
#endif // FEAT_HAVE_PARMETIS

  typedef typename MeshExtrudeHelper<MeshType>::ExtrudedMeshType ExtrudedMeshType;

  /**
   * \brief Returns a descriptive string
   *
   * \returns The class name as string
   */
  static String name()
  {
    return "MeshoptScrewsApp";
  }

  /**
   * \brief The routine that does the actual work
   */
  static int run(const String& meshopt_section_key, PropertyMap* meshopt_config, PropertyMap* solver_config,
  Geometry::MeshFileReader* mesh_file_reader, Geometry::MeshFileReader* chart_file_reader,
  int lvl_max, int lvl_min, const DataType delta_t, const DataType t_end,
  const bool write_vtk, const bool test_mode)
  {
    XASSERT(delta_t > DataType(0));
    XASSERT(t_end >= DataType(0));

    TimeStamp at;

    // Minimum number of cells we want to have in each patch
    const Index part_min_elems(Util::Comm::size()*4);

    DomCtrl dom_ctrl(lvl_max, lvl_min, part_min_elems, mesh_file_reader, chart_file_reader);

    Index ncells(dom_ctrl.get_levels().back()->get_mesh().get_num_entities(MeshType::shape_dim));
#ifdef FEAT_HAVE_MPI
    Index my_cells(ncells);
    Util::Comm::allreduce(&my_cells, Index(1), &ncells, MPI_SUM);
#endif

    MeshExtrudeHelper<MeshType> extruder(dom_ctrl.get_levels().back()->get_mesh_node(),
    Index(10*(lvl_max+1)), DataType(0), DataType(1), "bottom", "top");

    // Print level information
    if(Util::Comm::rank() == 0)
    {
      std::cout << name() << "settings: " << std::endl;
      std::cout << "Timestep size: " << stringify_fp_fix(delta_t) << ", end time: " <<
        stringify_fp_fix(t_end) << std::endl;
      std::cout << "LVL-MAX: " <<
        dom_ctrl.get_levels().back()->get_level_index() << " [" << lvl_max << "]";
      std::cout << " LVL-MIN: " <<
        dom_ctrl.get_levels().front()->get_level_index() << " [" << lvl_min << "]" << std::endl;
      std::cout << "Cells: " << ncells << std::endl;
    }

    // Create MeshoptControl
    std::shared_ptr<Control::Meshopt::MeshoptControlBase<DomCtrl, TrafoType>> meshopt_ctrl(nullptr);
    meshopt_ctrl = Control::Meshopt::ControlFactory<Mem_, DT_, IT_, TrafoType>::create_meshopt_control(
      dom_ctrl, meshopt_section_key, meshopt_config, solver_config);

    String file_basename(name()+"_n"+stringify(Util::Comm::size()));

    // This is the centre reference point
    ImgPointType x_0(DataType(0));

    // Get inner boundary MeshPart. Can be nullptr if this process' patch does not lie on that boundary
    auto* inner_boundary = dom_ctrl.get_levels().back()->get_mesh_node()->find_mesh_part("inner");
    Geometry::TargetSet* inner_indices(nullptr);
    if(inner_boundary != nullptr)
      inner_indices = &(inner_boundary->template get_target_set<0>());

    // This is the centre point of the rotation of the inner screw
    ImgPointType x_inner(DataType(0));
    x_inner.v[0] = -excentricity_inner;
    const String inner_str(dom_ctrl.get_atlas()->find_mesh_chart("inner")->get_type());

    // Get outer boundary MeshPart. Can be nullptr if this process' patch does not lie on that boundary
    auto* outer_boundary = dom_ctrl.get_levels().back()->get_mesh_node()->find_mesh_part("outer");
    Geometry::TargetSet* outer_indices(nullptr);
    if(outer_boundary != nullptr)
      outer_indices = &(outer_boundary->template get_target_set<0>());

    // This is the centre point of the rotation of the outer screw
    ImgPointType x_outer(DataType(0));
    const String outer_str(dom_ctrl.get_atlas()->find_mesh_chart("outer")->get_type());

    // For test_mode = true
    DT_ min_quality(0);
    DT_ min_angle(0);
    {
      int deque_position(0);
      for(auto it = dom_ctrl.get_levels().begin(); it !=  dom_ctrl.get_levels().end(); ++it)
      {
        int lvl_index((*it)->get_level_index());

        // Write initial vtk output
        if(write_vtk)
        {
          String vtk_name = String(file_basename+"_pre_lvl_"+stringify(lvl_index));
          if(Util::Comm::rank() == 0)
            std::cout << "Writing " << vtk_name << std::endl;

          // Create a VTK exporter for our mesh
          Geometry::ExportVTK<MeshType> exporter(((*it)->get_mesh()));
          meshopt_ctrl->add_to_vtk_exporter(exporter, deque_position);
          exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
        }

        min_quality = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::compute(
          (*it)->get_mesh().template get_index_set<MeshType::shape_dim, 0>(), (*it)->get_mesh().get_vertex_set());

        min_angle = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::angle(
          (*it)->get_mesh().template get_index_set<MeshType::shape_dim, 0>(), (*it)->get_mesh().get_vertex_set());

#ifdef FEAT_HAVE_MPI
        DT_ min_quality_snd(min_quality);
        DT_ min_angle_snd(min_angle);

        Util::Comm::allreduce(&min_quality_snd, Index(1), &min_quality, MPI_MIN);
        Util::Comm::allreduce(&min_angle_snd, Index(1), &min_angle, MPI_MIN);
#endif
        if(Util::Comm::rank() == 0)
          std::cout << "Pre: Level " << lvl_index << ": Quality indicator " << " " <<
            stringify_fp_sci(min_quality) << ", minimum angle " << stringify_fp_fix(min_angle) << std::endl;

        ++deque_position;
      }

      if(write_vtk && extruder.extruded_mesh_node != nullptr)
      {
        // Create a VTK exporter for our mesh
        String vtk_name = String(file_basename+"_pre_extruded");

        if(Util::Comm::rank() == 0)
          std::cout << "Writing " << vtk_name << std::endl;

        Geometry::ExportVTK<ExtrudedMeshType> exporter(*(extruder.extruded_mesh_node->get_mesh()));
        exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
      }


      // Write Polyline charts if we have them
      if(Util::Comm::rank()==0)
      {
        if(inner_str == "polyline")
        {
          typedef Geometry::ConformalMesh<Shape::Hypercube<1>,2,2,DataType> PolylineMesh;

          auto* inner_chart = reinterpret_cast< Geometry::Atlas::Polyline<MeshType>*>
            (dom_ctrl.get_atlas()->find_mesh_chart("inner"));

          Geometry::PolylineFactory<2,2, DataType> pl_factory(inner_chart->get_world_points());
          PolylineMesh polyline(pl_factory);

          Geometry::ExportVTK<PolylineMesh> polyline_writer(polyline);
          polyline_writer.write(file_basename+"_inner");

        }

        if(outer_str == "polyline")
        {
          typedef Geometry::ConformalMesh<Shape::Hypercube<1>,2,2,DataType> PolylineMesh;

          auto* outer_chart = reinterpret_cast< Geometry::Atlas::Polyline<MeshType>*>
            (dom_ctrl.get_atlas()->find_mesh_chart("outer"));

          Geometry::PolylineFactory<2,2, DataType> pl_factory(outer_chart->get_world_points());
          PolylineMesh polyline(pl_factory);

          Geometry::ExportVTK<PolylineMesh> polyline_writer(polyline);
          polyline_writer.write(file_basename+"_outer");
        }
      }
    }

    // Check for the hard coded settings for test mode
    if(test_mode)
    {
      if( min_angle < DT_(10))
      {
        Util::mpi_cout("FAILED:");
        throw InternalError(__func__,__FILE__,__LINE__,
        "Initial min angle should be >= "+stringify_fp_fix(8)+ " but is "+stringify_fp_fix(min_angle));
      }
    }

    // Copy the vertex coordinates to the buffer and get them via get_coords()
    meshopt_ctrl->mesh_to_buffer();
    // A copy of the old vertex coordinates is kept here
    auto old_coords = meshopt_ctrl->get_coords().clone(LAFEM::CloneMode::Deep);
    auto new_coords = meshopt_ctrl->get_coords().clone(LAFEM::CloneMode::Deep);

    // Prepare the functional
    meshopt_ctrl->prepare(old_coords);
    // Optimise the mesh
    meshopt_ctrl->optimise();

    // Write output again
    {
      int deque_position(0);
      for(auto it = dom_ctrl.get_levels().begin(); it !=  dom_ctrl.get_levels().end(); ++it)
      {
        int lvl_index((*it)->get_level_index());

        if(write_vtk)
        {
          String vtk_name = String(file_basename+"_post_lvl_"+stringify(lvl_index));

          if(Util::Comm::rank() == 0)
            std::cout << "Writing " << vtk_name << std::endl;

          // Create a VTK exporter for our mesh
          Geometry::ExportVTK<MeshType> exporter(((*it)->get_mesh()));
          meshopt_ctrl->add_to_vtk_exporter(exporter, deque_position);
          exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
        }

        min_quality = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::compute(
          (*it)->get_mesh().template get_index_set<MeshType::shape_dim, 0>(), (*it)->get_mesh().get_vertex_set());

        min_angle = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::angle(
          (*it)->get_mesh().template get_index_set<MeshType::shape_dim, 0>(), (*it)->get_mesh().get_vertex_set());

#ifdef FEAT_HAVE_MPI
        DT_ min_quality_snd(min_quality);
        DT_ min_angle_snd(min_angle);

        Util::Comm::allreduce(&min_quality_snd, Index(1), &min_quality, MPI_MIN);
        Util::Comm::allreduce(&min_angle_snd, Index(1), &min_angle, MPI_MIN);
#endif
        if(Util::Comm::rank() == 0)
          std::cout << "Post: Level " << lvl_index << ": Quality indicator " << " " <<
            stringify_fp_sci(min_quality) << ", minimum angle " << stringify_fp_fix(min_angle) << std::endl;

        ++deque_position;
      }
    }

    if(write_vtk && extruder.extruded_mesh_node != nullptr)
    {
      // Compute mesh quality and worst angle
      const auto& finest_mesh = dom_ctrl.get_levels().back()->get_mesh();
      extruder.extrude_vertex_set(finest_mesh.get_vertex_set());

      // Create a VTK exporter for our mesh
      String vtk_name = String(file_basename+"_post_extruded");

      if(Util::Comm::rank() == 0)
        std::cout << "Writing " << vtk_name << std::endl;

      Geometry::ExportVTK<ExtrudedMeshType> exporter(*(extruder.extruded_mesh_node->get_mesh()));
      exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
    }


    // Check for the hard coded settings for test mode
    if(test_mode)
    {
      if( min_angle < DT_(10))
      {
        Util::mpi_cout("FAILED:");
        throw InternalError(__func__,__FILE__,__LINE__,
        "Post Initial min angle should be >= "+stringify_fp_fix(8)+ " but is "+stringify_fp_fix(min_angle));
      }
    }

    // Initial time
    DataType time(0);
    // Counter for timesteps
    Index n(0);
    // This is the absolute turning angle of the screws
    DataType alpha(0);
    // Need some pi for all the angles
    DataType pi(Math::pi<DataType>());

    // The mesh velocity is 1/delta_t*(coords_new - coords_old) and computed in each time step
    auto mesh_velocity = meshopt_ctrl->get_coords().clone();

    while(time < t_end)
    {
      n++;
      time+= delta_t;

      DataType alpha_old = alpha;
      alpha = -DataType(2)*pi*time;
      DataType delta_alpha = alpha - alpha_old;


      if(Util::Comm::rank() == 0)
        std::cout << "Timestep " << n << ": t = " << stringify_fp_fix(time) <<
          ", angle = " << stringify_fp_fix(alpha/(DataType(2)*pi)*DataType(360))  << " degrees" <<std::endl;

      // Save old vertex coordinates
      meshopt_ctrl->mesh_to_buffer();
      old_coords.clone(meshopt_ctrl->get_coords());

      // Get coords for modification
      auto& coords = (meshopt_ctrl->get_coords());
      auto& coords_loc = *coords;

      // Update boundary of the inner screw
      // This is the 2x2 matrix representing the turning by the angle delta_alpha of the inner screw
      Tiny::Matrix<DataType, 2, 2> rot(DataType(0));

      rot(0,0) = Math::cos(delta_alpha*DataType(7)/DataType(6));
      rot(0,1) = - Math::sin(delta_alpha*DataType(7)/DataType(6));
      rot(1,0) = -rot(0,1);
      rot(1,1) = rot(0,0);

      ImgPointType tmp(DataType(0));
      ImgPointType tmp2(DataType(0));

      if(inner_indices != nullptr)
      {
        for(Index i(0); i < inner_indices->get_num_entities(); ++i)
        {
          // Index of boundary vertex i in the mesh
          Index j(inner_indices->operator[](i));
          // Translate the point to the centre of rotation
          tmp = coords_loc(j) - x_inner;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          // Translate the point by the new centre of rotation
          coords_loc(j, x_inner + tmp2);
        }
      }

      // Rotate the chart. This has to use an evil downcast for now
      if(inner_str == "polyline")
      {
        auto* inner_chart = reinterpret_cast< Geometry::Atlas::Polyline<MeshType>*>
          (dom_ctrl.get_atlas()->find_mesh_chart("inner"));

        auto& vtx_inner = inner_chart->get_world_points();

        for(auto& it : vtx_inner)
        {
          tmp = it - x_inner;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          // Translate the point by the new centre of rotation
          it = x_inner + tmp2;
        }
      }
      else if(inner_str == "bezier")
      {
        auto* inner_chart = reinterpret_cast< Geometry::Atlas::Bezier<MeshType>*>
          (dom_ctrl.get_atlas()->find_mesh_chart("inner"));

        auto& vtx_inner = inner_chart->get_world_points();

        for(auto& it : vtx_inner)
        {
          tmp = it - x_inner;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          // Translate the point by the new centre of rotation
          it = x_inner + tmp2;
        }

        auto& vtx_control = inner_chart->get_control_points();

        for(auto& it : vtx_control)
        {
          tmp = it - x_inner;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          // Translate the point by the new centre of rotation
          it = x_inner + tmp2;
        }
      }
      else
        throw InternalError(__func__,__FILE__,__LINE__,"Unhandled inner chart type string "+inner_str);

      // The outer screw has 7 teeth as opposed to the inner screw with 6, and it rotates at 6/7 of the speed
      rot(0,0) = Math::cos(delta_alpha);
      rot(0,1) = - Math::sin(delta_alpha);
      rot(1,0) = -rot(0,1);
      rot(1,1) = rot(0,0);

      // The outer screw rotates centrically, so x_outer remains the same at all times
      if(outer_indices != nullptr)
      {
        for(Index i(0); i < outer_indices->get_num_entities(); ++i)
        {
          // Index of boundary vertex i in the mesh
          Index j(outer_indices->operator[](i));
          tmp = coords_loc(j) - x_outer;

          tmp2.set_vec_mat_mult(tmp, rot);

          coords_loc(j, x_outer+tmp2);
        }
      }

      // Rotate the outer chart. This has to use an evil downcast for now
      if(outer_str == "polyline")
      {
        auto* outer_chart = reinterpret_cast<Geometry::Atlas::Polyline<MeshType>*>
          (dom_ctrl.get_atlas()->find_mesh_chart("outer"));
        auto& vtx_outer = outer_chart->get_world_points();

        for(auto& it :vtx_outer)
        {
          tmp = it - x_outer;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          it = x_outer + tmp2;
        }
      }
      else if(outer_str == "bezier")
      {
        auto* outer_chart = reinterpret_cast<Geometry::Atlas::Bezier<MeshType>*>
          (dom_ctrl.get_atlas()->find_mesh_chart("outer"));
        auto& vtx_outer = outer_chart->get_world_points();

        for(auto& it :vtx_outer)
        {
          tmp = it - x_outer;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          it = x_outer + tmp2;
        }

        auto& vtx_control = outer_chart->get_control_points();

        for(auto& it :vtx_control)
        {
          tmp = it - x_outer;
          // Rotate
          tmp2.set_vec_mat_mult(tmp, rot);
          it = x_outer + tmp2;
        }
      }
      else
        throw InternalError(__func__,__FILE__,__LINE__,"Unhandled outer chart type string "+outer_str);

      // Now prepare the functional
      meshopt_ctrl->prepare(coords);

      meshopt_ctrl->optimise();

      // Compute mesh velocity
      mesh_velocity.axpy(meshopt_ctrl->get_coords(), old_coords, DataType(-1));
      mesh_velocity.scale(mesh_velocity, DataType(1)/delta_t);

      // Compute maximum of the mesh velocity
      DataType max_mesh_velocity(0);
      for(IT_ i(0); i < (*mesh_velocity).size(); ++i)
        max_mesh_velocity = Math::max(max_mesh_velocity, (*mesh_velocity)(i).norm_euclid());

      if(Util::Comm::rank() == 0)
        std::cout << "max. mesh velocity: " << stringify_fp_sci(max_mesh_velocity) << std::endl;

      // Compute mesh quality and worst angle
      const auto& finest_mesh = dom_ctrl.get_levels().back()->get_mesh();

      min_quality = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::compute(
        finest_mesh.template get_index_set<MeshType::shape_dim, 0>(), finest_mesh.get_vertex_set());

      min_angle = Geometry::MeshQualityHeuristic<typename MeshType::ShapeType>::angle(
        finest_mesh.template get_index_set<MeshType::shape_dim, 0>(), finest_mesh.get_vertex_set());

#ifdef FEAT_HAVE_MPI
      DT_ min_quality_snd(min_quality);
      DT_ min_angle_snd(min_angle);

      Util::Comm::allreduce(&min_quality_snd, Index(1), &min_quality, MPI_MIN);
      Util::Comm::allreduce(&min_angle_snd, Index(1), &min_angle, MPI_MIN);
#endif
      if(Util::Comm::rank() == 0)
        std::cout << "Quality indicator " << " " << stringify_fp_sci(min_quality) <<
          ", minimum angle " << stringify_fp_fix(min_angle) << std::endl;

      if(write_vtk)
      {
        String vtk_name(file_basename+"_post_"+stringify(n));

        if(Util::Comm::rank() == 0)
          std::cout << "Writing " << vtk_name << std::endl;

        // Create a VTK exporter for our mesh
        Geometry::ExportVTK<MeshType> exporter(dom_ctrl.get_levels().back()->get_mesh());
        // Add mesh velocity
        exporter.add_vertex_vector("mesh_velocity", *mesh_velocity);
        // Add everything from the MeshoptControl
        meshopt_ctrl->add_to_vtk_exporter(exporter, int(dom_ctrl.get_levels().size())-1);
        // Write the file
        exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
      }

      if(write_vtk && extruder.extruded_mesh_node != nullptr)
      {
        extruder.extrude_vertex_set(finest_mesh.get_vertex_set());
        // Create a VTK exporter for our mesh
        String vtk_name = String(file_basename+"_post_extruded_"+stringify(n));

        if(Util::Comm::rank() == 0)
          std::cout << "Writing " << vtk_name << std::endl;

        Geometry::ExportVTK<ExtrudedMeshType> exporter(*(extruder.extruded_mesh_node->get_mesh()));
        exporter.write(vtk_name, int(Util::Comm::rank()), int(Util::Comm::size()));
      }

      // Check for the hard coded settings for test mode
      if(test_mode)
      {
        if( min_angle < DT_(9.8))
        {
          Util::mpi_cout("FAILED:");
          throw InternalError(__func__,__FILE__,__LINE__,
          "Final min angle should be >= "+stringify_fp_fix(8)+ " but is "+stringify_fp_fix(min_angle));
        }
      }

    } // time loop

    if(Util::Comm::rank() == 0)
    {
      TimeStamp bt;
      std::cout << "Elapsed time: " << bt.elapsed(at) << std::endl;
    }

    return 0;

  }
}; // struct MeshSmootherApp

int main(int argc, char* argv[])
{
  // Even though this *looks* configureable, it is not: All HyperelasticityFunctionals are implemented for Mem::Main
  // only
  typedef Mem::Main MemType;
  // Floating point type
  typedef double DataType;
  // Index type
  typedef Index IndexType;

  // This is the list of all supported meshes that could appear in the mesh file
  typedef Geometry::ConformalMesh<Shape::Simplex<2>, 2, 2, Real> S2M2D;
  typedef Geometry::ConformalMesh<Shape::Hypercube<2>, 2, 2, Real> H2M2D;
  //typedef Geometry::ConformalMesh<Shape::Simplex<2>, 3, 3, Real> S2M3D;
  //typedef Geometry::ConformalMesh<Shape::Simplex<3>, 3, 3, Real> S3M3D;
  //typedef Geometry::ConformalMesh<Shape::Hypercube<1>, 1, 1, Real> H1M1D;
  //typedef Geometry::ConformalMesh<Shape::Hypercube<1>, 2, 2, Real> H1M2D;
  //typedef Geometry::ConformalMesh<Shape::Hypercube<1>, 3, 3, Real> H1M3D;
  //typedef Geometry::ConformalMesh<Shape::Hypercube<2>, 3, 3, Real> H2M3D;
  //typedef Geometry::ConformalMesh<Shape::Hypercube<3>, 3, 3, Real> H3M3D;

  int rank(0);
  int nprocs(0);

  // initialise
  FEAT::Runtime::initialise(argc, argv, rank, nprocs);
#ifdef FEAT_HAVE_MPI
  if (rank == 0)
  {
    std::cout << "NUM-PROCS: " << nprocs << std::endl;
  }
#endif

  // Mininum refinement level, parsed from the application config file
  int lvl_min(-1);
  // Maximum refinement level, parsed from the application config file
  int lvl_max(-1);
  // Timestep size, parsed from the application config file
  DataType delta_t(0);
  // End time, parsed from the application config file
  DataType t_end(0);
  // Filename to read the mesh from, parsed from the application config file
  String mesh_filename("");
  // Filename to read the charts from (if any), parsed from the application config file
  String chart_filename("");
  // String containing the mesh type, read from the header of the mesh file
  String mesh_type("");
  // Do we want to write vtk files. Read from the command line arguments
  bool write_vtk(false);
  // Is the application running as a test? Read from the command line arguments
  bool test_mode(false);

  // Streams for synchronising information read from files
  std::stringstream synchstream_mesh;
  std::stringstream synchstream_chart;
  std::stringstream synchstream_app_config;
  std::stringstream synchstream_meshopt_config;
  std::stringstream synchstream_solver_config;

  // Create a parser for command line arguments.
  SimpleArgParser args(argc, argv);
  args.support("application_config");
  args.support("help");
  args.support("testmode");
  args.support("vtk");

  if( args.check("help") > -1 || args.num_args()==1)
    display_help();

  // Get unsupported command line arguments
  std::deque<std::pair<int,String> > unsupported = args.query_unsupported();
  if( !unsupported.empty() )
  {
    // print all unsupported options to cerr
    for(auto it = unsupported.begin(); it != unsupported.end(); ++it)
      std::cerr << "ERROR: unsupported option '--" << (*it).second << "'" << std::endl;
  }

  if( args.check("testmode") >=0 )
  {
    Util::mpi_cout("Running in test mode, all other command line arguments and configuration files are ignored.\n");
    test_mode = true;
  }


  // Application settings, has to be created here because it gets filled differently according to test_mode
  PropertyMap* application_config = new PropertyMap;

  // If we are not in test mode, parse command line arguments, read files, synchronise streams
  if(! test_mode)
  {
    // Check if we want to write vtk files
    if(args.check("vtk") >= 0 )
      write_vtk = true;

    // Read the application config file on rank 0
    if(Util::Comm::rank() == 0)
    {
      // Input application configuration file name, required
      String application_config_filename("");
      // Check and parse --application_config
      if(args.check("application_config") != 1 )
      {
        std::cout << "You need to specify a application configuration file with --application_config.";
        throw InternalError(__func__, __FILE__, __LINE__, "Invalid option for --application_config");
      }
      else
      {
        args.parse("application_config", application_config_filename);
        std::cout << "Reading application configuration from file " << application_config_filename << std::endl;
        std::ifstream ifs(application_config_filename);
        if(!ifs.good())
          throw FileNotFound(application_config_filename);

        synchstream_app_config << ifs.rdbuf();
      }
    }

#ifdef FEAT_HAVE_MPI
    // If we are in parallel mode, we need to synchronise the stream
    Util::Comm::synch_stringstream(synchstream_app_config);
#endif

    // Parse the application config from the (synchronised) stream
    application_config->parse(synchstream_app_config, true);

    // Get the application settings section
    auto app_settings_section = application_config->query_section("ApplicationSettings");
    XASSERTM(app_settings_section != nullptr,
    "Application config is missing the mandatory ApplicationSettings section!");

    // We read the files only on rank 0. After reading, we synchronise the streams like above.
    if(Util::Comm::rank() == 0)
    {
      // Read the mesh file to stream
      auto mesh_filename_p = app_settings_section->query("mesh_file");
      XASSERTM(mesh_filename_p.second,
      "ApplicationSettings section is missing the mandatory mesh_file entry!");
      {
        mesh_filename = mesh_filename_p.first;
        std::ifstream ifs(mesh_filename);
        if(!ifs.good())
          throw FileNotFound(mesh_filename);

        std::cout << "Reading mesh from file " << mesh_filename << std::endl;
        synchstream_mesh << ifs.rdbuf();
      }

      // Read the chart file to stream
      auto chart_filename_p = app_settings_section->query("chart_file");
      if(chart_filename_p.second)
      {
        chart_filename = chart_filename_p.first;
        std::ifstream ifs(chart_filename);
        if(!ifs.good())
          throw FileNotFound(chart_filename);

        std::cout << "Reading charts from file " << chart_filename << std::endl;
        synchstream_chart << ifs.rdbuf();
      }

      // Read configuration for mesh optimisation to stream
      auto meshopt_config_filename_p = app_settings_section->query("meshopt_config_file");
      XASSERTM(meshopt_config_filename_p.second,
      "ApplicationConfig section is missing the mandatory meshopt_config_file entry!");
      {
        std::ifstream ifs(meshopt_config_filename_p.first);
        if(!ifs.good())
          throw FileNotFound(meshopt_config_filename_p.first);

        std::cout << "Reading mesh optimisation config from file " <<meshopt_config_filename_p.first << std::endl;
        synchstream_meshopt_config << ifs.rdbuf();
      }

      // Read solver configuration to stream
      auto solver_config_filename_p = app_settings_section->query("solver_config_file");
      XASSERTM(solver_config_filename_p.second,
      "ApplicationConfig section is missing the mandatory solver_config_file entry!");
      {
        std::ifstream ifs(solver_config_filename_p.first);
        if(!ifs.good())
          throw FileNotFound(solver_config_filename_p.first);
        else
        {
          std::cout << "Reading solver config from file " << solver_config_filename_p.first << std::endl;
          synchstream_solver_config << ifs.rdbuf();
        }
      }
    } // Util::Comm::rank() == 0

#ifdef FEAT_HAVE_MPI
    // Synchronise all those streams in parallel mode
    Util::Comm::synch_stringstream(synchstream_mesh);
    Util::Comm::synch_stringstream(synchstream_chart);
    Util::Comm::synch_stringstream(synchstream_meshopt_config);
    Util::Comm::synch_stringstream(synchstream_solver_config);
#endif
  }
  // If we are in test mode, all streams are filled by the hard coded stuff below
  else
  {
    read_test_mode_application_config(synchstream_app_config);
    // Parse the application config from the (synchronised) stream
    application_config->parse(synchstream_app_config, true);

    read_test_mode_meshopt_config(synchstream_meshopt_config);
    read_test_mode_solver_config(synchstream_solver_config);
    read_test_mode_mesh(synchstream_mesh);
    read_test_mode_chart(synchstream_chart);
  }

  // Create a MeshFileReader and parse the mesh stream
  Geometry::MeshFileReader* mesh_file_reader(new Geometry::MeshFileReader(synchstream_mesh));
  mesh_file_reader->read_root_markup();

  Geometry::MeshFileReader* chart_file_reader(nullptr);
  if(!synchstream_chart.str().empty())
    chart_file_reader = new Geometry::MeshFileReader(synchstream_chart);

  // Create PropertyMaps and parse the configuration streams
  PropertyMap* meshopt_config = new PropertyMap;
  meshopt_config->parse(synchstream_meshopt_config, true);

  PropertyMap* solver_config = new PropertyMap;
  solver_config->parse(synchstream_solver_config, true);

  // Get the application settings section
  auto app_settings_section = application_config->query_section("ApplicationSettings");
  XASSERTM(app_settings_section != nullptr,
  "Application config is missing the mandatory ApplicationSettings section!");

  // Get the coarse mesh and finest mesh levels from the application settings
  auto lvl_min_p = app_settings_section->query("lvl_min");
  if(!lvl_min_p.second)
    lvl_min = 0;
  else
    lvl_min = std::stoi(lvl_min_p.first);

  auto lvl_max_p = app_settings_section->query("lvl_max");
  if(!lvl_max_p.second)
    lvl_max = lvl_min;
  else
    lvl_max = std::stoi(lvl_max_p.first);

  // Get timestep size
  auto delta_t_p = app_settings_section->query("delta_t");
  XASSERTM(delta_t_p.second, "ApplicationConfig section is missing the mandatory delta_t entry!");
  delta_t = std::stod(delta_t_p.first);

  // Get end time
  auto t_end_p = app_settings_section->query("t_end");
  XASSERTM(delta_t_p.second, "ApplicationConfig section is missing the mandatory t_end entry!");
  t_end = std::stod(t_end_p.first);

  // Get the mesh optimiser key from the application settings
  auto meshoptimiser_key_p = app_settings_section->query("mesh_optimiser");
  XASSERTM(meshoptimiser_key_p.second,
  "ApplicationConfig section is missing the mandatory meshoptimiser entry!");

  int ret(1);

  // Get the mesh type sting from the parsed mesh so we know with which template parameter to call the application
  mesh_type = mesh_file_reader->get_meshtype_string();

  // Call the appropriate class' run() function
  if(mesh_type == "conformal:hypercube:2:2")
  {
    ret = MeshoptScrewsApp<MemType, DataType, IndexType, H2M2D>::run(
      meshoptimiser_key_p.first, meshopt_config, solver_config, mesh_file_reader, chart_file_reader,
      lvl_max, lvl_min, delta_t, t_end, write_vtk, test_mode);
  }

  if(mesh_type == "conformal:simplex:2:2")
  {
    ret = MeshoptScrewsApp<MemType, DataType, IndexType, S2M2D>::run(
      meshoptimiser_key_p.first, meshopt_config, solver_config, mesh_file_reader, chart_file_reader,
      lvl_max, lvl_min, delta_t, t_end, write_vtk, test_mode);
  }

  delete application_config;
  delete meshopt_config;
  delete solver_config;
  delete mesh_file_reader;
  if(chart_file_reader != nullptr)
    delete chart_file_reader;

  FEAT::Runtime::finalise();
  return ret;
}

static void display_help()
{
  if(Util::Comm::rank() == 0)
  {
    std::cout << "meshopt_screws-app: Two excentrically rotating screws"
    << std::endl;
    std::cout << "Mandatory arguments:" << std::endl;
    std::cout << " --application_config: Path to the application configuration file" << std::endl;
    std::cout << "Optional arguments:" << std::endl;
    std::cout << " --testmode: Run as a test. Ignores configuration files and uses hard coded settings." << std::endl;
    std::cout << " --vtk: If this is set, vtk files are written" << std::endl;
    std::cout << " --help: Displays this text" << std::endl;
  }
}

static void read_test_mode_application_config(std::stringstream& iss)
{
  iss << "[ApplicationSettings]" << std::endl;
  iss << "mesh_file = ./screws_2d_mesh_quad_360_1.xml" << std::endl;
  iss << "chart_file = ./screws_2d_chart_bezier_24_28.xml" << std::endl;
  iss << "meshopt_config_file = ./meshopt_config.ini" << std::endl;
  iss << "mesh_optimiser = DuDvDefault" << std::endl;
  iss << "solver_config_file = ./solver_config.ini" << std::endl;
  iss << "lvl_min = 0" << std::endl;
  iss << "lvl_max = 1" << std::endl;
  iss << "delta_t = 1e-4" << std::endl;
  iss << "t_end = 2e-4" << std::endl;
}

static void read_test_mode_meshopt_config(std::stringstream& iss)
{
  iss << "[HyperElasticityDefault]" << std::endl;
  iss << "type = Hyperelasticity" << std::endl;
  iss << "config_section = HyperelasticityDefaultParameters" << std::endl;
  iss << "dirichlet_boundaries = inner outer" << std::endl;

  iss << "[DuDvDefault]" << std::endl;
  iss << "type = DuDv" << std::endl;
  iss << "config_section = DuDvDefaultParameters" << std::endl;
  iss << "dirichlet_boundaries = inner outer" << std::endl;

  iss << "[DuDvDefaultParameters]" << std::endl;
  iss << "solver_config = PCG-MGV" << std::endl;

  iss << "[HyperelasticityDefaultParameters]" << std::endl;
  iss << "global_functional = HyperelasticityFunctional" << std::endl;
  iss << "local_functional = RumpfFunctional" << std::endl;
  iss << "solver_config = NLCG" << std::endl;
  iss << "fac_norm = 1.0" << std::endl;
  iss << "fac_det = 1.0" << std::endl;
  iss << "fac_cof = 0.0" << std::endl;
  iss << "fac_reg = 1e-8" << std::endl;
  iss << "scale_computation = current_concentration" << std::endl;
  iss << "conc_function = GapWidth" << std::endl;

  iss << "[GapWidth]" << std::endl;
  iss << "type = ChartDistance" << std::endl;
  iss << "function_type = default" << std::endl;
  iss << "chart_list = inner outer" << std::endl;
}

static void read_test_mode_solver_config(std::stringstream& iss)
{
  iss << "[NLCG]" << std::endl;
  iss << "type = NLCG" << std::endl;
  iss << "precon = DuDvPrecon" << std::endl;
  iss << "plot = 1" << std::endl;
  iss << "tol_rel = 1e-8" << std::endl;
  iss << "max_iter = 1000" << std::endl;
  iss << "linesearch = StrongWolfeLinesearch" << std::endl;
  iss << "direction_update = DYHSHybrid" << std::endl;
  iss << "keep_iterates = 0" << std::endl;

  iss << "[DuDvPrecon]" << std::endl;
  iss << "type = DuDvPrecon" << std::endl;
  iss << "dirichlet_boundaries = inner outer" << std::endl;
  iss << "linear_solver = PCG-MGV" << std::endl;

  iss << "[PCG-MGV]" << std::endl;
  iss << "type = pcg" << std::endl;
  iss << "max_iter = 100" << std::endl;
  iss << "tol_rel = 1e-8" << std::endl;
  iss << "plot = 1" << std::endl;
  iss << "precon = mgv" << std::endl;

  iss << "[strongwolfelinesearch]" << std::endl;
  iss << "type = StrongWolfeLinesearch" << std::endl;
  iss << "plot = 0" << std::endl;
  iss << "max_iter = 20" << std::endl;
  iss << "tol_decrease = 1e-3" << std::endl;
  iss << "tol_curvature = 0.3" << std::endl;
  iss << "keep_iterates = 0" << std::endl;

  iss << "[rich]" << std::endl;
  iss << "type = richardson" << std::endl;
  iss << "max_iter = 4" << std::endl;
  iss << "min_iter = 4" << std::endl;
  iss << "precon = jac" << std::endl;

  iss << "[jac]" << std::endl;
  iss << "type = jac" << std::endl;
  iss << "omega = 0.5" << std::endl;

  iss << "[mgv]" << std::endl;
  iss << "type = mgv" << std::endl;
  iss << "smoother = rich" << std::endl;
  iss << "coarse = pcg" << std::endl;

  iss << "[pcg]" << std::endl;
  iss << "type = pcg" << std::endl;
  iss << "max_iter = 10" << std::endl;
  iss << "tol_rel = 1e-8" << std::endl;
  iss << "precon = jac" << std::endl;
}

static void read_test_mode_mesh(std::stringstream& iss)
{
  if(Util::Comm::rank() == 0)
  {
    String mesh_filename(FEAT_SRC_DIR);
    mesh_filename +="/data/meshes/screws_2d_mesh_quad_360_1.xml";

    std::ifstream ifs(mesh_filename);
    if(!ifs.good())
      throw FileNotFound(mesh_filename);

    iss << ifs.rdbuf();
  }
#ifdef FEAT_HAVE_MPI
  Util::Comm::synch_stringstream(iss);
#endif
}

// This does nothing as for the currently used mesh, there is no separate chart file" << std::endl;
static void read_test_mode_chart(std::stringstream& iss)
{
  if(Util::Comm::rank() == 0)
  {
    String chart_filename(FEAT_SRC_DIR);
    chart_filename+="/data/meshes/screws_2d_chart_bezier_24_28.xml";

    std::ifstream ifs(chart_filename);
    if(!ifs.good())
      throw FileNotFound(chart_filename);

    iss << ifs.rdbuf();
  }
#ifdef FEAT_HAVE_MPI
  Util::Comm::synch_stringstream(iss);
#endif
}
