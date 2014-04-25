#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <test_system/test_system.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/sparse_matrix_ell.hpp>
#include <kernel/util/binary_stream.hpp>

#include <cstdio>
#include <sstream>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::TestSystem;

/**
* \brief Test class for the sparse matrix ell class.
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
  typename Algo_,
  typename DT_,
  typename IT_>
class SparseMatrixELLTest
  : public FullTaggedTest<Mem_, Algo_, DT_, IT_>
{
public:
  SparseMatrixELLTest()
    : FullTaggedTest<Mem_, Algo_, DT_, IT_>("SparseMatrixELLTest")
  {
  }

  virtual void run() const
  {
    SparseMatrixELL<Mem_, DT_, IT_> zero1;
    SparseMatrixELL<Mem::Main, DT_, IT_> zero2;
    TEST_CHECK_EQUAL(zero1, zero2);

    SparseMatrixCOO<Mem::Main, DT_, IT_> a(10, 12);
    a(1,2,7);
    a.format();
    a(1,2,7);
    a(5,5,2);
    SparseMatrixELL<Mem_, DT_, IT_> b(a);
    TEST_CHECK_EQUAL(b.used_elements(), a.used_elements());
    TEST_CHECK_EQUAL(b.size(), a.size());
    TEST_CHECK_EQUAL(b.rows(), a.rows());
    TEST_CHECK_EQUAL(b.columns(), a.columns());
    TEST_CHECK_EQUAL(b(1, 2), a(1, 2));
    TEST_CHECK_EQUAL(b(5, 5), a(5, 5));

    SparseMatrixELL<Mem_, DT_, IT_> bl(b.layout());
    TEST_CHECK_EQUAL(bl.used_elements(), b.used_elements());
    TEST_CHECK_EQUAL(bl.size(), b.size());
    TEST_CHECK_EQUAL(bl.rows(), b.rows());
    TEST_CHECK_EQUAL(bl.columns(), b.columns());

    bl = b.layout();
    TEST_CHECK_EQUAL(bl.used_elements(), b.used_elements());
    TEST_CHECK_EQUAL(bl.size(), b.size());
    TEST_CHECK_EQUAL(bl.rows(), b.rows());
    TEST_CHECK_EQUAL(bl.columns(), b.columns());

    SparseMatrixELL<Mem_, DT_, IT_> z;
    z.convert(b);
    TEST_CHECK_EQUAL(z.used_elements(), 2ul);
    TEST_CHECK_EQUAL(z.size(), a.size());
    TEST_CHECK_EQUAL(z.rows(), a.rows());
    TEST_CHECK_EQUAL(z.columns(), a.columns());
    TEST_CHECK_EQUAL(z.stride(), b.stride());
    TEST_CHECK_EQUAL(z.num_cols_per_row(), b.num_cols_per_row());
    TEST_CHECK_EQUAL(z(1, 2), a(1, 2));
    TEST_CHECK_EQUAL(z(5, 5), a(5, 5));
    TEST_CHECK_EQUAL(z(1, 3), a(1, 3));

    SparseMatrixELL<Mem::Main, DT_, IT_> e;
    e.convert(b);
    TEST_CHECK_EQUAL(e, b);
    e.copy(b);
    TEST_CHECK_EQUAL(e, b);

    SparseMatrixELL<Mem_, DT_, IT_> c;
    c.clone(b);
    TEST_CHECK_EQUAL(c, b);
    TEST_CHECK_NOT_EQUAL((void*)c.Ax(), (void*)b.Ax());
    TEST_CHECK_EQUAL((void*)c.Aj(), (void*)b.Aj());
    c = b.clone(true);
    TEST_CHECK_EQUAL(c, b);
    TEST_CHECK_NOT_EQUAL((void*)c.Ax(), (void*)b.Ax());
    TEST_CHECK_NOT_EQUAL((void*)c.Aj(), (void*)b.Aj());

    decltype(b) y(b.layout());
    TEST_CHECK_EQUAL((void*)y.Arl(), (void*)b.Arl());


    SparseMatrixCOO<Mem::Main, DT_, IT_> fcoo(10, 10);
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
    SparseMatrixELL<Mem_, DT_, IT_> f(fcoo);

    BinaryStream bs;
    f.write_out(FileMode::fm_ell, bs);
    bs.seekg(0);
    SparseMatrixELL<Mem_, DT_, IT_> g(FileMode::fm_ell, bs);
    TEST_CHECK_EQUAL(g, f);

    std::stringstream ts;
    f.write_out(FileMode::fm_m, ts);
    SparseMatrixELL<Mem::Main, DT_, IT_> i(FileMode::fm_m, ts);
    TEST_CHECK_EQUAL(i, f);

    f.write_out(FileMode::fm_mtx, ts);
    SparseMatrixELL<Mem::Main, DT_, IT_> j(FileMode::fm_mtx, ts);
    TEST_CHECK_EQUAL(j, f);
  }
};
SparseMatrixELLTest<Mem::Main, NotSet, float, unsigned long> cpu_sparse_matrix_ell_test_float_ulong;
SparseMatrixELLTest<Mem::Main, NotSet, double, unsigned long> cpu_sparse_matrix_ell_test_double_ulong;
SparseMatrixELLTest<Mem::Main, NotSet, float, unsigned int> cpu_sparse_matrix_ell_test_float_uint;
SparseMatrixELLTest<Mem::Main, NotSet, double, unsigned int> cpu_sparse_matrix_ell_test_double_uint;
#ifdef FEAST_HAVE_QUADMATH
SparseMatrixELLTest<Mem::Main, NotSet, __float128, unsigned long> cpu_sparse_matrix_ell_test_float128_ulong;
SparseMatrixELLTest<Mem::Main, NotSet, __float128, unsigned int> cpu_sparse_matrix_ell_test_float128_uint;
#endif
#ifdef FEAST_BACKENDS_CUDA
SparseMatrixELLTest<Mem::CUDA, NotSet, float, unsigned long> cuda_sparse_matrix_ell_test_float_ulong;
SparseMatrixELLTest<Mem::CUDA, NotSet, double, unsigned long> cuda_sparse_matrix_ell_test_double_ulong;
SparseMatrixELLTest<Mem::CUDA, NotSet, float, unsigned int> cuda_sparse_matrix_ell_test_float_uint;
SparseMatrixELLTest<Mem::CUDA, NotSet, double, unsigned int> cuda_sparse_matrix_ell_test_double_uint;
#endif


template<
  typename Mem_,
  typename Algo_,
  typename DT_,
  typename IT_>
class SparseMatrixELLApplyTest
  : public FullTaggedTest<Mem_, Algo_, DT_, IT_>
{
public:
  SparseMatrixELLApplyTest()
    : FullTaggedTest<Mem_, Algo_, DT_, IT_>("SparseMatrixELLApplyTest")
  {
  }

  virtual void run() const
  {
    DT_ s(DT_(4711.1));
    for (Index size(1) ; size < 1e3 ; size*=2)
    {
      SparseMatrixCOO<Mem::Main, DT_, IT_> a_local(size, size);
      DenseVector<Mem::Main, DT_, IT_> x_local(size);
      DenseVector<Mem::Main, DT_, IT_> y_local(size);
      DenseVector<Mem::Main, DT_, IT_> ref_local(size);
      DenseVector<Mem_, DT_, IT_> ref(size);
      DenseVector<Mem::Main, DT_, IT_> result_local(size);
      for (Index i(0) ; i < size ; ++i)
      {
        x_local(i, DT_(i % 100 * DT_(1.234)));
        y_local(i, DT_(2 - DT_(i % 42)));
      }
      DenseVector<Mem_, DT_, IT_> x(size);
      x.copy(x_local);
      DenseVector<Mem_, DT_, IT_> y(size);
      y.copy(y_local);

      Index ue(0);
      for (Index row(0) ; row < a_local.rows() ; ++row)
      {
        for (Index col(0) ; col < a_local.columns() ; ++col)
        {
          if(row == col)
          {
            a_local(row, col, DT_(2));
            ++ue;
          }
          else if((row == col+1) || (row+1 == col))
          {
            a_local(row, col, DT_(-1));
            ++ue;
          }
        }
      }
      SparseMatrixELL<Mem_,DT_, IT_> a(a_local);

      DenseVector<Mem_, DT_, IT_> r(size);
      //r.template axpy<Algo_>(s, a, x, y);
      a.template apply<Algo_>(r, x, y, s);
      result_local.copy(r);

      //ref.template product_matvec<Algo_>(a, x);
      a.template apply<Algo_>(ref, x);
      ref.template scale<Algo_>(ref, s);
      ref.template axpy<Algo_>(ref, y);
      ref_local.copy(ref);

      for (Index i(0) ; i < size ; ++i)
        TEST_CHECK_EQUAL_WITHIN_EPS(result_local(i), ref_local(i), 1e-2);
    }
  }
};
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, float, unsigned long> sm_ell_apply_test_float_ulong;
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, double, unsigned long> sm_ell_apply_test_double_ulong;
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, float, unsigned int> sm_ell_apply_test_float_uint;
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, double, unsigned int> sm_ell_apply_test_double_uint;
#ifdef FEAST_HAVE_QUADMATH
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, __float128, unsigned long> sm_ell_apply_test_float128_ulong;
SparseMatrixELLApplyTest<Mem::Main, Algo::Generic, __float128, unsigned int> sm_ell_apply_test_float128_uint;
#endif
#ifdef FEAST_BACKENDS_CUDA
SparseMatrixELLApplyTest<Mem::CUDA, Algo::CUDA, float, unsigned long> cuda_sm_ell_apply_test_float_ulong;
SparseMatrixELLApplyTest<Mem::CUDA, Algo::CUDA, double, unsigned long> cuda_sm_ell_apply_test_double_ulong;
SparseMatrixELLApplyTest<Mem::CUDA, Algo::CUDA, float, unsigned int> cuda_sm_ell_apply_test_float_uint;
SparseMatrixELLApplyTest<Mem::CUDA, Algo::CUDA, double, unsigned int> cuda_sm_ell_apply_test_double_uint;
#endif

template<
  typename Mem_,
  typename Algo_,
  typename DT_,
  typename IT_>
class SparseMatrixELLScaleTest
  : public FullTaggedTest<Mem_, Algo_, DT_, IT_>
{
public:
  SparseMatrixELLScaleTest()
    : FullTaggedTest<Mem_, Algo_, DT_, IT_>("SparseMatrixELLScaleTest")
  {
  }

  virtual void run() const
  {
    for (Index size(2) ; size < 3e2 ; size*=2)
    {
      DT_ s(DT_(4.321));

      SparseMatrixCOO<Mem::Main, DT_, IT_> a_local(size, size + 2);
      SparseMatrixCOO<Mem::Main, DT_, IT_> ref_local(size, size + 2);
      for (Index row(0) ; row < a_local.rows() ; ++row)
      {
        for (Index col(0) ; col < a_local.columns() ; ++col)
        {
          if(row == col)
            a_local(row, col, DT_(2));
          else if((row == col+1) || (row+1 == col))
            a_local(row, col, DT_(-1));

          if(row == col)
            ref_local(row, col, DT_(2) * s);
          else if((row == col+1) || (row+1 == col))
            ref_local(row, col, DT_(-1) * s);
        }
      }

      SparseMatrixELL<Mem_, DT_, IT_> a(a_local);
      SparseMatrixELL<Mem_, DT_, IT_> b(a.clone());

      b.template scale<Algo_>(a, s);
      SparseMatrixCOO<Mem::Main, DT_, IT_> b_local(b);
      TEST_CHECK_EQUAL(b_local, ref_local);

      a.template scale<Algo_>(a, s);
      SparseMatrixCOO<Mem_, DT_, IT_> a_coo(a);
      a_local.convert(a_coo);
      TEST_CHECK_EQUAL(a_local, ref_local);
    }
  }
};
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, float, unsigned int> sm_ell_scale_test_float_uint;
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, double, unsigned int> sm_ell_scale_test_double_uint;
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, float, unsigned long> sm_ell_scale_test_float_ulong;
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, double, unsigned long> sm_ell_scale_test_double_ulong;
#ifdef FEAST_HAVE_QUADMATH
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, __float128, unsigned int> sm_ell_scale_test_float128_uint;
SparseMatrixELLScaleTest<Mem::Main, Algo::Generic, __float128, unsigned long> sm_ell_scale_test_float128_ulong;
#endif
#ifdef FEAST_BACKENDS_MKL
SparseMatrixELLScaleTest<Mem::Main, Algo::MKL, float, unsigned int> mkl_sm_ell_scale_test_float_uint;
SparseMatrixELLScaleTest<Mem::Main, Algo::MKL, double, unsigned int> mkl_sm_ell_scale_test_double_uint;
SparseMatrixELLScaleTest<Mem::Main, Algo::MKL, float, unsigned long> mkl_sm_ell_scale_test_float_ulong;
SparseMatrixELLScaleTest<Mem::Main, Algo::MKL, double, unsigned long> mkl_sm_ell_scale_test_double_ulong;
#endif
#ifdef FEAST_BACKENDS_CUDA
SparseMatrixELLScaleTest<Mem::CUDA, Algo::CUDA, float, unsigned int> cuda_sm_ell_scale_test_float_uint;
SparseMatrixELLScaleTest<Mem::CUDA, Algo::CUDA, double, unsigned int> cuda_sm_ell_scale_test_double_uint;
SparseMatrixELLScaleTest<Mem::CUDA, Algo::CUDA, float, unsigned long> cuda_sm_ell_scale_test_float_ulong;
SparseMatrixELLScaleTest<Mem::CUDA, Algo::CUDA, double, unsigned long> cuda_sm_ell_scale_test_double_ulong;
#endif
