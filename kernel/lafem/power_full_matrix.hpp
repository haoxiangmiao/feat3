#pragma once
#ifndef KERNEL_LAFEM_POWER_FULL_MATRIX_HPP
#define KERNEL_LAFEM_POWER_FULL_MATRIX_HPP 1

// includes, FEAST
#include <kernel/lafem/power_row_matrix.hpp>
#include <kernel/lafem/power_col_matrix.hpp>
#include <kernel/lafem/power_vector.hpp>
#include <kernel/lafem/sparse_layout.hpp>

namespace FEAST
{
  namespace LAFEM
  {
    /**
     * \brief Power-Full-Matrix meta class template
     *
     * This class template implements a composition of \e m x\e n sub-matrices of the same class.
     * This can be interpreted as an m-by-n matrix of other matrices.
     *
     * \tparam SubType_
     * The type of the sub-matrix.
     *
     * \tparam width_
     * The number of sub-matrix blocks per row.
     *
     * \tparam height_
     * The number of sub-matrix blocks per column.
     *
     * \author Peter Zajac
     */
    template<
      typename SubType_,
      Index width_,
      Index height_>
    class PowerFullMatrix :
      protected PowerColMatrix<PowerRowMatrix<SubType_, width_>, height_>
    {
      /// base-class typedef
      typedef  PowerColMatrix<PowerRowMatrix<SubType_, width_>, height_> BaseClass;

    public:
      /// sub-matrix type
      typedef SubType_ SubMatrixType;
      /// sub-matrix memory type
      typedef typename SubMatrixType::MemType MemType;
      /// sub-matrix data type
      typedef typename SubMatrixType::DataType DataType;
      /// sub-matrix index type
      typedef typename SubMatrixType::IndexType IndexType;
      /// sub-matrix layout type
      static constexpr SparseLayoutId layout_id = SubMatrixType::layout_id;
      /// Compatible L-vector type
      typedef PowerVector<typename SubMatrixType::VectorTypeL, height_> VectorTypeL;
      /// Compatible R-vector type
      typedef PowerVector<typename SubMatrixType::VectorTypeR, width_> VectorTypeR;

      /// dummy enum
      enum
      {
        /// number of row blocks (vertical size)
        num_row_blocks = height_,
        /// number of column blocks (horizontal size)
        num_col_blocks = width_
      };

    protected:
      /// base-class emplacement constructor
      explicit PowerFullMatrix(BaseClass&& base) :
        BaseClass(base)
      {
      }

    public:
      /// default ctor
      PowerFullMatrix()
      {
      }

      /// sub-matrix layout ctor
      explicit PowerFullMatrix(const SparseLayout<MemType, IndexType, layout_id>& layout) :
        BaseClass(layout)
      {
      }

      /// move ctor
      PowerFullMatrix(PowerFullMatrix&& other) :
        BaseClass(static_cast<BaseClass&&>(other))
      {
      }

      /// move-assign operator
      PowerFullMatrix& operator=(PowerFullMatrix&& other)
      {
        BaseClass::operator=(static_cast<BaseClass&&>(other));
        return *this;
      }

      /// deleted copy-ctor
      PowerFullMatrix(const PowerFullMatrix&) = delete;
      /// deleted copy-assign operator
      PowerFullMatrix& operator=(const PowerFullMatrix&) = delete;

      /// virtual destructor
      virtual ~PowerFullMatrix()
      {
      }

      /**
       * \brief Creates and returns a deep copy of this matrix.
       */
      PowerFullMatrix clone() const
      {
        return PowerFullMatrix(BaseClass::clone());
      }

      /**
       * \brief Returns a sub-matrix block.
       *
       * \tparam i_
       * The row index of the sub-matrix block that is to be returned.
       *
       * \tparam j_
       * The column index of the sub-matrix block that is to be returned.
       *
       * \returns
       * A (const) reference to the sub-matrix at position <em>(i_,j_)</em>.
       */
      template<Index i_, Index j_>
      SubMatrixType& at()
      {
        static_assert(i_ < height_, "invalid sub-matrix index");
        static_assert(j_ < width_, "invalid sub-matrix index");
        return static_cast<BaseClass&>(*this).template at<i_, Index(0)>().template at<Index(0),j_>();
      }

      /** \copydoc at() */
      template<Index i_, Index j_>
      const SubMatrixType& at() const
      {
        static_assert(i_ < height_, "invalid sub-matrix index");
        static_assert(j_ < width_, "invalid sub-matrix index");
        return static_cast<const BaseClass&>(*this).template at<i_, Index(0)>().template at<Index(0),j_>();
      }

      /// \cond internal
      Index row_blocks() const
      {
        return Index(num_row_blocks);
      }

      Index col_blocks() const
      {
        return Index(num_col_blocks);
      }
      /// \endcond

      // Note: The following function definitions may seem pointless, but unfortunately the
      // MSVC compiler crashes with an internal error if they are missing...
      VectorTypeL create_vector_l() const
      {
        return BaseClass::create_vector_l();
      }

      VectorTypeR create_vector_r() const
      {
        return BaseClass::create_vector_r();
      }

      template<typename Algo_>
      void apply(VectorTypeL& r, const VectorTypeR& x)
      {
        BaseClass::template apply<Algo_>(r, x);
      }

      template<typename Algo_>
      void apply(VectorTypeL& r, const VectorTypeR& x, const VectorTypeL& y, DataType alpha = DataType(1))
      {
        BaseClass::template apply<Algo_>(r, x, y, alpha);
      }
    }; // class PowerFullMatrix
  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_POWER_FULL_MATRIX_HPP