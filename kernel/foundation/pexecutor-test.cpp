#include<kernel/base_header.hpp>
#include<test_system/test_system.hpp>

#ifdef FEAST_HAVE_PARMETIS
#include<kernel/archs.hpp>
#include<kernel/foundation/pexecutor.hpp>
#include<kernel/foundation/psynch.hpp>
#include<kernel/geometry/index_calculator.hpp>

using namespace FEAST;
using namespace FEAST::TestSystem;
using namespace FEAST::Foundation;
using namespace FEAST::Geometry;


template<typename Tag_= Mem::Main, typename IndexType_ = Index, typename DataType = float>
class PExecutorParmetisTest:
  public TaggedTest<Tag_, IndexType_>
{
  public:
    PExecutorParmetisTest(const std::string & tag) :
      TaggedTest<Tag_, IndexType_>("PExecutorParmetisTest<" + tag + ">")
    {
    }

    virtual void run() const override
    {
      Communicator comm( Communicator(MPI_COMM_WORLD) );
      PGraphParmetis pg(2, 1, 2, comm);

      //evoke-test creator function for dual graph
      // *--0--*--1--*
      // 0     1     2
      typedef ConformalMesh<Shape::Hypercube<1> > ConfmeshType1D;
      IndexType_* sizes = new IndexType_[2];
      sizes[0] = 3;
      sizes[1] = 2;
      ConfmeshType1D mesh(sizes);
      typename ConfmeshType1D::template IndexSet<1, 0>::Type& target_vertex_at_edge(mesh.template get_index_set<1, 0>());
      target_vertex_at_edge[0][0] = 0;
      target_vertex_at_edge[0][1] = 1;
      target_vertex_at_edge[1][0] = 1;
      target_vertex_at_edge[1][1] = 2;

      PGraphParmetis global_dual(mesh, 2, Communicator(MPI_COMM_WORLD));
      std::shared_ptr<PGraphBase<idx_t> > local_dual(global_dual.create_local());

      auto part(PExecutorParmetis<ParmetisModePartKway>::part(*((PGraphParmetis*)local_dual.get())));

      TEST_CHECK_EQUAL(part.get()[0], 0);
      TEST_CHECK_EQUAL(part.get()[1], 0);
    }
};
PExecutorParmetisTest<> have_parmetis_test("None, Index, float");
#endif