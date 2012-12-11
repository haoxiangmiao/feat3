#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <test_system/test_system.hpp>
#include <kernel/lafem/dense_matrix.hpp>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::TestSystem;

/**
* \brief Test class for the dense matrix class.
*
* \test test description missing
*
* \tparam Tag_
* description missing
*
* \tparam DT_
* description missing
*
* \author Dirk Ribbrock
*/
template<
  typename Tag_,
  typename DT_>
class DenseMatrixTest
  : public TaggedTest<Tag_, DT_>
{

public:

  DenseMatrixTest()
    : TaggedTest<Tag_, DT_>("dense_matrix_test")
  {
  }

  virtual void run() const
  {
    DenseMatrix<Tag_, DT_> a(10, 10);
    DenseMatrix<Tag_, DT_> b(10, 10, 5.);
    b(7, 6, DT_(42));
    DenseMatrix<Tag_, DT_> c(b);
    TEST_CHECK_EQUAL(c.size(), b.size());
    TEST_CHECK_EQUAL(c.rows(), b.rows());
    TEST_CHECK_EQUAL(c(7,6), b(7,6));
    TEST_CHECK_EQUAL(c, b);

    DenseMatrix<Tag_, DT_> e(11, 12, 5.);
    TEST_CHECK_EQUAL(e.rows(), 11ul);
    TEST_CHECK_EQUAL(e.columns(), 12ul);

    DenseMatrix<Tag_, DT_> f(11, 12, 42.);
    f = e;
    TEST_CHECK_EQUAL(f(7,8), e(7,8));
    TEST_CHECK_EQUAL(f, e);

    DenseMatrix<Mem::Main, DT_> g(f);
    DenseMatrix<Mem::Main, DT_> h;
    h = f;
    TEST_CHECK_EQUAL(g, f);
    TEST_CHECK_EQUAL(h, g);
    TEST_CHECK_EQUAL(h, f);

    h = f.clone();
    TEST_CHECK_EQUAL(h, f);
    h(1,2,3);
    TEST_CHECK_NOT_EQUAL(h, f);
    TEST_CHECK_NOT_EQUAL((unsigned long)h.elements(), (unsigned long)f.elements());
  }
};
DenseMatrixTest<Mem::Main, float> cpu_dense_matrix_test_float;
DenseMatrixTest<Mem::Main, double> cpu_dense_matrix_test_double;
#ifdef FEAST_BACKENDS_CUDA
DenseMatrixTest<Mem::CUDA, float> cuda_dense_matrix_test_float;
DenseMatrixTest<Mem::CUDA, double> cuda_dense_matrix_test_double;
#endif
