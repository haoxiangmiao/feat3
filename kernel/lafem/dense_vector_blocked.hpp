#pragma once
#ifndef KERNEL_LAFEM_DENSE_VECTOR_BLOCKED_HPP
#define KERNEL_LAFEM_DENSE_VECTOR_BLOCKED_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/lafem/forward.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/util/type_traits.hpp>
#include <kernel/util/math.hpp>
#include <kernel/lafem/container.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/vector_base.hpp>
#include <kernel/lafem/arch/sum.hpp>
#include <kernel/lafem/arch/difference.hpp>
#include <kernel/lafem/arch/dot_product.hpp>
#include <kernel/lafem/arch/norm.hpp>
#include <kernel/lafem/arch/scale.hpp>
#include <kernel/lafem/arch/axpy.hpp>
#include <kernel/lafem/arch/component_product.hpp>
#include <kernel/util/tiny_algebra.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>


namespace FEAST
{
  namespace LAFEM
  {
    /**
     * \brief Blocked Dense data vector class template.
     *
     * \tparam Mem_ The memory architecture to be used.
     * \tparam DT_ The datatype to be used.
     * \tparam IT_ The indextype to be used.
     * \tparam BlockSize_ The size of the represented blocks
     *
     * This class represents a vector of continuous data in memory.\n
     * Logical, the data are organized in small blocks of Blocksize_ length.\n\n
     * Data survey: \n
     * _elements[0]: raw number values \n
     * _scalar_index[0]: container size - aka block count
     *
     * \author Dirk Ribbrock
     */
    template <typename Mem_, typename DT_, typename IT_, Index BlockSize_>
    class DenseVectorBlocked : public Container<Mem_, DT_, IT_>, public VectorBase
    {
      private:
        Index & _size()
        {
          return this->_scalar_index.at(0);
        }

      public:
        /// Our datatype
        typedef DT_ DataType;
        /// Our indextype
        typedef IT_ IndexType;
        /// Our memory architecture type
        typedef Mem_ MemType;
        /// Our size of single blocks
        static constexpr Index BlockSize = BlockSize_;

        /**
         * \brief Constructor
         *
         * Creates an empty non dimensional vector.
         */
        explicit DenseVectorBlocked() :
          Container<Mem_, DT_, IT_> (0)
        {
          CONTEXT("When creating DenseVectorBlocked");
        }

        /**
         * \brief Constructor
         *
         * \param[in] size The size of the created vector. aka block count
         *
         * Creates a vector with a given block count.
         */
        explicit DenseVectorBlocked(Index size_in) :
          Container<Mem_, DT_, IT_>(size_in)
        {
          CONTEXT("When creating DenseVectorBlocked");

          this->_elements.push_back(MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(raw_size()));
          this->_elements_size.push_back(raw_size());
        }

        /**
         * \brief Constructor
         *
         * \param[in] size The size of the created vector.
         * \param[in] value The value, each element will be set to.
         *
         * Creates a vector with given size and value.
         */
        explicit DenseVectorBlocked(Index size_in, DT_ value) :
          Container<Mem_, DT_, IT_>(size_in)
        {
          CONTEXT("When creating DenseVectorBlocked");

          this->_elements.push_back(MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(raw_size()));
          this->_elements_size.push_back(raw_size());

          MemoryPool<Mem_>::instance()->set_memory(this->_elements.at(0), value, raw_size());
        }

        /**
         * \brief Constructor
         *
         * \param[in] size The block count of the created vector.
         * \param[in] data An array containing the value data.
         *
         * Creates a vector with given size and given data.
         */
        explicit DenseVectorBlocked(Index size_in, DT_ * data) :
          Container<Mem_, DT_, IT_>(size_in)
        {
          CONTEXT("When creating DenseVectorBlocked");

          this->_elements.push_back(data);
          this->_elements_size.push_back(raw_size());

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            MemoryPool<Mem_>::instance()->increase_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            MemoryPool<Mem_>::instance()->increase_memory(this->_indices.at(i));
        }

        /**
         * \brief Constructor
         *
         * \param[in] other The source DenseVector.
         *
         * Creates a vector from a DenseVector source
         */
        explicit DenseVectorBlocked(const DenseVector<Mem_, DT_, IT_> & other) :
          Container<Mem_, DT_, IT_>(other.size() / BlockSize_)
        {
          CONTEXT("When creating DenseVectorBlocked");
          convert(other);
        }

        /**
         * \brief Move Constructor
         *
         * \param[in] other The source vector.
         *
         * Moves another vector to this vector.
         */
        DenseVectorBlocked(DenseVectorBlocked && other) :
          Container<Mem_, DT_, IT_>(std::forward<DenseVectorBlocked>(other))
        {
          CONTEXT("When moving DenseVectorBlocked");
        }

        /**
         * \brief Assignment move operator
         *
         * \param[in] other The source vector.
         *
         * Moves another vector to the target vector.
         */
        DenseVectorBlocked & operator= (DenseVectorBlocked && other)
        {
          CONTEXT("When moving DenseVectorBlocked");

          this->move(std::forward<DenseVectorBlocked>(other));

          return *this;
        }


        /** \brief Clone operation
         *
         * Create a deep copy of itself.
         *
         */
        DenseVectorBlocked clone() const
        {
          DenseVectorBlocked t;
          t.clone(*this);
          return t;
        }

        using Container<Mem_, DT_, IT_>::clone;

        /**
         * \brief Convertion method
         *
         * \param[in] other The source vector.
         *
         * Use source vector content as content of current vector
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const DenseVectorBlocked<Mem2_, DT2_, IT2_, BlockSize_> & other)
        {
          CONTEXT("When converting DenseVectorBlocked");
          this->assign(other);
        }

        /**
         * \brief Convertion method
         *
         * \param[in] other The source scalar vector.
         *
         * Use source scalar vector content as content of current vector
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const DenseVector<Mem2_, DT2_, IT2_> & other)
        {
          CONTEXT("When converting DenseVectorBlocked");

          ASSERT(other.size() % BlockSize_ == 0, "Error: DenseVector cannot be partionated with given blocksize!");

          this->clear();

          this->_scalar_index.push_back(other.size() / BlockSize_);

          this->_elements.push_back(other.get_elements().at(0));
          this->_elements_size.push_back(raw_size());

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            MemoryPool<Mem_>::instance()->increase_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            MemoryPool<Mem_>::instance()->increase_memory(this->_indices.at(i));
        }

        /**
         * \brief Get a pointer to the data array.
         *
         * \returns Pointer to the data array.
         */
        Tiny::Vector<DT_, BlockSize_> * elements()
        {
          return (Tiny::Vector<DT_, BlockSize_>*)this->_elements.at(0);
        }

        Tiny::Vector<DT_, BlockSize_> const * elements() const
        {
          return (Tiny::Vector<DT_, BlockSize_>*)this->_elements.at(0);
        }

        /**
         * \brief Get a pointer to the raw data array.
         *
         * \returns Pointer to the raw data array.
         */
        DT_ * raw_elements()
        {
          return this->_elements.at(0);
        }

        DT_ const * raw_elements() const
        {
          return this->_elements.at(0);
        }

        /// The raw number of elements of type DT_
        Index raw_size() const
        {
          return this->size() * BlockSize_;
        }

        /**
         * \brief Retrieve specific vector element.
         *
         * \param[in] index The index of the vector element.
         *
         * \returns Specific Tiny::Vector element.
         */
        const Tiny::Vector<DT_, BlockSize_> operator()(Index index) const
        {
          CONTEXT("When retrieving DenseVectorBlocked element");

          ASSERT(index < this->size(), "Error: " + stringify(index) + " exceeds dense vector blocked size " + stringify(this->size()) + " !");
          Tiny::Vector<DT_, BlockSize_> t;
          MemoryPool<Mem_>::download(t.v, this->_elements.at(0) + index * BlockSize_, BlockSize_);
          return t;
        }

        /**
         * \brief Set specific vector element.
         *
         * \param[in] index The index of the vector element.
         * \param[in] value The value to be set.
         */
        void operator()(Index index, const Tiny::Vector<DT_, BlockSize_> & value)
        {
          CONTEXT("When setting DenseVectorBlocked element");

          ASSERT(index < this->size(), "Error: " + stringify(index) + " exceeds dense vector blocked size " + stringify(this->_size()) + " !");
          MemoryPool<Mem_>::upload(this->_elements.at(0) + index * BlockSize_, value.v, BlockSize_);
        }

        /**
         * \brief Returns a descriptive string.
         *
         * \returns A string describing the container.
         */
        static String name()
        {
          return "DenseVectorBlocked";
        }

        /**
         * \brief Performs \f$this \leftarrow x\f$.
         *
         * \param[in] x The vector to be copied.
         */
        void copy(const DenseVectorBlocked & x)
        {
          this->_copy_content(x);
        }

        /**
         * \brief Performs \f$this \leftarrow x\f$.
         *
         * \param[in] x The vector to be copied.
         */
        template <typename Mem2_>
        void copy(const DenseVectorBlocked<Mem2_, DT_, IT_, BlockSize_> & x)
        {
          this->_copy_content(x);
        }

        /**
         * \brief Calculate \f$this \leftarrow \alpha x + y\f$
         *
         * \param[in] x The first summand vector to be scaled.
         * \param[in] y The second summand vector
         * \param[in] alpha A scalar to multiply x with.
         */
        template <typename Algo_>
        void axpy(
          const DenseVectorBlocked & x,
          const DenseVectorBlocked & y,
          const DT_ alpha = DT_(1))
        {
          if (x.size() != y.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
          if (x.size() != this->size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          // check for special cases
          // r <- x + y
          if(Math::abs(alpha - DT_(1)) < Math::eps<DT_>())
            Arch::Sum<Mem_, Algo_>::value(raw_elements(), x.raw_elements(), y.raw_elements(), this->raw_size());
          // r <- y - x
          else if(Math::abs(alpha + DT_(1)) < Math::eps<DT_>())
            Arch::Difference<Mem_, Algo_>::value(raw_elements(), y.raw_elements(), x.raw_elements(), this->raw_size());
          // r <- y
          else if(Math::abs(alpha) < Math::eps<DT_>())
            this->copy(y);
          // r <- y + alpha*x
          else
            Arch::Axpy<Mem_, Algo_>::dv(raw_elements(), alpha, x.raw_elements(), y.raw_elements(), this->raw_size());
        }

        /**
         * \brief Calculate \f$this_i \leftarrow x_i \cdot y_i\f$
         *
         * \param[in] x The first factor.
         * \param[in] y The second factor.
         */
        template <typename Algo_>
        void component_product(const DenseVectorBlocked & x, const DenseVectorBlocked & y)
        {
          if (this->size() != x.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
          if (this->size() != y.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          Arch::ComponentProduct<Mem_, Algo_>::value(raw_elements(), x.raw_elements(), y.raw_elements(), this->raw_size());
        }

        /**
         * \brief Calculate \f$this_i \leftarrow x_i \cdot y_i + z_i\f$
         *
         * \param[in] x The first factor.
         * \param[in] y The second factor.
         * \param[in] z The second summand.
         */
        template <typename Algo_>
        void component_product(
          const DenseVectorBlocked & x,
          const DenseVectorBlocked & y,
          const DenseVectorBlocked & z)
        {
          if (this->size() != x.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
          if (this->size() != y.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
          if (this->size() != z.size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          Arch::Axpy<Mem_, Algo_>::dv(raw_elements(), x.raw_elements(), y.raw_elements(), z.raw_elements(), this->raw_size());
        }

        /**
         * \brief Calculate \f$this \leftarrow \alpha x \f$
         *
         * \param[in] x The vector to be scaled.
         * \param[in] alpha A scalar to scale x with.
         */
        template <typename Algo_>
        void scale(const DenseVectorBlocked & x, const DT_ alpha)
        {
          if (x.size() != this->size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          Arch::Scale<Mem_, Algo_>::value(raw_elements(), x.raw_elements(), alpha, this->raw_size());
        }

        /**
         * \brief Calculate \f$r \leftarrow this \cdot x\f$
         *
         * \param[out] r The dot product result.
         * \param[in] x The other vector.
         */
        template <typename Algo_>
        DataType dot(const DenseVectorBlocked & x) const
        {
          if (x.size() != this->size())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          return Arch::DotProduct<Mem_, Algo_>::value(raw_elements(), x.raw_elements(), this->raw_size());
        }

        /**
         * \brief Calculates and returns the euclid norm of this vector.
         */
        template <typename Algo_>
        DT_ norm2() const
        {
          return Arch::Norm2<Mem_, Algo_>::value(raw_elements(), this->raw_size());
        }

        /**
         * \brief Calculates and returns the squared euclid norm of this vector.
         */
        template <typename Algo_>
        DT_ norm2sqr() const
        {
          // fallback
          return Math::sqr(this->norm2<Algo_>());
        }
    }; // class DenseVectorBlocked<...>


    /**
     * \brief DenseVectorBlocked comparison operator
     *
     * \param[in] a A vector to compare with.
     * \param[in] b A vector to compare with.
     */
    template <typename Mem_, typename Mem2_, typename DT_, typename IT_, Index BlockSize_> bool operator== (const DenseVectorBlocked<Mem_, DT_, IT_, BlockSize_> & a, const DenseVectorBlocked<Mem2_, DT_, IT_, BlockSize_> & b)
    {
      CONTEXT("When comparing DenseVectorBlockeds");

      if (a.size() != b.size())
        return false;
      if (a.get_elements().size() != b.get_elements().size())
        return false;
      if (a.get_indices().size() != b.get_indices().size())
        return false;

      if(a.size() == 0 && b.size() == 0 && a.get_elements().size() == 0 && b.get_elements().size() == 0)
        return true;

      bool ret(true);

      DT_ * ta;
      DT_ * tb;

      if(std::is_same<Mem::Main, Mem_>::value)
        ta = (DT_*)a.elements();
      else
      {
        ta = new DT_[a.raw_size()];
        MemoryPool<Mem_>::instance()->template download<DT_>(ta, a.raw_elements(), a.raw_size());
      }
      if(std::is_same<Mem::Main, Mem2_>::value)
        tb = (DT_*)b.elements();
      else
      {
        tb = new DT_[b.raw_size()];
        MemoryPool<Mem2_>::instance()->template download<DT_>(tb, b.raw_elements(), b.raw_size());
      }

      for (Index i(0) ; i < a.raw_size() ; ++i)
        if (ta[i] != tb[i])
        {
          ret = false;
          break;
        }

      if(! std::is_same<Mem::Main, Mem_>::value)
        delete[] ta;
      if(! std::is_same<Mem::Main, Mem2_>::value)
        delete[] tb;

      return ret;
    }

    /**
     * \brief DenseVectorBlocked streaming operator
     *
     * \param[in] lhs The target stream.
     * \param[in] b The vector to be streamed.
     */
    template <typename Mem_, typename DT_, typename IT_, Index BlockSize_>
    std::ostream &
    operator<< (std::ostream & lhs, const DenseVectorBlocked<Mem_, DT_, IT_, BlockSize_> & b)
    {
      lhs << "[";
      for (Index i(0) ; i < b.size() ; ++i)
      {
        Tiny::Vector<DT_, BlockSize_> t = b(i);
        for (Index j(0) ; j < BlockSize_ ; ++j)
          lhs << "  " << t[j];
      }
      lhs << "]";

      return lhs;
    }

  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_DENSE_VECTOR_BLOCKED_HPP