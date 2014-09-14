#pragma once
#ifndef KERNEL_LAFEM_SPARSE_VECTOR_BLOCKED_HPP
#define KERNEL_LAFEM_SPARSE_VECTOR_BLOCKED_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/util/type_traits.hpp>
#include <kernel/lafem/container.hpp>
#include <kernel/lafem/vector_base.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/util/tiny_algebra.hpp>

namespace FEAST
{
  namespace LAFEM
  {
    /**
     * \brief Sparse vector class template.
     *
     * \tparam Mem_ The \ref FEAST::Mem "memory architecture" to be used.
     * \tparam DT_ The datatype to be used.
     * \tparam IT_ The indexing type to be used.
     * \tparam BlockSize_ The size of the represented blocks
     *
     * This class represents a vector with non zero element blocks in a sparse layout. \n
     * Logical, the data are organized in small blocks of BlockSize_ length.\n\n
     * Data survey: \n
     * _elements[0]: raw number values \n
     * _indices[0]: non zero indices \n
     * _scalar_index[0]: container size - aka block count \n
     * _scalar_index[1]: non zero element count (used elements) \n
     * _scalar_index[2]: allocated elements \n
     * _scalar_index[3]: allocation size increment \n
     * _scalar_index[4]: boolean flag, if container is sorted \n
     * _scalar_dt[0]: zero element
     *
     * Refer to \ref lafem_design for general usage informations.
     *
     * \author Dirk Ribbrock
     */
    template <typename Mem_, typename DT_, typename IT_, Index BlockSize_>
    class SparseVectorBlocked : public Container<Mem_, DT_, IT_>, public VectorBase
    {
      private:
        template <typename T1_, typename T2_>
        static void _insertion_sort(T1_ * key, T2_ * val1, Index size)
        {
          T1_ swap_key;
          Tiny::Vector<T2_, BlockSize_> swap1;
          for (Index i(1), j ; i < size ; ++i)
          {
            swap_key = Util::MemoryPool<Mem_>::get_element(key, i);
            Util::MemoryPool<Mem_>::download(swap1.v, val1 + i * BlockSize_, BlockSize_);
            j = i;
            while (j > 0 && Util::MemoryPool<Mem_>::get_element(key, j - 1) > swap_key)
            {
              Util::MemoryPool<Mem_>::copy(key + j, key + j - 1, 1);
              Util::MemoryPool<Mem_>::copy(val1 + j * BlockSize_, val1 + (j - 1) * BlockSize_, BlockSize_);
              --j;
            }
            Util::MemoryPool<Mem_>::set_memory(key + j, swap_key);
            Util::MemoryPool<Mem_>::upload(val1 + j * BlockSize_, swap1.v, BlockSize);
          }
        }

        Index & _used_elements()
        {
          return this->_scalar_index.at(1);
        }

        Index & _allocated_elements()
        {
          return this->_scalar_index.at(2);
        }

        Index & _sorted()
        {
          return this->_scalar_index.at(4);
        }

      public:
        /// Our datatype
        typedef DT_ DataType;
        /// Our indextype
        typedef IT_ IndexType;
        /// Our memory architecture type
        typedef Mem_ MemType;
        /// Our size of a single block
        static constexpr Index BlockSize = BlockSize_;

        /**
         * \brief Constructor
         *
         * Creates an empty non dimensional vector.
         */
        explicit SparseVectorBlocked() :
          Container<Mem_, DT_, IT_> (0)
        {
          CONTEXT("When creating SparseVectorBlocked");
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(1000);
          this->_scalar_index.push_back(1);
          this->_scalar_dt.push_back(DT_(0));
        }

        /**
         * \brief Constructor
         *
         * \param[in] size The size of the created vector.
         *
         * Creates a vector with a given size.
         */
        explicit SparseVectorBlocked(Index size_in) :
          Container<Mem_, DT_, IT_>(size_in)
        {
          CONTEXT("When creating SparseVectorBlocked");
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(1000);
          this->_scalar_index.push_back(1);
          this->_scalar_dt.push_back(DT_(0));
        }

        /**
         * \brief Move Constructor
         *
         * \param[in] other The source vector.
         *
         * Moves another vector to this vector.
         */
        SparseVectorBlocked(SparseVectorBlocked && other) :
          Container<Mem_, DT_, IT_>(std::forward<SparseVectorBlocked>(other))
        {
          CONTEXT("When moving SparseVectorBlocked");
        }

        /**
         * \brief Assignment move operator
         *
         * \param[in] other The source vector.
         *
         * Moves another vector to the target vector.
         */
        SparseVectorBlocked & operator= (SparseVectorBlocked && other)
        {
          CONTEXT("When moving SparseVectorBlocked");

          this->move(std::forward<SparseVectorBlocked>(other));

          return *this;
        }

        /** \brief Clone operation
         *
         * Create a deep copy of itself.
         * \param[in] clone_indices Should we create a deep copy of the index arrays, too ?
         *
         * \return A deep copy of itself.
         *
         */
        SparseVectorBlocked clone(bool clone_indices = true) const
        {
          CONTEXT("When cloning SparseVectorBlocked");
          SparseVectorBlocked t;
          t.clone(*this, clone_indices);
          return t;
        }

        /** \brief Clone operation
         *
         * Become a deep copy of a given vector.
         *
         * \param[in] other The source container.
         * \param[in] clone_indices Should we create a deep copy of the index arrays, too ?
         *
         */
        void clone(const SparseVectorBlocked & other, bool clone_indices = true)
        {
          CONTEXT("When cloning SparseVectorBlocked");
          Container<Mem_, DT_, IT_>::clone(other, clone_indices);
        }

        /** \brief Clone operation
         *
         * Become a deep copy of a given vector.
         *
         * \param[in] other The source container.
         * \param[in] clone_indices Should we create a deep copy of the index arrays, too ?
         *
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void clone(const SparseVectorBlocked<Mem2_, DT2_, IT2_, BlockSize_> & other, bool clone_indices = true)
        {
          CONTEXT("When cloning SparseVectorBlocked");
          SparseVectorBlocked<Mem_, DT_, IT_, BlockSize_> t;
          t.assign(other);
          Container<Mem_, DT_, IT_>::clone(t, clone_indices);
        }


        /**
         * \brief Conversion method
         *
         * Use source vector content as content of current vector
         *
         * \param[in] other The source container.
         *
         * \note This creates a deep copy in any case!
         *
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const SparseVectorBlocked<Mem2_, DT2_, IT2_, BlockSize_> & other)
        {
          CONTEXT("When converting SparseVectorBlocked");
          this->clone(other);
        }

        /**
         * \brief Get a pointer to the data array.
         *
         * \returns Pointer to the data array.
         */
        Tiny::Vector<DT_, BlockSize_> * elements()
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_elements.at(0);
        }

        Tiny::Vector<DT_, BlockSize_> const * elements() const
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_elements.at(0);
        }

        /**
         * \brief Get a pointer to the raw data array.
         *
         * \returns Pointer to the raw data array.
         */
        DT_ * raw_elements()
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_elements.at(0);
        }

        DT_ const * raw_elements() const
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_elements.at(0);
        }

        /**
         * \brief Get a pointer to the non zero indices array.
         *
         * \returns Pointer to the indices array.
         */
        IT_ * indices()
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_indices.at(0);
        }

        IT_ const * indices() const
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_indices.at(0);
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
         * \returns Specific vector element.
         */
        const Tiny::Vector<DT_, BlockSize_> operator()(Index index) const
        {
          CONTEXT("When retrieving SparseVectorBlocked element");

          ASSERT(index < this->_scalar_index.at(0), "Error: " + stringify(index) + " exceeds sparse vector size " + stringify(this->_scalar_index.at(0)) + " !");

          if (this->_elements.size() == 0)
            return zero_element();

          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();

          Index i(0);
          while (i < used_elements())
          {
            if (Util::MemoryPool<Mem_>::get_element(indices(), i) >= index)
              break;
            ++i;
          }

          if (i < used_elements() && Util::MemoryPool<Mem_>::get_element(indices(), i) == index)
          {
            Tiny::Vector<DT_, BlockSize_> t;
            Util::MemoryPool<Mem_>::download(t.v, this->_elements.at(0) + i * BlockSize_, BlockSize_);
            return t;
          }
          else
            return zero_element();
        }


        /**
         * \brief Set specific vector element.
         *
         * \param[in] index The index of the vector element.
         * \param[in] val The val to be set.
         */
        void operator()(Index index, Tiny::Vector<DT_, BlockSize_> & val)
        {
          CONTEXT("When setting SparseVectorBlocked element");

          ASSERT(index < this->_scalar_index.at(0), "Error: " + stringify(index) + " exceeds sparse vector size " + stringify(this->_scalar_index.at(0)) + " !");

          // flag container as not sorted anymore
          // CAUTION: do not use any method triggering resorting until we are finished
          _sorted() = 0;

          // vector is empty, no arrays allocated
          if (this->_elements.size() == 0)
          {
            this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(alloc_increment() * BlockSize_));
            this->_elements_size.push_back(alloc_increment() * BlockSize_);
            Util::MemoryPool<Mem_>::instance()->template set_memory<DT_>(this->_elements.back(), DT_(4711), alloc_increment() * BlockSize_);
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(alloc_increment()));
            this->_indices_size.push_back(alloc_increment());
            Util::MemoryPool<Mem_>::instance()->template set_memory<IT_>(this->_indices.back(), IT_(4711), alloc_increment());
            _allocated_elements() = alloc_increment();
            Util::MemoryPool<Mem_>::upload(this->_elements.at(0), val.v, BlockSize_);
            Util::MemoryPool<Mem_>::set_memory(this->_indices.at(0), IT_(index));
            _used_elements() = 1;
          }

          // append element in already allocated arrays
          else if(_used_elements() < allocated_elements())
          {
            Util::MemoryPool<Mem_>::upload(this->_elements.at(0) + _used_elements() * BlockSize_, val.v, BlockSize_);
            Util::MemoryPool<Mem_>::set_memory(this->_indices.at(0) + _used_elements(), IT_(index));
            ++_used_elements();
          }

          // reallocate arrays, append element
          else
          {
            _allocated_elements() += alloc_increment();

            DT_ * elements_new(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(allocated_elements() * BlockSize_));
            Util::MemoryPool<Mem_>::instance()->template set_memory<DT_>(elements_new, DT_(4711), allocated_elements() * BlockSize_);
            IT_ * indices_new(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(allocated_elements()));
            Util::MemoryPool<Mem_>::instance()->template set_memory<IT_>(indices_new, IT_(4711), allocated_elements());

            Util::MemoryPool<Mem_>::copy(elements_new, this->_elements.at(0), _used_elements() * BlockSize_);
            Util::MemoryPool<Mem_>::copy(indices_new, this->_indices.at(0), _used_elements());

            Util::MemoryPool<Mem_>::instance()->release_memory(this->_elements.at(0));
            Util::MemoryPool<Mem_>::instance()->release_memory(this->_indices.at(0));

            this->_elements.at(0) = elements_new;
            this->_indices.at(0) = indices_new;

            Util::MemoryPool<Mem_>::upload(this->_elements.at(0) + used_elements() * BlockSize_, val.v, BlockSize_);
            Util::MemoryPool<Mem_>::set_memory(this->_indices.at(0) + _used_elements(), IT_(index));

            ++_used_elements();
            this->_elements_size.at(0) = allocated_elements() * BlockSize_;
            this->_indices_size.at(0) = allocated_elements();
          }
        }

        void sort()
        {
          if (sorted() == 0)
          {
            //first of all, mark vector as sorted, because otherwise we would call ourselves inifite times
            // CAUTION: do not use any method triggering resorting until we are finished
            _sorted() = 1;

            // check if there is anything to be sorted
            if(_used_elements() == Index(0))
              return;

            _insertion_sort(this->_indices.at(0), this->_elements.at(0), _used_elements());

            // find and mark duplicate entries
            for (Index i(1) ; i < _used_elements() ; ++i)
            {
              if (Util::MemoryPool<Mem_>::get_element(this->_indices.at(0), i - 1) == Util::MemoryPool<Mem_>::get_element(this->_indices.at(0), i))
              {
                Util::MemoryPool<Mem_>::set_memory(this->_indices.at(0) + i - 1, std::numeric_limits<IT_>::max());
              }
            }

            // sort out marked duplicated elements
            _insertion_sort(this->_indices.at(0), this->_elements.at(0), _used_elements());
            Index junk(0);
            while (Util::MemoryPool<Mem_>::get_element(this->_indices.at(0), _used_elements() - 1 - junk) == std::numeric_limits<IT_>::max()
                && junk < _used_elements())
              ++junk;
            _used_elements() -= junk;
          }
        }

        /**
         * \brief Retrieve non zero element count.
         *
         * \returns Non zero element count.
         */
        const Index & used_elements() const override
        {
          if (sorted() == 0)
            const_cast<SparseVectorBlocked *>(this)->sort();
          return this->_scalar_index.at(1);
        }

        /**
         * \brief Retrieve non zero element.
         *
         * \returns Non zero element.
         */
        const Tiny::Vector<DT_, BlockSize_> zero_element() const
        {
          Tiny::Vector<DT_, BlockSize_> t(this->_scalar_dt.at(0));
          return t;
        }

        /**
         * \brief Retrieve amount of allocated elements.
         *
         * \return Allocated element count.
         */
        const Index & allocated_elements() const
        {
          return this->_scalar_index.at(2);
        }

        /**
         * \brief Retrieve allocation incrementation value.
         *
         * \return Allocation increment.
         */
        const Index & alloc_increment() const
        {
          return this->_scalar_index.at(3);
        }

        /**
         * \brief Retrieve status of element sorting.
         *
         * \return Sorting status.
         */
        const Index & sorted() const
        {
          return this->_scalar_index.at(4);
        }

        /**
         * \brief Returns a descriptive string.
         *
         * \returns A string describing the container.
         */
        static String name()
        {
          return "SparseVectorBlocked";
        }


        /**
         * \brief SparseVectorBlocked comparison operator
         *
         * \param[in] a A vector to compare with.
         * \param[in] b A vector to compare with.
         */
        template <typename Mem2_> friend bool operator== (const SparseVectorBlocked & a, const SparseVectorBlocked<Mem2_, DT_, IT_, BlockSize_> & b)
        {
          CONTEXT("When comparing SparseVectorBlockeds");

          if (a.size() != b.size())
            return false;
          if (a.get_elements().size() != b.get_elements().size())
            return false;
          if (a.get_indices().size() != b.get_indices().size())
            return false;

          if(a.size() == 0 && b.size() == 0 && a.get_elements().size() == 0 && b.get_elements().size() == 0)
            return true;

          for (Index i(0) ; i < a.size() ; ++i)
          {
            auto ta = a(i);
            auto tb = b(i);
            for (Index j(0) ; j < BlockSize_ ; ++j)
              if (ta[j] != tb[j])
                return false;
          }

          return true;
        }

        /**
         * \brief SparseVectorBlocked streaming operator
         *
         * \param[in] lhs The target stream.
         * \param[in] b The vector to be streamed.
         */
        friend std::ostream & operator<< (std::ostream & lhs, const SparseVectorBlocked & b)
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
    }; // class SparseVectorBlocked<...>


  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_SPARSE_VECTOR_BLOCKED_HPP