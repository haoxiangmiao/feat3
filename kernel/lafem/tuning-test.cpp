#include <test_system/test_system.hpp>
#include <kernel/base_header.hpp>
#include <kernel/lafem/tuning.hpp>
#include <kernel/archs.hpp>
#include <kernel/lafem/pointstar_factory.hpp>

using namespace FEAT;
using namespace FEAT::LAFEM;
using namespace FEAT::TestSystem;

/**
 * \brief Test class for the tuning class.
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
  typename DT_,
  typename IT_>
class TuningTest
  : public FullTaggedTest<Mem_, DT_, IT_>
{
public:
  TuningTest()
    : FullTaggedTest<Mem_, DT_, IT_>("TuningTest")
  {
  }

  virtual void run() const override
  {
    // create a pointstar factory
    PointstarFactoryFD<double> psf(270);

    // create a CSR matrix
    SparseMatrixELL<Mem_, DT_, IT_> mat_sys(psf.matrix_csr());

    Tuning::tune_cuda_blocksize(mat_sys);
  }
};
#ifndef FEAT_DEBUG_MODE
#ifdef FEAT_BACKENDS_CUDA
TuningTest<Mem::CUDA, float, unsigned long> cuda_tuning_test_float_ulong;
#endif
#endif
