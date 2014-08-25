#include <test_system/test_system.hpp>
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/lafem/sparse_vector.hpp>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::TestSystem;

/**
* \brief Test class for the sparse vector class.
*
* \test test description missing
*
* \tparam Mem_
* description missing
*
* \tparam DT_
* description missing
*
* \author Dirk Ribbrock
*/
template<
  typename Mem_,
  typename DT_>
class SparseVectorTest
  : public TaggedTest<Mem_, DT_>
{
public:
  SparseVectorTest()
    : TaggedTest<Mem_, DT_>("SparseVectorTest")
  {
  }

  virtual void run() const
  {
    SparseVector<Mem_, DT_> zero1;
    SparseVector<Mem::Main, DT_> zero2;
    TEST_CHECK_EQUAL(zero1, zero2);

    SparseVector<Mem_, DT_> a(10);
    a(3, DT_(7));
    a(3, DT_(3));
    a(6, DT_(1));
    a(5, DT_(6));
    a(6, DT_(8));
    TEST_CHECK_EQUAL(a.used_elements(), Index(3));
    TEST_CHECK_EQUAL(a(3), DT_(3));
    TEST_CHECK_EQUAL(a(2), DT_(0));
    TEST_CHECK_EQUAL(a(5), DT_(6));
    TEST_CHECK_EQUAL(a(6), DT_(8));

    std::stringstream ts;
    a.write_out(FileMode::fm_mtx, ts);
    SparseVector<Mem::Main, DT_> j(FileMode::fm_mtx, ts);
    TEST_CHECK_EQUAL(j, a);

    SparseVector<Mem_, DT_> b;
    b.convert(a);
    TEST_CHECK_EQUAL(a, b);
    b(6, DT_(1));
    TEST_CHECK_NOT_EQUAL(a, b);
    b.clone(a);
    b(6, DT_(3));
    TEST_CHECK_NOT_EQUAL(a, b);
    TEST_CHECK_NOT_EQUAL((void*)a.elements(), (void*)b.elements());
    TEST_CHECK_NOT_EQUAL((void*)a.indices(), (void*)b.indices());
    b = a.clone();
    TEST_CHECK_NOT_EQUAL((void*)a.elements(), (void*)b.elements());
    TEST_CHECK_NOT_EQUAL((void*)a.indices(), (void*)b.indices());

    SparseVector<Mem::Main, float, unsigned int> c;
    c.convert(a);
    SparseVector<Mem::Main, float, unsigned int> d;
    d.clone(c);
    SparseVector<Mem::Main, float, unsigned int> e;
    e.convert(a);
    TEST_CHECK_EQUAL(d, e);
    c(6, DT_(1));
    TEST_CHECK_NOT_EQUAL(c, e);

    a.format();
    TEST_CHECK_EQUAL(a.used_elements(), Index(3));
    TEST_CHECK_EQUAL(a(2), DT_(0));
    TEST_CHECK_EQUAL(a(3), DT_(0));
  }
};
SparseVectorTest<Mem::Main, float> cpu_sparse_vector_test_float;
SparseVectorTest<Mem::Main, double> cpu_sparse_vector_test_double;
//SparseVectorTest<Mem::Main, Index> cpu_sparse_vector_test_index;
#ifdef FEAST_BACKENDS_CUDA
SparseVectorTest<Mem::CUDA, float> cuda_sparse_vector_test_float;
SparseVectorTest<Mem::CUDA, double> cuda_sparse_vector_test_double;
//SparseVectorTest<Mem::CUDA, Index> cuda_sparse_vector_test_index;
#endif
