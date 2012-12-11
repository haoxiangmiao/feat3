#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <test_system/test_system.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/component_product.hpp>
#include <kernel/lafem/algorithm.hpp>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::TestSystem;

template<
  typename Arch_,
  typename Algo_,
  typename DT_>
class DVComponentProductTest
  : public TaggedTest<Arch_, DT_>
{

public:

  DVComponentProductTest()
    : TaggedTest<Arch_, DT_>("dv_component_product_test")
  {
  }

  virtual void run() const
  {
    for (Index size(1) ; size < 1e5 ; size*=2)
    {
      DenseVector<Mem::Main, DT_> a_local(size);
      DenseVector<Mem::Main, DT_> b_local(size);
      DenseVector<Mem::Main, DT_> ref(size);
      DenseVector<Mem::Main, DT_> ref2(size);
      DenseVector<Mem::Main, DT_> result_local(size);
      for (Index i(0) ; i < size ; ++i)
      {
        a_local(i, DT_(i * DT_(1.234)));
        b_local(i, DT_(size*2 - i));
        ref(i, a_local(i) * b_local(i));
        ref2(i, a_local(i) * a_local(i));
      }

      DenseVector<Arch_, DT_> a(size);
      copy(a, a_local);
      DenseVector<Arch_, DT_> b(size);
      copy(b, b_local);
      DenseVector<Arch_, DT_> c(size);

      ComponentProduct<Algo_>::value(c, a, b);
      copy(result_local, c);
      TEST_CHECK_EQUAL(result_local, ref);

      ComponentProduct<Algo_>::value(a, a, b);
      copy(result_local, a);
      TEST_CHECK_EQUAL(result_local, ref);

      copy(a, a_local);
      ComponentProduct<Algo_>::value(b, a, b);
      copy(result_local, b);
      TEST_CHECK_EQUAL(result_local, ref);

      copy(b, b_local);
      ComponentProduct<Algo_>::value(a, a, a);
      copy(result_local, a);
      TEST_CHECK_EQUAL(result_local, ref2);
    }
  }
};
DVComponentProductTest<Mem::Main, Algo::Generic, float> dv_component_product_test_float;
DVComponentProductTest<Mem::Main, Algo::Generic, double> dv_component_product_test_double;
#ifdef FEAST_BACKENDS_MKL
DVComponentProductTest<Mem::Main, Algo::MKL, float> mkl_dv_component_product_test_float;
DVComponentProductTest<Mem::Main, Algo::MKL, double> mkl_dv_component_product_test_double;
#endif
#ifdef FEAST_BACKENDS_CUDA
DVComponentProductTest<Mem::CUDA, Algo::CUDA, float> cuda_dv_component_product_test_float;
DVComponentProductTest<Mem::CUDA, Algo::CUDA, double> cuda_dv_component_product_test_double;
#endif
