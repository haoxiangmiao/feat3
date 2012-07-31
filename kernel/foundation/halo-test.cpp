#include <kernel/base_header.hpp>
#include <test_system/test_system.hpp>

#include <kernel/foundation/mesh.hpp>
#include <kernel/foundation/halo.hpp>
#include <kernel/archs.hpp>
#include<deque>

using namespace FEAST;
using namespace FEAST::TestSystem;

template<typename Tag_,
         typename IndexType_,
         template<typename, typename> class OT_, typename IT_>
class HaloTest:
  public TaggedTest<Tag_, IndexType_>
{
  public:
    HaloTest(const std::string & tag) :
      TaggedTest<Tag_, IndexType_>("HaloTest<" + tag + ">")
    {
    }

    void run() const
    {

      //##################################################################
      //     0  1
      //   0--1--2     *--*--*
      // 2 | 3|  |4    | 0| 1|
      //   3--4--5     *--*--*
      //    5  6

      Foundation::Mesh<Foundation::rnt_2D, Foundation::Topology<IndexType_, OT_, IT_> > m3(0);

      //configure attribute
      unsigned my_attribute_index(Foundation::MeshAttributeRegistration<Foundation::Mesh<Foundation::rnt_2D, Foundation::Topology<IndexType_, OT_, IT_> >, double>::execute(m3, Foundation::pl_vertex));

      //add vertices
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_polytope(Foundation::pl_vertex);
      m3.add_attribute_value(my_attribute_index, double(0));
      m3.add_attribute_value(my_attribute_index, double(0.5));
      m3.add_attribute_value(my_attribute_index, double(1));
      m3.add_attribute_value(my_attribute_index, double(0));
      m3.add_attribute_value(my_attribute_index, double(0.5));
      m3.add_attribute_value(my_attribute_index, double(1));

      //add edges
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);
      m3.add_polytope(Foundation::pl_edge);

      //add faces
      m3.add_polytope(Foundation::pl_face);
      m3.add_polytope(Foundation::pl_face);

      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 0, 0); //v->e is set automagically
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 0, 1);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 1, 1);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 1, 2);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 2, 0);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 2, 3);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 3, 1);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 3, 4);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 4, 2);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 4, 5);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 5, 3);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 5, 4);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 6, 4);
      m3.add_adjacency(Foundation::pl_edge, Foundation::pl_vertex, 6, 5);

      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 0, 0);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 0, 2);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 0, 3);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 0, 5);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 1, 1);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 1, 3);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 1, 4);
      m3.add_adjacency(Foundation::pl_face, Foundation::pl_edge, 1, 6);

      //clone mesh
      Foundation::Mesh<Foundation::rnt_2D, Foundation::Topology<IndexType_, OT_, IT_> > m4(1, m3);

      //init simple halo
      Foundation::Halo<0, Foundation::Mesh<Foundation::rnt_2D, Foundation::Topology<IndexType_, OT_, IT_> > > h(m3, 1);

      //add connections
      //
      // *--*--*
      // |0 | 1| m3
      // *--*--*
      //  5   6
      //  |   |
      //  0   1
      // *--*--*
      // |0 | 1| m4
      // *--*--*

      h.add_halo_element_pair(5u, 0u);
      h.add_halo_element_pair(6u, 1u);

      TEST_CHECK_EQUAL(h.size(), 2u);
      TEST_CHECK_EQUAL(h.get_element(0u), 5u);
      TEST_CHECK_EQUAL(h.get_element(1u), 6u);
      TEST_CHECK_EQUAL(h.get_element_counterpart(0u), 0u);
      TEST_CHECK_EQUAL(h.get_element_counterpart(1u), 1u);
    }
};
HaloTest<Archs::None, Index, std::vector, std::vector<Index> > halo_test_cpu_v_v("std::vector, std::vector");
HaloTest<Archs::None, Index, std::deque, std::vector<Index> > halo_test_cpu_d_v("std::deque, std::vector");
HaloTest<Archs::None, Index, std::vector, std::deque<Index> > halo_test_cpu_v_d("std::vector, std::deque");
HaloTest<Archs::None, Index, std::deque, std::deque<Index> > halo_test_cpu_d_d("std::deque, std::deque");
