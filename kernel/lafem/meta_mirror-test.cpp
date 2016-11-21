#include <test_system/test_system.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/power_vector.hpp>
#include <kernel/lafem/tuple_vector.hpp>
#include <kernel/lafem/tuple_mirror.hpp>
#include <kernel/lafem/power_mirror.hpp>
#include <kernel/lafem/vector_mirror.hpp>

using namespace FEAT;
using namespace FEAT::LAFEM;
using namespace FEAT::TestSystem;

template<typename MemType_, typename DataType_, typename IndexType_>
class MetaMirrorTest :
  public TestSystem::FullTaggedTest<MemType_, DataType_, IndexType_>
{
public:
  typedef DataType_ DataType;
  typedef IndexType_ IndexType;

  typedef DenseVector<Mem::Main, DataType, IndexType> BufferVector;

  typedef DenseVector<MemType_, DataType, IndexType> ScalarVector;
  typedef PowerVector<ScalarVector, 2> PowerVector2;
  typedef TupleVector<PowerVector2, ScalarVector> MetaVector;

  typedef VectorMirror<MemType_, DataType, IndexType> ScalarMirror;
  typedef PowerMirror<ScalarMirror, 2> PowerMirror2;
  typedef TupleMirror<PowerMirror2, ScalarMirror> MetaMirror;

  typedef SparseMatrixCSR<MemType_, DataType, IndexType> ScalarMatrix;

  MetaMirrorTest() :
    TestSystem::FullTaggedTest<MemType_, DataType_, IndexType_>("MetaMirrorTest")
  {
  }

  static ScalarMatrix gen_mir_x(IndexType m)
  {
    DenseVector<Mem::Main, IndexType> row_ptr(m+1);
    DenseVector<Mem::Main, IndexType> col_idx(m);
    DenseVector<Mem::Main, DataType> data(m, DataType(1));

    IndexType* rp(row_ptr.elements());
    for(IndexType i(0); i <= m; ++i)
      rp[i] = i;

    IndexType* ci(col_idx.elements());
    IndexType m2(m*(m-1));
    for(IndexType i(0); i < m; ++i)
      ci[i] = m2+i;

    return SparseMatrixCSR<Mem::Main, DataType, IndexType>(m, m*m, col_idx, data, row_ptr);
  }

  static ScalarMatrix gen_mir_y(IndexType m)
  {
    DenseVector<Mem::Main, IndexType> row_ptr(m+1);
    DenseVector<Mem::Main, IndexType> col_idx(m);
    DenseVector<Mem::Main, DataType> data(m, DataType(1));

    IndexType* rp(row_ptr.elements());
    for(IndexType i(0); i <= m; ++i)
      rp[i] = i;

    IndexType* ci(col_idx.elements());
    for(IndexType i(0); i < m; ++i)
      ci[i] = i;

    return SparseMatrixCSR<Mem::Main, DataType, IndexType>(m, m*m, col_idx, data, row_ptr);
  }

  virtual void run() const override
  {
    const DataType tol(Math::pow(Math::eps<DataType>(), DataType(0.7)));

    const Index m = 3;
    const Index n = m*m;

    // create the two mirror matrices
    ScalarMatrix gather_x(gen_mir_x(m));
    ScalarMatrix gather_y(gen_mir_y(m));

    // transpose for scatter
    ScalarMatrix scatter_x;
    scatter_x.transpose(gather_x);
    ScalarMatrix scatter_y;
    scatter_y.transpose(gather_y);

    // create the meta-mirrors
    MetaMirror mirror_x(PowerMirror2(ScalarMirror(gather_x.clone(), scatter_x.clone())), ScalarMirror(gather_x.clone(), scatter_x.clone()));
    MetaMirror mirror_y(PowerMirror2(ScalarMirror(gather_y.clone(), scatter_y.clone())), ScalarMirror(gather_y.clone(), scatter_y.clone()));

    // create meta-vectors
    MetaVector vec_x;
    vec_x.template at<0>().template at<0>() = ScalarVector(n, DataType(1));
    vec_x.template at<0>().template at<1>() = ScalarVector(n, DataType(2));
    vec_x.template at<1>() = ScalarVector(n, DataType(3));
    MetaVector vec_y;
    vec_y.template at<0>().template at<0>() = ScalarVector(n, DataType(-1));
    vec_y.template at<0>().template at<1>() = ScalarVector(n, DataType( 1));
    vec_y.template at<1>() = ScalarVector(n, DataType(-2));

    // create reference synced vectors
    MetaVector sync_x(vec_x.clone());
    MetaVector sync_y(vec_y.clone());
    for(Index i(0); i < m; ++i)
    {
      Index k(n-m+i);
      sync_x.template at<0>().template at<0>()(k, DataType_(0));
      sync_y.template at<0>().template at<0>()(i, DataType_(0));
      sync_x.template at<0>().template at<1>()(k, DataType_(3));
      sync_y.template at<0>().template at<1>()(i, DataType_(3));
      sync_x.template at<1>()(k, DataType_(1));
      sync_y.template at<1>()(i, DataType_(1));
    }

    // create two buffer-vectors
    BufferVector buf_x = mirror_x.create_buffer(vec_x);
    BufferVector buf_y = mirror_y.create_buffer(vec_y);

    // gather local vectors
    mirror_x.gather(buf_x, vec_x);
    mirror_y.gather(buf_y, vec_y);

    // scatter exchanged buffers
    mirror_x.scatter_axpy(vec_x, buf_y);
    mirror_y.scatter_axpy(vec_y, buf_x);

    // compute difference to reference
    vec_x.axpy(sync_x, vec_x, -DataType(1));
    vec_y.axpy(sync_y, vec_y, -DataType(1));

    // check difference norm
    TEST_CHECK_EQUAL_WITHIN_EPS(vec_x.norm2(), DataType_(0), tol);
    TEST_CHECK_EQUAL_WITHIN_EPS(vec_y.norm2(), DataType_(0), tol);
  }
};

MetaMirrorTest<Mem::Main, float, Index> meta_mirror_test_generic_float_index;
MetaMirrorTest<Mem::Main, double, Index> meta_mirror_test_generic_double_index;
