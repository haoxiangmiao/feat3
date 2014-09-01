#pragma once
#ifndef KERNEL_LAFEM_SPARSE_MATRIX_ELL_HPP
#define KERNEL_LAFEM_SPARSE_MATRIX_ELL_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/lafem/forward.hpp>
#include <kernel/lafem/container.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <kernel/lafem/sparse_matrix_ell.hpp>
#include <kernel/lafem/matrix_base.hpp>
#include <kernel/lafem/dense_vector.hpp>
#include <kernel/lafem/sparse_layout.hpp>
#include <kernel/lafem/arch/scale_row_col.hpp>
#include <kernel/lafem/arch/sum.hpp>
#include <kernel/lafem/arch/difference.hpp>
#include <kernel/lafem/arch/scale.hpp>
#include <kernel/lafem/arch/axpy.hpp>
#include <kernel/lafem/arch/product_matvec.hpp>
#include <kernel/lafem/arch/defect.hpp>
#include <kernel/lafem/arch/norm.hpp>
#include <kernel/adjacency/graph.hpp>

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <stdint.h>


namespace FEAST
{
  namespace LAFEM
  {
    /**
     * \brief ELL based sparse matrix.
     *
     * \tparam Mem_ The \ref FEAST::Mem "memory architecture" to be used.
     * \tparam DT_ The datatype to be used.
     * \tparam IT_ The indexing type to be used.
     *
     * This class represents a sparse matrix, that stores its non zero elements in the ELL-R format.\n\n
     * Data survey: \n
     * _elements[0]: Ax - raw non zero number values, stored in a (cols per row x stride) matrix \n
     * _indices[0]: Aj - column index per non zero element, stored in a (cols per row x stride) matrix \n
     * _indices[1]: Arl - length of every single row\n
     *
     * _scalar_index[0]: container size \n
     * _scalar_index[1]: row count \n
     * _scalar_index[2]: column count \n
     * _scalar_index[3]: stride, aka the row count, rounded up to a multiple of the warp size \n
     * _scalar_index[4]: column count per row \n
     * _scalar_index[5]: non zero element count (used elements) \n
     * _scalar_dt[0]: zero element
     *
     * Refer to \ref lafem_design for general usage informations.
     *
     * \author Dirk Ribbrock
     */
    template <typename Mem_, typename DT_, typename IT_ = Index>
    class SparseMatrixELL : public Container<Mem_, DT_, IT_>, public MatrixBase
    {
      private:
        void _read_from_mtx(String filename)
        {
          std::ifstream file(filename.c_str(), std::ifstream::in);
          if (! file.is_open())
            throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Matrix file " + filename);
          _read_from_mtx(file);
          file.close();
        }

        void _read_from_mtx(std::istream& file)
        {
          std::map<IT_, std::map<IT_, DT_> > entries; // map<row, map<column, value> >
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_dt.push_back(DT_(0));

          Index ue(0);
          String line;
          std::getline(file, line);
          const bool general((line.find("%%MatrixMarket matrix coordinate real general") != String::npos) ? true : false);
          const bool symmetric((line.find("%%MatrixMarket matrix coordinate real symmetric") != String::npos) ? true : false);

          if (symmetric == false && general == false)
          {
            throw InternalError(__func__, __FILE__, __LINE__, "Input-file is not a compatible mtx-file");
          }

          while(!file.eof())
          {
            std::getline(file,line);
            if (file.eof())
              throw InternalError(__func__, __FILE__, __LINE__, "Input-file is empty");

            String::size_type begin(line.find_first_not_of(" "));
            if (line.at(begin) != '%')
              break;
          }
          {
            String::size_type begin(line.find_first_not_of(" "));
            line.erase(0, begin);
            String::size_type end(line.find_first_of(" "));
            String srow(line, 0, end);
            Index row((Index)atol(srow.c_str()));
            line.erase(0, end);

            begin = line.find_first_not_of(" ");
            line.erase(0, begin);
            end = line.find_first_of(" ");
            String scol(line, 0, end);
            Index col((Index)atol(scol.c_str()));
            line.erase(0, end);
            _rows() = row;
            _columns() = col;
            _size() = this->rows() * this->columns();
          }
          while(!file.eof())
          {
            std::getline(file, line);
            if (file.eof())
              break;

            String::size_type begin(line.find_first_not_of(" "));
            line.erase(0, begin);
            String::size_type end(line.find_first_of(" "));
            String srow(line, 0, end);
            IT_ row((IT_)atol(srow.c_str()));
            --row;
            line.erase(0, end);

            begin = line.find_first_not_of(" ");
            line.erase(0, begin);
            end = line.find_first_of(" ");
            String scol(line, 0, end);
            IT_ col((IT_)atol(scol.c_str()));
            --col;
            line.erase(0, end);

            begin = line.find_first_not_of(" ");
            line.erase(0, begin);
            end = line.find_first_of(" ");
            String sval(line, 0, end);
            DT_ tval((DT_)atof(sval.c_str()));

            entries[(unsigned int)row].insert(std::pair<IT_, DT_>(col, tval));
            ++ue;
            if (symmetric == true && row != col)
            {
              entries[(unsigned int)col].insert(std::pair<IT_, DT_>(row, tval));
              ++ue;
            }
          }
          _size() = this->rows() * this->columns();
          _used_elements() = ue;
          _num_cols_per_row() = 0;

          IT_* tArl = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_rows());

          Index idx(0);
          for (auto row : entries)
          {
            tArl[idx] = IT_(row.second.size());
            _num_cols_per_row() = std::max(IT_(_num_cols_per_row()), tArl[idx]);
            ++idx;
          }

          Index alignment(32);
          _stride() = alignment * ((_rows() + alignment - 1)/ alignment);

          DT_* tAx = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAx, DT_(0), _num_cols_per_row() * _stride());
          IT_* tAj = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAj, IT_(0), _num_cols_per_row() * _stride());

          Index row_idx(0);
          for (auto row : entries)
          {
            Index target(0);
            for (auto col : row.second )
            {
              tAj[row_idx + target * stride()] = col.first;
              tAx[row_idx + target * stride()] = col.second;
              ++target;
            }
            row.second.clear();
            ++row_idx;
          }
          entries.clear();

          this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
          this->_elements_size.push_back(_num_cols_per_row() * _stride());
          this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride()));
          this->_indices_size.push_back(_num_cols_per_row() * _stride());
          this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_rows()));
          this->_indices_size.push_back(_rows());

          Util::MemoryPool<Mem_>::template upload<DT_>(this->get_elements().at(0), tAx, _num_cols_per_row() * _stride());
          Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(0), tAj, _num_cols_per_row() * _stride());
          Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(1), tArl, _rows());

          Util::MemoryPool<Mem::Main>::instance()->release_memory(tAx);
          Util::MemoryPool<Mem::Main>::instance()->release_memory(tAj);
          Util::MemoryPool<Mem::Main>::instance()->release_memory(tArl);
        }

        void _read_from_ell(String filename)
        {
          std::ifstream file(filename.c_str(), std::ifstream::in | std::ifstream::binary);
          if (! file.is_open())
            throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Matrix file " + filename);
          _read_from_ell(file);
          file.close();
        }

        void _read_from_ell(std::istream& file)
        {
          this->template _deserialise<double, uint64_t>(FileMode::fm_ell, file);
        }

        Index & _size()
        {
          return this->_scalar_index.at(0);
        }
        Index & _rows()
        {
          return this->_scalar_index.at(1);
        }
        Index & _columns()
        {
          return this->_scalar_index.at(2);
        }
        Index & _stride()
        {
          return this->_scalar_index.at(3);
        }
        Index & _num_cols_per_row()
        {
          return this->_scalar_index.at(4);
        }
        Index & _used_elements()
        {
          return this->_scalar_index.at(5);
        }

      public:
        /// Our datatype
        typedef DT_ DataType;
        /// Our indexype
        typedef IT_ IndexType;
        /// Our memory architecture type
        typedef Mem_ MemType;
        /// Compatible L-vector type
        typedef DenseVector<Mem_, DT_, IT_> VectorTypeL;
        /// Compatible R-vector type
        typedef DenseVector<Mem_, DT_, IT_> VectorTypeR;
        /// Our used layout type
        static constexpr SparseLayoutId layout_id = SparseLayoutId::lt_ell;
        /// Our 'base' class type
        template <typename Mem2_, typename DT2_, typename IT2_ = IT_>
        using ContainerType = class SparseMatrixELL<Mem2_, DT2_, IT2_>;

        /**
         * \brief Constructor
         *
         * Creates an empty non dimensional matrix.
         */
        explicit SparseMatrixELL() :
          Container<Mem_, DT_, IT_> (0)
        {
          CONTEXT("When creating SparseMatrixELL");
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_dt.push_back(DT_(0));
        }

        /**
         * \brief Constructor
         *
         * \param[in] layout The layout to be used.
         *
         * Creates an empty matrix with given layout.
         */
        explicit SparseMatrixELL(const SparseLayout<Mem_, IT_, layout_id> & layout_in) :
          Container<Mem_, DT_, IT_> (layout_in._scalar_index.at(0))
        {
          CONTEXT("When creating SparseMatrixELL");
          this->_indices.assign(layout_in._indices.begin(), layout_in._indices.end());
          this->_indices_size.assign(layout_in._indices_size.begin(), layout_in._indices_size.end());
          this->_scalar_index.assign(layout_in._scalar_index.begin(), layout_in._scalar_index.end());
          this->_scalar_dt.push_back(DT_(0));

          for (auto i : this->_indices)
            Util::MemoryPool<Mem_>::instance()->increase_memory(i);

          this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
          this->_elements_size.push_back(_num_cols_per_row() * _stride());
        }

        /**
         * \brief Constructor
         *
         * \param[in] other The source matrix.
         *
         * Creates a ELL matrix based on the source matrix.
         */
        template <typename MT_>
        explicit SparseMatrixELL(const MT_ & other) :
          Container<Mem_, DT_, IT_>(other.size())
        {
          CONTEXT("When creating SparseMatrixELL");

          convert(other);
        }

        /**
         * \brief Constructor
         *
         * \param[in] rows The row count of the created matrix.
         * \param[in] columns The column count of the created matrix.
         * \param[in] Ax Vector with non zero elements.
         * \param[in] Aj Vector with column indices.
         * \param[in] Arl Vector with row length.
         *
         * Creates a matrix with given dimensions and content.
         */
        explicit SparseMatrixELL(const Index rows_in, const Index columns_in,
            const Index stride_in, const Index num_cols_per_row_in, const Index used_elements_in,
            DenseVector<Mem_, DT_, IT_> & Ax_in,
            DenseVector<Mem_, IT_, IT_> & Aj_in,
            DenseVector<Mem_, IT_, IT_> & Arl_in) :
          Container<Mem_, DT_, IT_>(rows_in * columns_in)
        {
          CONTEXT("When creating SparseMatrixELL");
          this->_scalar_index.push_back(rows_in);
          this->_scalar_index.push_back(columns_in);
          this->_scalar_index.push_back(stride_in);
          this->_scalar_index.push_back(num_cols_per_row_in);
          this->_scalar_index.push_back(used_elements_in);
          this->_scalar_dt.push_back(DT_(0));

          this->_elements.push_back(Ax_in.elements());
          this->_elements_size.push_back(Ax_in.size());
          this->_indices.push_back(Aj_in.elements());
          this->_indices_size.push_back(Aj_in.size());
          this->_indices.push_back(Arl_in.elements());
          this->_indices_size.push_back(Arl_in.size());

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            Util::MemoryPool<Mem_>::instance()->increase_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            Util::MemoryPool<Mem_>::instance()->increase_memory(this->_indices.at(i));
        }

        /**
         * \brief Constructor
         *
         * \param[in] graph The.graph to create the matrix from
         *
         * Creates a ELL matrix based on a given adjacency graph, representing the sparsity pattern.
         */
        explicit SparseMatrixELL(const Adjacency::Graph & graph) :
          Container<Mem_, DT_, IT_>(0)
        {
          CONTEXT("When creating SparseMatrixELL");

          Index num_rows = graph.get_num_nodes_domain();
          Index num_cols = graph.get_num_nodes_image();
          Index num_nnze = graph.get_num_indices();

          const Index alignment(32);
          const Index tstride(alignment * ((num_cols + alignment - 1)/ alignment));

          const Index * dom_ptr(graph.get_domain_ptr());
          const Index * img_idx(graph.get_image_idx());

          // create temporary vector
          LAFEM::DenseVector<Mem::Main, IT_, IT_> tarl(num_rows);
          IT_ * ptarl(tarl.elements());

          Index tnum_cols_per_row(0);
          for (Index i(0); i < num_rows; ++i)
          {
            ptarl[i] = IT_(dom_ptr[i + 1] - dom_ptr[i]);
            if (ptarl[i] > tnum_cols_per_row)
              tnum_cols_per_row = ptarl[i];
          }

          // create temporary vectors
          LAFEM::DenseVector<Mem::Main, IT_, IT_> taj(tnum_cols_per_row * tstride);
          LAFEM::DenseVector<Mem::Main, DT_, IT_> tax(tnum_cols_per_row * tstride, DT_(0));
          IT_ * ptaj(taj.elements());

          for (Index row(0); row < num_rows; ++row)
          {
            Index target(0);
            for (Index i(0); i < ptarl[row]; ++i)
            {
              const Index row_start(dom_ptr[row]);
              ptaj[row + target * tstride] = IT_(img_idx[row_start + i]);
              target++;
            }
          }

          // build the matrix
          this->assign(SparseMatrixELL<Mem::Main, DT_, IT_>(num_rows, num_cols, tstride,
                                                            tnum_cols_per_row, num_nnze, tax, taj, tarl));
        }

        /**
         * \brief Constructor
         *
         * \param[in] mode The used file format.
         * \param[in] filename The source file.
         *
         * Creates a ELL matrix based on the source file.
         */
        explicit SparseMatrixELL(FileMode mode, String filename) :
          Container<Mem_, DT_, IT_>(0)
        {
          CONTEXT("When creating SparseMatrixELL");

          switch(mode)
          {
            case FileMode::fm_mtx:
              _read_from_mtx(filename);
              break;
            case FileMode::fm_ell:
              _read_from_ell(filename);
              break;
            default:
              throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
          }
        }

        /**
         * \brief Constructor
         *
         * \param[in] mode The used file format.
         * \param[in] file The source filestream.
         *
         * Creates a ELL matrix based on the source filestream.
         */
        explicit SparseMatrixELL(FileMode mode, std::istream& file) :
          Container<Mem_, DT_, IT_>(0)
        {
          CONTEXT("When creating SparseMatrixELL");

          switch(mode)
          {
            case FileMode::fm_mtx:
              _read_from_mtx(file);
              break;
            case FileMode::fm_ell:
              _read_from_ell(file);
              break;
            default:
              throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
          }
        }

        /**
         * \brief Constructor
         *
         * \param[in] std::pair<Index, char *> A std::pair, containing byte array size and byte array pointer.
         *
         * Creates a matrix from the given byte array.
         */
        template <typename DT2_ = DT_, typename IT2_ = IT_>
        explicit SparseMatrixELL(std::pair<Index, char *> input) :
          Container<Mem_, DT_, IT_>(0)
        {
          CONTEXT("When creating SparseMatrixELL");
          deserialise<DT2_, IT2_>(input);
        }

        /**
         * \brief Move Constructor
         *
         * \param[in] other The source matrix.
         *
         * Moves a given matrix to this matrix.
         */
        SparseMatrixELL(SparseMatrixELL && other) :
          Container<Mem_, DT_, IT_>(std::forward<SparseMatrixELL>(other))
        {
          CONTEXT("When moving SparseMatrixELL");
        }

        /**
         * \brief Move operator
         *
         * \param[in] other The source matrix.
         *
         * Moves another matrix to the target matrix.
         */
        SparseMatrixELL & operator= (SparseMatrixELL && other)
        {
          CONTEXT("When moving SparseMatrixELL");

          this->move(std::forward<SparseMatrixELL>(other));

          return *this;
        }

        /** \brief Clone operation
         *
         * Create a deep copy of itself.
         *
         * \param[in] clone_indices Should we create a deep copy of the index arrays, too ?
         *
         * \return A deep copy of itself.
         *
         */
        SparseMatrixELL clone(bool clone_indices = false) const
        {
          CONTEXT("When cloning SparseMatrixELL");
          SparseMatrixELL t;
          t.clone(*this, clone_indices);
          return t;
        }

        using Container<Mem_, DT_, IT_>::clone;

        /**
         * \brief Conversion method
         *
         * \param[in] other The source Matrix.
         *
         * Use source matrix content as content of current matrix
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const SparseMatrixELL<Mem2_, DT2_, IT2_> & other)
        {
          CONTEXT("When converting SparseMatrixELL");
          this->assign(other);
        }

        /**
         * \brief Conversion method
         *
         * \param[in] other The source Matrix.
         *
         * Use source matrix content as content of current matrix
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const SparseMatrixCOO<Mem2_, DT2_, IT2_> & other)
        {
          CONTEXT("When converting SparseMatrixELL");

          this->clear();

          this->_scalar_index.push_back(other.size());
          this->_scalar_index.push_back(other.rows());
          this->_scalar_index.push_back(other.columns());
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(other.used_elements());
          this->_scalar_dt.push_back(other.zero_element());

          SparseMatrixCOO<Mem::Main, DT_, IT_> cother;
          cother.convert(other);

          IT_ * tArl = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_rows());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tArl, IT_(0), _rows());

          _num_cols_per_row() = 0;
          for (Index i(0) ; i < _used_elements() ; ++i)

          {
            Index cur_row(cother.row_indices()[i]);
            ++tArl[cur_row];
            if (tArl[cur_row] > _num_cols_per_row())
              _num_cols_per_row() = tArl[cur_row];
          }

          Index alignment(32);
          _stride() = alignment * ((_rows() + alignment - 1)/ alignment);

          DT_ * tAx = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAx, DT_(0), _num_cols_per_row() * _stride());
          IT_ * tAj = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAj, IT_(0), _num_cols_per_row() * _stride());

          Index last_row(cother.row_indices()[0]);
          Index target(0);
          for (Index i(0) ; i < _used_elements() ; ++i)
          {
            Index row(cother.row_indices()[i]);
            if (row != last_row)
            {
              target = 0;
              last_row = row;
            }
            tAj[row + target * _stride()] = (cother.column_indices())[i];
            tAx[row + target * _stride()] = (cother.val())[i];
            target++;
          }

          this->_elements_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_rows());
          if (std::is_same<Mem_, Mem::Main>::value)
          {
            this->_elements.push_back(tAx);
            this->_indices.push_back(tAj);
            this->_indices.push_back(tArl);
          }
          else
          {
            this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_rows()));

            Util::MemoryPool<Mem_>::template upload<DT_>(this->get_elements().at(0), tAx, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(0), tAj, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(1), tArl, _rows());
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAx);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAj);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tArl);
          }
        }

        /**
         * \brief Conversion method
         *
         * \param[in] other The source Matrix.
         *
         * Use source matrix content as content of current matrix
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const SparseMatrixCSR<Mem2_, DT2_, IT2_> & other)
        {
          CONTEXT("When converting SparseMatrixELL");

          this->clear();

          this->_scalar_index.push_back(other.size());
          this->_scalar_index.push_back(other.rows());
          this->_scalar_index.push_back(other.columns());
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(other.used_elements());
          this->_scalar_dt.push_back(other.zero_element());

          SparseMatrixCSR<Mem::Main, DT_, IT_> cother;
          cother.convert(other);

          IT_ * tArl = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_rows());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tArl, IT_(0), _rows());

          _num_cols_per_row() = 0;
          for (Index i(0) ; i < _rows() ; ++i)
          {
            tArl[i] = cother.row_ptr()[i + 1] - cother.row_ptr()[i];
            if (tArl[i] > IT_(_num_cols_per_row()))
              _num_cols_per_row() = Index(tArl[i]);
          }

          Index alignment(32);
          _stride() = alignment * ((_rows() + alignment - 1)/ alignment);

          DT_ * tAx = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAx, DT_(0), _num_cols_per_row() * _stride());
          IT_ * tAj = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAj, IT_(0), _num_cols_per_row() * _stride());

          for (Index row(0); row < _rows() ; ++row)
          {
            Index target(0);
            for (IT_ i(0) ; i < tArl[row] ; ++i)
            {
              const IT_ row_start(cother.row_ptr()[row]);
              //if(cother.val()[row_start + i] != DT_(0))
              {
                tAj[row + target * _stride()] = (cother.col_ind())[row_start + i];
                tAx[row + target * _stride()] = (cother.val())[row_start + i];
                target++;
              }
            }
          }

          this->_elements_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_rows());
          if (std::is_same<Mem_, Mem::Main>::value)
          {
            this->_elements.push_back(tAx);
            this->_indices.push_back(tAj);
            this->_indices.push_back(tArl);
          }
          else
          {
            this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_rows()));

            Util::MemoryPool<Mem_>::template upload<DT_>(this->get_elements().at(0), tAx, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(0), tAj, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(1), tArl, _rows());
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAx);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAj);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tArl);
          }
        }

        /**
         * \brief Convertion method
         *
         * \param[in] other The source Matrix.
         *
         * Use source matrix content as content of current matrix
         */
        template <typename Mem2_, typename DT2_, typename IT2_>
        void convert(const SparseMatrixBanded<Mem2_, DT2_, IT2_> & other)
        {
          CONTEXT("When converting SparseMatrixELL");

          this->clear();

          this->_scalar_index.push_back(other.size());
          this->_scalar_index.push_back(other.rows());
          this->_scalar_index.push_back(other.columns());
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(0);
          this->_scalar_index.push_back(other.used_elements());
          this->_scalar_dt.push_back(other.zero_element());

          SparseMatrixBanded<Mem::Main, DT_, IT_> cother;
          cother.convert(other);

          IT_ * tArl = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_rows());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tArl, IT_(0), _rows());

          const DT_ * cval(cother.val());
          const IT_ * coffsets(cother.offsets());
          const Index cnum_of_offsets(cother.num_of_offsets());
          const Index crows(cother.rows());

          // Search first offset of the upper triangular matrix
          Index k(0);
          while (k < cnum_of_offsets && coffsets[k] + 1 < crows)
          {
            ++k;
          }

          if (cother.start_offset(0)
              <= cother.end_offset(cnum_of_offsets - 1))
          {
            _num_cols_per_row() = cnum_of_offsets;
          }
          else
          {
            for (Index i(k + 1); i > 0;)
            {
              --i;

              // iteration over all offsets of the upper triangular matrix
              for (Index j(cnum_of_offsets + 1); j > 0;)
              {
                --j;

                const Index start(Math::max(cother.start_offset(i),
                                            cother.end_offset(j) + 1));
                const Index end  (Math::min(cother.start_offset(i-1),
                                            cother.end_offset(j-1) + 1));
                if (j - i > _num_cols_per_row() &&
                    start < end)
                {
                  _num_cols_per_row() = j - i;
                }
              }
            }
          }

          Index alignment(32);
          _stride() = alignment * ((_rows() + alignment - 1)/ alignment);

          DT_ * tAx = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAx, DT_(0), _num_cols_per_row() * _stride());
          IT_ * tAj = Util::MemoryPool<Mem::Main>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride());
          Util::MemoryPool<Mem::Main>::instance()->set_memory(tAj, IT_(0), _num_cols_per_row() * _stride());

          for (Index i(k + 1); i > 0;)
          {
            --i;

            // iteration over all offsets of the upper triangular matrix
            for (Index j(cnum_of_offsets + 1); j > 0;)
            {
              --j;

              // iteration over all rows which contain the offsets between offset i and offset j
              const Index start(Math::max(cother.start_offset(i),
                                          cother.end_offset(j) + 1));
              const Index end  (Math::min(cother.start_offset(i-1),
                                          cother.end_offset(j-1) + 1));
              for (Index l(start); l < end; ++l)
              {
                tArl[l] = IT_(j - i);
                for (Index a(i); a < j; ++a)
                {
                  tAj[l + (a - i) * _stride()] = IT_(l + coffsets[a] + 1 - crows);
                  tAx[l + (a - i) * _stride()] = cval[a * crows + l];
                }
              }
            }
          }

          this->_elements_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_num_cols_per_row() * _stride());
          this->_indices_size.push_back(_rows());
          if (std::is_same<Mem_, Mem::Main>::value)
          {
            this->_elements.push_back(tAx);
            this->_indices.push_back(tAj);
            this->_indices.push_back(tArl);
          }
          else
          {
            this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_num_cols_per_row() * _stride()));
            this->_indices.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<IT_>(_rows()));

            Util::MemoryPool<Mem_>::template upload<DT_>(this->get_elements().at(0), tAx, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(0), tAj, _num_cols_per_row() * _stride());
            Util::MemoryPool<Mem_>::template upload<IT_>(this->get_indices().at(1), tArl, _rows());
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAx);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tAj);
            Util::MemoryPool<Mem::Main>::instance()->release_memory(tArl);
          }
        }

        /**
         * \brief Conversion method
         *
         * \param[in] a The input matrix.
         *
         * Converts any matrix to SparseMatrixELL-format
         */
        template <typename MT_>
        void convert(const MT_ & a)
        {
          CONTEXT("When converting SparseMatrixELL");

          typename MT_::template ContainerType<Mem::Main, DT_, IT_> ta;
          ta.convert(a);

          const Index arows(ta.rows());
          const Index acolumns(ta.columns());
          const Index aused_elements(ta.used_elements());

          Index alignment(32);
          const Index tastride(alignment * ((arows + alignment - 1)/ alignment));

          DenseVector<Mem::Main, IT_, IT_> arl(arows);
          IT_ * parl(arl.elements());

          Index tanum_cols_per_row(0);
          for (Index i(0); i < arows; ++i)
          {
            parl[i] = IT_(ta.get_length_of_line(i));
            if (tanum_cols_per_row < parl[i])
            {
              tanum_cols_per_row = parl[i];
            }
          }

          DenseVector<Mem::Main, DT_, IT_> ax(tastride * tanum_cols_per_row);
          DenseVector<Mem::Main, IT_, IT_> aj(tastride * tanum_cols_per_row);

          DT_ * pax(ax.elements());
          IT_ * paj(aj.elements());

          for (Index i(0); i < arows; ++i)
          {
            ta.set_line(i, pax + i, paj + i, 0, tastride);
          }

          SparseMatrixELL<Mem::Main, DT_, IT_> ta_ell(arows, acolumns, tastride, tanum_cols_per_row, aused_elements, ax, aj, arl);
          SparseMatrixELL<Mem_, DT_, IT_> a_ell;
          a_ell.convert(ta_ell);

          this->assign(a_ell);
        }

        /**
         * \brief Assignment operator
         *
         * \param[in] layout A sparse matrix layout.
         *
         * Assigns a new matrix layout, discarding all old data
         */
        SparseMatrixELL & operator= (const SparseLayout<Mem_, IT_, layout_id> & layout_in)
        {
          CONTEXT("When assigning SparseMatrixELL");

          for (Index i(0) ; i < this->_elements.size() ; ++i)
            Util::MemoryPool<Mem_>::instance()->release_memory(this->_elements.at(i));
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            Util::MemoryPool<Mem_>::instance()->release_memory(this->_indices.at(i));

          this->_elements.clear();
          this->_indices.clear();
          this->_elements_size.clear();
          this->_indices_size.clear();
          this->_scalar_index.clear();
          this->_scalar_dt.clear();

          this->_indices.assign(layout_in._indices.begin(), layout_in._indices.end());
          this->_indices_size.assign(layout_in._indices_size.begin(), layout_in._indices_size.end());
          this->_scalar_index.assign(layout_in._scalar_index.begin(), layout_in._scalar_index.end());
          this->_scalar_dt.push_back(DT_(0));

          for (auto i : this->_indices)
            Util::MemoryPool<Mem_>::instance()->increase_memory(i);

          this->_elements.push_back(Util::MemoryPool<Mem_>::instance()->template allocate_memory<DT_>(_num_cols_per_row() * _stride()));
          this->_elements_size.push_back(_num_cols_per_row() * _stride());

          return *this;
        }

        /**
         * \brief Deserialisation of complete container entity.
         *
         * \param[in] std::pair<Index, char *> A std::pair, containing byte array size and byte array pointer.
         *
         * Recreate a complete container entity by a single binary array.
         */
        template <typename DT2_ = DT_, typename IT2_ = IT_>
        void deserialise(std::pair<Index, char *> input)
        {
          this->template _deserialise<DT2_, IT2_>(FileMode::fm_ell, input);
        }

        /**
         * \brief Serialisation of complete container entity.
         *
         * \param[in] mode FileMode enum, describing the actual container specialisation.
         * \param[out] std::pair<Index, char *> A std::pair, containing byte array size and byte array pointer.
         *
         * Serialise a complete container entity into a single binary array.
         *
         * \warning The allocated array must be freed by the user!
         *
         * See \ref FEAST::LAFEM::Container::_serialise for details.
         */
        template <typename DT2_ = DT_, typename IT2_ = IT_>
        std::pair<Index, char *> serialise()
        {
          return this->template _serialise<DT2_, IT2_>(FileMode::fm_ell);
        }

        /**
         * \brief Write out matrix to file.
         *
         * \param[in] mode The used file format.
         * \param[in] filename The file where the matrix shall be stored.
         */
        void write_out(FileMode mode, String filename) const
        {
          CONTEXT("When writing out SparseMatrixELL");

          switch(mode)
          {
            case FileMode::fm_ell:
              write_out_ell(filename);
              break;
            case FileMode::fm_mtx:
              write_out_mtx(filename);
              break;
            default:
                throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
          }
        }

        /**
         * \brief Write out matrix to file.
         *
         * \param[in] mode The used file format.
         * \param[in] file The stream that shall be written to.
         */
        void write_out(FileMode mode, std::ostream& file) const
        {
          CONTEXT("When writing out SparseMatrixELL");

          switch(mode)
          {
            case FileMode::fm_ell:
              write_out_ell(file);
              break;
            case FileMode::fm_mtx:
              write_out_mtx(file);
              break;
            default:
                throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
          }
        }

        /**
         * \brief Write out matrix to ell binary file.
         *
         * \param[in] filename The file where the matrix shall be stored.
         */
        void write_out_ell(String filename) const
        {
          std::ofstream file(filename.c_str(), std::ofstream::out | std::ofstream::binary);
          if (! file.is_open())
            throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Matrix file " + filename);
          write_out_ell(file);
          file.close();
        }

        /**
         * \brief Write out matrix to ell binary file.
         *
         * \param[in] file The stream that shall be written to.
         */
        void write_out_ell(std::ostream& file) const
        {
          if (! std::is_same<DT_, double>::value)
            std::cout<<"Warning: You are writing out an ell matrix with less than double precission!"<<std::endl;

          this->template _serialise<double, uint64_t>(FileMode::fm_ell, file);
        }

        /**
         * \brief Write out matrix to MatrixMarktet mtx file.
         *
         * \param[in] filename The file where the matrix shall be stored.
         */
        void write_out_mtx(String filename) const
        {
          std::ofstream file(filename.c_str(), std::ofstream::out);
          if (! file.is_open())
            throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Matrix file " + filename);
          write_out_mtx(file);
          file.close();
        }

        /**
         * \brief Write out matrix to MatrixMarktet mtx file.
         *
         * \param[in] file The stream that shall be written to.
         */
        void write_out_mtx(std::ostream& file) const
        {
          SparseMatrixELL<Mem::Main, DT_, IT_> temp;
          temp.convert(*this);

          file << "%%MatrixMarket matrix coordinate real general" << std::endl;
          file << temp.rows() << " " << temp.columns() << " " << temp.used_elements() << std::endl;

          for (Index row(0) ; row < rows() ; ++row)
          {
            const IT_ * tAj(temp.Aj());
            const DT_ * tAx(temp.Ax());
            tAj += row;
            tAx += row;

            const IT_ max(temp.Arl()[row]);
            for(IT_ n(0); n < max ; n++)
            {
              file << stringify(row + 1) << " " << *tAj + 1 << " " << std::scientific << *tAx << std::endl;

              tAj += stride();
              tAx += stride();
            }
          }
        }

        /**
         * \brief Retrieve specific matrix element.
         *
         * \param[in] row The row of the matrix element.
         * \param[in] col The column of the matrix element.
         *
         * \returns Specific matrix element.
         */
        DT_ operator()(Index row, Index col) const
        {
          CONTEXT("When retrieving SparseMatrixELL element");

          ASSERT(row < rows(), "Error: " + stringify(row) + " exceeds sparse matrix ell row size " + stringify(rows()) + " !");
          ASSERT(col < columns(), "Error: " + stringify(col) + " exceeds sparse matrix ell column size " + stringify(columns()) + " !");

          Index max(Index(Util::MemoryPool<Mem_>::get_element(this->_indices.at(1), row)));
          for (Index i(row), j(0) ; j < max && Index(Util::MemoryPool<Mem_>::get_element(this->_indices.at(0), i)) <= col ; i += stride(), ++j)
          {
            if (Index(Util::MemoryPool<Mem_>::get_element(this->_indices.at(0), i)) == col)
              return Util::MemoryPool<Mem_>::get_element(this->_elements.at(0), i);
          }
          return zero_element();
        }

        /**
         * \brief Retrieve convenient sparse matrix layout object.
         *
         * \return An object containing the sparse matrix layout.
         */
        SparseLayout<Mem_, IT_, layout_id> layout() const
        {
          return SparseLayout<Mem_, IT_, layout_id>(this->_indices, this->_indices_size, this->_scalar_index);
        }

        /**
         * \brief Retrieve matrix row count.
         *
         * \returns Matrix row count.
         */
        const Index & rows() const
        {
          return this->_scalar_index.at(1);
        }

        /**
         * \brief Retrieve matrix column count.
         *
         * \returns Matrix column count.
         */
        const Index & columns() const
        {
          return this->_scalar_index.at(2);
        }

        /**
         * \brief Retrieve non zero element count.
         *
         * \returns Non zero element count.
         */
        const Index & used_elements() const override
        {
          return this->_scalar_index.at(5);
        }

        /**
         * \brief Retrieve column indices array.
         *
         * \returns Column indices array.
         */
        IT_ const * Aj() const
        {
          return this->_indices.at(0);
        }

        /**
         * \brief Retrieve non zero element array.
         *
         * \returns Non zero element array.
         */
        DT_ * Ax()
        {
          return this->_elements.at(0);
        }

        DT_ const * Ax() const
        {
          return this->_elements.at(0);
        }

        /**
         * \brief Retrieve row length array.
         *
         * \returns Row lenght array.
         */
        IT_ const * Arl() const
        {
          return this->_indices.at(1);
        }

        /**
         * \brief Retrieve non zero element.
         *
         * \returns Non zero element.
         */
        DT_ zero_element() const
        {
          return this->_scalar_dt.at(0);
        }

        /**
         * \brief Retrieve stride, i.e. lowest common multiple of row count and warp size.
         *
         * \returns Stride.
         */
        const Index & stride() const
        {
          return this->_scalar_index.at(3);
        }

        /**
         * \brief Retrieve the maximum amount of non zero columns in a single row.
         *
         * \returns Columns per row count.
         */
        const Index & num_cols_per_row() const
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
          return "SparseMatrixELL";
        }

        /**
         * \brief Performs \f$this \leftarrow x\f$.
         *
         * \param[in] x The Matrix to be copied.
         */
        void copy(const SparseMatrixELL & x)
        {
          this->_copy_content(x);
        }

        /**
         * \brief Performs \f$this \leftarrow x\f$.
         *
         * \param[in] x The Matrix to be copied.
         */
        template <typename Mem2_>
        void copy(const SparseMatrixELL<Mem2_, DT_, IT_> & x)
        {
          this->_copy_content(x);
        }

        ///@name Linear algebra operations
        ///@{
        /**
         * \brief Calculate \f$this \leftarrow y + \alpha x\f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[in] x The first summand matrix to be scaled.
         * \param[in] y The second summand matrix
         * \param[in] alpha A scalar to multiply x with.
         */
        template <typename Algo_>
        void axpy(
          const SparseMatrixELL & x,
          const SparseMatrixELL & y,
          DT_ alpha = DT_(1))
        {
          if (x.rows() != y.rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix rows do not match!");
          if (x.rows() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix rows do not match!");
          if (x.columns() != y.columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix columns do not match!");
          if (x.columns() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix columns do not match!");
          if (x.used_elements() != y.used_elements())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix used_elements do not match!");
          if (x.used_elements() != this->used_elements())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix used_elements do not match!");
          if (x.stride() != y.stride())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix stride do not match!");
          if (x.stride() != this->stride())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix stride do not match!");
          if (x.num_cols_per_row() != y.num_cols_per_row())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix num_cols_per_row do not match!");
          if (x.num_cols_per_row() != this->num_cols_per_row())
            throw InternalError(__func__, __FILE__, __LINE__, "Matrix num_cols_per_row do not match!");

          // check for special cases
          // r <- x + y
          if(Math::abs(alpha - DT_(1)) < Math::eps<DT_>())
            Arch::Sum<Mem_, Algo_>::value(this->Ax(), x.Ax(), y.Ax(), this->stride() * this->num_cols_per_row());
          // r <- y - x
          else if(Math::abs(alpha + DT_(1)) < Math::eps<DT_>())
            Arch::Difference<Mem_, Algo_>::value(this->Ax(), y.Ax(), x.Ax(), this->stride() * this->num_cols_per_row());
          // r <- y
          else if (Math::abs(alpha) < Math::eps<DT_>())
            this->copy(y);
          // r <- y + alpha*x
          else
            Arch::Axpy<Mem_, Algo_>::dv(this->Ax(), alpha, x.Ax(), y.Ax(), this->stride() * this->num_cols_per_row());
        }

        /**
         * \brief Calculate \f$this \leftarrow \alpha x\f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[in] x The matrix to be scaled.
         * \param[in] alpha A scalar to scale x with.
         */
        template <typename Algo_>
        void scale(const SparseMatrixELL & x, const DT_ alpha)
        {
          if (x.rows() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Row count does not match!");
          if (x.columns() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Column count does not match!");
          if (x.used_elements() != this->used_elements())
            throw InternalError(__func__, __FILE__, __LINE__, "Nonzero count does not match!");

          Arch::Scale<Mem_, Algo_>::value(this->Ax(), x.Ax(), alpha, this->stride() * this->num_cols_per_row());
        }

        /**
         * \brief Calculates the Frobenius norm of this matrix.
         *
         * \returns The Frobenius norm of this matrix.
         */
        template <typename Algo_>
        DT_ norm_frobenius() const
        {
          return Arch::Norm2<Mem_, Algo_>::value(this->Ax(), this->stride() * this->num_cols_per_row());
        }

        /**
         * \brief Calculate \f$this^\top \f$
         *
         * \return The transposed matrix
         */
        SparseMatrixELL transpose()
        {
          SparseMatrixELL x_t;
          x_t.transpose(*this);
          return x_t;
        }

        /**
         * \brief Calculate \f$this \leftarrow x^\top \f$
         *
         * \param[in] x The matrix to be transposed.
         */
        void transpose(const SparseMatrixELL & x)
        {
          SparseMatrixELL<Mem::Main, DT_, IT_> tx;
          tx.convert(x);

          const Index txrows(tx.rows());
          const Index txcolumns(tx.columns());
          const Index txused_elements(tx.used_elements());
          const Index txstride(tx.stride());

          const DT_ * ptxax(tx.Ax());
          const IT_ * ptxaj(tx.Aj());
          const IT_ * ptxarl(tx.Arl());

          const Index alignment(32);
          const Index tstride(alignment * ((txcolumns + alignment - 1)/ alignment));

          DenseVector<Mem::Main, IT_, IT_> tarl(txcolumns, IT_(0));
          IT_ * ptarl(tarl.elements());

          for (Index i(0); i < txrows; ++i)
          {
            for (Index j(i); j < i + ptxarl[i] * txstride; j += txstride)
            {
              ++ptarl[ptxaj[j]];
            }
          }

          Index tnum_cols_per_row(0);
          for (Index i(0); i < txcolumns; ++i)
          {
            if (tnum_cols_per_row < ptarl[i])
            {
              tnum_cols_per_row = ptarl[i];
            }
            ptarl[i] = 0;
          }

          DenseVector<Mem::Main, IT_, IT_> taj(tstride * tnum_cols_per_row);
          DenseVector<Mem::Main, DT_, IT_> tax(tstride * tnum_cols_per_row);

          IT_ * ptaj(taj.elements());
          DT_ * ptax(tax.elements());

          for (Index i(0); i < txrows; ++i)
          {
            for (Index j(i); j < i + ptxarl[i] * txstride; j += txstride)
            {
              const Index k(ptxaj[j]);
              ptaj[k + ptarl[k] * tstride] = IT_(i);
              ptax[k + ptarl[k] * tstride] = ptxax[j];
              ++ptarl[k];
            }
          }

          SparseMatrixELL<Mem::Main, DT_, IT_> tx_t(txcolumns, txrows, tstride, tnum_cols_per_row, txused_elements, tax, taj, tarl);

          SparseMatrixELL<Mem_, DT_, IT_> x_t;
          x_t.convert(tx_t);
          this->assign(x_t);
        }

        /**
         * \brief Calculate \f$ this_{ij} \leftarrow x_{ij}\cdot s_i\f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[in] x The matrix whose rows are to be scaled.
         * \param[in] s The vector to the scale the rows by.
         */
        template<typename Algo_>
        void scale_rows(const SparseMatrixELL & x, const DenseVector<Mem_,DT_,IT_> & s)
        {
          if (x.rows() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Row count does not match!");
          if (x.columns() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Column count does not match!");
          if (x.used_elements() != this->used_elements())
            throw InternalError(__func__, __FILE__, __LINE__, "Nonzero count does not match!");
          if (s.size() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          Arch::ScaleRows<Mem_, Algo_>::ell(this->Ax(), x.Ax(), this->Aj(), this->Arl(),
            s.elements(), stride(), rows());
        }

        /**
         * \brief Calculate \f$ this_{ij} \leftarrow x_{ij}\cdot s_j\f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[in] x The matrix whose columns are to be scaled.
         * \param[in] s The vector to the scale the columns by.
         */
        template<typename Algo_>
        void scale_cols(const SparseMatrixELL & x, const DenseVector<Mem_,DT_,IT_> & s)
        {
          if (x.rows() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Row count does not match!");
          if (x.columns() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Column count does not match!");
          if (x.used_elements() != this->used_elements())
            throw InternalError(__func__, __FILE__, __LINE__, "Nonzero count does not match!");
          if (s.size() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

          Arch::ScaleCols<Mem_, Algo_>::ell(this->Ax(), x.Ax(), this->Aj(), this->Arl(),
            s.elements(), stride(), rows());
        }

        /**
         * \brief Calculate \f$ r \leftarrow this\cdot x \f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[out] r The vector that recieves the result.
         * \param[in] x The vector to be multiplied by this matrix.
         */
        template<typename Algo_>
        void apply(DenseVector<Mem_,DT_, IT_> & r, const DenseVector<Mem_, DT_, IT_> & x) const
        {
          if (r.size() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of r does not match!");
          if (x.size() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of x does not match!");

          Arch::ProductMatVec<Mem_, Algo_>::ell(r.elements(), this->Ax(), this->Aj(), this->Arl(),
            x.elements(), this->stride(), this->rows());
        }

        /**
         * \brief Calculate \f$ r \leftarrow this\cdot x \f$, global version.
         *
         * \param[out] r The vector that recieves the result.
         * \param[in] x The vector to be multiplied by this matrix.
         * \param[in] gate The gate base pointer
         */
        template<typename Algo_>
        void apply(DenseVector<Mem_,DT_, IT_>& r,
                   const DenseVector<Mem_, DT_, IT_>& x,
                   Arch::ProductMat0Vec1GatewayBase<Mem_, Algo_, DenseVector<Mem_, DT_, IT_>, SparseMatrixELL<Mem_, DT_, IT_> >* gate
                  )
        {
          if (r.size() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of r does not match!");
          if (x.size() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of x does not match!");

          gate->value(r, *this, x);
        }

        /**
         * \brief Calculate \f$r \leftarrow y + \alpha this\cdot x \f$
         *
         * \tparam Algo_ The \ref FEAST::Algo "algorithm" to be used.
         *
         * \param[out] r The vector that recieves the result.
         * \param[in] x The vector to be multiplied by this matrix.
         * \param[in] y The summand vector.
         * \param[in] alpha A scalar to scale the product with.
         */
        template<typename Algo_>
        void apply(
          DenseVector<Mem_,DT_, IT_>& r,
          const DenseVector<Mem_, DT_, IT_>& x,
          const DenseVector<Mem_, DT_, IT_>& y,
          const DT_ alpha = DT_(1)) const
        {
          if (r.size() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of r does not match!");
          if (x.size() != this->columns())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of x does not match!");
          if (y.size() != this->rows())
            throw InternalError(__func__, __FILE__, __LINE__, "Vector size of y does not match!");

          // check for special cases
          // r <- y - A*x
          if(Math::abs(alpha + DT_(1)) < Math::eps<DT_>())
          {
            Arch::Defect<Mem_, Algo_>::ell(r.elements(), y.elements(), this->Ax(), this->Aj(),
              this->Arl(), x.elements(), this->stride(), this->rows());
          }
          // r <- y
          else if (Math::abs(alpha) < Math::eps<DT_>())
            r.copy(y);
          // r <- y + alpha*x
          else
          {
            Arch::Axpy<Mem_, Algo_>::ell(r.elements(), alpha, x.elements(), y.elements(),
              this->Ax(), this->Aj(), this->Arl(), this->stride(), this->rows());
          }
        }
        ///@}

        /// Returns a new compatible L-Vector.
        VectorTypeL create_vector_l() const
        {
          return VectorTypeL(this->rows());
        }

        /// Returns a new compatible R-Vector.
        VectorTypeR create_vector_r() const
        {
          return VectorTypeR(this->columns());
        }

        /// Returns the number of NNZ-elements of the selected row
        Index get_length_of_line(const Index row) const
        {
          return this->Arl()[row];
        }

        /// \cond internal

        /// Writes the non-zero-values and matching col-indices of the selected row in allocated arrays
        void set_line(const Index row, DT_ * const pval_set, IT_ * const pcol_set,
                              const Index col_start, const Index stride_in = 1) const
        {
          const IT_ * parl(this->Arl());
          const IT_ * paj(this->Aj() + row);
          const DT_ * pax(this->Ax() + row);
          const Index astride(this->stride());

          const Index length(parl[row]);

          for (Index i(0); i < length; ++i)
          {
            pval_set[i * stride_in] = pax[i * astride];
            pcol_set[i * stride_in] = paj[i * astride] + IT_(col_start);
          }
        }
        /// \endcond

        /**
         * \brief SparseMatrixELL comparison operator
         *
         * \param[in] a A matrix to compare with.
         * \param[in] b A matrix to compare with.
         */
        template <typename Mem2_> friend bool operator== (const SparseMatrixELL & a, const SparseMatrixELL<Mem2_, DT_, IT_> & b)
        {
          CONTEXT("When comparing SparseMatrixELLs");

          if (a.rows() != b.rows())
            return false;
          if (a.columns() != b.columns())
            return false;
          if (a.used_elements() != b.used_elements())
            return false;
          if (a.zero_element() != b.zero_element())
            return false;
          if (a.stride() != b.stride())
            return false;
          if (a.num_cols_per_row() != b.num_cols_per_row())
            return false;

          if(a.size() == 0 && b.size() == 0 && a.get_elements().size() == 0 && a.get_indices().size() == 0 && b.get_elements().size() == 0 && b.get_indices().size() == 0)
            return true;

          IT_ * aj_a;
          IT_ * aj_b;
          DT_ * ax_a;
          DT_ * ax_b;
          IT_ * arl_a;
          IT_ * arl_b;

          bool ret(true);

          if(std::is_same<Mem::Main, Mem_>::value)
          {
            aj_a = (IT_*)a.Aj();
            ax_a = (DT_*)a.Ax();
            arl_a = (IT_*)a.Arl();
          }
          else
          {
            aj_a = new IT_[a.stride() * a.num_cols_per_row()];
            Util::MemoryPool<Mem_>::instance()->template download<IT_>(aj_a, a.Aj(), a.stride() * a.num_cols_per_row());
            ax_a = new DT_[a.stride() * a.num_cols_per_row()];
            Util::MemoryPool<Mem_>::instance()->template download<DT_>(ax_a, a.Ax(), a.stride() * a.num_cols_per_row());
            arl_a = new IT_[a.rows()];
            Util::MemoryPool<Mem_>::instance()->template download<IT_>(arl_a, a.Arl(), a.rows());
          }
          if(std::is_same<Mem::Main, Mem2_>::value)
          {
            aj_b = (IT_*)b.Aj();
            ax_b = (DT_*)b.Ax();
            arl_b = (IT_*)b.Arl();
          }
          else
          {
            aj_b = new IT_[b.stride() * b.num_cols_per_row()];
            Util::MemoryPool<Mem2_>::instance()->template download<IT_>(aj_b, b.Aj(), b.stride() * b.num_cols_per_row());
            ax_b = new DT_[b.stride() * b.num_cols_per_row()];
            Util::MemoryPool<Mem2_>::instance()->template download<DT_>(ax_b, b.Ax(), b.stride() * b.num_cols_per_row());
            arl_b = new IT_[b.rows()];
            Util::MemoryPool<Mem2_>::instance()->template download<IT_>(arl_b, b.Arl(), b.rows());
          }

          for (Index i(0) ; i < a.rows() ; ++i)
          {
            if(arl_a[i] != arl_b[i])
              ret = false;
            break;
          }

          if (ret)
          {
            Index stride(a.stride());
            for (Index row(0) ; row < a.rows() ; ++row)
            {
              const IT_ * tAj(aj_a);
              const DT_ * tAx(ax_a);
              const IT_ * tAj2(aj_b);
              const DT_ * tAx2(ax_b);
              tAj += row;
              tAx += row;
              tAj2 += row;
              tAx2 += row;

              const IT_ max(arl_a[row]);
              for(IT_ n(0); n < max ; n++)
              {
                if (*tAj != *tAj2 || *tAx != *tAx2)
                {
                  ret = false;
                  break;
                }
                tAj += stride;
                tAx += stride;
                tAj2 += stride;
                tAx2 += stride;
              }
            }
          }

          if(! std::is_same<Mem::Main, Mem_>::value)
          {
            delete[] aj_a;
            delete[] ax_a;
            delete[] arl_a;
          }
          if(! std::is_same<Mem::Main, Mem2_>::value)
          {
            delete[] aj_b;
            delete[] ax_b;
            delete[] arl_b;
          }

          return ret;
        }

        /**
         * \brief SparseMatrixELL streaming operator
         *
         * \param[in] lhs The target stream.
         * \param[in] b The matrix to be streamed.
         */
        friend std::ostream & operator<< (std::ostream & lhs, const SparseMatrixELL & b)
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
    };

  } // namespace LAFEM
} // namespace FEAST

#endif // KERNEL_LAFEM_SPARSE_MATRIX_ELL_HPP
