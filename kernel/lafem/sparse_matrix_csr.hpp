#pragma once
#ifndef KERNEL_LAFEM_SPARSE_MATRIX_CSR_HPP
#define KERNEL_LAFEM_SPARSE_MATRIX_CSR_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/lafem/container.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/algorithm.hpp>



namespace FEAST
{
  namespace LAFEM
  {
    template <typename Arch_, typename DT_>
    class SparseMatrixCSR : public Container<Arch_, DT_>
    {
      private:
        Index * _Aj;
        DT_ * _Ax;
        Index * _Ar;
        Index _rows;
        Index _columns;
        DT_ _zero_element;
        Index _used_elements;

      public:
        typedef DT_ data_type;
        typedef Arch_ arch_type;

        explicit SparseMatrixCSR() :
          Container<Arch_, DT_> (0),
          _rows(0),
          _columns(0),
          _zero_element(DT_(0)),
          _used_elements(0)
        {
        }

        explicit SparseMatrixCSR(const SparseMatrixCOO<Arch_, DT_> & other) :
          Container<Arch_, DT_>(other.size()),
          _rows(other.rows()),
          _columns(other.columns()),
          _zero_element(DT_(0)),
          _used_elements(other.used_elements())
        {
          this->_elements.push_back((DT_*)MemoryPool<Arch_>::instance()->allocate_memory(_used_elements * sizeof(DT_)));
          this->_elements_size.push_back(_used_elements);
          this->_indices.push_back((Index*)MemoryPool<Arch_>::instance()->allocate_memory(_used_elements * sizeof(Index)));
          this->_indices_size.push_back(_used_elements);
          this->_indices.push_back((Index*)MemoryPool<Arch_>::instance()->allocate_memory((2 * _rows) * sizeof(Index)));
          this->_indices_size.push_back(2 * _rows);

          _Aj = this->_indices.at(0);
          _Ax = this->_elements.at(0);
          _Ar = this->_indices.at(1);

          Index ait(0);
          Index current_row(0);
          _Ar[current_row * 2] = 0;
          for (typename std::map<unsigned long, DT_>::const_iterator it(other.elements().begin()) ; it != other.elements().end() ; ++it)
          {
            Index row(it->first / _columns);
            Index column(it->first % _columns);

            if (current_row < row)
            {
              _Ar[current_row * 2 + 1] = ait;
              for (unsigned long i(current_row + 1) ; i < row ; ++i)
              {
                _Ar[i * 2] = ait;
                _Ar[(i * 2) + 1] = ait;
              }
              current_row = row;
              _Ar[current_row * 2] = ait;
            }
            _Ax[ait] = it->second;
            _Aj[ait] = column;
            ++ait;
          }
          _Ar[current_row * 2 + 1] = ait;
          for (unsigned long i(current_row + 1) ; i < _rows ; ++i)
          {
            _Ar[i * 2] = ait;
            _Ar[i * 2 + 1] = ait;
          }
        }

        explicit SparseMatrixCSR(Index rows, Index columns, const DenseVector<Arch_, Index> & Aj, const DenseVector<Arch_, DT_> & Ax, const DenseVector<Arch_, Index> & Ar) :
          Container<Arch_, DT_>(rows * columns),
          _rows(rows),
          _columns(columns),
          _zero_element(DT_(0)),
          // \TODO use real element count - be aware of non used elements
          _used_elements(Ax.size())
        {
          // \TODO create real copies
          DenseVector<Arch_, Index> tAj(Aj.size());
          copy(tAj, Aj);
          DenseVector<Arch_, DT_> tAx(Ax.size());
          copy(tAx, Ax);
          DenseVector<Arch_, Index> tAr(Ar.size());
          copy(tAr, Ar);

          this->_elements.push_back(tAx.get_elements().at(0));
          this->_elements_size.push_back(tAx.size());
          this->_indices.push_back(tAj.get_elements().at(0));
          this->_indices_size.push_back(tAj.size());
          this->_indices.push_back(tAr.get_elements().at(0));
          this->_indices_size.push_back(tAr.size());

          this->_Ax = this->_elements.at(0);
          this->_Aj = this->_indices.at(0);
          this->_Ar = this->_indices.at(1);

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            MemoryPool<Arch_>::instance()->increase_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            MemoryPool<Arch_>::instance()->increase_memory(this->_indices.at(i));
        }

        SparseMatrixCSR(const SparseMatrixCSR<Arch_, DT_> & other) :
          Container<Arch_, DT_>(other),
          _rows(other._rows),
          _columns(other._columns),
          _zero_element(other._zero_element),
          _used_elements(other._used_elements)
        {
          this->_Ax = this->_elements.at(0);
          this->_Aj = this->_indices.at(0);
          this->_Ar = this->_indices.at(1);
        }

        template <typename Arch2_, typename DT2_>
        SparseMatrixCSR(const SparseMatrixCSR<Arch2_, DT2_> & other) :
          Container<Arch_, DT_>(other),
          _rows(other._rows),
          _columns(other._columns),
          _used_elements(other._used_elements)
        {
          this->_Ax = this->_elements.at(0);
          this->_Aj = this->_indices.at(0);
          this->_Ar = this->_indices.at(1);
        }

        SparseMatrixCSR<Arch_, DT_> & operator= (const SparseMatrixCSR<Arch_, DT_> & other)
        {
          if (this == &other)
            return *this;

          this->_size = other.size();
          this->_rows = other.rows();
          this->_columns = other.columns();
          this->_used_elements = other.used_elements();
          this->_zero_element = other._zero_element;

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            MemoryPool<Arch_>::instance()->release_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            MemoryPool<Arch_>::instance()->release_memory(this->_indices.at(i));

          this->_elements.clear();
          this->_indices.clear();
          this->_elements_size.clear();
          this->_indices_size.clear();

          std::vector<DT_ *> new_elements = other.get_elements();
          std::vector<Index *> new_indices = other.get_indices();

          this->_elements.assign(new_elements.begin(), new_elements.end());
          this->_indices.assign(new_indices.begin(), new_indices.end());
          this->_elements_size.assign(other.get_elements_size().begin(), other.get_elements_size().end());
          this->_indices_size.assign(other.get_indices_size().begin(), other.get_indices_size().end());

          _Aj = this->_indices.at(0);
          _Ax = this->_elements.at(0);
          _Ar = this->_indices.at(1);

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            MemoryPool<Arch_>::instance()->increase_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            MemoryPool<Arch_>::instance()->increase_memory(this->_indices.at(i));

          return *this;
        }

        template <typename Arch2_, typename DT2_>
        SparseMatrixCSR<Arch_, DT_> & operator= (const SparseMatrixCSR<Arch2_, DT2_> & other)
        {
          if (this == &other)
            return *this;

          //TODO copy memory from arch2 to arch
          return *this;
        }

        const DT_ & operator()(Index row, Index col) const
        {
          ASSERT(row < this->_rows, "Error: " + stringify(row) + " exceeds sparse matrix csr row size " + stringify(this->_rows) + " !");
          ASSERT(col < this->_columns, "Error: " + stringify(col) + " exceeds sparse matrix csr column size " + stringify(this->_columns) + " !");

          for (unsigned long i(_Ar[row * 2]) ; i < _Ar[(row * 2) + 1] ; ++i)
          {
            if (_Aj[i] == col)
              return _Ax[i];
            if (_Aj[i] > col)
              return _zero_element;
          }
          return _zero_element;
        }

        const Index & rows() const
        {
          return this->_rows;
        }

        const Index & columns() const
        {
          return this->_columns;
        }

        const Index & used_elements() const
        {
          return this->_used_elements;
        }

        Index * Aj() const
        {
          return _Aj;
        }

        DT_ * Ax() const
        {
          return _Ax;
        }

        Index * Ar() const
        {
          return _Ar;
        }

        const DT_ zero_element() const
        {
          return _zero_element;
        }
    };

    template <typename Arch_, typename DT_> bool operator== (const SparseMatrixCSR<Arch_, DT_> & a, const SparseMatrixCSR<Arch_, DT_> & b)
    {
      if (a.rows() != b.rows())
        return false;
      if (a.columns() != b.columns())
        return false;
      if (a.used_elements() != b.used_elements())
        return false;
      if (a.zero_element() != b.zero_element())
        return false;

      for (Index i(0) ; i < a.rows() ; ++i)
      {
        for (Index j(0) ; j < a.columns() ; ++j)
        {
          if (a(i, j) != b(i, j))
            return false;
        }
      }

      return true;
    }

    template <typename Arch_, typename DT_>
    std::ostream &
    operator<< (std::ostream & lhs, const SparseMatrixCSR<Arch_, DT_> & b)
    {
      lhs << "[" << std::endl;
      for (Index i(0) ; i < b.rows() ; ++i)
      {
        lhs << "[";
        for (Index j(0) ; j < b.columns() ; ++j)
        {
          lhs << "  " << b(i, j);
        }
        lhs << "]" << std::endl;
      }
      lhs << "]" << std::endl;

      return lhs;
    }

  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_SPARSE_MATRIX_CSR_HPP
