#pragma once
#ifndef KERNEL_LAFEM_AXPY_HPP
#define KERNEL_LAFEM_AXPY_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/util/exception.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>


namespace FEAST
{
  namespace LAFEM
  {
    template <typename Algo_>
    struct Axpy
    {
    };

    /**
     * \brief Axpy calculations.
     *
     * This class calculates axpy operations.
     *
     * \author Dirk Ribbrock
     */
    template <>
    struct Axpy <Algo::Generic>
    {
      /**
       * \brief Calculate \f$r \leftarrow \alpha x + y\f$
       *
       * \param[out] r The axpy result.
       * \param[in] a A scalar to scale x with.
       * \param[in] x The vector to be scaled.
       * \param[in] y The other vector
       */
      template <typename DT_>
      static void value(DenseVector<Mem::Main, DT_> & r, const DT_ a, const DenseVector<Mem::Main, DT_> & x, const DenseVector<Mem::Main, DT_> & y)
      {
        if (x.size() != y.size())
          throw InternalError("Vector size does not match!");
        if (x.size() != r.size())
          throw InternalError("Vector size does not match!");

        const DT_ * xp(x.elements());
        const DT_ * yp(y.elements());
        DT_ * rp(r.elements());
        const Index size(r.size());

        if (rp == yp)
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] += a * xp[i];
          }
        }
        else if (rp == xp)
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] *= a;
            rp[i]+= yp[i];
          }
        }
        else
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] = a * xp[i] + yp[i];
          }
        }
      }

      /**
       * \brief Calculate \f$r \leftarrow \alpha x + y\f$
       *
       * \param[out] r The axpy result.
       * \param[in] a A vector to scale x with.
       * \param[in] x The vector to be scaled.
       * \param[in] y The other vector
       */
      template <typename DT_>
      static void value(DenseVector<Mem::Main, DT_> & r, const DenseVector<Mem::Main, DT_> & a, const DenseVector<Mem::Main, DT_> & x, const DenseVector<Mem::Main, DT_> & y)
      {
        if (x.size() != y.size())
          throw InternalError("Vector size does not match!");
        if (x.size() != r.size())
          throw InternalError("Vector size does not match!");
        if (a.size() != r.size())
          throw InternalError("Vector size does not match!");

        const DT_ * ap(a.elements());
        const DT_ * xp(x.elements());
        const DT_ * yp(y.elements());
        DT_ * rp(r.elements());
        const Index size(r.size());

        if (rp == yp)
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] += ap[i] * xp[i];
          }
        }
        else if(rp == xp)
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] *= ap[i];
            rp[i]+= yp[i];
          }
        }
        else if(rp == ap)
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] *= xp[i];
            rp[i] += yp[i];
          }
        }
        else
        {
          for (Index i(0) ; i < size ; ++i)
          {
            rp[i] = ap[i] * xp[i] + yp[i];
          }
        }
      }

      /**
       * \brief Calculate \f$r \leftarrow \alpha \cdot P \cdot x + y\f$
       *
       * \param[out] r The axpy result.
       * \param[in] a A scalar to scale Px with.
       * \param[in] x The vector to be multiplied and scaled.
       * \param[in] P The matrix to be multiplied and scaled.
       * \param[in] y The other vector
       */
      template <typename DT_>
      static void value(DenseVector<Mem::Main, DT_> & r, const DT_ a, const SparseMatrixCSR<Mem::Main, DT_> & P, const DenseVector<Mem::Main, DT_> & x, const DenseVector<Mem::Main, DT_> & y)
      {
        //ProductMatVec<Algo::Generic>::value(r, P, x);
        //Scale<Algo::Generic>::value(r, r, a);
        //Sum<Algo::Generic>::value(r, r, y);

        if (x.size() != P.columns())
          throw InternalError("Vector size does not match!");
        if (P.rows() != r.size())
          throw InternalError("Vector size does not match!");
        if (y.size() != r.size())
          throw InternalError("Vector size does not match!");

        const DT_ * xp(x.elements());
        const DT_ * yp(y.elements());
        const Index * col_ind(P.col_ind());
        const DT_ * val(P.val());
        const Index * row_ptr(P.row_ptr());
        const Index * row_ptr_end(P.row_ptr_end());
        DT_ * rp(r.elements());
        const Index rows(P.rows());

        for (Index row(0) ; row < rows ; ++row)
        {
          DT_ sum(0);
          const Index end(row_ptr_end[row]);
          for (Index i(row_ptr[row]) ; i < end ; ++i)
          {
            sum += val[i] * xp[col_ind[i]];
          }
          rp[row] = (sum * a)+ yp[row];
        }
      }
    };

    template <>
    struct Axpy <Algo::CUDA>
    {
      template <typename DT_>
      static void value(DenseVector<Mem::CUDA, DT_> & r, const DT_ a, const DenseVector<Mem::CUDA, DT_> & x, const DenseVector<Mem::CUDA, DT_> & y);

      template <typename DT_>
      static void value(DenseVector<Mem::CUDA, DT_> & r, const DenseVector<Mem::CUDA, DT_> & a, const DenseVector<Mem::CUDA, DT_> & x, const DenseVector<Mem::CUDA, DT_> & y);

      template <typename DT_>
      static void value(DenseVector<Mem::CUDA, DT_> & r, const DT_ a, const SparseMatrixCSR<Mem::CUDA, DT_> & P, const DenseVector<Mem::CUDA, DT_> & x, const DenseVector<Mem::CUDA, DT_> & y);
    };

  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_AXPY_HPP
