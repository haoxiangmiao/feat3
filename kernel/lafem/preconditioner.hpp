#pragma once
#ifndef KERNEL_LAFEM_PRECONDITIONER_HPP
#define KERNEL_LAFEM_PRECONDITIONER_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/util/exception.hpp>
#include <kernel/util/math.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/dense_matrix.hpp>
#include <vector>

namespace FEAST
{
  namespace LAFEM
  {
    /**
     * Supported sparse precon types.
     */
    enum class SparsePreconType
    {
      pt_none = 0,
      pt_file,
      pt_jacobi,
      pt_gauss_seidel,
      pt_polynomial,
      pt_ilu,
      pt_sor,
      pt_ssor,
      pt_spai
    };

    /**
     * \brief Preconditioner base class
     *
     * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
     *
     */
    template <typename Algo_, typename MT_, typename VT_>
    class Preconditioner
    {
    public:
      virtual ~Preconditioner()
      {
      }

      virtual void apply(VT_ & out, const VT_ & in) = 0;
    };


    /**
     * \brief No preconditioner.
     *
     * This class represents a dummy for a preconditioner.
     *
     * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
     *
     * \author Dirk Ribbrock
     */
    template <typename Algo_, typename MT_, typename VT_>
    class NonePreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    private:
      typedef typename MT_::DataType DT_;
      typedef typename MT_::IndexType IT_;

      const DT_ _damping;

    public:
      /// Our algotype
      typedef Algo_ AlgoType;
      /// Our datatype
      typedef typename MT_::DataType DataType;
      /// Our indextype
      typedef typename MT_::IndexType IndexType;
      /// Our memory architecture type
      typedef typename MT_::MemType MemType;
      /// Our vectortype
      typedef VT_ VectorType;
      /// Our matrixtype
      typedef MT_ MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_none;

      // ensure matrix and vector have the same mem-, data- and index-type
      static_assert(std::is_same<MemType, typename VT_::MemType>::value,
                    "matrix and vector have different mem-types");
      static_assert(std::is_same<DataType, typename VT_::DataType>::value,
                    "matrix and vector have different data-types");
      static_assert(std::is_same<IndexType, typename VT_::IndexType>::value,
                    "matrix and vector have different index-types");

      virtual ~NonePreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] damping A damping-parameter
       *
       * Creates a dummy preconditioner
       */
      NonePreconditioner(const DT_ damping = DT_(1)) : _damping(damping)
      {
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "None_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector, which is applied to the preconditioning.
       */
      virtual void apply(VT_ & out, const VT_ & in) override
      {
        if (_damping == DT_(1))
        {
          out.copy(in);
        }
        else
        {
          out.template scale<Algo_>(in, _damping);
        }
      }
    };

    /**
     * \brief File-Preconditioner.
     *
     * This class represents the a preconditioner, using a supplied matrix file as input.
     *
     * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
     *
     * \author Dirk Ribbrock
     */
    template <typename Algo_, typename MT_, typename VT_>
    class FilePreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    private:
      MT_ _mat;

    public:
      /// Our algotype
      typedef Algo_ AlgoType;
      /// Our datatype
      typedef typename MT_::DataType DataType;
      /// Our indextype
      typedef typename MT_::IndexType IndexType;
      /// Our memory architecture type
      typedef typename MT_::MemType MemType;
      /// Our vectortype
      typedef VT_ VectorType;
      /// Our matrixtype
      typedef MT_ MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_file;

      // ensure matrix and vector have the same mem-, data- and index-type
      static_assert(std::is_same<MemType, typename VT_::MemType>::value,
                    "matrix and vector have different mem-types");
      static_assert(std::is_same<DataType, typename VT_::DataType>::value,
                    "matrix and vector have different data-types");
      static_assert(std::is_same<IndexType, typename VT_::IndexType>::value,
                    "matrix and vector have different index-types");

      virtual ~FilePreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] mode A FileMode
       *
       * param[in] filename A filename to the input matrix
       *
       * Creates a Matrix preconditioner from given source matrix file name.
       */
      FilePreconditioner(const FileMode mode, const String filename) :
        _mat(mode, filename)
      {
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "File_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(VT_ & out, const VT_ & in) override
      {
        _mat.template apply<Algo_>(out, in);
      }
    };


    /**
     * \brief Jacobi-Preconditioner.
     *
     * This class represents the Jacobi-Preconditioner \f$M = D\f$.
     *
     * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
     *
     * \author Dirk Ribbrock
     */
    template <typename Algo_, typename MT_, typename VT_>
    class JacobiPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    private:
      VT_ _jac;

    public:
      /// Our algotype
      typedef Algo_ AlgoType;
      /// Our datatype
      typedef typename MT_::DataType DataType;
      /// Our indextype
      typedef typename MT_::IndexType IndexType;
      /// Our memory architecture type
      typedef typename MT_::MemType MemType;
      /// Our vectortype
      typedef VT_ VectorType;
      /// Our matrixtype
      typedef MT_ MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_jacobi;

      // ensure matrix and vector have the same mem-, data- and index-type
      static_assert(std::is_same<MemType, typename VT_::MemType>::value,
                    "matrix and vector have different mem-types");
      static_assert(std::is_same<DataType, typename VT_::DataType>::value,
                    "matrix and vector have different data-types");
      static_assert(std::is_same<IndexType, typename VT_::IndexType>::value,
                    "matrix and vector have different index-types");

      virtual ~JacobiPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] damping A damping-parameter
       *
       * Creates a Jacobi preconditioner to the given matrix and damping-parameter
       */
      JacobiPreconditioner(const MT_ & A, const typename VT_::DataType damping = DataType(1)) :
        _jac(A.rows())
      {
        if (A.columns() != A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        const Index n(A.rows());

        for (Index i(0) ; i < n ; ++i)
        {
          _jac(i, damping / A(i, i));
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "Jacobi_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(VT_ & out, const VT_ & in) override
      {
        out.template component_product<Algo_>(_jac, in);
      }
    };


    /**
     * \brief Gauss-Seidel-Preconditioner.
     *
     * This class represents a dummy class for the Gauss-Seidel-Preconditioner \f$M = (D + L)\f$.
     *
     * \author Christoph Lohmann
     */
    template <typename Algo_, typename MT_, typename VT_>
    class GaussSeidelPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    };


    /**
     * \brief Gauss-Seidel-Preconditioner for CSR-matrices.
     *
     * This class specializes the Gauss-Seidel-Preconditioner \f$M = (D + L)\f$ for CSR-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class GaussSeidelPreconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                                    DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const DT_ _damping;
      const SparseMatrixCSR<Mem_, DT_, IT_> & _A;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCSR<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_gauss_seidel;

      virtual ~GaussSeidelPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] damping A damping-parameter
       *
       * Creates a Gauss-Seidel preconditioner to the given matrix and damping-parameter
       */
      GaussSeidelPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & A,
                                const DT_ damping = DT_(1)) :
        _damping(damping),
        _A(A)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "GaussSeidel_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * prow_ptr(_A.row_ptr());
        const IT_ n(IT_(_A.rows()));

        // __forward-insertion__
        // iteration over all rows
        for (IT_ i(0), col; i < n; ++i)
        {
          DT_ d(0);

          // iteration over all elements on the left side of the main-diagonal
          for (col = prow_ptr[i]; pcol_ind[col] < i; ++col)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }

          // divide by the element on the main-diagonal
          pout[i] = (pin[i] - d) / pval[col];
        }

        // damping of solution
        out.template scale<Algo::Generic>(out, _damping);
      }
    };


    /**
     * \brief Gauss-Seidel-Preconditioner for COO-matrices.
     *
     * This class specializes the Gauss-Seidel-Preconditioner \f$M = (D + L)\f$ for COO-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class GaussSeidelPreconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                                    DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const DT_ _damping;
      const SparseMatrixCOO<Mem_, DT_, IT_> & _A;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCOO<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_gauss_seidel;

      virtual ~GaussSeidelPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] damping A damping-parameter
       *
       * Creates a Gauss-Seidel preconditioner to the given matrix and damping-parameter
       */
      GaussSeidelPreconditioner(const SparseMatrixCOO<Mem_, DT_, IT_> & A,
                                const DT_ damping = DT_(1)) :
        _damping(damping),
        _A(A)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "GaussSeidel_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol(_A.column_indices());
        const IT_ * prow(_A.row_indices());
        const IT_ n(IT_(_A.rows()));

        // __forward-insertion__
        // iteration over all rows
        for (IT_ i(0), col(0); i < n; ++i)
        {
          DT_ d(0);

          // iteration over all elements on the left side of the main-diagonal
          for (; prow[col] < i; ++col);

          for (; pcol[col] < i; ++col)
          {
            d += pval[col] * pout[pcol[col]];
          }

          // divide by the element on the main-diagonal
          pout[i] = (pin[i] - d) / pval[col];
        }

        // damping of solution
        out.template scale<Algo::Generic>(out, _damping);
      }
    };


    /**
     * \brief Gauss-Seidel-Preconditioner for ELL-matrices.
     *
     * This class specializes the Gauss-Seidel-Preconditioner \f$M = (D+L)\f$ for ELL-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class GaussSeidelPreconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                                    DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const DT_ _damping;
      const SparseMatrixELL<Mem_, DT_, IT_> & _A;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixELL<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_gauss_seidel;

      virtual ~GaussSeidelPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] damping A damping-parameter
       *
       * Creates a Gauss-Seidel preconditioner to the given matrix and damping-parameter
       */
      GaussSeidelPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & A,
                                const DT_ damping = DT_(1)) :
        _damping(damping),
        _A(A)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "GaussSeidel_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * pcs(_A.cs());
        const IT_ C(IT_(_A.C()));
        const IT_ n(IT_(_A.rows()));


        // __forward-insertion__
        // iteration over all rows
        for (IT_ i(0), col; i < n; ++i)
        {
          DT_ d(0);

          // iteration over all elements on the left side of the main-diagonal
          for (col = pcs[i/C] + i%C; pcol_ind[col] < i; col += C)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }

          // divide by the element on the main-diagonal
          pout[i] = (pin[i] - d) / pval[col];
        }

        // damping of solution
        out.template scale<Algo::Generic>(out, _damping);
      }
    };


    /**
     * \brief ILU(p)-Preconditioner.
     *
     * This class represents a dummy class for the ILU(p)-Preconditioner \f$M = \tilde L \cdot \tilde U\f$.
     *
     * \author Christoph Lohmann
     */
    template <typename Algo_, typename MT_, typename VT_>
    class ILUPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    };


    /**
     * \brief ILU(p)-Preconditioner for CSR-matrices.
     *
     * This class specializes the ILU(p)-Preconditioner \f$M = \tilde L \cdot \tilde U\f$ for CSR-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class ILUPreconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixCSR<Mem_, DT_, IT_> & _A;
      SparseMatrixCSR<Mem_, DT_, IT_> _LU;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCSR<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ilu;

      virtual ~ILUPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] p level of fillin
       *           if p = 0, the layout of A is used for the ILU-decomposition
       *
       * Creates a ILU preconditioner to the given matrix and level of fillin
       */
      ILUPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & A, const Index p) :
        _A(A)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (p == 0)
        {
          _LU = SparseMatrixCSR<Mem_, DT_, IT_>(A.layout());

          _copy_entries(false);
        }
        else
        {
          _symbolic_lu_factorisation((int) p);
          _copy_entries();
        }

        _create_lu();
      }

      /**
       * \brief Constructor
       *
       * param[in] LU external LU-matrix
       *           the LU-decomposition is not calculated internally;
       *           so this preconditioner only solves \f$y \leftarrow L^{-1} U^{-1} x\f$
       *
       * Creates a ILU preconditioner to the given LU-decomposition
       */
      ILUPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & LU) :
        _A(LU)
      {
        if (_LU.columns() != _LU.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
        this->_LU.convert(LU);
      }

      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] layout An external layout for the LU-decomposition
       *
       * Creates a ILU preconditioner to the given matrix and layout
       */
      ILUPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & A,
                        const SparseLayout<Mem_, IT_, SparseMatrixCSR<Mem_, DT_, IT_>::layout_id> & layout) :
        _A(A),
        _LU(layout)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (_LU.columns() != _LU.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (_A.columns() != _LU.columns())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrices have different sizes!");
        }

        _copy_entries();
        _create_lu();
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "ILU_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pval(_LU.val());
        const IT_ * pcol_ind(_LU.col_ind());
        const IT_ * prow_ptr(_LU.row_ptr());
        const Index n(_LU.rows());

        IT_ col;

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0); i < n; ++i)
        {
          // iteration over all elements on the left side of the main-diagonal
          col = prow_ptr[i];

          while (pcol_ind[col] < i)
          {
            pout[i] -= pval[col] * pout[pcol_ind[col]];
            ++col;
          }
        }

        // __backward-insertion__
        // iteration over all rows
        for (Index i(n); i > 0;)
        {
          --i;

          // iteration over all elements on the right side of the main-diagonal
          col = prow_ptr[i+1]-1;

          while (pcol_ind[col] > i)
          {
            pout[i] -= pval[col] * pout[pcol_ind[col]];
            --col;
          }

          // divide by the element on the main-diagonal
          pout[i] /= pval[col];
        }
      } // function apply

    private:
      void _create_lu()
      {
        /**
         * This algorithm has been adopted by
         *    Dominik Goeddeke - Schnelle Loeser (Vorlesungsskript) 2013/2014
         *    section 5.5.3; Algo 5.13.; page 132
         */

        DT_ * plu(_LU.val());
        const IT_ * pcol(_LU.col_ind());
        const IT_ * prow_ptr(_LU.row_ptr());
        const Index n(_LU.rows());

        // integer work array of length n
        //   for saving the position of the diagonal-entries
        IT_ * pw = new IT_[n];

        IT_ row_start;
        IT_ row_end;

        // iteration over all columns
        for (Index i(0); i < n; ++i)
        {
          row_start = prow_ptr[i];
          row_end = prow_ptr[i + 1];

          // iteration over all elements on the left side of the main-diagonal
          // k -> \tilde a_{ik}
          IT_ k = row_start;
          while (pcol[k] < i)
          {
            plu[k] /= plu[pw[pcol[k]]];
            IT_ m(pw[pcol[k]] + 1);
            // m -> \tilde a_{kj}
            for (IT_ j(k+1); j < row_end; ++j)
            {
              // j -> \tilde a_{ij}
              while (m < prow_ptr[pcol[k] + 1])
              {
                if (pcol[m] == pcol[j])
                {
                  plu[j] -= plu[k] * plu[m];
                  ++m;
                  break;
                }
                else if (pcol[m] > pcol[j])
                {
                  break;
                }
                ++m;
              }
            }
            ++k;
          }
          // save the position of the diagonal-entry
          pw[i] = k;
        }

        delete[] pw;
      } // function _create_lu


      void _symbolic_lu_factorisation(int p)
      {
        /**
         * This algorithm has been adopted by
         *    Dominik Goeddeke - Schnelle Loeser (Vorlesungsskript) 2013/2014
         *    section 5.5.6; Algo 5.19.; page 142
         */

        const Index n(_A.rows());
        const IT_ * pacol(_A.col_ind());
        const IT_ * parow(_A.row_ptr());

        // type of list-entries
        typedef std::pair<int, IT_> PAIR_;
        // lists for saving the non-zero entries of L per row
        std::list<PAIR_> * ll = new std::list<PAIR_>[n];

        // vector for saving the iterators to the diag-entries
        typename std::list<PAIR_>::iterator * pldiag = new typename std::list<PAIR_>::iterator[n];
        // auxillary-iterators
        typename std::list<PAIR_>::iterator it, it1, it2;

        IT_ col_begin, col_end, col, col2;
        int l, l2, neues_level;

        // fill list with non-zero entries of A
        // and save iterators to the diag- and last entry of each row
        for (Index row(0); row < n; ++row)
        {
          std::list<PAIR_> & ll_row (ll[row]);
          col_begin = parow[row];
          col_end = parow[row + 1] - 1;

          for (IT_ k(col_begin); k <= col_end; ++k)
          {
            col = pacol[k];

            ll_row.emplace_back((int) n, col);

            if (col == row)
            {
              pldiag[row] = std::prev(ll_row.end());
            }
          }
        }

        // calculate "new" entries of LU
        for (Index row(1); row < n; ++row)
        {
          // iterate from the beginning of the line to the diag-entry
          it = ll[row].begin();
          while (it != pldiag[row])
          {
            col = it->second;
            l = it->first;

            // search non-zero entries in the col-th row
            it1 = it;
            it2 = std::next(pldiag[col]);
            while (it2 != ll[col].end())
            {
              col2 = it2->second;
              l2 = it2->first;

              neues_level = 2* (int) n - l - l2 + 1;

              // if new entries must be created, find the correct position in the list
              if (neues_level <= p)
              {
                while (it1 != ll[row].end() && it1->second < col2)
                {
                  if (it1 != ll[row].end() && it1 == ll[row].end())
                  {
                    ll[row].emplace_back(neues_level, col2);
                    break;
                  }
                  ++it1;
                }
                if (it1 == ll[row].end() || it1->second != col2)
                {
                  it1 = ll[row].emplace(it1, neues_level, col2);
                }
              }
              ++it2;
            }
            ++it;
          }
        }

        // Create LU-matrix
        // calculate number of non-zero-elements
        Index nnz(0);
        for (Index i(0); i < n; ++i)
        {
          nnz += Index(ll[i].size());
        }

        DenseVector<Mem_, DT_, IT_> val(nnz);
        DenseVector<Mem_, IT_, IT_> col_ind(nnz);
        DenseVector<Mem_, IT_, IT_> row_ptr(n+1);
        IT_ * pcol_ind(col_ind.elements());
        IT_ * prow_ptr(row_ptr.elements());

        IT_ k1(0);
        prow_ptr[0] = 0;

        for (Index i(0); i < n; ++i)
        {
          for (it = ll[i].begin(); it != ll[i].end(); ++it)
          {
            pcol_ind[k1] = it->second;
            ++k1;
          }
          prow_ptr[i+1] = k1;
        }

        _LU = SparseMatrixCSR<Mem_, DT_, IT_>(n, n, col_ind, val, row_ptr);

        delete[] ll;
        delete[] pldiag;
      } // _symbolic_lu_factorisation

      void _copy_entries(bool check = true)
      {
        if (check == false)
        {
          DT_ * plu(_LU.val());
          const DT_ * pa(_A.val());
          const Index used_elements(_LU.used_elements());

          // initialize LU array to A
          for (Index i(0); i < used_elements; ++i)
          {
            plu[i] = pa[i];
          }
        }
        else
        {
          DT_ * plu(_LU.val());
          const IT_ * plucol(_LU.col_ind());
          const IT_ * plurow(_LU.row_ptr());

          const DT_ * pa(_A.val());
          const IT_ * pacol(_A.col_ind());
          const IT_ * parow(_A.row_ptr());

          const Index n(_LU.rows());
          Index k;
          // initialize LU array to A
          for (Index i(0); i < n; ++i)
          {
            k = parow[i];
            for (IT_ j(plurow[i]); j < plurow[i + 1]; ++j)
            {
              plu[j] = DT_(0.0);
              while (k < parow[i + 1] && plucol[j] >= pacol[k])
              {
                if (plucol[j] == pacol[k])
                {
                  plu[j] = pa[k];
                  ++k;
                  break;
                }
                ++k;
              }
            }
          }
        }
      } // function _copy_entries
    };


    /**
     * \brief ILU(p)-Preconditioner for ELL-matrices.
     *
     * This class specializes the ILU(p)-Preconditioner \f$M = \tilde L \cdot \tilde U\f$ for ELL-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class ILUPreconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixELL<Mem_, DT_, IT_> & _A;
      SparseMatrixELL<Mem_, DT_, IT_> _LU;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixELL<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ilu;

      virtual ~ILUPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] p level of fillin
       *           if p = 0, the layout of A is used for the ILU-decomposition
       *
       * Creates a ILU preconditioner to the given matrix and level of fillin
       */
      ILUPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & A, const Index p) :
        _A(A)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (p == 0)
        {
          _LU = SparseMatrixELL<Mem_, DT_, IT_>(A.layout());

          _copy_entries(false);
        }
        else
        {
          _symbolic_lu_factorisation((int) p);
          _copy_entries();
        }

        _create_lu();
      }

      /**
       * \brief Constructor
       *
       * param[in] LU external LU-matrix
       *           the LU-decomposition is not calculated internally;
       *           so this preconditioner only solves \f$y \leftarrow L^{-1} U^{-1} x\f$
       *
       * Creates a ILU preconditioner to the given LU-decomposition
       */
      ILUPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & LU) :
        _A(LU)
      {
        if (_LU.columns() != _LU.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        this->_LU.convert(LU);
      }

      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] layout An external layout for the LU-decomposition
       *
       * Creates a ILU preconditioner to the given matrix and layout
       */
      ILUPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & A,
                        const SparseLayout<Mem_, IT_, SparseMatrixELL<Mem_, DT_>::layout_id> & layout) :
        _A(A),
        _LU(layout)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (_LU.columns() != _LU.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (_A.columns() != _LU.columns())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrices have different sizes!");
        }

        if (_A.C() != _LU.C())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrices have different chunk sizes!");
        }

        _copy_entries();

        _create_lu();
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "ILU_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pval(_LU.val());
        const IT_ * pcol_ind(_LU.col_ind());
        const IT_ * pcs(_LU.cs());
        const IT_ * prl(_LU.rl());
        const Index C(_LU.C());
        const Index n(_LU.rows());

        Index col;

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0); i < n; ++i)
        {
          // iteration over all elements on the left side of the main-diagonal
          col = pcs[i/C] + i%C;

          while (pcol_ind[col] < i)
          {
            pout[i] -= pval[col] * pout[pcol_ind[col]];
            col += C;
          }
        }

        // __backward-insertion__
        // iteration over all rows
        for (Index i(n); i > 0;)
        {
          --i;

          // iteration over all elements on the right side of the main-diagonal
          col = pcs[i/C] + i%C + C * (prl[i] - 1);

          while (pcol_ind[col] > i)
          {
            pout[i] -= pval[col] * pout[pcol_ind[col]];
            col -= C;
          }

          // divide by the element on the main-diagonal
          pout[i] /= pval[col];
        }
      } // function apply

    private:
      void _create_lu()
      {
        /**
         * This algorithm has been adopted by
         *    Dominik Goeddeke - Schnelle Loeser (Vorlesungsskript) 2013/2014
         *    section 5.5.3; Algo 5.13.; page 132
         */

        const Index n(_A.rows());
        DT_ * pval(_LU.val());
        const IT_ * pcol_ind(_LU.col_ind());
        const IT_ * pcs(_LU.cs());
        const IT_ * prl(_LU.rl());
        IT_ C(IT_(_LU.C()));

        // integer work array of length n
        //   for saving the position of the diagonal-entries
        IT_ * pw = new IT_[n];

        // iteration over all columns
        for (IT_ i(0); i < IT_(n); ++i)
        {
          // iteration over all elements on the left side of the main-diagonal
          // k -> \tilde a_{ik}
          IT_ k(pcs[i/C] + i%C);
          while (pcol_ind[k] < i)
          {
            pval[k] /= pval[pw[pcol_ind[k]]];
            Index m(pw[pcol_ind[k]] + C);
            // m -> \tilde a_{kj}
            for (Index j(k + C); j < pcs[i/C] + i%C + C*prl[i]; j += C)
            {
              // j -> \tilde a_{ij}
              while (m < pcs[pcol_ind[k]/C] + pcol_ind[k]%C + prl[pcol_ind[k]] * C)
              {
                if (pcol_ind[m] == pcol_ind[j])
                {
                  pval[j] -= pval[k] * pval[m];
                  m += C;
                  break;
                }
                else if (pcol_ind[m] > pcol_ind[j])
                {
                  break;
                }
                m += C;
              }
            }
            k += C;
          }
          // save the position of the diagonal-entry
          pw[i] = k;
        }

        delete[] pw;
      } // function _create_lu

      void _symbolic_lu_factorisation(int p)
      {
        /**
         * This algorithm has been adopted by
         *    Dominik Goeddeke - Schnelle Loeser (Vorlesungsskript) 2013/2014
         *    section 5.5.6; Algo 5.19.; page 142
         */

        const Index n(_A.rows());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * pcs(_A.cs());
        const IT_ * prl(_A.rl());
        const Index C(_A.C());

        // type of list-entries
        typedef std::pair<int, IT_> PAIR_;
        // list for saving the non-zero entries of L
        std::list<PAIR_> * ll = new std::list<PAIR_>[n];

        // vector for saving the iterators to the diag-entries
        typename std::list<PAIR_>::iterator * pldiag = new typename std::list<PAIR_>::iterator[n];
        // auxillary-iterators
        typename std::list<PAIR_>::iterator it, it1, it2;

        Index col, col2;
        int l, l2, neues_level;

        // fill list with non-zero entries of A
        // and save iterators to the diag- and last entry of each row
        for (Index row(0); row < n; ++row)
        {
          std::list<PAIR_> & ll_row (ll[row]);
          for (Index k(0); k < prl[row]; ++k)
          {
            col = pcol_ind[pcs[row/C] + row%C + C * k];

            ll_row.emplace_back((int) n, col);

            if (col == row)
            {
              pldiag[row] = std::prev(ll_row.end());
            }
          }
        }

        // calculate "new" entries of LU
        for (Index row(1); row < n; ++row)
        {
          // iterate from the beginning of the line to the diag-entry
          it = ll[row].begin();
          while (it != pldiag[row])
          {
            col = it->second;
            l = it->first;

            // search non-zero entries in the col-th row
            it1 = it;
            it2 = std::next(pldiag[col]);
            while (it2 != ll[col].end())
            {
              col2 = it2->second;
              l2 = it2->first;

              neues_level = 2*(int) n - l - l2 + 1;

              // if new entries must be created, find the correct position in the list
              if (neues_level <= p)
              {
                while (it1 != ll[row].end() && it1->second < col2)
                {
                  if (it1 != ll[row].end() && it1 == ll[row].end())
                  {
                    ll[row].emplace_back(neues_level, col2);
                    break;
                  }
                  ++it1;
                }
                if (it1 == ll[row].end() || it1->second != col2)
                {
                  it1 = ll[row].emplace(it1, neues_level, col2);
                }
              }
              ++it2;
            }
            ++it;
          }
        }

        // Create LU-matrix
        // calculate cl-array and fill rl-array
        Index num_of_chunks(Index(ceil(n / float(C))));
        DenseVector<Mem_, IT_, IT_> lucl(num_of_chunks, IT_(0));
        DenseVector<Mem_, IT_, IT_> lucs(num_of_chunks + 1);
        DenseVector<Mem_, IT_, IT_> lurl(n);
        IT_ * plucl(lucl.elements());
        IT_ * plucs(lucs.elements());
        IT_ * plurl(lurl.elements());

        Index nnz(0);

        for (Index i(0); i < n; ++i)
        {
          plurl[i] = IT_(ll[i].size());
          plucl[i/C] = Math::max(plucl[i/C], plurl[i]);
          nnz += Index(ll[i].size());
        }

        // calculuate cs-array
        plucs[0] = IT_(0);
        for (Index i(0); i < num_of_chunks; ++i)
        {
          plucs[i+1] = plucs[i] + IT_(C) * plucl[i];
        }

        Index val_size = Index(plucs[num_of_chunks]);

        DenseVector<Mem_, DT_, IT_> luval(val_size);
        DenseVector<Mem_, IT_, IT_> lucol_ind(val_size);
        DT_ * pluval    (luval.elements());
        IT_ * plucol_ind(lucol_ind.elements());

        Index k1(0);

        for (Index i(0); i < n; ++i)
        {
          k1 = 0;
          for (it = ll[i].begin(); it != ll[i].end(); ++it, ++k1)
          {
            plucol_ind[plucs[i/C] + i%C + k1 * C] = it->second;
          }
          for (k1 = plurl[i]; k1 < plurl[i]; ++k1)
          {
            plucol_ind[plucs[i/C] + i%C + k1 * C] = IT_(0);
            pluval    [plucs[i/C] + i%C + k1 * C] = DT_(0);
          }
        }

        _LU = SparseMatrixELL<Mem_, DT_, IT_>(n, n, nnz, luval, lucol_ind, lucs, lucl, lurl, C);

        delete[] ll;
        delete[] pldiag;
      } // _symbolic_lu_factorisation

      void _copy_entries(bool check = true)
      {
        if (check == false)
        {
          DT_ * pluval(_LU.val());
          const DT_ * paval(_A.val());
          const Index data_length(_LU.val_size());

          // initialize LU array to A
          for (Index i(0); i < data_length; ++i)
          {
            pluval[i] = paval[i];
          }
        }
        else
        {
          DT_ * pluval(_LU.val());
          const IT_ * plucol_ind(_LU.col_ind());
          const IT_ * plucs(_LU.cs());
          const IT_ * plurl(_LU.rl());

          const DT_ * paval(_A.val());
          const IT_ * pacol_ind(_A.col_ind());
          const IT_ * pacs(_A.cs());
          const IT_ * parl(_A.rl());
          const Index C(_A.C());

          const Index n(_A.rows());

          Index k, ctr;

          // iteration over all rows
          for (Index row(0); row < n; ++row)
          {
            k  = pacs[row/C] + row%C;
            ctr = 0;
            for (Index j(0); j < plurl[row]; ++j)
            {
              pluval[plucs[row/C] + row%C + j * C] = DT_(0.0);
              while (ctr < parl[row] && pacol_ind[k] <= plucol_ind[plucs[row/C] + row%C + j * C])
              {
                if (plucol_ind[plucs[row/C] + row%C + j * C] == pacol_ind[k])
                {
                  pluval[plucs[row/C] + row%C + j * C] = paval[k];
                  ++ctr;
                  k = k + C;
                  break;
                }
                ++ctr;
                k = k + C;
              }
            }
          }
        }
      } // function _copy_entries
    };


    /**
     * \brief ILU(p)-Preconditioner for COO-matrices.
     *
     * This class specializes the ILU(p)-Preconditioner \f$M = \tilde L \cdot \tilde U\f$ for COO-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class ILUPreconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      ILUPreconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                        DenseVector<Mem_, DT_, IT_> > _precond;
    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCOO<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ilu;

      virtual ~ILUPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] p level of fillin
       *           if p = 0, the layout of A is used for the ILU-decomposition
       *
       * Creates a ILU preconditioner to the given matrix and level of fillin
       */
      ILUPreconditioner(const SparseMatrixCOO<Mem_, DT_, IT_> & A, const Index p) :
        _precond(SparseMatrixCSR<Mem_, DT_, IT_> (A), p)
      {
      }

      /**
       * \brief Constructor
       *
       * param[in] LU external LU-matrix
       *           the LU-decomposition is not calculated internally;
       *           so this preconditioner only solves \f$y \leftarrow L^{-1} U^{-1} x\f$
       *
       * Creates a ILU preconditioner to the given LU-decomposition
       */
      ILUPreconditioner(const SparseMatrixCOO<Mem_, DT_, IT_> & LU) :
        _precond(SparseMatrixCSR<Mem_, DT_, IT_> (LU))
      {
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "ILU_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        _precond.apply(out, in);
      }
    };


    /**
     * \brief SOR-Preconditioner.
     *
     * This class represents a dummy class for the SOR-Preconditioner \f$ \frac 1 \omega (D + \omega L)\f$.
     *
     * \author Christoph Lohmann
     */
    template <typename Algo_, typename MT_, typename VT_>
    class SORPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    };


    /**
     * \brief SOR-Preconditioner for CSR-matrices.
     *
     * This class specializes the SOR-Preconditioner \f$ \frac 1 \omega (D + \omega L)\f$ for CSR-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SORPreconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixCSR<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCSR<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_sor;

      virtual ~SORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 0.7)
       *
       * Creates a SOR preconditioner to the given matrix and parameter
       */
      SORPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & A,
                        const DT_ omega = DT_(0.7)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * prow_ptr(_A.row_ptr());
        const Index n(_A.rows());

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col; i < n; ++i)
        {
          DT_ d(0);
          // iteration over all elements on the left side of the main-diagonal
          for (col = prow_ptr[i]; pcol_ind[col] < i; ++col)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] = _omega * (pin[i] - d) / pval[col];
        }
      }
    };


    /**
     * \brief SOR-Preconditioner for COO-matrices.
     *
     * This class specializes the SOR-Preconditioner \f$ \frac 1 \omega (D + \omega L)\f$ for COO-matrices.
     *(
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SORPreconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixCOO<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      // Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCOO<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_sor;

      virtual ~SORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 0.7)
       *
       * Creates a SOR preconditioner to the given matrix and parameter
       */
      SORPreconditioner(const SparseMatrixCOO<Mem_, DT_, IT_> & A,
                        const DT_ omega = DT_(0.7)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol(_A.column_indices());
        const IT_ * prow(_A.row_indices());
        const Index n(_A.rows());

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col(0); i < n; ++i)
        {
          // iteration over all elements on the left side of the main-diagonal
          while (prow[col] < i)
          {
            ++col;
          }
          DT_ d(0);
          while (pcol[col] < i)
          {
            d += pval[col] * pout[pcol[col]];
            ++col;
          }
          pout[i] = _omega * (pin[i] - d) / pval[col];
        }
      }
    };


    /**
     * \brief SOR-Preconditioner for ELL-matrices.
     *
     * This class specializes the SOR-Preconditioner \f$ \frac 1 \omega (D + \omega L)\f$ for ELL-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SORPreconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                            DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixELL<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixELL<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_sor;

      virtual ~SORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 0.7)
       *
       * Creates a SOR preconditioner to the given matrix and parameter
       */
      SORPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & A,
                        const DT_ omega = DT_(0.7)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // copy in-vector to out-vector
        out.copy(in);

        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin(in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * pcs(_A.cs());
        const Index C(_A.C());
        const Index n(_A.rows());


        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col; i < n; ++i)
        {
          DT_ d(0);
          // iteration over all elements on the left side of the main-diagonal
          for (col = pcs[i/C] + i%C; pcol_ind[col] < i; col += C)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] = _omega * (pin[i] - d) / pval[col];
        }
      }
    };


    /**
     * \brief SSOR-Preconditioner.
     *
     * This class represents a dummy class for the SSOR-Preconditioner \f$ \frac 1 {\omega (2 - \omega)} (D + \omega L) D^{-1} (D + \omega R)\f$.
     *
     * \author Christoph Lohmann
     */
    template <typename Algo_, typename MT_, typename VT_>
    class SSORPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    };


    /**
     * \brief SSOR-Preconditioner for CSR-matrices.
     *
     * This class specializes the SSOR-Preconditioner \f$ \frac 1 {\omega (2 - \omega)} (D + \omega L) D^{-1} (D + \omega R)\f$ for CSR-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SSORPreconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                             DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixCSR<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCSR<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ssor;

      virtual ~SSORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A The system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 1.3)
       *
       * Creates a SSOR preconditioner to the given matrix and parameter
       */
      SSORPreconditioner(const SparseMatrixCSR<Mem_, DT_, IT_> & A, const DT_ omega = DT_(1.3)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (Math::abs(_omega - DT_(2.0)) < 1e-10)
        {
          throw InternalError(__func__, __FILE__, __LINE__, "omega too close to 2!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SSOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin (in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * prow_ptr(_A.row_ptr());
        const Index n(_A.rows());

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col; i < n; ++i)
        {
          DT_ d(0);
          // iteration over all elements on the left side of the main-diagonal
          for (col = prow_ptr[i]; pcol_ind[col] < i; ++col)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] = (pin[i] - _omega * d) / pval[col];
        }

        // __backward-insertion__
        // iteration over all rows
        for (Index i(n), col; i > 0;)
        {
          --i;
          DT_ d(0);
          // iteration over all elements on the right side of the main-diagonal
          for (col = prow_ptr[i+1]-1; pcol_ind[col] > i; --col)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] -= _omega * d / pval[col];
        }

        out.template scale<Algo::Generic>(out, _omega * (DT_(2.0) - _omega));
      } // function apply
    };


    /**
     * \brief SSOR-Preconditioner for COO-matrices.
     *
     * This class specializes the SSOR-Preconditioner \f$ \frac 1 {\omega (2 - \omega)} (D + \omega L) D^{-1} (D + \omega R)\f$ for COO-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SSORPreconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                             DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixCOO<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_, IT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixCOO<Mem_, DT_, IT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ssor;

      virtual ~SSORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A The system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 1.3)
       *
       * Creates a SSOR preconditioner to the given matrix and parameter
       */
      SSORPreconditioner(const SparseMatrixCOO<Mem_, DT_, IT_> & A, const DT_ omega = DT_(1.3)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (Math::abs(_omega - DT_(2.0)) < 1e-10)
        {
          throw InternalError(__func__, __FILE__, __LINE__, "omega too close to 2!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SSOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin (in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol(_A.column_indices());
        const IT_ * prow(_A.row_indices());
        const Index n(_A.rows());

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col(0); i < n; ++i)
        {
          // iteration over all elements on the left side of the main-diagonal
          while (prow[col] < i)
          {
            ++col;
          }
          DT_ d(0);
          while (pcol[col] < i)
          {
            d += pval[col] * pout[pcol[col]];
            ++col;
          }
          pout[i] = (pin[i] - _omega * d) / pval[col];
        }

        // __backward-insertion__
        // iteration over all rows
        for (Index i(n), col(_A.used_elements() - 1); i > 0;)
        {
          --i;

          // iteration over all elements on the right side of the main-diagonal
          while (prow[col] > i)
          {
            --col;
          }
          DT_ d(0);
          while (pcol[col] > i)
          {
            d += pval[col] * pout[pcol[col]];
            --col;
          }
          pout[i] -= _omega * d / pval[col];
        }

        out.template scale<Algo::Generic>(out, _omega * (DT_(2.0) - _omega));
      } // function apply
    };


    /**
     * \brief SSOR-Preconditioner for ELL-matrices.
     *
     * This class specializes the SSOR-Preconditioner \f$ \frac 1 {\omega (2 - \omega)} (D + \omega L) D^{-1} (D + \omega R)\f$ for ELL-matrices.
     *
     * \author Christoph Lohmann
     */
    template <typename Mem_, typename DT_, typename IT_>
    class SSORPreconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                             DenseVector<Mem_, DT_, IT_> >
      : public Preconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                              DenseVector<Mem_, DT_, IT_> >
    {
    private:
      const SparseMatrixELL<Mem_, DT_, IT_> & _A;
      const DT_ _omega;

    public:
      /// Our algotype
      typedef Algo::Generic AlgoType;
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our vectortype
      typedef DenseVector<Mem_, DT_> VectorType;
      /// Our matrixtype
      typedef SparseMatrixELL<Mem_, DT_> MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_ssor;

      virtual ~SSORPreconditioner()
      {
      }
      /**
       * \brief Constructor
       *
       * param[in] A The system-matrix
       *
       * param[in] omega A parameter of the preconditioner (default omega = 1.3)
       *
       * Creates a SSOR preconditioner to the given matrix and parameter
       */
      SSORPreconditioner(const SparseMatrixELL<Mem_, DT_, IT_> & A, const DT_ omega = DT_(1.3)) :
        _A(A),
        _omega(omega)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        if (Math::abs(_omega - DT_(2.0)) < 1e-10)
        {
          throw InternalError(__func__, __FILE__, __LINE__, "omega too close to 2!");
        }
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SSOR_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        // create pointers
        DT_ * pout(out.elements());
        const DT_ * pin (in.elements());
        const DT_ * pval(_A.val());
        const IT_ * pcol_ind(_A.col_ind());
        const IT_ * pcs(_A.cs());
        const IT_ * prl(_A.rl());
        const Index C(_A.C());
        const Index n(_A.rows());

        // __forward-insertion__
        // iteration over all rows
        for (Index i(0), col; i < n; ++i)
        {
          DT_ d(0);
          // iteration over all elements on the left side of the main-diagonal
          for (col = pcs[i/C] + i%C; pcol_ind[col] < i; col += C)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] = (pin[i] - _omega * d) / pval[col];
        }

        // __backward-insertion__
        // iteration over all rows
        for (Index i(n), col; i > 0;)
        {
          --i;

          DT_ d(0);
          // iteration over all elements on the right side of the main-diagonal
          for (col = pcs[i/C] + i%C + C * (prl[i] - 1); pcol_ind[col] > i; col -= C)
          {
            d += pval[col] * pout[pcol_ind[col]];
          }
          pout[i] -= _omega * d / pval[col];
        }

        out.template scale<Algo::Generic>(out, _omega * (DT_(2.0) - _omega));
      } // function apply
    };


    /**
     * \brief SPAI-Preconditioner.
     *
     * This class represents the SPAI-Preconditioner \f$M \approx A^{-1}\f$.
     *
     * \author Christoph Lohmann
     */
    /// \cond internal
    namespace Intern
    {
      template <typename Algo_, typename MT_, typename VT_>
      class SPAIPreconditionerMTdepending : public Preconditioner<Algo_, MT_, VT_>
      {
      };

      template <typename Mem_, typename DT_, typename IT_>
      class SPAIPreconditionerMTdepending<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                                          DenseVector<Mem_, DT_, IT_> >
        : public Preconditioner<Algo::Generic, SparseMatrixCSR<Mem_, DT_, IT_>,
                                DenseVector<Mem_, DT_, IT_> >
      {
      private:
        typedef std::pair<DT_, IT_> PAIR_;
        typedef SparseMatrixCSR<Mem_, DT_, IT_> MatrixType;
        typedef DenseVector<Mem_, DT_, IT_> VectorType;

      protected:
        const MatrixType & _A;
        const SparseLayout<Mem_, IT_, MatrixType::layout_id> _layout;
        const Index _m;
        MatrixType _M;
        std::list<PAIR_> * _m_columns;
        std::vector<std::list<PAIR_> > _a_columnwise;

      public:
        SPAIPreconditionerMTdepending(const MatrixType & A, const Index m) :
          _A(A),
          _layout(_A.layout()),
          _m(m),
          _a_columnwise(_A.rows())
        {
        }

        SPAIPreconditionerMTdepending(const MatrixType & A,
                                      SparseLayout<Mem_, IT_, MatrixType::layout_id> && layout) :
          _A(A),
          _layout(std::move(layout)),
          _m((Index) -1),
          _a_columnwise(_A.rows())
        {
        }

        virtual ~SPAIPreconditionerMTdepending()
        {
        }

        void create_initial_m_columns ()
        {
          const Index n(_A.rows());

          const IT_ * playoutcol(_layout.get_indices().at(0));
          const IT_ * playoutrow(_layout.get_indices().at(1));

          for (Index i(0); i < n; ++i)
          {
            for (Index l(playoutrow[i]); l < playoutrow[i + 1]; ++l)
            {
              _m_columns[playoutcol[l]].emplace_back(DT_(0.0), i);
            }
          }
        } // function create_initial_m_columns

        void create_a_columnwise ()
        {
          const DT_ * pval(_A.val());
          const IT_ * pcol_ind(_A.col_ind());
          const IT_ * prow_ptr(_A.row_ptr());
          const Index n(_A.columns());

          for (Index i(0); i < n; ++i)
          {
            for (Index l(prow_ptr[i]); l < prow_ptr[i + 1]; ++l)
            {
              _a_columnwise[pcol_ind[l]].emplace_back(pval[l], i);
            }
          }
        } // function create_a_aolumnwise

        void create_m_transpose ()
        {
          const Index n(_A.rows());

          // calculate number of used elements
          Index nnz = 0;
          for (Index k(0); k < n; ++k)
          {
            nnz += Index(_m_columns[k].size());
          }

          DenseVector<Mem_, DT_, IT_> val(nnz);
          DenseVector<Mem_, IT_, IT_> col_ind(nnz);
          DenseVector<Mem_, IT_, IT_> row_ptr(n+1);
          DT_ * pval = val.elements();
          IT_ * pcol_ind = col_ind.elements();
          IT_ * prow_ptr = row_ptr.elements();
          IT_ nz(0);
          prow_ptr[0] = 0;

          for (Index k(0); k < n; ++k)
          {
            for (auto it_J = _m_columns[k].begin(); it_J != _m_columns[k].end(); ++it_J)
            {
              pcol_ind[nz] = it_J->second;
              pval[nz] = it_J->first;
              ++nz;
            }
            prow_ptr[k+1] = nz;
          }

          _M = MatrixType(n, n, col_ind, val, row_ptr);
        } // function create_m_transpose

        void create_m()
        {
          const Index n(_A.rows());

          DenseVector<Mem_, IT_, IT_> trow_ptr(n + 1, IT_(0));
          IT_ * ptrow_ptr(trow_ptr.elements());

          ptrow_ptr[0] = 0;

          Index used_elements(0);
          for (Index i(0); i < n; ++i)
          {
            used_elements += Index(_m_columns[i].size());
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              ++ptrow_ptr[it_J->second + 1];
            }
          }

          for (Index i(1); i < n - 1; ++i)
          {
            ptrow_ptr[i + 1] += ptrow_ptr[i];
          }

          DenseVector<Mem_, IT_, IT_> tcol_ind(used_elements);
          DenseVector<Mem_, DT_, IT_> tval(used_elements);
          IT_ * ptcol_ind(tcol_ind.elements());
          DT_ * ptval(tval.elements());

          for (IT_ i(0); i < IT_(n); ++i)
          {
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              const Index l(it_J->second);
              const Index j(ptrow_ptr[l]);
              ptval[j] = it_J->first;
              ptcol_ind[j] = i;
              ++ptrow_ptr[l];
            }
          }

          for (Index i(n); i > 0; --i)
          {
            ptrow_ptr[i] = ptrow_ptr[i - 1];
          }
          ptrow_ptr[0] = 0;

          _M = MatrixType(n, n, tcol_ind, tval, trow_ptr);
        } // function create_m

        void create_m_without_new_entries ()
        {
          const Index n(_A.rows());

          if (_m != (Index) -1)
          {
            const Index used_elements(n * (1 + 2 * _m) - _m * (_m + 1));

            DenseVector<Mem_, DT_, IT_> val(used_elements);
            DenseVector<Mem_, IT_, IT_> col_ind(used_elements);
            DenseVector<Mem_, IT_, IT_> row_ptr(n+1);
            DT_ * pval(val.elements());
            IT_ * pcol_ind(col_ind.elements());
            IT_ * prow_ptr(row_ptr.elements());

            prow_ptr[0] = IT_(0);

            IT_ k(0);
            for (IT_ i(0); i < IT_(n); ++i)
            {
              for (IT_ l((_m > i) ? 0 : i - IT_(_m)); l < Math::min(n, _m + i + 1); ++l)
              {
                pcol_ind[k] = l;
                ++k;
              }
              prow_ptr[i+1] = k;
            }

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                const IT_ tmp(std::min(it_J->second, IT_(_m)));
                pval[prow_ptr[it_J->second] + i - it_J->second + tmp] = it_J->first;
              }
            }

            _M = MatrixType(n, n, col_ind, val, row_ptr);
          }
          else
          {
            _M = MatrixType(_layout);

            DT_ * pval(_M.val());
            const IT_ * pcol_ind(_M.col_ind());
            const IT_ * prow_ptr(_M.row_ptr());

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                IT_ k = prow_ptr[it_J->second];

                while (pcol_ind[k] != i)
                {
                  ++k;
                }

                pval[k] = it_J->first;
              }
            }
          }
        } // function create_m_without_new_entries

        void apply_m_transpose (VectorType & out, const VectorType & in)
        {
          const Index n(_M.rows());
          const IT_ * pmcol(_M.col_ind());
          const IT_ * pmrow(_M.row_ptr());
          const DT_ * pm(_M.val());
          const DT_ * pin(in.elements());
          DT_ * pout(out.elements());

          for (Index i(0); i < n; i++)
          {
            pout[i] = DT_(0.0);
          }

          for (Index i(0); i < n; i++)
          {
            for (Index c(pmrow[i]); c < pmrow[i + 1]; c++)
            {
              pout[pmcol[c]] += pm[c] * pin[i];
            }
          }
        } // function apply_m_transpose
      };


      template <typename Mem_, typename DT_, typename IT_>
      class SPAIPreconditionerMTdepending<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                                          DenseVector<Mem_, DT_, IT_> >
        : public Preconditioner<Algo::Generic, SparseMatrixCOO<Mem_, DT_, IT_>,
                                DenseVector<Mem_, DT_, IT_> >
      {
      private:
        typedef std::pair<DT_, IT_> PAIR_;
        typedef SparseMatrixCOO<Mem_, DT_, IT_> MatrixType;
        typedef DenseVector<Mem_, DT_, IT_> VectorType;

      protected:
        const MatrixType & _A;
        const SparseLayout<Mem_, IT_, MatrixType::layout_id> _layout;
        const Index _m;
        MatrixType _M;
        std::list<PAIR_> * _m_columns;
        std::vector<std::list<PAIR_> > _a_columnwise;

      public:
        SPAIPreconditionerMTdepending(const MatrixType & A, const Index m) :
          _A(A),
          _layout(_A.layout()),
          _m(m),
          _a_columnwise(_A.rows())
        {
        }

        SPAIPreconditionerMTdepending(const MatrixType & A,
                                      SparseLayout<Mem_, IT_, MatrixType::layout_id> && layout) :
          _A(A),
          _layout(std::move(layout)),
          _m((Index) -1),
          _a_columnwise(_A.rows())
        {
        }

        virtual ~SPAIPreconditionerMTdepending()
        {
        }

        void create_initial_m_columns ()
        {
          const IT_ * playoutcol(_layout.get_indices().at(1));
          const IT_ * playoutrow(_layout.get_indices().at(0));
          const Index used_elements(_layout.get_scalar_index().at(3));

          for (Index i(0); i < used_elements; ++i)
          {
            _m_columns[playoutcol[i]].emplace_back(DT_(0.0), playoutrow[i]);
          }
        } // function create_initial_m_columns

        void create_a_columnwise ()
        {
          const DT_ * pa(_A.get_elements().at(0));
          const IT_ * pacol(_A.get_indices().at(1));
          const IT_ * parow(_A.get_indices().at(0));
          const Index used_elements(_A.get_scalar_index().at(3));

          for (Index i(0); i < used_elements; ++i)
          {
            _a_columnwise[pacol[i]].emplace_back(pa[i], parow[i]);
          }
        } // function create_a_aolumnwise

        void create_m_transpose ()
        {
          const Index n(_A.rows());

          // calculate number of used elements
          Index nnz = 0;
          for (Index k(0); k < n; ++k)
          {
            nnz += Index(_m_columns[k].size());
          }

          DenseVector<Mem_, DT_, IT_> val(nnz);
          DenseVector<Mem_, IT_, IT_> col_ind(nnz);
          DenseVector<Mem_, IT_, IT_> row_ind(nnz);
          DT_ * pval = val.elements();
          IT_ * pcol_ind = col_ind.elements();
          IT_ * prow_ind = row_ind.elements();
          Index nz(0);

          for (IT_ k(0); k < IT_(n); ++k)
          {
            for (auto it_J = _m_columns[k].begin(); it_J != _m_columns[k].end(); ++it_J)
            {
              pcol_ind[nz] = it_J->second;
              prow_ind[nz] = k;
              pval[nz] = it_J->first;
              ++nz;
            }
          }

          _M = MatrixType(n, n, row_ind, col_ind, val);
        } // function create_m_transpose

        void create_m()
        {
          const Index n(_A.rows());

          DenseVector<Mem_, IT_, IT_> trow_ptr(n + 1, IT_(0));
          IT_ * ptrow_ptr(trow_ptr.elements());

          Index used_elements(0);
          for (Index i(0); i < n; ++i)
          {
            used_elements += Index(_m_columns[i].size());
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              ++ptrow_ptr[it_J->second + 1];
            }
          }
          ptrow_ptr[0] = IT_(0);

          for (Index i(1); i < n - 1; ++i)
          {
            ptrow_ptr[i + 1] += ptrow_ptr[i];
          }

          DenseVector<Mem_, IT_, IT_> tcol_ind(used_elements);
          DenseVector<Mem_, IT_, IT_> trow_ind(used_elements);
          DenseVector<Mem_, DT_, IT_> tval(used_elements);
          IT_ * ptcol_ind(tcol_ind.elements());
          IT_ * ptrow_ind(trow_ind.elements());
          DT_ * ptval(tval.elements());

          for (IT_ i(0); i < IT_(n); ++i)
          {
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              const Index l(it_J->second);
              const Index j(ptrow_ptr[l]);
              ptval[j] = it_J->first;
              ptcol_ind[j] = i;
              ptrow_ind[j] = it_J->second;
              ++ptrow_ptr[l];
            }
          }

          _M = MatrixType(n, n, trow_ind, tcol_ind, tval);
        } // function create_m

        void create_m_without_new_entries ()
        {
          const Index n(_A.rows());

          if (_m != (Index) -1)
          {
            const Index used_elements(n * (1 + 2 * _m) - _m * (_m + 1));

            DenseVector<Mem_, DT_, IT_> val(used_elements);
            DenseVector<Mem_, IT_, IT_> col_ind(used_elements);
            DenseVector<Mem_, IT_, IT_> row_ind(used_elements);
            DenseVector<Mem_, IT_, IT_> row_ptr(n+1);
            DT_ * pval(val.elements());
            IT_ * pcol_ind(col_ind.elements());
            IT_ * prow_ind(row_ind.elements());
            IT_ * prow_ptr(row_ptr.elements());

            prow_ptr[0] = IT_(0);

            IT_ k(0);
            for (IT_ i(0); i < IT_(n); ++i)
            {
              for (IT_ l((_m > i) ? 0 : i - IT_(_m)); l < Math::min(n, _m + i + 1); ++l)
              {
                pcol_ind[k] = l;
                prow_ind[k] = i;
                ++k;
              }
              prow_ptr[i+1] = k;
            }

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                const Index tmp(std::min(it_J->second, IT_(_m)));
                pval[prow_ptr[it_J->second] + i - it_J->second + tmp] = it_J->first;
              }
            }

            _M = MatrixType(n, n, row_ind, col_ind, val);
          }
          else
          {
            _M = MatrixType(_layout);

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                _M(it_J->second, i, it_J->first);
              }
            }
          }
        } // function create_m_without_new_entries

        void apply_m_transpose (VectorType & out, const VectorType & in)
        {
          const Index used_elements(_M.used_elements());
          const Index n(_M.rows());
          const DT_ * pval(_M.val());
          const IT_ * pcol(_M.column_indices());
          const IT_ * prow(_M.row_indices());
          const DT_ * pin(in.elements());
          DT_ * pout(out.elements());

          for (Index i(0); i < n; i++)
          {
            pout[i] = DT_(0.0);
          }

          for (Index i(0); i < used_elements; i++)
          {
            pout[pcol[i]] += pval[i] * pin[prow[i]];
          }
        } // function apply_m_transpose
      };


      template <typename Mem_, typename DT_, typename IT_>
      class SPAIPreconditionerMTdepending<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                                          DenseVector<Mem_, DT_, IT_> >
        : public Preconditioner<Algo::Generic, SparseMatrixELL<Mem_, DT_, IT_>,
                                DenseVector<Mem_, DT_, IT_> >
      {
      private:
        typedef std::pair<DT_, IT_> PAIR_;
        typedef SparseMatrixELL<Mem_, DT_, IT_> MatrixType;
        typedef DenseVector<Mem_, DT_, IT_> VectorType;

      protected:
        const MatrixType & _A;
        const SparseLayout<Mem_, IT_, MatrixType::layout_id> _layout;
        const Index _m;
        MatrixType _M;
        std::list<PAIR_> * _m_columns;
        std::vector<std::list<PAIR_> > _a_columnwise;

      public:
        SPAIPreconditionerMTdepending(const MatrixType & A, const Index m) :
          _A(A),
          _layout(_A.layout()),
          _m(m),
          _a_columnwise(_A.rows())
        {
        }

        SPAIPreconditionerMTdepending(const MatrixType & A,
                                      SparseLayout<Mem_, IT_, MatrixType::layout_id> && layout) :
          _A(A),
          _layout(std::move(layout)),
          _m((Index) -1),
          _a_columnwise(_A.rows())
        {
        }

        virtual ~SPAIPreconditionerMTdepending()
        {
        }

        void create_initial_m_columns ()
        {
          const Index n(_A.rows());

          const IT_ * playoutcol_ind(_layout.get_indices().at(0));
          const IT_ * playoutcs(_layout.get_indices().at(1));
          const IT_ * playoutrl(_layout.get_indices().at(3));
          const Index C(_layout.get_scalar_index().at(3));

          for (Index i(0); i < n; ++i)
          {
            for (Index l(playoutcs[i/C] + i%C); l < playoutcs[i/C] + i%C + playoutrl[i] * C; l += C)
            {
              _m_columns[playoutcol_ind[l]].emplace_back(DT_(0.0), i);
            }
          }
        } // function create_initial_m_columns

        void create_a_columnwise ()
        {
          const Index n(_A.rows());
          const DT_ * pval(_A.val());
          const IT_ * pacol_ind(_A.col_ind());
          const IT_ * pacs(_A.cs());
          const IT_ * parl(_A.rl());
          const Index C(_A.C());

          for (Index i(0); i < n; ++i)
          {
            for (Index l(pacs[i/C] + i%C); l < pacs[i/C] + i%C + parl[i] * C; l += C)
            {
              _a_columnwise[pacol_ind[l]].emplace_back(pval[l], i);
            }
          }
        } // function create_a_aolumnwise

        void create_m_transpose ()
        {
          const Index n(_A.rows());
          const Index C(_A.C());

          // calculate cl-array and fill rl-array
          Index num_of_chunks(Index(ceil(n / float(C))));
          DenseVector<Mem_, IT_, IT_> mcl(num_of_chunks, IT_(0));
          DenseVector<Mem_, IT_, IT_> mcs(num_of_chunks + 1);
          DenseVector<Mem_, IT_, IT_> mrl(n);
          IT_ * pmcl(mcl.elements());
          IT_ * pmcs(mcs.elements());
          IT_ * pmrl(mrl.elements());

          Index nnz(0);

          for (Index i(0); i < n; ++i)
          {
            pmrl[i] = IT_(_m_columns[i].size());
            pmcl[i/C] = Math::max(pmcl[i/C], pmrl[i]);
            nnz += Index(_m_columns[i].size());
          }

          // calculuate cs-array
          pmcs[0] = IT_(0);
          for (Index i(0); i < num_of_chunks; ++i)
          {
            pmcs[i+1] = pmcs[i] + IT_(C) * pmcl[i];
          }

          Index val_size = Index(pmcs[num_of_chunks]);

          DenseVector<Mem_, DT_, IT_> mval(val_size);
          DenseVector<Mem_, IT_, IT_> mcol_ind(val_size);
          DT_ * pmval    (mval.elements());
          IT_ * pmcol_ind(mcol_ind.elements());

          for (Index i(0), k; i < n; ++i)
          {
            k = pmcs[i/C] + i%C;
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J, k+=C)
            {
              pmcol_ind[k] = it_J->second;
              pmval    [k] = it_J->first;
            }
            for (k+=C; k < pmcs[i/C + 1]; ++k)
            {
              pmcol_ind[k] = IT_(0);
              pmval    [k] = DT_(0);
            }
          }

          _M = MatrixType(n, n, nnz, mval, mcol_ind, mcs, mcl, mrl, C);
        } // function create_m_transpose

        void create_m()
        {
          const Index n(_A.rows());
          const Index C(_A.C());

          // calculate cl-array and fill rl-array
          Index num_of_chunks(Index(ceil(n / float(C))));
          DenseVector<Mem_, IT_, IT_> mcl(num_of_chunks, IT_(0));
          DenseVector<Mem_, IT_, IT_> mcs(num_of_chunks + 1);
          DenseVector<Mem_, IT_, IT_> mrl(n, IT_(0));
          IT_ * pmcl(mcl.elements());
          IT_ * pmcs(mcs.elements());
          IT_ * pmrl(mrl.elements());

          Index nnz(0);

          for (Index i(0); i < n; ++i)
          {
            nnz += Index(_m_columns[i].size());
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              ++pmrl[it_J->second];
            }
          }

          for (Index i(0); i < n; ++i)
          {
            pmcl[i/C] = Math::max(pmcl[i/C], pmrl[i]);
            pmrl[i] = IT_(0);
          }

          // calculuate cs-array
          pmcs[0] = IT_(0);
          for (Index i(0); i < num_of_chunks; ++i)
          {
            pmcs[i+1] = pmcs[i] + IT_(C) * pmcl[i];
          }

          Index val_size = Index(pmcs[num_of_chunks]);

          DenseVector<Mem_, DT_, IT_> mval(val_size);
          DenseVector<Mem_, IT_, IT_> mcol_ind(val_size);
          DT_ * pmval    (mval.elements());
          IT_ * pmcol_ind(mcol_ind.elements());


          for (IT_ i(0); i < n; ++i)
          {
            for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
            {
              const Index k(it_J->second);
              pmcol_ind[pmcs[k/C] + k%C + pmrl[k] * C] = i;
              pmval    [pmcs[k/C] + k%C + pmrl[k] * C] = it_J->first;
              ++pmrl[k];
            }
          }

          for (Index i(0); i < n; ++i)
          {
            for (Index k(pmcs[i/C] + i%C + pmrl[i] * C); k < pmcs[i/C + 1]; k+=C)
            {
              pmcol_ind[k] = IT_(0);
              pmval    [k] = DT_(0);
            }
          }

          _M = MatrixType(n, n, nnz, mval, mcol_ind, mcs, mcl, mrl, C);
        } // function create_m

        void create_m_without_new_entries ()
        {
          const Index n(_A.rows());
          if (_m != (Index) -1)
          {
            const Index used_elements(n * (1 + 2 * _m) - _m * (_m + 1));
            const Index C(_A.C());

            // calculate cl-array and fill rl-array
            Index num_of_chunks(Index(ceil(n / float(C))));
            DenseVector<Mem_, IT_, IT_> mcl(num_of_chunks, IT_(0));
            DenseVector<Mem_, IT_, IT_> mcs(num_of_chunks + 1);
            DenseVector<Mem_, IT_, IT_> mrl(n);
            IT_ * pmcl(mcl.elements());
            IT_ * pmcs(mcs.elements());
            IT_ * pmrl(mrl.elements());

            for (Index i(0); i < n; ++i)
            {
              pmrl[i] = IT_(_m + 1 + std::min(std::min(i, _m), n - i - 1));
              pmcl[i/C] = Math::max(pmcl[i/C], pmrl[i]);
            }

            // calculuate cs-array
            pmcs[0] = IT_(0);
            for (Index i(0); i < num_of_chunks; ++i)
            {
              pmcs[i+1] = pmcs[i] + IT_(C) * pmcl[i];
            }

            Index val_size = Index(pmcs[num_of_chunks]);

            DenseVector<Mem_, DT_, IT_> mval(val_size);
            DenseVector<Mem_, IT_, IT_> mcol_ind(val_size);
            DT_ * pmval    (mval.elements());
            IT_ * pmcol_ind(mcol_ind.elements());

            for (Index i(0); i < n; ++i)
            {
              for (Index l(IT_((_m > i) ? 0 : i - _m)), k(pmcs[i/C] + i%C); l < Math::min(n, _m + i + 1); ++l, k+= C)
              {
                pmcol_ind[k] = IT_(l);
              }
              for (Index k(pmcs[i/C] + i%C + pmrl[i] * C); k < pmcs[i/C + 1]; k+=C)
              {
                pmcol_ind[k] = IT_(0);
                pmval    [k] = DT_(0);
              }
            }

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                const Index tmp(std::min(it_J->second, IT_(_m)));
                pmval[pmcs[it_J->second/C] + it_J->second%C + C * (i - it_J->second + tmp)] = it_J->first;
              }
            }

            _M = MatrixType(n, n, used_elements, mval, mcol_ind, mcs, mcl, mrl, C);
          }
          else
          {
            _M = MatrixType(_layout);

            DT_ * pmval(_M.val());
            const IT_ * pmcol_ind(_M.col_ind());
            const IT_ * pmcs(_M.cs());
            const Index C(_M.C());

            for (Index i(0); i < n; ++i)
            {
              for (auto it_J = _m_columns[i].begin(); it_J != _m_columns[i].end(); ++it_J)
              {
                Index k = pmcs[it_J->second/C] + it_J->second%C;

                while (pmcol_ind[k] != i)
                {
                  k += C;
                }

                pmval[k] = it_J->first;
              }
            }
          }
        } // function create_m_without_new_entries

        void apply_m_transpose (VectorType & out, const VectorType & in)
        {
          const Index n(_M.rows());
          const Index C(_M.C());
          const DT_ * pval(_M.val());
          const IT_ * pcol_ind(_M.col_ind());
          const IT_ * pcs(_M.cs());
          const IT_ * prl(_M.rl());
          const DT_ * pin(in.elements());
          DT_ * pout(out.elements());

          for (Index i(0); i < n; i++)
          {
            pout[i] = DT_(0.0);
          }

          for (Index i(0); i < n; i++)
          {
            for (Index c(pcs[i/C] + i%C); c < pcs[i/C] + i%C + prl[i] * C; c += C)
            {
              pout[pcol_ind[c]] += pval[c] * pin[i];
            }
          }
        } // function apply_m_transpose
      };
    } // namespace Intern
    /// \endcond


    template <typename Algo_, typename MT_, typename VT_>
    class SPAIPreconditioner : public Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>
    {
    private:
      typedef typename MT_::DataType DT_;
      typedef typename MT_::MemType Mem_;
      typedef typename MT_::IndexType IT_;
      typedef std::pair<DT_, IT_> PAIR_;

      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_A;
      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_m;
      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_layout;
      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_M;
      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_m_columns;
      using Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>::_a_columnwise;


      const DT_ _eps_res;
      const Index _fill_in;
      const Index _max_iter;
      const DT_ _eps_res_comp;
      const DT_ _max_rho;
      const bool _transpose;

    public:
      /// Our algotype
      typedef Algo_ AlgoType;
      /// Our datatype
      typedef typename MT_::DataType DataType;
      /// Our indextype
      typedef typename MT_::IndexType IndexType;
      /// Our memory architecture type
      typedef typename MT_::MemType MemType;
      /// Our vectortype
      typedef VT_ VectorType;
      /// Our matrixtype
      typedef MT_ MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_spai;

      virtual ~SPAIPreconditioner()
      {
      }

      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] m full band-matrix with m diagonals on either side as initial layout (\f$2m + 1\f$ bands)
       *
       * param[in] max_iter maximal number of iterations for creating new fill-in (default max_iter = 10)
       *
       * param[in] eps_res stopping-criterion for new fill-in: norm of residuum (default eps_res = 1e-2)
       *
       * param[in] fill_in stopping-criterion for new fill-in: maximal number of fill-in per column (default fill_in = 10)
       *
       * param[in] eps_res_comp criterion for accepting a residual-component (default eps_res_comp = 1e-3)
       *
       * param[in] max_rho criterion for acceptiong a rho-component (default max_rho = 1e-3)
       *
       * param[in] transpose If you do only want to calculate _M^T, set transpose = true (default transpose = false)
       *
       * Creates a SPAI preconditioner to the given matrix and the initial layout defined by a band-matrix with \f$2m + 1\f$ bands
       */
      SPAIPreconditioner(const MT_ & A,
                         const Index m,
                         const Index max_iter = 10,
                         const DT_ eps_res = 1e-2,
                         const Index fill_in = 10,
                         const DT_ eps_res_comp = 1e-3,
                         const DT_ max_rho = 1e-3,
                         const bool transpose = false) :
        Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>(A, m),
        _eps_res(eps_res),
        _fill_in(fill_in),
        _max_iter(max_iter),
        _eps_res_comp(eps_res_comp),
        _max_rho(max_rho),
        _transpose(transpose)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }

        const Index n(_A.columns());

        _m_columns = new std::list<PAIR_>[n];

        // __get initial structure of the j-th column of M__
        for (Index i(0); i < n; ++i)
        {
          for (Index l((m > i) ? 0 : i - m); l < Math::min(n, m + i + 1); ++l)
          {
            _m_columns[i].emplace_back(DT_(0.0), l);
          }
        }

        _create_M();
      } // constructor

      /**
       * \brief Constructor
       *
       * param[in] A system-matrix
       *
       * param[in] layout the initial layout of the approximate inverse \f$M \approx A^{-1}\f$
       *
       * param[in] max_iter maximal number of iterations for creating new fill-in (default max_iter = 10)
       *
       * param[in] eps_res stopping-criterion for new fill-in: norm of residuum (default eps_res = 1e-2)
       *
       * param[in] fill_in stopping-criterion for new fill-in: maximal number of fill-in per column (default fill_in = 10)
       *
       * param[in] eps_res_comp criterion for accepting a residual-component (default eps_res_comp = 1e-3)
       *
       * param[in] max_rho criterion for acceptiong a rho-component (default max_rho = 1e-3)
       *
       * param[in] transpose If you do only want to calculate _M^T, set transpose = true (default transpose = false)
       *
       * Creates a SPAI preconditioner to the given matrix and given initial layout
       */
      SPAIPreconditioner(const MT_ & A,
                         SparseLayout<Mem_, typename MT_::IndexType, MT_::layout_id> && layout,
                         const Index max_iter = 10,
                         const DT_ eps_res = 1e-2,
                         const Index fill_in = 10,
                         const DT_ eps_res_comp = 1e-3,
                         const DT_ max_rho = 1e-3,
                         const bool transpose = false) :
        Intern::SPAIPreconditionerMTdepending<Algo_, MT_, VT_>(A, std::move(layout)),
        _eps_res(eps_res),
        _fill_in(fill_in),
        _max_iter(max_iter),
        _eps_res_comp(eps_res_comp),
        _max_rho(max_rho),
        _transpose(transpose)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
        if (_layout.get_scalar_index().at(1) != _layout.get_scalar_index().at(2))
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Precon-layout is not square!");
        }
        if (_A.columns() != _layout.get_scalar_index().at(1))
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Precon-layout and matrix do not match!");
        }

        _m_columns = new std::list<PAIR_>[_A.rows()];

        // __get initial structure of the j-th column of M__
        this->create_initial_m_columns();

        _create_M();
      } // constructor

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "SPAI_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector, which is applied to the preconditioning
       */
      virtual void apply(DenseVector<Mem_, DT_, IT_> & out,
                         const DenseVector<Mem_, DT_, IT_> & in) override
      {
        if (_max_iter > 0 && _transpose == true)
        {
          if (in.elements() == out.elements())
          {
            throw InternalError(__func__, __FILE__, __LINE__, "Input- and output-vectors must be different!");
          }

          this->apply_m_transpose(out, in);
        }
        else
        {
          _M.template apply<Algo::Generic>(out, in);
        }
      }

    private:
      void _create_M()
      {
        /**
         * This algorithm has been adopted by
         *    Dominik Goeddeke - Schnelle Loeser (Vorlesungsskript) 2013/2014
         *    section 5.6.1; Algo 5.21.; page 154
         */

        Index n(_A.rows());
        Index mm, nn, mm_new, nn_new;
        DT_ res;
        std::vector<DT_> d(0);

        typename std::list<PAIR_>::iterator it_I, it_J, it_I_end, it_J_end;
        typename std::list<std::pair<Index, Index> >::iterator it_I_sorted;

        // __create column-structure of matrix A__
        this->create_a_columnwise();

        // Iteration over each row of \f$M \approx A^{-1}\f$
        for (Index k(0); k < n; ++k)
        {
          nn = 0;
          mm = 0;

          // Allocate memory for indices I and J of the algorithms
          // J saves the entries of the matrix \f$M \approx A^{-1}\f$, too,
          // I saves the residual \f$r_k\f$, too.
          std::list<PAIR_> & J (_m_columns[k]);
          std::list<PAIR_> I;

          it_I_end = I.begin();
          it_J_end = J.begin();

          // save size of J
          nn_new = Index(J.size());

          // __get row-indices I of matching matrix-entries__
          for (it_J = J.begin(); it_J != J.end(); ++it_J)
          {
            Index col = it_J->second;
            it_I = I.begin();
            for (auto it_col = _a_columnwise[col].begin();
                 it_col != _a_columnwise[col].end(); ++it_col)
            {
              Index row = it_col->second;
              while (it_I != I.end() && it_I->second < row)
              {
                ++it_I;
              }
              if (it_I == I.end() || it_I->second != row)
              {
                I.emplace(it_I, DT_(0.0), row);
              }
            }
          }

          // save size of I
          mm_new = Index(I.size());

          // save sorted list of I
          std::list<std::pair<Index, Index> > I_sorted(I.size());
          it_I_sorted = I_sorted.begin();
          it_I = I.begin();
          for (Index i(0); i < mm_new; ++i, ++it_I_sorted, ++it_I)
          {
            it_I_sorted->second = it_I->second;
            it_I_sorted->first = i;
          }

          // allocate dynamic matrix for saving the transposed matrices \f$QR(I,J)^\top\f$ and \f$A(I,J)^\top\f$
          std::vector<std::vector<DT_> > qr;
          std::vector<std::vector<DT_> > local;

          // __While \f$res = \|r_k\| > eps\f$, the maximal number of iterations and fill-in is not reached, repeat...__
          Index iter(0);
          while (true)
          {
            // resize matrices qr and local
            qr.resize(nn_new, std::vector<DT_>(mm_new, DT_(0.0)));
            local.resize(nn_new, std::vector<DT_>(mm_new, DT_(0.0)));

            // fill temporary matrix qr and local with entries of A(I,J)
            it_J = ((it_J_end == J.begin()) ? J.begin() : std::next(it_J_end));
            for (Index j(nn); j < nn_new; ++j, ++it_J)
            {
              Index col = it_J->second;
              it_I_sorted = I_sorted.begin();
              for (auto it_col = _a_columnwise[col].begin();
                   it_col != _a_columnwise[col].end(); ++it_col)
              {
                while (it_I_sorted->second < it_col->second)
                {
                  ++it_I_sorted;
                }
                qr[j][it_I_sorted->first] = it_col->first;
                local[j][it_I_sorted->first] = it_col->first;
              }
            }

            // mulitply last/new rows of matrix qr with \f$Q^\top\f$
            for (Index k1(nn); k1 < nn_new; ++k1)
            {
              // calculate \f$e_k1 = Q^\top \cdot e_k1\f$
              for (Index j(0); j < nn; ++j)
              {
                DT_ s = DT_(0.0);
                for (Index l(j); l < qr[j].size(); ++l)
                {
                  s += qr[j][l] * qr[k1][l];
                }
                for (Index l(j); l < qr[j].size(); ++l)
                {
                  qr[k1][l] -= qr[j][l] * s;
                }
              }
            }

            // __calculate the qr-decomposition of \f$A(\tilde I, \tilde J\f$__
            // notice: we only have to calculate the qr-decomposition for the new columns \f$\tilde J \setminus J\f$
            // resize the dynamic vector for saving the diagonal entries of R
            d.resize(nn_new);

            for (Index j(nn); j < nn_new; ++j)
            {
              DT_ s = DT_(0.0);
              for (Index i(j); i < mm_new; ++i)
              {
                s += Math::sqr(qr[j][i]);
              }
              s = Math::sqrt(s);

              if (qr[j][j] > 0)
              {
                d[j] = -s;
              }
              else
              {
                d[j] = s;
              }

              DT_ fak = Math::sqrt(s * (s + Math::abs(qr[j][j])));
              qr[j][j] -= d[j];

              for (Index l(j); l < mm_new; ++l)
              {
                qr[j][l] /= fak;
              }

              for (Index i(j + 1); i < nn_new; ++i)
              {
                s = DT_(0.0);
                for (Index l = j; l < mm_new; ++l)
                {
                  s += qr[j][l] * qr[i][l];
                }
                for (Index l(j); l < mm_new; ++l)
                {
                  qr[i][l] -= qr[j][l] * s;
                }
              }
            }

            // __calculate m_k as the solution of the least-squares-problem \f$A(I,J)^\top \cdot A(I,J) \cdot m_k = A^T(I,J) \cdot e_k\f$__
            // calculate \f$e_k\f$
            std::vector<DT_> e(mm_new);
            it_I = I.begin();
            for (Index i(0); i < mm_new; ++i, ++it_I)
            {
              if (it_I->second == k)
              {
                e[i] = DT_(1.0);
                it_I->first = DT_(-1.0);
              }
              else
              {
                e[i] = DT_(0.0);
                it_I->first = DT_(0.0);
              }
            }

            // calculate \f$e_k = Q^\top \cdot e_k\f$
            for (Index j(0); j < nn_new; ++j)
            {
              DT_ s = DT_(0.0);
              for (Index l(j); l < qr[j].size(); ++l)
              {
                s += qr[j][l] * e[l];
              }
              for (Index l(j); l < qr[j].size(); ++l)
              {
                e[l] -= qr[j][l] * s;
              }
            }

            // solve \f$R^{-1} e_k = e_k\f$
            for (Index i(nn_new); i > 0;)
            {
              --i;
              for (Index j(i+1); j < nn_new; ++j)
              {
                e[i] -= qr[j][i] * e[j];
              }
              e[i] /= d[i];
            }

            // save values of e_k in J->first
            it_J = J.begin();
            for (Index j(0); j < nn_new; ++j, ++it_J)
            {
              it_J->first = e[j];
            }

            // termination condition
            if (iter >= _max_iter || nn_new >= _fill_in)
            {
              break;
            }
            ++iter;

            // __calculate the residual \f$r_k = A(I,J) \cdot J\to first - e_k\f$__
            it_J = J.begin();
            for (Index j(0); j < nn_new; ++j, ++it_J)
            {
              it_I = I.begin();
              for (Index i(0); i < qr[j].size(); ++i, ++it_I)
              {
                it_I->first += local[j][i] * it_J->first;
              }
            }

            // __calculate the norm of the residual \f$res = \|r_k\|\f$
            res = DT_(0.0);
            for (it_I = I.begin(); it_I != I.end(); ++it_I)
            {
              res += Math::sqr(it_I->first);
            }
            res = Math::sqrt(res);

            // set old dimensions of I and J to the new one
            mm = mm_new;
            nn = nn_new;

            // termination condition
            if (res < _eps_res)
            {
              break;
            }

            // allocate memory for \f$ \rho \f$
            std::vector<std::pair<DT_, Index> > rho(mm);

            // __Search indices \f$i \in I\f$ with \f$i \not\in J\f$ and \f$r_k(i) \not=0\f$ and calculate \f$\rho_i\f$__
            // notice: if iter > 1 J is not sorted
            it_I = I.begin();
            for (Index i(0); i < mm; ++i, ++it_I)
            {
              if (Math::abs(it_I->first) < _eps_res_comp)
              {
                rho[i].first = DT_(0.0);
                rho[i].second = it_I->second;
                continue;
              }

              it_J = J.begin();
              while (it_J->second != it_I->second && std::next(it_J) != J.end())
              {
                ++it_J;
              }
              if (it_J->second == it_I->second)
              {
                continue;
              }

              DT_ s(DT_(0.0));
              auto it = I.begin();
              for (auto it_col = _a_columnwise[it_I->second].begin();
                   it_col != _a_columnwise[it_I->second].end(); ++it_col)
              {
                s += Math::sqr(it_col->first);
                while (it->second != it_col->second && std::next(it) != I.end())
                {
                  ++it;
                }
                if (it->second == it_col->second)
                {
                  rho[i].first += it->first * it_col->first;
                }
              }
              rho[i].first = Math::sqr(res) - Math::sqr(rho[i].first) / s;
              rho[i].second = it_I->second;
            }

            // save the iterators to the last entries of I and J
            it_I_end = std::prev(I.end());
            it_J_end = std::prev(J.end());

            // __add promising entries of \f$ \tilde J \f$ to \f$ J \f$
            bool first = true;
            while (J.size() < _fill_in)
            {
              // search maximal value in rho
              DT_ max_val = DT_(0.0);
              Index max_ind(0), max_sec(0);

              for (Index i(0); i < mm; ++i)
              {
                if (rho[i].first > max_val)
                {
                  max_val = rho[i].first;
                  max_sec = rho[i].second;
                  max_ind = i;
                }
              }

              if (max_val > _max_rho)
              {
                rho[max_ind].first = DT_(0.0);
                if (first == true)
                {
                  J.emplace_back(DT_(0.0), max_sec);
                  first = false;
                  it_J = std::next(it_J_end);
                }
                else
                {
                  if (it_J->second > max_sec)
                  {
                    it_J = std::next(it_J_end);
                  }
                  while (it_J != J.end() && it_J->second < max_sec)
                  {
                    ++it_J;
                  }
                  it_J = J.emplace(it_J, DT_(0.0), max_sec);
                }
              }
              else
              {
                break;
              }
            }

            // save new size of J
            nn_new = Index(J.size());

            // we can stop if no new entries have been added
            if (nn_new == nn)
            {
              break;
            }

            // calculate new indices \f$ \tilde I \f$
            for (it_J = std::next(it_J_end); it_J != J.end(); ++it_J)
            {
              Index col = it_J->second;
              it_I_sorted = I_sorted.begin();
              for (auto it_col = _a_columnwise[col].begin();
                   it_col != _a_columnwise[col].end(); ++it_col)
              {
                Index row = it_col->second;
                while (it_I_sorted != I_sorted.end() && it_I_sorted->second < row)
                {
                  ++it_I_sorted;
                }
                if (it_I_sorted == I_sorted.end() || it_I_sorted->second != row)
                {
                  I_sorted.emplace(it_I_sorted, -1, row);
                  it_I = std::next(it_I_end);
                  while (it_I != I.end() && it_I->second < row)
                  {
                    ++it_I;
                  }
                  I.emplace(it_I, DT_(0.0), row);
                }
              }
            }

            // save new size of I
            mm_new = Index(I.size());

            // fill sorted vector sorted_I with new entries of I
            for (it_I_sorted = I_sorted.begin(); it_I_sorted != I_sorted.end(); ++it_I_sorted)
            {
              if (it_I_sorted->first != (Index) -1)
              {
                continue;
              }
              it_I = std::next(it_I_end);
              Index i(mm);
              while (it_I->second != it_I_sorted->second)
              {
                ++it_I;
                ++i;
              }
              it_I_sorted->first = i;
            }
          }
        } // end for-loop over each row of \f$M \approx A^{-1}\f$

        // Create matrix M with calculated entries
        if (_max_iter > 0)
        {
          if (_transpose == true)
          {
            this->create_m_transpose();
          }
          else
          {
            this->create_m();
          }
        }
        else
        {
          this->create_m_without_new_entries();
        }

        delete[] _m_columns;
      } // function _create_m
    };


    /**
     * \brief Polynomial-Preconditioner.
     *
     * This class represents the Neumann-Polynomial-Preconditioner \f$M^{-1} = \sum_{k=0}^m (I - \tilde M^{-1}A)^k \tilde M^{-1}\f$
     *
     * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
     *
     * \author Christoph Lohmann
     */
    template <typename Algo_, typename MT_, typename VT_>
    class PolynomialPreconditioner : public Preconditioner<Algo_, MT_, VT_>
    {
    private:
      typedef typename MT_::MemType Mem_;
      typedef typename MT_::DataType DT_;

      const MT_ & _A;                              // system-matrix
      const Index _m;                              // order m of preconditioner
      const Index _num_of_auxs;                    // number of auxilary-vectors
                                                   // 1 for NonePreconditioner
                                                   // 2 if out and in can be the same vectors for _precon.apply
                                                   // 3 else
      VT_ _aux1, _aux2, _aux3;                     // auxilary-vector
      Preconditioner<Algo_, MT_, VT_> * _precond;

    public:
      /// Our algotype
      typedef Algo_ AlgoType;
      /// Our datatype
      typedef typename MT_::DataType DataType;
      /// Our memory architecture type
      typedef typename MT_::MemType MemType;
      /// Our vectortype
      typedef VT_ VectorType;
      /// Our matrixtype
      typedef MT_ MatrixType;
      /// Our used precon type
      const static SparsePreconType PreconType = SparsePreconType::pt_polynomial;

      /**
       * \brief Destructor
       *
       * deletes the preconditionier _precond
       */
      virtual ~PolynomialPreconditioner()
      {
        delete _precond;
      }

      /**
       * \brief Constructor of Neumann-Polynomial-Preconditioner.
       *
       * param[in] A system-matrix
       * param[in] m order of the polynom
       * param[in] precond A preconditioner used for the Polynomial preconditioner
       *
       * Creates a Polynomial preconditioner to the given matrix and the given order
       */
      PolynomialPreconditioner(const MT_ & A, Index m, Preconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(3),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(_A.rows()),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      /// \cond internal
      PolynomialPreconditioner(const MT_ & A, Index m, NonePreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(1),
        _aux1(_A.rows()),
        _aux2(),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      PolynomialPreconditioner(const MT_ & A, Index m, JacobiPreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(2),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      PolynomialPreconditioner(const MT_ & A, Index m, GaussSeidelPreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(2),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      PolynomialPreconditioner(const MT_ & A, Index m, ILUPreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(2),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      PolynomialPreconditioner(const MT_ & A, Index m, SORPreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(2),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }

      PolynomialPreconditioner(const MT_ & A, Index m, SSORPreconditioner<Algo_, MT_, VT_> * precond) :
        _A(A),
        _m(m),
        _num_of_auxs(2),
        _aux1(_A.rows()),
        _aux2(_A.rows()),
        _aux3(),
        _precond(precond)
      {
        if (_A.columns() != _A.rows())
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Matrix is not square!");
        }
      }
      /// \endcond

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "Polynomial_Preconditioner";
      }

      /**
       * \brief apply the preconditioner
       *
       * \param[out] out The preconditioner result.
       * \param[in] in The vector to be preconditioned.
       */
      virtual void apply(VT_ & out, const VT_ & in) override
      {
        /*
         * preconditioner is given by
         *   \f$ M^-1 = \left(I + (I - \tilde M^{-1}A) + ... + (I - \tilde M^{-1 }A)^m\right) \tilde M^{-1} \f$
         *
         * the preconditioner only works, if
         *   ||I - \tilde M^{-1} A||_2 < 1.
         */

        if (_num_of_auxs == 1) // if preconditioner = NonePreconditioner
        {
          out.copy(in);

          for (Index i = 1; i <= _m; ++i)
          {
            // _A.template apply<Algo_>(_aux1, in, out, DataType(-1.0));
            // out.template axpy<Algo_>(out, _aux1);

            _A.template apply<Algo_>(_aux1, out);
            out.template axpy<Algo_>(out, in);
            out.template axpy<Algo_>(_aux1, out, DataType(-1.0));
          }
        }
        else if (_num_of_auxs == 2) // if out and in can be the same vectors
        {
          _precond->apply(out, in);
          _aux2.copy(out);

          for (Index i = 1; i <= _m; ++i)
          {
            _A.template apply<Algo_>(_aux1, out);
            _precond->apply(_aux1, _aux1);
            out.template axpy<Algo_>(out, _aux2);
            out.template axpy<Algo_>(_aux1, out, DataType(-1.0));
          }
        }
        else // if out and in must be different vectors
        {
          _precond->apply(out, in);
          _aux3.copy(out);

          for (Index i = 1; i <= _m; ++i)
          {
            _A.template apply<Algo_>(_aux1, out);
            _precond->apply(_aux2, _aux1);
            out.template axpy<Algo_>(out, _aux3);
            out.template axpy<Algo_>(_aux2, out, DataType(-1.0));
          }
        }
      } // function apply
    };
  }// namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_PRECONDITIONER_HPP
