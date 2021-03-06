// FEAT3: Finite Element Analysis Toolbox, Version 3
// Copyright (C) 2010 - 2020 by Stefan Turek & the FEAT group
// FEAT3 is released under the GNU General Public License version 3,
// see the file 'copyright.txt' in the top level directory for details.

#pragma once
#ifndef KERNEL_LAFEM_UNIT_FILTER_HPP
#define KERNEL_LAFEM_UNIT_FILTER_HPP 1

// includes, FEAT
#include <kernel/base_header.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/lafem/sparse_matrix_ell.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_vector.hpp>
#include <kernel/lafem/arch/unit_filter.hpp>

namespace FEAT
{
  namespace LAFEM
  {
    /**
     * \brief Unit Filter class template.
     *
     * \author Peter Zajac
     */
    template<
      typename Mem_,
      typename DT_,
      typename IT_ = Index>
    class UnitFilter
    {
    public:
      /// mem-type typedef
      typedef Mem_ MemType;
      /// data-type typedef
      typedef DT_ DataType;
      /// index-type typedef
      typedef IT_ IndexType;

      /// our supported vector type
      typedef DenseVector<MemType, DataType, IndexType> VectorType;

      /// Our 'base' class type
      template <typename Mem2_, typename DT2_ = DT_, typename IT2_ = IT_>
      using FilterType = UnitFilter<Mem2_, DT2_, IT2_>;

      /// this typedef lets you create a filter with new Memory, Datatape and Index types
      template <typename Mem2_, typename DataType2_, typename IndexType2_>
      using FilterTypeByMDI = FilterType<Mem2_, DataType2_, IndexType2_>;

      static constexpr bool is_global = false;
      static constexpr bool is_local = true;

    private:
      /// SparseVector, containing all entries of the unit filter
      SparseVector<Mem_, DT_, IT_> _sv;

    public:
      /// default constructor
      UnitFilter() :
        _sv()
      {
      }

      /**
       * \brief Constructor.
       *
       * \param[in] size_in
       * The total size of the unit filter.
       */
      explicit UnitFilter(Index size_in) :
        _sv(size_in)
      {
      }

      /**
       * \brief Constructor.
       *
       * \param[in] size_in The size of the created filter.
       * \param[in] values DenseVector containing element values
       * \param[in] indices DenseVector containing element indices
       */
      explicit UnitFilter(Index size_in, DenseVector<Mem_, DT_, IT_> & values, DenseVector<Mem_, IT_, IT_> & indices) :
        _sv(size_in, values, indices)
      {
        XASSERTM(values.size() == indices.size(), "Vector size mismatch!");
      }

      /// move-ctor
      UnitFilter(UnitFilter && other) :
        _sv(std::move(other._sv))
      {
      }

      /// move-assignment operator
      UnitFilter & operator=(UnitFilter && other)
      {
        if(this != &other)
        {
          _sv = std::forward<decltype(other._sv)>(other._sv);
        }
        return *this;
      }

      /// virtual destructor
      virtual ~UnitFilter()
      {
      }

      /// \brief Creates a clone of itself
      UnitFilter clone(CloneMode clone_mode = CloneMode::Deep) const
      {
        UnitFilter other;
        other.clone(*this, clone_mode);
        return other;
      }

      /// \brief Clones data from another UnitFilter
      void clone(const UnitFilter & other, CloneMode clone_mode = CloneMode::Deep)
      {
        _sv.clone(other.get_filter_vector(), clone_mode);
      }

      /// \brief Converts data from another UnitFilter
      template<typename Mem2_, typename DT2_, typename IT2_>
      void convert(const UnitFilter<Mem2_, DT2_, IT2_>& other)
      {
        _sv.convert(other.get_filter_vector());
      }

      /// \brief Clears the underlying data (namely the SparseVector)
      void clear()
      {
        _sv.clear();
      }

      /// \brief Returns the total amount of bytes allocated.
      std::size_t bytes() const
      {
        return _sv.bytes();
      }

      /// \cond internal
      SparseVector<Mem_, DT_, IT_>& get_filter_vector()
      {
        return _sv;
      }

      const SparseVector<Mem_, DT_, IT_>& get_filter_vector() const
      {
        return _sv;
      }
      /// \endcond

      /**
       * \brief Adds one element to the filter
       *
       * \param[in] idx Index where to add
       * \param[in] val Value to add
       *
       **/
      void add(IndexType idx, DataType val)
      {
        _sv(idx, val);
      }

      /// \returns The total size of the filter.
      Index size() const
      {
        return _sv.size();
      }

      /// \returns The number of entries in the filter.
      Index used_elements() const
      {
        return _sv.used_elements();
      }

      /// \returns The index array.
      IT_* get_indices()
      {
        return _sv.indices();
      }

      /// \returns The index array.
      const IT_* get_indices() const
      {
        return _sv.indices();
      }

      /// \returns The value array.
      DT_* get_values()
      {
        return _sv.elements();
      }

      /// \returns The value array.
      const DT_* get_values() const
      {
        return _sv.elements();
      }

      /// Permutate internal vector according to the given Permutation
      void permute(Adjacency::Permutation & perm)
      {
        _sv.permute(perm);
      }

#ifdef DOXYGEN
      // The following documentation block is visible to Doxygen only. The actual implementation is matrix type
      // specific and provided below.

      /**
       * \brief Applies the filter onto a system matrix.
       *
       * \param[in,out] matrix
       * A reference to the matrix to be filtered.
       *
       */
      void filter_mat(MatrixType & matrix) const
      {
      }

      /**
       * \brief Filter the non-diagonal entries, row wise
       *
       * \param[in,out] matrix
       * A reference to the matrix to be filtered.
       *
       */
      void filter_offdiag_row_mat(MatrixType & matrix) const
      {
      }

      /**
       * \brief Filter the non-diagonal entries, column wise
       *
       * \param[in,out] matrix
       * A reference to the matrix to be filtered.
       *
       */
      void filter_offdiag_col_mat(MatrixType & matrix) const
      {
      }

#endif
      ///\cond internal
      void filter_mat(SparseMatrixCSR<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const IndexType* row_ptr(matrix.row_ptr());
        const IndexType* col_idx(matrix.col_ind());
        DT_* v(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          Index ix(_sv.indices()[i]);
          // replace by unit row
          for(IndexType j(row_ptr[ix]); j < row_ptr[ix + 1]; ++j)
          {
            v[j] = (col_idx[j] == ix) ? DT_(1) : DT_(0);
          }
        }
      }

      void filter_offdiag_row_mat(SparseMatrixCSR<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const IndexType* row_ptr(matrix.row_ptr());
        DT_* v(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          Index ix(_sv.indices()[i]);
          // replace by null row
          for(IndexType j(row_ptr[ix]); j < row_ptr[ix + 1]; ++j)
          {
            v[j] = DT_(0);
          }
        }
      }

      void filter_offdiag_col_mat(SparseMatrixCSR<Mem::Main, DT_, IT_> &) const
      {
        // nothing to do here
      }

      void filter_mat(SparseMatrixCOO<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const Index tused_elements(matrix.used_elements());
        const IndexType* row_idx(matrix.row_indices());
        const IndexType* col_idx(matrix.column_indices());
        DT_* v(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          IndexType ix(_sv.indices()[i]);
          // replace by unit row
          Index j(0);
          while(row_idx[j] < ix)
          {
            ++j;
          }
          while (j < tused_elements && row_idx[j] <= ix)
          {
            v[j] = (col_idx[j] == ix) ? DT_(1) : DT_(0);
            ++j;
          }
        }
      }

      void filter_offdiag_row_mat(SparseMatrixCOO<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const Index tused_elements(matrix.used_elements());
        const Index* row_idx(matrix.row_indices());
        DT_* v(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          IndexType ix(_sv.indices()[i]);
          // replace by null row
          Index j(0);
          while(row_idx[j] < ix)
          {
            ++j;
          }
          while (j < tused_elements && row_idx[j] <= ix)
          {
            v[j] = DT_(0);
            ++j;
          }
        }
      }

      void filter_offdiag_col_mat(SparseMatrixCOO<Mem::Main, DT_, IT_> &) const
      {
        // nothing to do here
      }

      void filter_mat(SparseMatrixELL<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const Index tC(matrix.C());
        const IndexType * pcs(matrix.cs());
        const IndexType * pcol_ind(matrix.col_ind());
        DT_* pval(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          IndexType ix(_sv.indices()[i]);
          // replace by unit row
          for(IndexType j(pcs[ix/tC] + ix%tC); j < pcs[ix/tC + 1]; j += tC)
          {
            pval[j] = (pcol_ind[j] == ix) ? DT_(1) : DT_(0);
          }
        }
      }

      void filter_offdiag_row_mat(SparseMatrixELL<Mem::Main, DT_, IT_> & matrix) const
      {
        XASSERTM(_sv.size() == matrix.rows(), "Matrix size does not match!");

        const Index tC(matrix.C());
        const IndexType * pcs(matrix.cs());
        DT_* pval(matrix.val());

        for(Index i(0); i < _sv.used_elements(); ++i)
        {
          IndexType ix(_sv.indices()[i]);
          // replace by null row
          for(IndexType j(pcs[ix/tC] + ix%tC); j < pcs[ix/tC + 1]; j += tC)
          {
            pval[j] = DT_(0);
          }
        }
      }

      void filter_offdiag_col_mat(SparseMatrixELL<Mem::Main, DT_, IT_> &) const
      {
        // nothing to do here
      }
      /// \endcond

      /**
       * \brief Applies the filter onto the right-hand-side vector.
       *
       * \param[in,out] vector
       * A reference to the right-hand-side vector to be filtered.
       */
      void filter_rhs(DenseVector<Mem_, DT_, IT_> & vector) const
      {
        XASSERTM(_sv.size() == vector.size(), "Vector size does not match!");
        if(_sv.used_elements() > Index(0))
          Arch::UnitFilter<Mem_>::filter_rhs(vector.elements(), _sv.elements(), _sv.indices(), _sv.used_elements());
      }

      /**
       * \brief Applies the filter onto the solution vector.
       *
       * \param[in,out] vector
       * A reference to the solution vector to be filtered.
       */
      void filter_sol(DenseVector<Mem_, DT_, IT_> & vector) const
      {
        // same as rhs
        filter_rhs(vector);
      }

      /**
       * \brief Applies the filter onto a defect vector.
       *
       * \param[in,out] vector
       * A reference to the defect vector to be filtered.
       */
      void filter_def(DenseVector<Mem_, DT_, IT_> & vector) const
      {
        XASSERTM(_sv.size() == vector.size(), "Vector size does not match!");
        if(_sv.used_elements() > Index(0))
          Arch::UnitFilter<Mem_>::filter_def(vector.elements(), _sv.indices(), _sv.used_elements());
      }

      /**
       * \brief Applies the filter onto a correction vector.
       *
       * \param[in,out] vector
       * A reference to the correction vector to be filtered.
       */
      void filter_cor(DenseVector<Mem_, DT_, IT_> & vector) const
      {
        // same as def
        filter_def(vector);
      }
    }; // class UnitFilter<...>
  } // namespace LAFEM
} // namespace FEAT

#endif // KERNEL_LAFEM_UNIT_FILTER_HPP
