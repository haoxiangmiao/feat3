#pragma once
#ifndef KERNEL_LAFEM_FILTER_CHAIN_HPP
#define KERNEL_LAFEM_FILTER_CHAIN_HPP 1

// includes, FEAST
#include <kernel/lafem/meta_element.hpp>

namespace FEAST
{
  namespace LAFEM
  {
    /**
     * \brief Filter Chainclass template
     *
     * This class template implements a chain of filters which are successively applied
     * onto a vector.
     *
     * \tparam First_, Rest_...
     * The filters to be chained.
     *
     * \author Peter Zajac
     */
    template<
      typename First_,
      typename... Rest_>
    class FilterChain
    {
      template<typename,typename...>
      friend class FilterChain;

      typedef FilterChain<Rest_...> RestClass;

    public:
      /// number of vector blocks
      static constexpr int num_blocks = FilterChain<Rest_...>::num_blocks + 1;

      /// sub-filter mem-type
      typedef typename First_::MemType MemType;
      /// sub-filter data-type
      typedef typename First_::DataType DataType;
      /// sub-filter index-type
      typedef typename First_::IndexType IndexType;

      // ensure that all sub-vector have the same mem- and data-type
      static_assert(std::is_same<MemType, typename RestClass::MemType>::value,
                    "sub-filters have different mem-types");
      static_assert(std::is_same<DataType, typename RestClass::DataType>::value,
                    "sub-filters have different data-types");
      static_assert(std::is_same<IndexType, typename RestClass::IndexType>::value,
                    "sub-filters have different index-types");

    protected:
      /// the first sub-filter
      First_ _first;
      /// all remaining sub-filters
      RestClass _rest;

      /// data-emplacement ctor; this one is protected for a reason
      explicit FilterChain(First_&& the_first, RestClass&& the_rest) :
        _first(std::move(the_first)),
        _rest(std::move(the_rest))
      {
      }

    public:
      /// default ctor
      FilterChain()
      {
      }

      /// sub-filter emplacement ctor
      explicit FilterChain(First_&& the_first, Rest_&&... the_rest) :
        _first(std::move(the_first)),
        _rest(std::move(the_rest...))
      {
      }

      /// move-ctor
      FilterChain(FilterChain&& other) :
        _first(std::move(other._first)),
        _rest(std::move(other._rest))
      {
      }

      /// move-assign operator
      FilterChain& operator=(FilterChain&& other)
      {
        if(this != &other)
        {
          _first = std::move(other._first);
          _rest = std::move(other._rest);
        }
        return *this;
      }

      FilterChain clone() const
      {
        return FilterChain(_first.clone(), _rest.clone());
      }

      template<typename... SubFilter2_>
      void convert(const FilterChain<SubFilter2_...>& other)
      {
        _first.convert(other._first);
        _rest.convert(other._rest);
      }

      /// \cond internal
      First_& first()
      {
        return _first;
      }

      const First_& first() const
      {
        return _first;
      }

      RestClass& rest()
      {
        return _rest;
      }

      const RestClass& rest() const
      {
        return _rest;
      }
      /// \endcond

      template<int i_>
      typename TupleElement<i_, First_, Rest_...>::Type& at()
      {
        static_assert((0 <= i_) && (i_ < num_blocks), "invalid sub-filter index");
        return TupleElement<i_, First_, Rest_...>::get(*this);
      }

      /** \copydoc at() */
      template<int i_>
      typename TupleElement<i_, First_, Rest_...>::Type const& at() const
      {
        static_assert((0 <= i_) && (i_ < num_blocks), "invalid sub-filter index");
        return TupleElement<i_, First_, Rest_...>::get(*this);
      }

      /** \copydoc UnitFilter::filter_rhs() */
      template<typename Vector_>
      void filter_rhs(Vector_& vector) const
      {
        first().filter_rhs(vector);
        rest().filter_rhs(vector);
      }

      /** \copydoc UnitFilter::filter_sol() */
      template<typename Vector_>
      void filter_sol(Vector_& vector) const
      {
        first().filter_sol(vector);
        rest().filter_sol(vector);
      }

      /** \copydoc UnitFilter::filter_def() */
      template<typename Vector_>
      void filter_def(Vector_& vector) const
      {
        first().filter_def(vector);
        rest().filter_def(vector);
      }

      /** \copydoc UnitFilter::filter_cor() */
      template<typename Vector_>
      void filter_cor(Vector_& vector) const
      {
        first().filter_cor(vector);
        rest().filter_cor(vector);
      }
    }; // class FilterChain

    /// \cond internal
    template<typename First_>
    class FilterChain<First_>
    {
      template<typename,typename...>
      friend class FilterChain;

    public:
      static constexpr int num_blocks = 1;

      /// sub-filter mem-type
      typedef typename First_::MemType MemType;
      /// sub-filter data-type
      typedef typename First_::DataType DataType;
      /// sub-filter index-type
      typedef typename First_::IndexType IndexType;

    protected:
      /// the first sub-filter
      First_ _first;

    public:
      /// default ctor
      FilterChain()
      {
      }

      /// sub-filter emplacement ctor
      explicit FilterChain(First_&& the_first) :
        _first(std::move(the_first))
      {
      }

      /// move-ctor
      FilterChain(FilterChain&& other) :
        _first(std::move(other._first))
      {
      }

      /// move-assign operator
      FilterChain& operator=(FilterChain&& other)
      {
        if(this != &other)
        {
          _first = std::move(other._first);
        }
        return *this;
      }

      FilterChain clone() const
      {
        return FilterChain(_first.clone());
      }

      /// \compilerhack MSVC 2013 template bug workaround
#ifdef FEAST_COMPILER_MICROSOFT
      template<typename... SubFilter2_>
      void convert(const FilterChain<SubFilter2_...>& other)
      {
        static_assert(sizeof...(SubFilter2_) == std::size_t(1), "invalid FilterChain size");
        _first.convert(other._first);
      }
#else
      template<typename SubFilter2_>
      void convert(const FilterChain<SubFilter2_>& other)
      {
        _first.convert(other._first);
      }
#endif

      First_& first()
      {
        return _first;
      }

      const First_& first() const
      {
        return _first;
      }

      template<int i_>
      typename TupleElement<i_, First_>::Type& at()
      {
        static_assert(i_ == 0, "invalid sub-filter index");
        return first();
      }

      template<int i_>
      typename TupleElement<i_, First_>::Type const& at() const
      {
        static_assert(i_ == 0, "invalid sub-filter index");
        return first();
      }

      /** \copydoc UnitFilter::filter_rhs() */
      template<typename Vector_>
      void filter_rhs(Vector_& vector) const
      {
        first().filter_rhs(vector);
      }

      /** \copydoc UnitFilter::filter_sol() */
      template<typename Vector_>
      void filter_sol(Vector_& vector) const
      {
        first().filter_sol(vector);
      }

      /** \copydoc UnitFilter::filter_def() */
      template<typename Vector_>
      void filter_def(Vector_& vector) const
      {
        first().filter_def(vector);
      }

      /** \copydoc UnitFilter::filter_cor() */
      template<typename Vector_>
      void filter_cor(Vector_& vector) const
      {
        first().filter_cor(vector);
      }
    }; // class FilterChain
  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_FILTER_CHAIN_HPP