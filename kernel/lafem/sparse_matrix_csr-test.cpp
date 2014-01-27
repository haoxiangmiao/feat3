#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <test_system/test_system.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/util/binary_stream.hpp>

#include <sstream>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::TestSystem;

/**
* \brief Test class for the sparse matrix coo class.
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
class SparseMatrixCSRTest
  : public TaggedTest<Mem_, DT_>
{

public:

  SparseMatrixCSRTest()
    : TaggedTest<Mem_, DT_>("sparse_matrix_csr_test")
  {
  }

  virtual void run() const
  {
    SparseMatrixCOO<Mem::Main, DT_> a(10, 10);
    a(1,2,7);
    a.clear();
    a(1,2,7);
    a(5,5,2);
    SparseMatrixCSR<Mem_, DT_> b(a);
    TEST_CHECK_EQUAL(b.used_elements(), a.used_elements());
    TEST_CHECK_EQUAL(b.size(), a.size());
    TEST_CHECK_EQUAL(b.rows(), a.rows());
    TEST_CHECK_EQUAL(b.columns(), a.columns());
    TEST_CHECK_EQUAL(b(1, 2), a(1, 2));
    TEST_CHECK_EQUAL(b(5, 5), a(5, 5));

    SparseMatrixCSR<Mem_, DT_> bl(b.layout());
    TEST_CHECK_EQUAL(bl.used_elements(), b.used_elements());
    TEST_CHECK_EQUAL(bl.size(), b.size());
    TEST_CHECK_EQUAL(bl.rows(), b.rows());
    TEST_CHECK_EQUAL(bl.columns(), b.columns());

    bl = b.layout();
    TEST_CHECK_EQUAL(bl.used_elements(), b.used_elements());
    TEST_CHECK_EQUAL(bl.size(), b.size());
    TEST_CHECK_EQUAL(bl.rows(), b.rows());
    TEST_CHECK_EQUAL(bl.columns(), b.columns());

    SparseMatrixCSR<Mem_, DT_> z(b);
    TEST_CHECK_EQUAL(z.used_elements(), 2ul);
    TEST_CHECK_EQUAL(z.size(), a.size());
    TEST_CHECK_EQUAL(z.rows(), a.rows());
    TEST_CHECK_EQUAL(z.columns(), a.columns());
    TEST_CHECK_EQUAL(z(1, 2), a(1, 2));
    TEST_CHECK_EQUAL(z(5, 5), a(5, 5));

    SparseMatrixCSR<Mem_, DT_> c;
    c = b;
    TEST_CHECK_EQUAL(c.used_elements(), b.used_elements());
    TEST_CHECK_EQUAL(c(0,2), b(0,2));
    TEST_CHECK_EQUAL(c(1,2), b(1,2));
    TEST_CHECK_EQUAL(c, b);

    DenseVector<Mem_, Index> col_ind(c.used_elements(), c.col_ind());
    DenseVector<Mem_, DT_> val(c.used_elements(), c.val());
    DenseVector<Mem_, Index> row_ptr(c.rows() + 1, c.row_ptr());
    SparseMatrixCSR<Mem_, DT_> d(c.rows(), c.columns(), col_ind, val, row_ptr);
    TEST_CHECK_EQUAL(d, c);

    SparseMatrixCSR<Mem::Main, DT_> e(c);
    TEST_CHECK_EQUAL(e, c);
    e = c;
    TEST_CHECK_EQUAL(e, c);
    e = c.clone();
    TEST_CHECK_EQUAL(e, c);

    TEST_CHECK_NOT_EQUAL((std::size_t)e.val(), (std::size_t)c.val());

    SparseMatrixCOO<Mem::Main, DT_> fcoo(10, 10);
    for (Index row(0) ; row < fcoo.rows() ; ++row)
    {
      for (Index col(0) ; col < fcoo.columns() ; ++col)
      {
        if(row == col)
          fcoo(row, col, DT_(2));
        else if((row == col+1) || (row+1 == col))
          fcoo(row, col, DT_(-1));
      }
    }
    SparseMatrixCSR<Mem_, DT_> f(fcoo);

    BinaryStream bs;
    f.write_out(fm_csr, bs);
    bs.seekg(0);
    SparseMatrixCSR<Mem_, DT_> g(bs);
    TEST_CHECK_EQUAL(g, f);

    std::stringstream ts;
    f.write_out(fm_m, ts);
    SparseMatrixCOO<Mem::Main, DT_> i(fm_m, ts);
    TEST_CHECK_EQUAL(i, fcoo);

    f.write_out(fm_mtx, ts);
    SparseMatrixCOO<Mem::Main, DT_> j(fm_mtx, ts);
    TEST_CHECK_EQUAL(j, fcoo);
  }
};
SparseMatrixCSRTest<Mem::Main, float> cpu_sparse_matrix_csr_test_float;
SparseMatrixCSRTest<Mem::Main, double> cpu_sparse_matrix_csr_test_double;
#ifdef FEAST_GMP
SparseMatrixCSRTest<Mem::Main, mpf_class> cpu_sparse_matrix_csr_test_mpf_class;
#endif
#ifdef FEAST_BACKENDS_CUDA
SparseMatrixCSRTest<Mem::CUDA, float> cuda_sparse_matrix_csr_test_float;
SparseMatrixCSRTest<Mem::CUDA, double> cuda_sparse_matrix_csr_test_double;
#endif
