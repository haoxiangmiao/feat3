#include <kernel/base_header.hpp>

#include <kernel/foundation/comm_base.hpp>

#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/cell_sub_set.hpp>
#include <kernel/archs.hpp>
#include <kernel/foundation/communication.hpp>
#include <kernel/foundation/halo.hpp>
#include <kernel/foundation/attribute.hpp>
#include <kernel/foundation/topology.hpp>
#include <kernel/foundation/mesh.hpp>
#include <kernel/foundation/refinement.hpp>
#include <kernel/foundation/partitioning.hpp>
#include <kernel/foundation/mesh_control.hpp>
#include <kernel/foundation/halo_control.hpp>
#include <kernel/foundation/halo_interface.hpp>
#include <kernel/foundation/global_dot.hpp>
#include <kernel/foundation/global_synch_vec.hpp>
#include <kernel/foundation/global_product_mat_vec.hpp>
#include <kernel/foundation/global_defect.hpp>
#include <kernel/foundation/global_norm.hpp>
#include <kernel/foundation/gateway.hpp>
#include <kernel/foundation/aura.hpp>
#include <kernel/foundation/halo_frequencies.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/trafo/standard/mapping.hpp>
#include <kernel/space/lagrange1/element.hpp>
#include <kernel/assembly/common_operators.hpp>
#include <kernel/assembly/common_functionals.hpp>
#include <kernel/assembly/common_functions.hpp>
#include <kernel/assembly/mirror_assembler.hpp>
#include <kernel/assembly/symbolic_assembler.hpp>
#include <kernel/assembly/bilinear_operator_assembler.hpp>
#include <kernel/assembly/linear_functional_assembler.hpp>
#include <kernel/assembly/dirichlet_assembler.hpp>
#include <kernel/scarc/scarc_functor.hpp>
#include <kernel/scarc/matrix_conversion.hpp>

#include <iostream>
#include <limits>

using namespace FEAST;
using namespace Foundation;
using namespace Geometry;
using namespace ScaRC;

template<typename DT1_, typename DT2_, typename DT3_>
struct TestResult
{
  TestResult(DT1_ l, DT2_ r, DT3_ eps) :
    left(l),
    right(r),
    epsilon(eps)
  {
    //passed = std::abs(l - r) < eps ? true : false;
    passed = (l < r ? r - l : l - r) < eps ? true : false;
  }

  TestResult()
  {
  }

  DT1_ left;
  DT2_ right;
  DT3_ epsilon;
  bool passed;
};

template<typename DT1_, typename DT2_, typename DT3_>
TestResult<DT1_, DT2_, DT3_> test_check_equal_within_eps(DT1_ l, DT2_ r, DT3_ eps)
{
  return TestResult<DT1_, DT2_, DT3_>(l, r, eps);
}

void check_scarc_rich_rich_1D(Index rank)
{
  /* (0)  (1)
   *  *----*
   */

  std::vector<Attribute<double>, std::allocator<Attribute<double> > > attrs;
  attrs.push_back(Attribute<double>()); //vertex x-coords

  attrs.at(0).get_data().push_back(double(0));
  attrs.at(0).get_data().push_back(double(1));
  /*
   *  *--0-*
   *  0    1
   */

  //creating foundation mesh
  Mesh<Dim1D> m(0);

  m.add_polytope(pl_vertex);
  m.add_polytope(pl_vertex);

  m.add_polytope(pl_edge);

  m.add_adjacency(pl_vertex, pl_edge, 0, 0);
  m.add_adjacency(pl_vertex, pl_edge, 1, 0);

  std::vector<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> >, std::allocator<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> > > > halos;

  Mesh<Dim1D> m_fine(m);

  //set up halos

  Refinement<Mem::Main,
             Algo::Generic,
             mrt_standard,
             hrt_refine>::execute(m_fine, &halos, attrs);

  /*  *----*----*
   *      (2)
   */

  std::vector<Halo<0, PLVertex, Mesh<Dim1D> > > boundaries;
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.at(0).push_back(0);
  boundaries.at(1).push_back(1);

  auto p0(Partitioning<Mem::Main,
                       Algo::Generic,
                       Dim1D,
                       0,
                       pl_vertex>::execute(m_fine,
                                           boundaries,
                                           2, rank,
                                           attrs
                                           ));

  typedef ConformalMesh<Shape::Hypercube<1> > confmeshtype_;

  Index* size_set(new Index[2]);
  MeshControl<dim_1D>::fill_sizes(*((Mesh<Dim1D>*)(p0.submesh.get())), size_set);

  confmeshtype_ confmesh(size_set);
  MeshControl<dim_1D>::fill_adjacencies(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh);
  MeshControl<dim_1D>::fill_vertex_sets(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh, attrs.at(0));

  auto cell_sub_set(HaloInterface<0, Dim1D>::convert(p0.comm_halos.at(0).get()));

  Trafo::Standard::Mapping<ConformalMesh<Shape::Hypercube<1> > > trafo(confmesh);
  Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > space(trafo);
  VectorMirror<Mem::Main, double> target_mirror;
  Assembly::MirrorAssembler::assemble_mirror(target_mirror, space, cell_sub_set);

  std::vector<VectorMirror<Mem::Main, double> > mirrors;
  mirrors.push_back(std::move(target_mirror));

  std::vector<DenseVector<Mem::Main, double> > sendbufs;
  std::vector<DenseVector<Mem::Main, double> > recvbufs;
  DenseVector<Mem::Main, double> sbuf(mirrors.at(0).size());
  DenseVector<Mem::Main, double> rbuf(mirrors.at(0).size());
  sendbufs.push_back(std::move(sbuf));
  recvbufs.push_back(std::move(rbuf));

  std::vector<Index> other_ranks;
  other_ranks.push_back(p0.comm_halos.at(0)->get_other());

  std::vector<DenseVector<Mem::Main, double> > mirror_buffers;
  DenseVector<Mem::Main, double> mbuf(mirrors.at(0).size());
  mirror_buffers.push_back(std::move(mbuf));


  SparseMatrixCSR<Mem::Main, double> mat_sys;
  Assembly::SymbolicMatrixAssembler<>::assemble1(mat_sys, space);
  mat_sys.format();
  Cubature::DynamicFactory cubature_factory("gauss-legendre:2");
  Assembly::Common::LaplaceOperator laplace;
  Assembly::BilinearOperatorAssembler::assemble_matrix1(mat_sys, laplace, space, cubature_factory);

  std::vector<DenseVector<Mem::Main, double> > freq_buffers;
  DenseVector<Mem::Main, double> fbuf(mat_sys.rows());
  freq_buffers.push_back(std::move(fbuf));
  auto frequencies(HaloFrequencies<Mem::Main, Algo::Generic>::value(mirrors, mirror_buffers, freq_buffers));

  DenseVector<Mem::Main, double> vec_rhs(space.get_num_dofs(), double(0));
  Assembly::Common::ConstantFunction rhs_func(1.0);
  Assembly::Common::ForceFunctional<Assembly::Common::ConstantFunction> rhs_functional(rhs_func);
  Assembly::LinearFunctionalAssembler::assemble_vector(vec_rhs, rhs_functional, space, cubature_factory);

  Assembly::DirichletAssembler<Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > > dirichlet(space);
  auto bound_sub_set(HaloInterface<0, Dim1D>::convert(p0.boundaries.at(0)));
  dirichlet.add_cell_set(bound_sub_set);

  DenseVector<Mem::Main, double> vec_sol(space.get_num_dofs(), double(0));

  UnitFilter<Mem::Main, double> filter(space.get_num_dofs());
  dirichlet.assemble(filter);

  auto mat_localsys(MatrixConversion<Mem::Main, double, Index, SparseMatrixCSR>::value(mat_sys, mirrors, other_ranks));

  SparseMatrixCOO<Mem::Main, double> mat_precon_temp(mat_localsys.rows(), mat_localsys.columns());
  for(Index i(0) ; i < mat_localsys.rows() ; ++i)
    mat_precon_temp(i, i, double(0.75) * (double(1)/mat_localsys(i, i)));

  SparseMatrixCSR<Mem::Main, double> mat_precon(mat_precon_temp);

  ///filter system
  filter.filter_mat<Algo::Generic>(mat_sys);
  filter.filter_mat<Algo::Generic>(mat_localsys);
  filter.filter_rhs<Algo::Generic>(vec_rhs);
  filter.filter_sol<Algo::Generic>(vec_sol);
  filter.filter_mat<Algo::Generic>(mat_precon); //TODO: check if -> NO! we do this in the solver program when applying the correction filter after preconditioning

  SynchronisedPreconditionedFilteredScaRCData<double,
                                              Mem::Main,
                                              DenseVector<Mem::Main, double>,
                                              VectorMirror<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              UnitFilter<Mem::Main, double> > data(std::move(mat_sys), std::move(mat_precon), std::move(vec_sol), std::move(vec_rhs), std::move(filter));

  data.vector_mirrors() = std::move(mirrors);
  data.vector_mirror_sendbufs() = std::move(sendbufs);
  data.vector_mirror_recvbufs() = std::move(recvbufs);
  data.dest_ranks() = std::move(other_ranks);

#ifndef SERIAL
  Communicator c(MPI_COMM_WORLD);
#else
  Communicator c(0);
#endif
  data.communicators().push_back(std::move(c));

  //data.source_ranks() = std::move(sourceranks);
  data.localsys() = std::move(mat_localsys);

  data.halo_frequencies() = std::move(frequencies);

  ///layer 1 (global layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > solver(new ScaRCFunctorRichardson0<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 1 (global layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > block_smoother(new ScaRCFunctorPreconBlock<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_solver(new ScaRCFunctorRichardson1<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_precon(new ScaRCFunctorPreconSpM1V1<double,
                                                                                              Mem::Main,
                                                                                              DenseVector<Mem::Main, double>,
                                                                                              VectorMirror<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              UnitFilter<Mem::Main, double>,
                                                                                              std::vector,
                                                                                              Index,
                                                                                              Algo::Generic>(data) );


  solver->reset_preconditioner(block_smoother);
  block_smoother->reset_preconditioner(local_solver);
  local_solver->reset_preconditioner(local_precon);

  solver->execute();
  std::cout << rank << ", #iters global Rich: " << solver->iterations() << std::endl;
  std::cout << rank << ", #iters local Rich: " << local_solver->iterations() << std::endl;

  TestResult<double, double, double> res0;
  TestResult<double, double, double> res1;
  res0 = test_check_equal_within_eps(data.sol()(0), rank == 0 ? 0. : 0.25, std::numeric_limits<double>::epsilon() * 1e8);
  res1 = test_check_equal_within_eps(data.sol()(1), rank == 0 ? 0.25 : 0., std::numeric_limits<double>::epsilon() * 1e8);
  if(res0.passed && res1.passed)
    std::cout << "PASSED (rank " << rank <<"): scarc_test_1D (Rich/Rich)" << std::endl;
  else if(!res0.passed)
    std::cout << "FAILED: " << res0.left << " not within range (eps = " << res0.epsilon << ") of " << res0.right << "! (scarc_test_1D (Rich/Rich)) " << std::endl;
  else if(!res1.passed)
    std::cout << "FAILED: " << res1.left << " not within range (eps = " << res1.epsilon << ") of " << res1.right << "! (scarc_test_1D (Rich/Rich)) " << std::endl;
}

void check_scarc_pcg_rich_1D(Index rank)
{
  /* (0)  (1)
   *  *----*
   */

  std::vector<Attribute<double>, std::allocator<Attribute<double> > > attrs;
  attrs.push_back(Attribute<double>()); //vertex x-coords

  attrs.at(0).get_data().push_back(double(0));
  attrs.at(0).get_data().push_back(double(1));
  /*
   *  *--0-*
   *  0    1
   */

  //creating foundation mesh
  Mesh<Dim1D> m(0);

  m.add_polytope(pl_vertex);
  m.add_polytope(pl_vertex);

  m.add_polytope(pl_edge);

  m.add_adjacency(pl_vertex, pl_edge, 0, 0);
  m.add_adjacency(pl_vertex, pl_edge, 1, 0);

  std::vector<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> >, std::allocator<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> > > > halos;

  Mesh<Dim1D> m_fine(m);

  //set up halos

  Refinement<Mem::Main,
             Algo::Generic,
             mrt_standard,
             hrt_refine>::execute(m_fine, &halos, attrs);

  /*  *----*----*
   *      (2)
   */

  std::vector<Halo<0, PLVertex, Mesh<Dim1D> > > boundaries;
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.at(0).push_back(0);
  boundaries.at(1).push_back(1);

  auto p0(Partitioning<Mem::Main,
                       Algo::Generic,
                       Dim1D,
                       0,
                       pl_vertex>::execute(m_fine,
                                           boundaries,
                                           2, rank,
                                           attrs
                                           ));

  typedef ConformalMesh<Shape::Hypercube<1> > confmeshtype_;

  Index* size_set(new Index[2]);
  MeshControl<dim_1D>::fill_sizes(*((Mesh<Dim1D>*)(p0.submesh.get())), size_set);

  confmeshtype_ confmesh(size_set);
  MeshControl<dim_1D>::fill_adjacencies(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh);
  MeshControl<dim_1D>::fill_vertex_sets(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh, attrs.at(0));

  auto cell_sub_set(HaloInterface<0, Dim1D>::convert(p0.comm_halos.at(0).get()));

  Trafo::Standard::Mapping<ConformalMesh<Shape::Hypercube<1> > > trafo(confmesh);
  Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > space(trafo);
  VectorMirror<Mem::Main, double> target_mirror;
  Assembly::MirrorAssembler::assemble_mirror(target_mirror, space, cell_sub_set);

  std::vector<VectorMirror<Mem::Main, double> > mirrors;
  mirrors.push_back(std::move(target_mirror));

  std::vector<DenseVector<Mem::Main, double> > sendbufs;
  std::vector<DenseVector<Mem::Main, double> > recvbufs;
  DenseVector<Mem::Main, double> sbuf(mirrors.at(0).size());
  DenseVector<Mem::Main, double> rbuf(mirrors.at(0).size());
  sendbufs.push_back(std::move(sbuf));
  recvbufs.push_back(std::move(rbuf));

  std::vector<Index> other_ranks;
  other_ranks.push_back(p0.comm_halos.at(0)->get_other());

  std::vector<DenseVector<Mem::Main, double> > mirror_buffers;
  DenseVector<Mem::Main, double> mbuf(mirrors.at(0).size());
  mirror_buffers.push_back(std::move(mbuf));


  SparseMatrixCSR<Mem::Main, double> mat_sys;
  Assembly::SymbolicMatrixAssembler<>::assemble1(mat_sys, space);
  mat_sys.format();
  Cubature::DynamicFactory cubature_factory("gauss-legendre:2");
  Assembly::Common::LaplaceOperator laplace;
  Assembly::BilinearOperatorAssembler::assemble_matrix1(mat_sys, laplace, space, cubature_factory);

  std::vector<DenseVector<Mem::Main, double> > freq_buffers;
  DenseVector<Mem::Main, double> fbuf(mat_sys.rows());
  freq_buffers.push_back(std::move(fbuf));
  auto frequencies(HaloFrequencies<Mem::Main, Algo::Generic>::value(mirrors, mirror_buffers, freq_buffers));

  DenseVector<Mem::Main, double> vec_rhs(space.get_num_dofs(), double(0));
  Assembly::Common::ConstantFunction rhs_func(1.0);
  Assembly::Common::ForceFunctional<Assembly::Common::ConstantFunction> rhs_functional(rhs_func);
  Assembly::LinearFunctionalAssembler::assemble_vector(vec_rhs, rhs_functional, space, cubature_factory);

  Assembly::DirichletAssembler<Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > > dirichlet(space);
  auto bound_sub_set(HaloInterface<0, Dim1D>::convert(p0.boundaries.at(0)));
  dirichlet.add_cell_set(bound_sub_set);

  DenseVector<Mem::Main, double> vec_sol(space.get_num_dofs(), double(0));

  UnitFilter<Mem::Main, double> filter(space.get_num_dofs());
  dirichlet.assemble(filter);

  auto mat_localsys(MatrixConversion<Mem::Main, double, Index, SparseMatrixCSR>::value(mat_sys, mirrors, other_ranks));

  SparseMatrixCOO<Mem::Main, double> mat_precon_temp(mat_localsys.rows(), mat_localsys.columns());
  for(Index i(0) ; i < mat_localsys.rows() ; ++i)
    mat_precon_temp(i, i, double(0.75) * (double(1)/mat_localsys(i, i)));

  SparseMatrixCSR<Mem::Main, double> mat_precon(mat_precon_temp);

  ///filter system
  filter.filter_mat<Algo::Generic>(mat_sys);
  filter.filter_mat<Algo::Generic>(mat_localsys);
  filter.filter_rhs<Algo::Generic>(vec_rhs);
  filter.filter_sol<Algo::Generic>(vec_sol);
  filter.filter_mat<Algo::Generic>(mat_precon); //TODO: check if -> NO! we do this in the solver program when applying the correction filter after preconditioning

  SynchronisedPreconditionedFilteredScaRCData<double,
                                              Mem::Main,
                                              DenseVector<Mem::Main, double>,
                                              VectorMirror<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              UnitFilter<Mem::Main, double> > data(std::move(mat_sys), std::move(mat_precon), std::move(vec_sol), std::move(vec_rhs), std::move(filter));

  data.vector_mirrors() = std::move(mirrors);
  data.vector_mirror_sendbufs() = std::move(sendbufs);
  data.vector_mirror_recvbufs() = std::move(recvbufs);
  data.dest_ranks() = std::move(other_ranks);

#ifndef SERIAL
  Communicator c(MPI_COMM_WORLD);
#else
  Communicator c(0);
#endif
  data.communicators().push_back(std::move(c));

  //data.source_ranks() = std::move(sourceranks);
  data.localsys() = std::move(mat_localsys);

  data.halo_frequencies() = std::move(frequencies);

  ///layer 1 (global layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > solver(new ScaRCFunctorPCG0<double,
                                                                                Mem::Main,
                                                                                DenseVector<Mem::Main, double>,
                                                                                VectorMirror<Mem::Main, double>,
                                                                                SparseMatrixCSR<Mem::Main, double>,
                                                                                SparseMatrixCSR<Mem::Main, double>,
                                                                                UnitFilter<Mem::Main, double>,
                                                                                std::vector,
                                                                                Index,
                                                                                Algo::Generic>(data) );

  ///layer 1 (global layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > block_smoother(new ScaRCFunctorPreconBlock<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_solver(new ScaRCFunctorRichardson1<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_precon(new ScaRCFunctorPreconSpM1V1<double,
                                                                                              Mem::Main,
                                                                                              DenseVector<Mem::Main, double>,
                                                                                              VectorMirror<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              UnitFilter<Mem::Main, double>,
                                                                                              std::vector,
                                                                                              Index,
                                                                                              Algo::Generic>(data) );


  solver->reset_preconditioner(block_smoother);
  block_smoother->reset_preconditioner(local_solver);
  local_solver->reset_preconditioner(local_precon);

  solver->execute();

  std::cout << rank << ", #iters global PCG: " << solver->iterations() << std::endl;
  std::cout << rank << ", #iters local RICH: " << local_solver->iterations() << std::endl;

  TestResult<double, double, double> res0;
  TestResult<double, double, double> res1;
  res0 = test_check_equal_within_eps(data.sol()(0), rank == 0 ? 0. : 0.25, std::numeric_limits<double>::epsilon());
  res1 = test_check_equal_within_eps(data.sol()(1), rank == 0 ? 0.25 : 0., std::numeric_limits<double>::epsilon());
  if(res0.passed && res1.passed)
    std::cout << "PASSED (rank " << rank <<"): scarc_test_1D (PCG/RICH)" << std::endl;
  else if(!res0.passed)
    std::cout << "FAILED: " << res0.left << " not within range (eps = " << res0.epsilon << ") of " << res0.right << "! (scarc_test_1D (PCG/RICH)) " << std::endl;
  else if(!res1.passed)
    std::cout << "FAILED: " << res1.left << " not within range (eps = " << res1.epsilon << ") of " << res1.right << "! (scarc_test_1D (PCG/RICH)) " << std::endl;
}

void check_scarc_rich_pcg_1D(Index rank)
{
  /* (0)  (1)
   *  *----*
   */

  std::vector<Attribute<double>, std::allocator<Attribute<double> > > attrs;
  attrs.push_back(Attribute<double>()); //vertex x-coords

  attrs.at(0).get_data().push_back(double(0));
  attrs.at(0).get_data().push_back(double(1));
  /*
   *  *--0-*
   *  0    1
   */

  //creating foundation mesh
  Mesh<Dim1D> m(0);

  m.add_polytope(pl_vertex);
  m.add_polytope(pl_vertex);

  m.add_polytope(pl_edge);

  m.add_adjacency(pl_vertex, pl_edge, 0, 0);
  m.add_adjacency(pl_vertex, pl_edge, 1, 0);

  std::vector<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> >, std::allocator<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> > > > halos;

  Mesh<Dim1D> m_fine(m);

  //set up halos

  Refinement<Mem::Main,
             Algo::Generic,
             mrt_standard,
             hrt_refine>::execute(m_fine, &halos, attrs);

  /*  *----*----*
   *      (2)
   */

  std::vector<Halo<0, PLVertex, Mesh<Dim1D> > > boundaries;
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.at(0).push_back(0);
  boundaries.at(1).push_back(1);

  auto p0(Partitioning<Mem::Main,
                       Algo::Generic,
                       Dim1D,
                       0,
                       pl_vertex>::execute(m_fine,
                                           boundaries,
                                           2, rank,
                                           attrs
                                           ));

  typedef ConformalMesh<Shape::Hypercube<1> > confmeshtype_;

  Index* size_set(new Index[2]);
  MeshControl<dim_1D>::fill_sizes(*((Mesh<Dim1D>*)(p0.submesh.get())), size_set);

  confmeshtype_ confmesh(size_set);
  MeshControl<dim_1D>::fill_adjacencies(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh);
  MeshControl<dim_1D>::fill_vertex_sets(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh, attrs.at(0));

  auto cell_sub_set(HaloInterface<0, Dim1D>::convert(p0.comm_halos.at(0).get()));

  Trafo::Standard::Mapping<ConformalMesh<Shape::Hypercube<1> > > trafo(confmesh);
  Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > space(trafo);
  VectorMirror<Mem::Main, double> target_mirror;
  Assembly::MirrorAssembler::assemble_mirror(target_mirror, space, cell_sub_set);

  std::vector<VectorMirror<Mem::Main, double> > mirrors;
  mirrors.push_back(std::move(target_mirror));

  std::vector<DenseVector<Mem::Main, double> > sendbufs;
  std::vector<DenseVector<Mem::Main, double> > recvbufs;
  DenseVector<Mem::Main, double> sbuf(mirrors.at(0).size());
  DenseVector<Mem::Main, double> rbuf(mirrors.at(0).size());
  sendbufs.push_back(std::move(sbuf));
  recvbufs.push_back(std::move(rbuf));

  std::vector<Index> other_ranks;
  other_ranks.push_back(p0.comm_halos.at(0)->get_other());

  std::vector<DenseVector<Mem::Main, double> > mirror_buffers;
  DenseVector<Mem::Main, double> mbuf(mirrors.at(0).size());
  mirror_buffers.push_back(std::move(mbuf));


  SparseMatrixCSR<Mem::Main, double> mat_sys;
  Assembly::SymbolicMatrixAssembler<>::assemble1(mat_sys, space);
  mat_sys.format();
  Cubature::DynamicFactory cubature_factory("gauss-legendre:2");
  Assembly::Common::LaplaceOperator laplace;
  Assembly::BilinearOperatorAssembler::assemble_matrix1(mat_sys, laplace, space, cubature_factory);

  std::vector<DenseVector<Mem::Main, double> > freq_buffers;
  DenseVector<Mem::Main, double> fbuf(mat_sys.rows());
  freq_buffers.push_back(std::move(fbuf));
  auto frequencies(HaloFrequencies<Mem::Main, Algo::Generic>::value(mirrors, mirror_buffers, freq_buffers));

  DenseVector<Mem::Main, double> vec_rhs(space.get_num_dofs(), double(0));
  Assembly::Common::ConstantFunction rhs_func(1.0);
  Assembly::Common::ForceFunctional<Assembly::Common::ConstantFunction> rhs_functional(rhs_func);
  Assembly::LinearFunctionalAssembler::assemble_vector(vec_rhs, rhs_functional, space, cubature_factory);

  Assembly::DirichletAssembler<Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > > dirichlet(space);
  auto bound_sub_set(HaloInterface<0, Dim1D>::convert(p0.boundaries.at(0)));
  dirichlet.add_cell_set(bound_sub_set);

  DenseVector<Mem::Main, double> vec_sol(space.get_num_dofs(), double(0));

  UnitFilter<Mem::Main, double> filter(space.get_num_dofs());
  dirichlet.assemble(filter);

  auto mat_localsys(MatrixConversion<Mem::Main, double, Index, SparseMatrixCSR>::value(mat_sys, mirrors, other_ranks));

  SparseMatrixCOO<Mem::Main, double> mat_precon_temp(mat_localsys.rows(), mat_localsys.columns());
  for(Index i(0) ; i < mat_localsys.rows() ; ++i)
    mat_precon_temp(i, i, double(0.75) * (double(1)/mat_localsys(i, i)));

  SparseMatrixCSR<Mem::Main, double> mat_precon(mat_precon_temp);

  ///filter system
  filter.filter_mat<Algo::Generic>(mat_sys);
  filter.filter_mat<Algo::Generic>(mat_localsys);
  filter.filter_rhs<Algo::Generic>(vec_rhs);
  filter.filter_sol<Algo::Generic>(vec_sol);
  filter.filter_mat<Algo::Generic>(mat_precon); //TODO: check if -> NO! we do this in the solver program when applying the correction filter after preconditioning

  SynchronisedPreconditionedFilteredScaRCData<double,
                                              Mem::Main,
                                              DenseVector<Mem::Main, double>,
                                              VectorMirror<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              UnitFilter<Mem::Main, double> > data(std::move(mat_sys), std::move(mat_precon), std::move(vec_sol), std::move(vec_rhs), std::move(filter));

  data.vector_mirrors() = std::move(mirrors);
  data.vector_mirror_sendbufs() = std::move(sendbufs);
  data.vector_mirror_recvbufs() = std::move(recvbufs);
  data.dest_ranks() = std::move(other_ranks);

#ifndef SERIAL
  Communicator c(MPI_COMM_WORLD);
#else
  Communicator c(0);
#endif
  data.communicators().push_back(std::move(c));

  //data.source_ranks() = std::move(sourceranks);
  data.localsys() = std::move(mat_localsys);

  data.halo_frequencies() = std::move(frequencies);

  ///layer 1 (global layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > solver(new ScaRCFunctorRichardson0<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 1 (global layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > block_smoother(new ScaRCFunctorPreconBlock<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_solver(new ScaRCFunctorPCG1<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_precon(new ScaRCFunctorPreconSpM1V1<double,
                                                                                              Mem::Main,
                                                                                              DenseVector<Mem::Main, double>,
                                                                                              VectorMirror<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              UnitFilter<Mem::Main, double>,
                                                                                              std::vector,
                                                                                              Index,
                                                                                              Algo::Generic>(data) );


  solver->reset_preconditioner(block_smoother);
  block_smoother->reset_preconditioner(local_solver);
  local_solver->reset_preconditioner(local_precon);

  solver->execute();
  std::cout << rank << ", #iters global RICH: " << solver->iterations() << std::endl;
  std::cout << rank << ", #iters local PCG: " << local_solver->iterations() << std::endl;

  TestResult<double, double, double> res0;
  TestResult<double, double, double> res1;
  res0 = test_check_equal_within_eps(data.sol()(0), rank == 0 ? 0. : 0.25, std::numeric_limits<double>::epsilon() * 1e8);
  res1 = test_check_equal_within_eps(data.sol()(1), rank == 0 ? 0.25 : 0., std::numeric_limits<double>::epsilon() * 1e8);
  if(res0.passed && res1.passed)
    std::cout << "PASSED (rank " << rank <<"): scarc_test_1D (RICH/PCG)" << std::endl;
  else if(!res0.passed)
    std::cout << "FAILED: " << res0.left << " not within range (eps = " << res0.epsilon << ") of " << res0.right << "! (scarc_test_1D (RICH/PCG)) " << std::endl;
  else if(!res1.passed)
    std::cout << "FAILED: " << res1.left << " not within range (eps = " << res1.epsilon << ") of " << res1.right << "! (scarc_test_1D (RICH/PCG)) " << std::endl;
}
void check_scarc_pcg_pcg_1D(Index rank)
{
  /* (0)  (1)
   *  *----*
   */

  std::vector<Attribute<double>, std::allocator<Attribute<double> > > attrs;
  attrs.push_back(Attribute<double>()); //vertex x-coords

  attrs.at(0).get_data().push_back(double(0));
  attrs.at(0).get_data().push_back(double(1));
  /*
   *  *--0-*
   *  0    1
   */

  //creating foundation mesh
  Mesh<Dim1D> m(0);

  m.add_polytope(pl_vertex);
  m.add_polytope(pl_vertex);

  m.add_polytope(pl_edge);

  m.add_adjacency(pl_vertex, pl_edge, 0, 0);
  m.add_adjacency(pl_vertex, pl_edge, 1, 0);

  std::vector<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> >, std::allocator<std::shared_ptr<HaloBase<Mesh<Dim1D>, double> > > > halos;

  Mesh<Dim1D> m_fine(m);

  //set up halos

  Refinement<Mem::Main,
             Algo::Generic,
             mrt_standard,
             hrt_refine>::execute(m_fine, &halos, attrs);

  /*  *----*----*
   *      (2)
   */

  std::vector<Halo<0, PLVertex, Mesh<Dim1D> > > boundaries;
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.push_back(std::move(Halo<0, PLVertex, Mesh<Dim1D> >(m_fine)));
  boundaries.at(0).push_back(0);
  boundaries.at(1).push_back(1);

  auto p0(Partitioning<Mem::Main,
                       Algo::Generic,
                       Dim1D,
                       0,
                       pl_vertex>::execute(m_fine,
                                           boundaries,
                                           2, rank,
                                           attrs
                                           ));

  typedef ConformalMesh<Shape::Hypercube<1> > confmeshtype_;

  Index* size_set(new Index[2]);
  MeshControl<dim_1D>::fill_sizes(*((Mesh<Dim1D>*)(p0.submesh.get())), size_set);

  confmeshtype_ confmesh(size_set);
  MeshControl<dim_1D>::fill_adjacencies(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh);
  MeshControl<dim_1D>::fill_vertex_sets(*((Mesh<Dim1D>*)(p0.submesh.get())), confmesh, attrs.at(0));

  auto cell_sub_set(HaloInterface<0, Dim1D>::convert(p0.comm_halos.at(0).get()));

  Trafo::Standard::Mapping<ConformalMesh<Shape::Hypercube<1> > > trafo(confmesh);
  Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > space(trafo);
  VectorMirror<Mem::Main, double> target_mirror;
  Assembly::MirrorAssembler::assemble_mirror(target_mirror, space, cell_sub_set);

  std::vector<VectorMirror<Mem::Main, double> > mirrors;
  mirrors.push_back(std::move(target_mirror));

  std::vector<DenseVector<Mem::Main, double> > sendbufs;
  std::vector<DenseVector<Mem::Main, double> > recvbufs;
  DenseVector<Mem::Main, double> sbuf(mirrors.at(0).size());
  DenseVector<Mem::Main, double> rbuf(mirrors.at(0).size());
  sendbufs.push_back(std::move(sbuf));
  recvbufs.push_back(std::move(rbuf));

  std::vector<Index> other_ranks;
  other_ranks.push_back(p0.comm_halos.at(0)->get_other());

  std::vector<DenseVector<Mem::Main, double> > mirror_buffers;
  DenseVector<Mem::Main, double> mbuf(mirrors.at(0).size());
  mirror_buffers.push_back(std::move(mbuf));


  SparseMatrixCSR<Mem::Main, double> mat_sys;
  Assembly::SymbolicMatrixAssembler<>::assemble1(mat_sys, space);
  mat_sys.format();
  Cubature::DynamicFactory cubature_factory("gauss-legendre:2");
  Assembly::Common::LaplaceOperator laplace;
  Assembly::BilinearOperatorAssembler::assemble_matrix1(mat_sys, laplace, space, cubature_factory);

  std::vector<DenseVector<Mem::Main, double> > freq_buffers;
  DenseVector<Mem::Main, double> fbuf(mat_sys.rows());
  freq_buffers.push_back(std::move(fbuf));
  auto frequencies(HaloFrequencies<Mem::Main, Algo::Generic>::value(mirrors, mirror_buffers, freq_buffers));

  DenseVector<Mem::Main, double> vec_rhs(space.get_num_dofs(), double(0));
  Assembly::Common::ConstantFunction rhs_func(1.0);
  Assembly::Common::ForceFunctional<Assembly::Common::ConstantFunction> rhs_functional(rhs_func);
  Assembly::LinearFunctionalAssembler::assemble_vector(vec_rhs, rhs_functional, space, cubature_factory);

  Assembly::DirichletAssembler<Space::Lagrange1::Element<Trafo::Standard::Mapping<Geometry::ConformalMesh<Shape::Hypercube<1> > > > > dirichlet(space);
  auto bound_sub_set(HaloInterface<0, Dim1D>::convert(p0.boundaries.at(0)));
  dirichlet.add_cell_set(bound_sub_set);

  DenseVector<Mem::Main, double> vec_sol(space.get_num_dofs(), double(0));

  UnitFilter<Mem::Main, double> filter(space.get_num_dofs());
  dirichlet.assemble(filter);

  auto mat_localsys(MatrixConversion<Mem::Main, double, Index, SparseMatrixCSR>::value(mat_sys, mirrors, other_ranks));

  SparseMatrixCOO<Mem::Main, double> mat_precon_temp(mat_localsys.rows(), mat_localsys.columns());
  for(Index i(0) ; i < mat_localsys.rows() ; ++i)
    mat_precon_temp(i, i, double(0.75) * (double(1)/mat_localsys(i, i)));

  SparseMatrixCSR<Mem::Main, double> mat_precon(mat_precon_temp);

  ///filter system
  filter.filter_mat<Algo::Generic>(mat_sys);
  filter.filter_mat<Algo::Generic>(mat_localsys);
  filter.filter_rhs<Algo::Generic>(vec_rhs);
  filter.filter_sol<Algo::Generic>(vec_sol);
  filter.filter_mat<Algo::Generic>(mat_precon); //TODO: check if -> NO! we do this in the solver program when applying the correction filter after preconditioning

  SynchronisedPreconditionedFilteredScaRCData<double,
                                              Mem::Main,
                                              DenseVector<Mem::Main, double>,
                                              VectorMirror<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              SparseMatrixCSR<Mem::Main, double>,
                                              UnitFilter<Mem::Main, double> > data(std::move(mat_sys), std::move(mat_precon), std::move(vec_sol), std::move(vec_rhs), std::move(filter));

  data.vector_mirrors() = std::move(mirrors);
  data.vector_mirror_sendbufs() = std::move(sendbufs);
  data.vector_mirror_recvbufs() = std::move(recvbufs);
  data.dest_ranks() = std::move(other_ranks);

#ifndef SERIAL
  Communicator c(MPI_COMM_WORLD);
#else
  Communicator c(0);
#endif
  data.communicators().push_back(std::move(c));

  //data.source_ranks() = std::move(sourceranks);
  data.localsys() = std::move(mat_localsys);

  data.halo_frequencies() = std::move(frequencies);

  ///layer 1 (global layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > solver(new ScaRCFunctorPCG0<double,
                                                                                Mem::Main,
                                                                                DenseVector<Mem::Main, double>,
                                                                                VectorMirror<Mem::Main, double>,
                                                                                SparseMatrixCSR<Mem::Main, double>,
                                                                                SparseMatrixCSR<Mem::Main, double>,
                                                                                UnitFilter<Mem::Main, double>,
                                                                                std::vector,
                                                                                Index,
                                                                                Algo::Generic>(data) );

  ///layer 1 (global layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > block_smoother(new ScaRCFunctorPreconBlock<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer)
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_solver(new ScaRCFunctorPCG1<double,
                                                                                       Mem::Main,
                                                                                       DenseVector<Mem::Main, double>,
                                                                                       VectorMirror<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       SparseMatrixCSR<Mem::Main, double>,
                                                                                       UnitFilter<Mem::Main, double>,
                                                                                       std::vector,
                                                                                       Index,
                                                                                       Algo::Generic>(data) );

  ///layer 0 (local layer), preconditioner
  std::shared_ptr<ScaRCFunctorBase<double,
                                   Mem::Main,
                                   DenseVector<Mem::Main, double>,
                                   VectorMirror<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   SparseMatrixCSR<Mem::Main, double>,
                                   UnitFilter<Mem::Main, double>,
                                   std::vector,
                                   Index,
                                   Algo::Generic> > local_precon(new ScaRCFunctorPreconSpM1V1<double,
                                                                                              Mem::Main,
                                                                                              DenseVector<Mem::Main, double>,
                                                                                              VectorMirror<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              SparseMatrixCSR<Mem::Main, double>,
                                                                                              UnitFilter<Mem::Main, double>,
                                                                                              std::vector,
                                                                                              Index,
                                                                                              Algo::Generic>(data) );


  solver->reset_preconditioner(block_smoother);
  block_smoother->reset_preconditioner(local_solver);
  local_solver->reset_preconditioner(local_precon);

  solver->execute();

  std::cout << rank << ", #iters global PCG: " << solver->iterations() << std::endl;
  std::cout << rank << ", #iters local PCG: " << local_solver->iterations() << std::endl;

  TestResult<double, double, double> res0;
  TestResult<double, double, double> res1;
  res0 = test_check_equal_within_eps(data.sol()(0), rank == 0 ? 0. : 0.25, std::numeric_limits<double>::epsilon());
  res1 = test_check_equal_within_eps(data.sol()(1), rank == 0 ? 0.25 : 0., std::numeric_limits<double>::epsilon());
  if(res0.passed && res1.passed)
    std::cout << "PASSED (rank " << rank <<"): scarc_test_1D (PCG/PCG)" << std::endl;
  else if(!res0.passed)
    std::cout << "FAILED: " << res0.left << " not within range (eps = " << res0.epsilon << ") of " << res0.right << "! (scarc_test_1D (PCG/PCG)) " << std::endl;
  else if(!res1.passed)
    std::cout << "FAILED: " << res1.left << " not within range (eps = " << res1.epsilon << ") of " << res1.right << "! (scarc_test_1D (PCG/PCG)) " << std::endl;
}

int main(int argc, char* argv[])
{
  int me(0);
#ifndef SERIAL
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &me);
#endif
  (void)argc;
  (void)argv;
  std::cout<<"CTEST_FULL_OUTPUT"<<std::endl;

#ifndef SERIAL
  check_scarc_rich_rich_1D((Index)me);
  check_scarc_pcg_rich_1D((Index)me);
  check_scarc_rich_pcg_1D((Index)me);
  check_scarc_pcg_pcg_1D((Index)me);
#else
  std::cout << "Parallel tests unavailable on sole process " << me << std::endl;
#endif

#ifndef SERIAL
  MPI_Finalize();
#endif

  return 0;
}
