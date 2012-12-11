// includes, FEAST
#include <kernel/lafem/defect.hpp>

#include <mkl.h>

using namespace FEAST;
using namespace FEAST::LAFEM;

void Defect<Algo::MKL>::value(DenseVector<Mem::Main, float> & r, const DenseVector<Mem::Main, float> & rhs, const SparseMatrixCSR<Mem::Main, float> & a, const DenseVector<Mem::Main, float> & b)
{
  MKL_INT rows = a.rows();
  char trans = 'N';
  mkl_cspblas_scsrgemv(&trans, &rows, (float *)a.val(), (MKL_INT*)a.row_ptr(), (MKL_INT*)a.col_ind(), (float *)b.elements(), r.elements());
  vsSub(r.size(), rhs.elements(), r.elements(), r.elements());
}

void Defect<Algo::MKL>::value(DenseVector<Mem::Main, double> & r, const DenseVector<Mem::Main, double> & rhs, const SparseMatrixCSR<Mem::Main, double> & a, const DenseVector<Mem::Main, double> & b)
{
  MKL_INT rows = a.rows();
  char trans = 'N';
  mkl_cspblas_dcsrgemv(&trans, &rows, (double *)a.val(), (MKL_INT*)a.row_ptr(), (MKL_INT*)a.col_ind(), (double *)b.elements(), r.elements());
  vdSub(r.size(), rhs.elements(), r.elements(), r.elements());
}
