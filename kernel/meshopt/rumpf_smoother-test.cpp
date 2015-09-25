#include <kernel/base_header.hpp>
#ifdef FEAST_HAVE_ALGLIB
#include <test_system/test_system.hpp>
#include <kernel/geometry/boundary_factory.hpp>
#include <kernel/geometry/reference_cell_factory.hpp>
#include <kernel/meshopt/rumpf_smoother.hpp>
#include <kernel/meshopt/rumpf_smoother_q1hack.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1_d1.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1_d2.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_q1hack.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_p1_d1.hpp>
#include <kernel/meshopt/rumpf_functionals/2d_p1_d2.hpp>

using namespace FEAST;
using namespace FEAST::TestSystem;

/// \cond internal

/// \brief Helper class for resizing tests
template<typename ShapeType_>
struct helperclass;

/// \cond internal

/**
 * \brief Test for Rumpf smoothers and functionals
 *
 * The input mesh consists of a single Rumpf reference cell of some target scaling. This is then rescaled and the
 * mesh optimiser is supposed to scale it back to the original scaling.
 *
 * If the resulting cell is optimal in the defined sense, the Frobenius norm term should be zero and the determinant
 * should be 1 (mind the scaling from fac_det etc. in the functional!).
 *
 * \author Jordi Paul
 **/
template
<
  typename DT_,
  typename ShapeType_,
  template<typename, typename> class FunctionalType_,
  template<typename ... > class RumpfSmootherType_
>
class RumpfSmootherTest_2d
: public TestSystem::FullTaggedTest<Mem::Main, DT_, Index>
{
  public:
    typedef Mem::Main MemType;
    typedef Index IndexType;
    typedef DT_ DataType;

    typedef ShapeType_ ShapeType;
    typedef Geometry::ConformalMesh<ShapeType, ShapeType::dimension, ShapeType::dimension, DataType> MeshType;
    typedef Trafo::Standard::Mapping<MeshType> TrafoType;

    typedef FunctionalType_<DataType, ShapeType> FunctionalType;
    typedef RumpfSmootherType_<TrafoType, FunctionalType> RumpfSmootherType;

    RumpfSmootherTest_2d() :
      TestSystem::FullTaggedTest<MemType, DataType, IndexType>("rumpf_smoother_test")
      {
      }

    virtual void run() const
    {
      // Mesh
      Geometry::ReferenceCellFactory<ShapeType, DataType> mesh_factory;
      MeshType* mesh(new MeshType(mesh_factory));
      Geometry::RootMeshNode<MeshType>* rmn(new Geometry::RootMeshNode<MeshType>(mesh, nullptr));

      // As we set no boundary conditions, these lists remain empty
      std::deque<String> dirichlet_list;
      std::deque<String> slip_list;

      // In 2d, the cofactor matrix is not used
      DataType fac_norm = DataType(1e0),fac_det = DataType(2),fac_cof = DataType(0), fac_reg(DataType(1e-8));
      FunctionalType my_functional(fac_norm, fac_det, fac_cof, fac_reg);

      // Create the smoother
      RumpfSmootherType rumpflpumpfl(rmn, dirichlet_list, slip_list, my_functional);

      // This transforms the unit element to the Rumpf reference element
      DataType target_scale(DataType(1.1));
      helperclass<ShapeType>::set_coords(rumpflpumpfl._coords, target_scale);
      // init() sets the coordinates in the mesh and computes h
      rumpflpumpfl.init();

      // Now we rescale the Rumpf reference element again, so the optimiser has some work to do
      DataType scaling(DataType(2.75));
      helperclass<ShapeType>::set_coords(rumpflpumpfl._coords, scaling);
      rumpflpumpfl.set_coords();

      DataType func_norm(0), func_det(0), func_rec_det(0);
      // Compute initial functional value
      DataType fval_pre = rumpflpumpfl.compute_functional();
      // Optimise the mesh
      rumpflpumpfl.optimise();
      // Compute new functional value
      DataType fval_post = rumpflpumpfl.compute_functional(&func_norm, &func_det, &func_rec_det);

      const DataType eps = Math::pow(Math::eps<DataType>(),DataType(0.5));

      // Only check func_norm and func_det. Because of the different factors fac_rec_det depending on the
      // functionals, func_rec_det is not the same in every case. If func_det==1, we have the correct volume anyway.
      TEST_CHECK(fval_pre > fval_post);
      TEST_CHECK_EQUAL_WITHIN_EPS(func_norm, DataType(0), eps);
      TEST_CHECK_EQUAL_WITHIN_EPS(func_det, my_functional._fac_det*DataType(1), eps);

      // Now do the negative test: Change the functional in a nonsensical manner. Calling the optimiser should NOT
      // give the correctly scaled element
      my_functional._fac_rec_det = DataType(0.6676);

      // Compute initial functional value
      fval_pre = rumpflpumpfl.compute_functional();
      // Optimise the mesh
      rumpflpumpfl.optimise();
      // Compute new functional value
      fval_post = rumpflpumpfl.compute_functional(&func_norm, &func_det, &func_rec_det);

      // With the new functional, the functional value should still have decreased
      TEST_CHECK(fval_pre > fval_post);
      // These differences should all be greater than eps
      TEST_CHECK(Math::abs(func_norm - DataType(0)) > eps);
      TEST_CHECK(Math::abs(func_det - my_functional._fac_det*DataType(1)) > eps);

      delete rmn;

    }
};

template<typename A, typename B>
using MySmoother = Meshopt::RumpfSmoother<A, B>;

template<typename A, typename B>
using MySmootherQ1Hack = Meshopt::RumpfSmootherQ1Hack<A, B>;

RumpfSmootherTest_2d<float, Shape::Hypercube<2>, Meshopt::RumpfFunctional, MySmoother> test_hc_1;
RumpfSmootherTest_2d<double, Shape::Hypercube<2>, Meshopt::RumpfFunctional_D2, MySmoother> test_hc_2;
RumpfSmootherTest_2d<double, Shape::Simplex<2>, Meshopt::RumpfFunctional, MySmoother> test_s_1;
RumpfSmootherTest_2d<float, Shape::Simplex<2>, Meshopt::RumpfFunctional_D2, MySmoother> test_s_2;

template<typename A, typename B>
using MyFunctionalQ1Hack = Meshopt::RumpfFunctionalQ1Hack<A, B, Meshopt::RumpfFunctional>;

template<typename A, typename B>
using MyFunctionalQ1Hack_D2 = Meshopt::RumpfFunctionalQ1Hack<A, B, Meshopt::RumpfFunctional_D2>;

RumpfSmootherTest_2d<float, Shape::Hypercube<2>, MyFunctionalQ1Hack, MySmootherQ1Hack> test_q1hack_f_1;
RumpfSmootherTest_2d<double, Shape::Hypercube<2>, MyFunctionalQ1Hack_D2, MySmootherQ1Hack> test_q1hack_d_2;

/// \brief Specialisation for hypercubes
template<int shape_dim_>
struct helperclass< FEAST::Shape::Hypercube<shape_dim_> >
{
  /// \brief Sets coordinates so we deal the the reference element
  template<typename VectorType_, typename DataType_>
  static void set_coords(VectorType_& coords_, const DataType_& scaling)
  {
    for(Index i(0); i < Index(1 << shape_dim_); ++i)
    {
      Tiny::Vector<DataType_, VectorType_::BlockSize, VectorType_::BlockSize> tmp;
      for(int d(0); d < shape_dim_; ++d)
        tmp(d) = (DataType_(((i >> d) & 1) << 1) - DataType_(1)) * scaling ;

      coords_(i, tmp);
    }
  }

};

/// \brief Specialisation for 2d simplices
template<>
struct helperclass< FEAST::Shape::Simplex<2> >
{
  /// \brief Sets coordinates so we deal the the Rumpf reference element
  template<typename VectorType_, typename DataType_>
  static void set_coords(VectorType_& coords_, const DataType_& scaling)
  {
    Tiny::Vector<DataType_, 2, 2> tmp(0);
    coords_(0, tmp);

    tmp(0) = scaling;
    coords_(1, tmp);

    tmp(0) = DataType_(0.5) * scaling;
    tmp(1) = DataType_(0.5)*Math::sqrt(DataType_(3))*scaling;
    coords_(2, tmp);
  }
};

#endif