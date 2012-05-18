#include <kernel/base_header.hpp>
#include <test_system/test_system.hpp>

#include <kernel/foundation/topology.hpp>
#include<deque>

using namespace FEAST;
using namespace FEAST::TestSystem;

template<typename Tag_, typename IndexType_, template<typename, typename> class OT_, typename IT_>
class TopologyTest:
  public TaggedTest<Tag_, IndexType_>
{
  public:
    TopologyTest(const std::string & tag) :
      TaggedTest<Tag_, IndexType_>("TopologyTest<" + tag + ">")
    {
    }

    void run() const
    {
      Foundation::Topology<> t;

      t.push_back();

      TEST_CHECK_EQUAL(t.size(), 1ul);

      t.at(0).push_back(1123);
      t[0].push_back(878);
      TEST_CHECK_EQUAL(t.at(0).at(0), 1123ul);
      TEST_CHECK_EQUAL(t[0][1], 878ul);

      TEST_CHECK_EQUAL(t.size(), 1ul);

      Foundation::Topology<IndexType_, OT_, IT_> t2;
      t2.push_back();

      TEST_CHECK_EQUAL(t2.size(), 1ul);

      t2.at(0).push_back(1123);
      t2[0].push_back(878);
      TEST_CHECK_EQUAL(t2.at(0).at(0), 1123ul);
      TEST_CHECK_EQUAL(t2[0][1], 878ul);

      TEST_CHECK_EQUAL(t2.size(), 1ul);
    }
};
TopologyTest<Nil, unsigned long, std::vector, std::vector<unsigned long> > topology_test_cpu_v_v("std::vector, std::vector");
TopologyTest<Nil, unsigned long, std::deque, std::vector<unsigned long> > topology_test_cpu_d_v("std::deque, std::vector");
TopologyTest<Nil, unsigned long, std::vector, std::deque<unsigned long> > topology_test_cpu_v_d("std::vector, std::deque");
TopologyTest<Nil, unsigned long, std::deque, std::deque<unsigned long> > topology_test_cpu_d_d("std::deque, std::deque");

/*TopologyTest<Nil, unsigned long, std::vector, Foundation::DenseDataWrapper<15, unsigned long> > topology_test_cpu_v_ddw("std::vector, DV");
  TopologyTest<Nil, unsigned long, std::deque, Foundation::DenseDataWrapper<15, unsigned long> > topology_test_cpu_d_ddw("std::deque, DV");
  TopologyTest<Nil, unsigned long, std::list, Foundation::DenseDataWrapper<15, unsigned long> > topology_test_cpu_l_ddw("std::list, DV");*/
