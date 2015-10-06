#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/geometry/boundary_factory.hpp>
#include <kernel/geometry/conformal_factories.hpp>
#include <kernel/geometry/export_vtk.hpp>
#include <kernel/geometry/mesh_node.hpp>
#include <kernel/geometry/mesh_streamer_factory.hpp>
#include <kernel/meshopt/rumpf_smoother.hpp>
#include <kernel/meshopt/rumpf_smoother_q1hack.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_p1_d1.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_p1_d2.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1_d1.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1_d2.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1hack.hpp>
#include <kernel/util/math.hpp>
#include <kernel/util/mesh_streamer.hpp>
#include <kernel/util/simple_arg_parser.hpp>
#include <kernel/util/tiny_algebra.hpp>

using namespace FEAST;

  template<typename PointType, typename DataType>
  void centre_point_outer(PointType& my_point, DataType time)
  {
    my_point.v[0] = DataType(0.5) - DataType(0.125)*Math::cos(DataType(2)*Math::pi<DataType>()*DataType(time));
    my_point.v[1] = DataType(0.5) - DataType(0.125)*Math::sin(DataType(2)*Math::pi<DataType>()*DataType(time));
  }

  template<typename PointType, typename DataType>
  void centre_point_inner(PointType& my_point, DataType time)
  {
    my_point.v[0] = DataType(0.5) - DataType(0.1875)*Math::cos(DataType(2)*Math::pi<DataType>()*DataType(time));
    my_point.v[1] = DataType(0.5) - DataType(0.1875)*Math::sin(DataType(2)*Math::pi<DataType>()*DataType(time));
  }

/**
 * \brief This application demonstrates the usage of some of the RumpfSmoother classes for boundary deformations
 *
 * \note Because an application of the (nonlinear) Rumpf smoother requires operations similar to a matrix assembly,
 * Rumpf smoothers are implemented for Mem::Main only.
 *
 * In this application, a mesh with two excentric screws is read from a mesh. The screws rotate with different
 * angular velocities, so large mesh deformations occur.
 *
 * \author Jordi Paul
 *
 * \tparam DT_
 * The precision of the mesh etc.
 *
 * \tparam MeshType
 * The mesh type, has to be known because we stream the mesh from a file
 *
 * \tparam FunctionalType
 * The Rumpf functional variant to use
 *
 * \tparam RumpfSmootherType_
 * The Rumpf smoother variant to use
 *
 **/
/**
 * \brief Wrapper struct as functions do not seem to agree with template template parameters
 **/
template
<
  typename DT_,
  typename MeshType_,
  template<typename, typename> class FunctionalType_,
  template<typename ... > class RumpfSmootherType_
 >
  struct RumpfSmootherExcentricApp
{
  /// Precision for meshes etc, everything else uses the same data type
  typedef DT_ DataType;
  /// Rumpf Smoothers are implemented for Mem::Main only
  typedef Mem::Main MemType;
  /// So we use Index
  typedef Index IndexType;
  /// The type of the mesh
  typedef MeshType_ MeshType;
  /// Shape of the mesh cells
  typedef typename MeshType::ShapeType ShapeType;
  /// Shape of mesh facets
  typedef typename Shape::FaceTraits<ShapeType, ShapeType::dimension - 1>::ShapeType FacetShapeType;
  /// Type of a surface mesh of facets
  typedef typename Geometry::ConformalMesh
  <FacetShapeType, MeshType::world_dim, MeshType::world_dim, typename MeshType::CoordType> SurfaceMeshType;

  /// The corresponding transformation
  typedef Trafo::Standard::Mapping<MeshType> TrafoType;
  /// Our functional type
  typedef FunctionalType_<DataType, ShapeType> FunctionalType;
  /// The Rumpf smoother
  typedef RumpfSmootherType_<TrafoType, FunctionalType> RumpfSmootherType;
  /// Type for points in the mesh
  typedef Tiny::Vector<DataType, MeshType::world_dim> ImgPointType;

  /**
   * \brief Routine that does the actual work
   *
   * \param[in] my_streamer
   * MeshStreamer that contains the data from the mesh file.
   *
   * \param[in] level
   * Number of refines.
   */
  static int run(MeshStreamer& my_streamer, Index lvl_max, DT_ deltat)
  {
    // Read mesh from the MeshStreamer and create the MeshAtlas
    std::cout << "Creating mesh atlas..." << std::endl;
    Geometry::MeshAtlas<MeshType_>* atlas = nullptr;
    try
    {
      atlas = new Geometry::MeshAtlas<MeshType_>(my_streamer);
    }
    catch(std::exception& exc)
    {
      std::cerr << "ERROR: " << exc.what() << std::endl;
      return 1;
    }

    // Create mesh node
    std::cout << "Creating mesh node..." << std::endl;
    Geometry::RootMeshNode<MeshType_>* rmn = nullptr;
    try
    {
      rmn = new Geometry::RootMeshNode<MeshType_>(my_streamer, atlas);
      rmn ->adapt();
    }
    catch(std::exception& exc)
    {
      std::cerr << "ERROR: " << exc.what() << std::endl;
      return 1;
    }

    // refine
    for(Index lvl(1); lvl <= lvl_max; ++lvl)
    {
      std::cout << "Refining up to level " << lvl << "..." << std::endl;
      auto* old = rmn;
      rmn = old->refine();
      delete old;
    }

    MeshType* mesh = rmn->get_mesh();

    std::deque<String> dirichlet_list;
    dirichlet_list.push_back("inner");
    std::deque<String> slip_list;
    slip_list.push_back("outer");

    // This is the centre reference point
    ImgPointType x_0(DataType(0));

    // This is the centre point of the rotation of the inner screw
    ImgPointType x_1(DataType(0));
    DataType excentricity_inner(DataType(0.2833));
    x_1.v[0] = -excentricity_inner;
    // The indices for the inner screw
    auto& inner_indices = rmn->find_mesh_part("inner")->template get_target_set<0>();

    // This is the centre point of the rotation of the outer screw
    ImgPointType x_2(DataType(0));
    // The indices for the outer screw
    auto& outer_indices = rmn->find_mesh_part("outer")->template get_target_set<0>();

    // Parameters for the Rumpf functional
    DataType fac_norm = DataType(1e-3),fac_det = DataType(1e0),fac_cof = DataType(0), fac_reg(DataType(1e-8));
    FunctionalType my_functional(fac_norm, fac_det, fac_cof, fac_reg);
    my_functional.print();

    // The smoother in all its template glory
    RumpfSmootherType rumpflpumpfl(rmn, dirichlet_list, slip_list, my_functional);
    rumpflpumpfl.init();
    rumpflpumpfl.print();

    // Arrays for saving the contributions of the different Rumpf functional parts
    DataType* func_norm(new DataType[mesh->get_num_entities(MeshType::shape_dim)]);
    DataType* func_det(new DataType[mesh->get_num_entities(MeshType::shape_dim)]);
    DataType* func_rec_det(new DataType[mesh->get_num_entities(MeshType::shape_dim)]);

    // Compute initial functional value
    DataType fval(0);
    fval = rumpflpumpfl.compute_functional(func_norm, func_det, func_rec_det);
    std::cout << "fval pre optimisation = " << scientify(fval) << std::endl;

    // Compute initial functional gradient
    rumpflpumpfl.compute_gradient();

    // Write initial state to file
    Geometry::ExportVTK<MeshType> writer_initial_pre(*mesh);
    writer_initial_pre.add_field_cell_blocked_vector("h", rumpflpumpfl._h);
    writer_initial_pre.add_field_cell("fval", func_norm, func_det, func_rec_det);
    writer_initial_pre.add_field_vertex_blocked_vector("grad", rumpflpumpfl._grad);
    writer_initial_pre.write("pre_initial");

    // Smooth the mesh
    rumpflpumpfl.optimise();

    // Call prepare() again because the mesh changed due to the optimisation and it was not called again after the
    // last iteration
    rumpflpumpfl.prepare();
    fval = rumpflpumpfl.compute_functional(func_norm, func_det, func_rec_det);
    rumpflpumpfl.compute_gradient();

    std::cout << "fval post optimisation = " << scientify(fval) << std::endl;

    // Write optimised initial mesh
    Geometry::ExportVTK<MeshType> writer_initial_post(*mesh);
    writer_initial_post.add_field_cell_blocked_vector("h", rumpflpumpfl._h);
    writer_initial_post.add_field_cell("fval", func_norm, func_det, func_rec_det);
    writer_initial_post.add_field_vertex_blocked_vector("grad", rumpflpumpfl._grad);
    writer_initial_post.write("post_initial");

    // For saving the old coordinates
    LAFEM::DenseVectorBlocked<MemType, DataType, IndexType, MeshType::world_dim>
      coords_old(mesh->get_num_entities(0),DataType(0));
    // For computing the mesh velocity
    LAFEM::DenseVectorBlocked<MemType, DataType, IndexType, MeshType::world_dim>
      mesh_velocity(mesh->get_num_entities(0), DataType(0));

    // Initial time
    DataType time(0);
    // Timestep size
    std::cout << "deltat = " << scientify(deltat) << std::endl;

    // Counter for timesteps
    Index n(0);
    // Filename for writing .vtu output
    std::string filename;

    // This is the absolute turning angle of the screws
    DataType alpha(0);
    // Need some pi for all the angles
    DataType pi(Math::pi<DataType>());

    while(time < DataType(1))
    {
      std::cout << "timestep " << n << std::endl;
      time+= deltat;

      // Save old vertex coordinates
      coords_old.clone(rumpflpumpfl._coords);

      DataType alpha_old = alpha;
      alpha = -DataType(2)*pi*time;

      DataType delta_alpha = alpha - alpha_old;

      // Update boundary of the inner screw
      // This is the 2x2 matrix representing the turning by the angle delta_alpha of the inner screw
      Tiny::Matrix<DataType, 2, 2> rot(DataType(0));

      rot(0,0) = Math::cos(delta_alpha);
      rot(0,1) = - Math::sin(delta_alpha);
      rot(1,0) = -rot(0,1);
      rot(1,1) = rot(0,0);

      // This is the old centre point
      ImgPointType x_1_old(x_1);

      // This is the new centre point
      x_1.v[0] = x_0.v[0] - excentricity_inner*Math::cos(alpha);
      x_1.v[1] = x_0.v[1] - excentricity_inner*Math::sin(alpha);

      ImgPointType tmp(DataType(0));
      ImgPointType tmp2(DataType(0));
      for(Index i(0); i < inner_indices.get_num_entities(); ++i)
      {
        // Index of boundary vertex i in the mesh
        Index j(inner_indices[i]);
        // Translate the point to the centre of rotation
        tmp = rumpflpumpfl._coords(j) - x_1_old;
        // Rotate
        tmp2.set_vec_mat_mult(tmp, rot);
        // Translate the point by the new centre of rotation
        rumpflpumpfl._coords(j, x_1 + tmp2);
      }

      // Rotate the mesh in the discrete chart. This has to use an evil downcast for now
      auto* inner_chart = reinterpret_cast< Geometry::Atlas::DiscreteChart<MeshType, SurfaceMeshType>*>
        (atlas->find_mesh_chart("inner"));

      auto& vtx_inner = inner_chart->_surface_mesh->get_vertex_set();

      for(Index i(0); i < inner_chart->_surface_mesh->get_num_entities(0); ++i)
      {
        tmp = vtx_inner[i] - x_1_old;
        // Rotate
        tmp2.set_vec_mat_mult(tmp, rot);
        // Translate the point by the new centre of rotation
        vtx_inner[i] = x_1 + tmp2;
      }

      // The outer screw has 7 teeth as opposed to the inner screw with 6, and it rotates at 6/7 of the speed
      rot(0,0) = Math::cos(delta_alpha*DataType(6)/DataType(7));
      rot(0,1) = - Math::sin(delta_alpha*DataType(6)/DataType(7));
      rot(1,0) = -rot(0,1);
      rot(1,1) = rot(0,0);

      // The outer screw rotates centrically, so x_2 remains the same at all times

      for(Index i(0); i < outer_indices.get_num_entities(); ++i)
      {
        // Index of boundary vertex i in the mesh
        Index j(outer_indices[i]);
        tmp = rumpflpumpfl._coords(j) - x_2;

        tmp2.set_vec_mat_mult(tmp, rot);

        rumpflpumpfl._coords(j, x_2 + tmp2);
      }

      // Rotate the mesh in the discrete chart. This has to use an evil downcast for now
      auto* outer_chart = reinterpret_cast<Geometry::Atlas::DiscreteChart<MeshType, SurfaceMeshType>*>
        (atlas->find_mesh_chart("outer"));

      auto& vtx_outer = outer_chart->_surface_mesh->get_vertex_set();

      for(Index i(0); i < outer_chart->_surface_mesh->get_num_entities(0); ++i)
      {
        tmp = vtx_outer[i] - x_2;
        // Rotate
        tmp2.set_vec_mat_mult(tmp, rot);
        vtx_outer[i] = x_2 + tmp2;
      }

      // Write new boundary to mesh
      rumpflpumpfl.set_coords();

      rumpflpumpfl.prepare();
      fval = rumpflpumpfl.compute_functional(func_norm, func_det, func_rec_det);
      rumpflpumpfl.compute_gradient();
      std::cout << "fval pre optimisation = " << scientify(fval) << std::endl;

      // Write pre-optimisation mesh
      filename = "pre_" + stringify(n);
      Geometry::ExportVTK<MeshType> writer_pre(*mesh);
      writer_pre.add_field_cell_blocked_vector("h", rumpflpumpfl._h);
      writer_pre.add_field_cell("fval", func_norm, func_det, func_rec_det);
      writer_pre.add_field_vertex_blocked_vector("grad", rumpflpumpfl._grad);
      writer_pre.add_field_vertex_blocked_vector("mesh_velocity", mesh_velocity);
      std::cout << "Writing " << filename << std::endl;
      writer_pre.write(filename);

      // Optimise the mesh
      rumpflpumpfl.optimise();

      rumpflpumpfl.prepare();
      fval = rumpflpumpfl.compute_functional(func_norm, func_det, func_rec_det);
      rumpflpumpfl.compute_gradient();
      std::cout << "fval post optimisation = " << scientify(fval) << std::endl;

      // Compute max. mesh velocity
      DataType max_mesh_velocity(-1e10);
      DataType ideltat = DataType(1)/deltat;

      for(Index i(0); i < mesh->get_num_entities(0); ++i)
      {
        mesh_velocity(i, ideltat*(rumpflpumpfl._coords(i) - coords_old(i)));

        DataType my_mesh_velocity(mesh_velocity(i).norm_euclid());

        if(my_mesh_velocity > max_mesh_velocity)
          max_mesh_velocity = my_mesh_velocity;
      }
      std::cout << "max mesh velocity = " << scientify(max_mesh_velocity) << std::endl;

      // Write post-optimisation mesh
      filename = "post_" + stringify(n);
      Geometry::ExportVTK<MeshType> writer_post(*mesh);
      writer_pre.add_field_cell_blocked_vector("h", rumpflpumpfl._h);
      writer_pre.add_field_cell("fval", func_norm, func_det, func_rec_det);
      writer_pre.add_field_vertex_blocked_vector("grad", rumpflpumpfl._grad);
      writer_pre.add_field_vertex_blocked_vector("mesh_velocity", mesh_velocity);
      std::cout << "Writing " << filename << std::endl;
      writer_post.write(filename);

      n++;
    }

    // Clean up
    delete rmn;
    if(atlas != nullptr)
      delete atlas;

    delete[] func_norm;
    delete[] func_det;
    delete[] func_rec_det;

    return 0;

  }


}; // struct LevelsetApp

template<typename A, typename B>
using MyFunctional= Meshopt::RumpfFunctional<A, B>;

template<typename A, typename B>
using MyFunctionalQ1Hack = Meshopt::RumpfFunctionalQ1Hack<A, B, Meshopt::RumpfFunctional>;

template<typename A, typename B>
using MySmoother = Meshopt::RumpfSmoother<A, B>;

template<typename A, typename B>
using MySmootherQ1Hack = Meshopt::RumpfSmootherQ1Hack<A, B>;


/**
 * \cond internal
 *
 * Mesh Streamer Application
 *
 */
int main(int argc, char* argv[])
{
  // Creata a parser for command line arguments.
  SimpleArgParser args(argc, argv);

  if( args.check("help") > -1 || args.num_args()==1)
  {
    std::cout << "Rumpf Smoother Application for Excentric Screws usage: " << std::endl;
    std::cout << "Required arguments: --filename [String]: Path to a FEAST mesh file." << std::endl;
    std::cout << "Optional arguments: --level [unsigned int]: Number of refines, defaults to 0." << std::endl;
    exit(1);
  }
  // Specify supported command line switches
  args.support("level");
  args.support("filename");
  args.support("help");
  // Refinement level
  Index lvl_max(0);
  // Input file name, required
  FEAST::String filename;
  // Get unsupported command line arguments
  std::deque<std::pair<int,String> > unsupported = args.query_unsupported();
  if( !unsupported.empty() )
  {
    // print all unsupported options to cerr
    for(auto it = unsupported.begin(); it != unsupported.end(); ++it)
      std::cerr << "ERROR: unsupported option '--" << (*it).second << "'" << std::endl;
  }

  // Check and parse --filename
  if(args.check("filename") != 1 )
    throw InternalError(__func__, __FILE__, __LINE__, "Invalid option for --filename");
  else
  {
    args.parse("filename", filename);
    std::cout << "Reading mesh from file " << filename << std::endl;
  }

  // Check and parse --level
  if(args.check("level") != 1)
    std::cout << "No refinement level specified, defaulting to 0." << std::endl;
  else
  {
    args.parse("level", lvl_max);
    std::cout << "Refinement level " << lvl_max << std::endl;
  }

  // Create a MeshStreamer and read the mesh file
  MeshStreamer my_streamer;
  my_streamer.parse_mesh_file(filename);

  // This is the raw mesh data my_streamer read from filename
  auto& mesh_data = my_streamer.get_root_mesh_node()->mesh_data;
  // Marker int for the MeshType
  int mesh_type = mesh_data.mesh_type;
  // Marker int for the ShapeType
  int shape_type = mesh_data.shape_type;

  ASSERT(mesh_type == mesh_data.mt_conformal, "This application only works for conformal meshes!");

  typedef double DataType;

  DataType deltat(DataType(1e-4));

  // This is the list of all supported meshes that could appear in the mesh file
  typedef Geometry::ConformalMesh<Shape::Simplex<2>, 2, 2, Real> Simplex2Mesh_2d;
  typedef Geometry::ConformalMesh<Shape::Hypercube<2>, 2, 2, Real> Hypercube2Mesh_2d;

  // Call the run() method of the appropriate wrapper class
  if(shape_type == mesh_data.st_tria)
    return RumpfSmootherExcentricApp<DataType, Simplex2Mesh_2d, MyFunctional, MySmoother>::
      run(my_streamer, lvl_max, deltat);
  if(shape_type == mesh_data.st_quad)
    return RumpfSmootherExcentricApp<DataType, Hypercube2Mesh_2d, MyFunctional, MySmoother>::
      run(my_streamer, lvl_max, deltat);

  // If no MeshType from the list was in the file, return 1
  return 1;
}
/// \endcond
