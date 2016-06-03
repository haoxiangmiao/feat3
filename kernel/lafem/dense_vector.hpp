#pragma once
#ifndef KERNEL_LAFEM_DENSE_VECTOR_HPP
#define KERNEL_LAFEM_DENSE_VECTOR_HPP 1

// includes, FEAT
#include <kernel/base_header.hpp>
#include <kernel/lafem/forward.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/util/type_traits.hpp>
#include <kernel/util/math.hpp>
#include <kernel/lafem/container.hpp>
#include <kernel/lafem/dense_vector_blocked.hpp>
#include <kernel/lafem/edi.hpp>
#include <kernel/lafem/arch/sum.hpp>
#include <kernel/lafem/arch/component_invert.hpp>
#include <kernel/lafem/arch/difference.hpp>
#include <kernel/lafem/arch/dot_product.hpp>
#include <kernel/lafem/arch/norm.hpp>
#include <kernel/lafem/arch/scale.hpp>
#include <kernel/lafem/arch/axpy.hpp>
#include <kernel/lafem/arch/component_product.hpp>
#include <kernel/adjacency/permutation.hpp>
#include <kernel/util/statistics.hpp>
#include <kernel/util/time_stamp.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <stdint.h>


namespace FEAT
{
  namespace LAFEM
  {
    /**
     * \brief Dense data vector class template.
     *
     * \tparam Mem_ The \ref FEAT::Mem "memory architecture" to be used.
     * \tparam DT_ The datatype to be used.
     * \tparam IT_ The indextype to be used.
     *
     * This class represents a vector of continuous data in memory. \n \n
     * Data survey: \n
     * _elements[0]: raw number values \n
     * _scalar_index[0]: container size
     * _scalar_index[1]: boolean flag, signaling DenseVectorRange usage
     *
     * Refer to \ref lafem_design for general usage informations.
     *
     * \author Dirk Ribbrock
     */
    template <typename Mem_, typename DT_, typename IT_ = Index>
    class DenseVector : public Container<Mem_, DT_, IT_>
    {
    public:
      /**
       * \brief Scatter-Axpy operation for DenseVector
       *
       * \author Peter Zajac
       */
      class ScatterAxpy
      {
      public:
        typedef LAFEM::DenseVector<Mem::Main, DT_, IT_> VectorType;
        typedef Mem::Main MemType;
        typedef DT_ DataType;
        typedef IT_ IndexType;

      private:
        Index _num_entries;
        DT_* _data;

      public:
        explicit ScatterAxpy(VectorType& vector) :
          _num_entries(vector.size()),
          _data(vector.elements())
        {
        }

        template<typename LocalVector_, typename Mapping_>
        void operator()(const LocalVector_& loc_vec, const Mapping_& mapping, DT_ alpha = DT_(1))
        {
          // loop over all local entries
          for(int i(0); i < mapping.get_num_local_dofs(); ++i)
          {
            // pre-multiply local entry by alpha
            DT_ dx(alpha * loc_vec[i]);

            // loop over all entry contributions
            for(int ic(0); ic < mapping.get_num_contribs(i); ++ic)
            {
              // get dof index
              Index dof_idx = mapping.get_index(i, ic);
              ASSERT_(dof_idx < _num_entries);

              // update vector data
              _data[dof_idx] += DT_(mapping.get_weight(i, ic)) * dx;
            }
          }
        }
      }; // class ScatterAxpy

      /**
       * \brief Gather-Axpy operation for DenseVector
       *
       * \author Peter Zajac
       */
      class GatherAxpy
      {
      public:
        typedef LAFEM::DenseVector<Mem::Main, DT_, IT_> VectorType;
        typedef Mem::Main MemType;
        typedef DT_ DataType;
        typedef IT_ IndexType;

      private:
        Index _num_entries;
        const DT_* _data;

      public:
        explicit GatherAxpy(const VectorType& vector) :
          _num_entries(vector.size()),
          _data(vector.elements())
        {
        }

        template<typename LocalVector_, typename Mapping_>
        void operator()(LocalVector_& loc_vec, const Mapping_& mapping, DT_ alpha = DT_(1))
        {
          // loop over all local entries
          for(int i(0); i < mapping.get_num_local_dofs(); ++i)
          {
            // clear accumulation entry
            DT_ dx(DT_(0));

            // loop over all entry contributions
            for(int ic(0); ic < mapping.get_num_contribs(i); ++ic)
            {
              // get dof index
              Index dof_idx = mapping.get_index(i, ic);
              ASSERT_(dof_idx < _num_entries);

              // update accumulator
              dx += DT_(mapping.get_weight(i, ic)) * _data[dof_idx];
            }

            // update local vector data
            loc_vec[i] += alpha * dx;
          }
        }
      }; // class GatherAxpy

    private:
      Index & _size()
      {
        return this->_scalar_index.at(0);
      }

      /// \cond internal
      template <typename VT_, typename IT2_>
      void _convert(const typename VT_::template ContainerType<Mem_, DT_, IT2_> & a)
      {
        DenseVector vec(a.template size<Perspective::pod>());
        a.set_vec(vec.elements());

        this->assign(vec);
      }

      template <typename VT_>
      void _convert(const VT_ & a)
      {
        typename VT_::template ContainerType<Mem_, DT_, IT_> ta;
        ta.convert(a);

        this->convert(ta);
      }

      template <typename VT_, typename Mem2_, typename IT2_>
      void _copy(const typename VT_::template ContainerType<Mem2_, DT_, IT2_> & a)
      {
        if (std::is_same<Mem_, Mem2_>::value)
        {
          a.set_vec(this->elements());
        }
        else
        {
          typename VT_::template ContainerType<Mem_, DT_, IT2_> ta;
          ta.convert(a);

          this->copy(ta);
        }
      }

      template <typename VT_, typename Mem2_, typename IT2_>
      void _copy_inv(typename VT_::template ContainerType<Mem2_, DT_, IT2_> & a) const
      {
        if (std::is_same<Mem_, Mem2_>::value)
        {
          a.set_vec_inv(this->elements());
        }
        else
        {
          DenseVector<Mem2_, DT_, IT_> t_this;
          t_this.convert(*this);

          t_this.copy_inv(a);
        }
      }
      /// \endcond

    public:
      /// Our datatype
      typedef DT_ DataType;
      /// Our indextype
      typedef IT_ IndexType;
      /// Our memory architecture type
      typedef Mem_ MemType;
      /// Our value type
      typedef DT_ ValueType;
      /// Our 'base' class type
      template <typename Mem2_, typename DT2_ = DT_, typename IT2_ = IT_>
      using ContainerType = class DenseVector<Mem2_, DT2_, IT2_>;

      /**
       * \brief Constructor
       *
       * Creates an empty non dimensional vector.
       */
      explicit DenseVector() :
        Container<Mem_, DT_, IT_> (0)
      {
        this->_scalar_index.push_back(0);
      }

      /**
       * \brief Constructor
       *
       * \param[in] size The size of the created vector.
       * \param[in] pinned_allocation True if the memory should be allocated in a pinned manner.
       *
       * \warning Pinned memory allocation is only possible in main memory and needs cuda support.
       *
       * Creates a vector with a given size.
       */
      explicit DenseVector(Index size_in, bool pinned_allocation = false) :
        Container<Mem_, DT_, IT_>(size_in)
      {
        ASSERT(! (pinned_allocation && (typeid(Mem_) != typeid(Mem::Main))), "Error: Pinned memory allocation only possible in main memory!");

        this->_scalar_index.push_back(0);

        if (pinned_allocation)
        {
#ifdef FEAT_BACKENDS_CUDA
          this->_elements.push_back(MemoryPool<Mem::Main>::template allocate_pinned_memory<DT_>(size_in));
#else
          // no cuda support enabled - we cannot serve and do not need pinned memory support
          this->_elements.push_back(MemoryPool<Mem_>::template allocate_memory<DT_>(size_in));
#endif
        }
        else
        {
          this->_elements.push_back(MemoryPool<Mem_>::template allocate_memory<DT_>(size_in));
        }

        this->_elements_size.push_back(size_in);
      }

      /**
       * \brief Constructor
       *
       * \param[in] size The size of the created vector.
       * \param[in] value The value, each element will be set to.
       *
       * Creates a vector with given size and value.
       */
      explicit DenseVector(Index size_in, DT_ value) :
        Container<Mem_, DT_, IT_>(size_in)
      {
        this->_scalar_index.push_back(0);
        this->_elements.push_back(MemoryPool<Mem_>::template allocate_memory<DT_>(size_in));
        this->_elements_size.push_back(size_in);

        MemoryPool<Mem_>::set_memory(this->_elements.at(0), value, size_in);
      }

      /**
       * \brief Constructor
       *
       * \param[in] size The size of the created vector.
       * \param[in] data An array containing the value data.
       *
       * Creates a vector with given size and given data.
       *
       * \note The array must be allocated by FEAT's own memory pool
       */
      explicit DenseVector(Index size_in, DT_ * data) :
        Container<Mem_, DT_, IT_>(size_in)
      {
        this->_scalar_index.push_back(0);
        this->_elements.push_back(data);
        this->_elements_size.push_back(size_in);

        for (Index i(0) ; i < this->_elements.size() ; ++i)
          MemoryPool<Mem_>::increase_memory(this->_elements.at(i));
        for (Index i(0) ; i < this->_indices.size() ; ++i)
          MemoryPool<Mem_>::increase_memory(this->_indices.at(i));
      }

      /**
       * \brief Constructor
       *
       * \param[in] dv_ind The source DenseVector
       * \param[in] size_in The size of the created vector range.
       * \param[in] offset The starting element of the created vector range in relation to the source vector.
       *
       * Creates a vector range from a given DenseVector
       *
       * \note The created DenseVector has no own memory management nor own allocated memory and should be used carefully!
       */
      explicit DenseVector(const DenseVector & dv_in, Index size_in, Index offset_in) :
        Container<Mem_, DT_, IT_>(size_in)
      {
        ASSERT(size_in + offset_in <= dv_in.size(), "Ranged vector part exceeds orig vector size!");

        this->_scalar_index.push_back(1);
        DT_ * te(const_cast<DT_*>(dv_in.elements()));
        this->_elements.push_back(te + offset_in);
        this->_elements_size.push_back(size_in);
      }

      /**
       * \brief Constructor
       *
       * \param[in] other The source blocked vector
       *
       * Creates a vector from a given source blocked vector
       */
      template <int BS_>
      explicit DenseVector(const DenseVectorBlocked<Mem_, DT_, IT_, BS_> & other) :
        Container<Mem_, DT_, IT_>(other.template size<Perspective::pod>())
      {
        this->_scalar_index.push_back(0);
        convert(other);
      }

      /**
       * \brief Constructor
       *
       * \param[in] mode The used file format.
       * \param[in] filename The source file.
       *
       * Creates a vector from the given source file.
       */
      explicit DenseVector(FileMode mode, String filename) :
        Container<Mem_, DT_, IT_>(0)
      {
        read_from(mode, filename);
      }

      /**
       * \brief Constructor
       *
       * \param[in] mode The used file format.
       * \param[in] file The stream that is to be read from.
       *
       * Creates a vector from the given source file.
       */
      explicit DenseVector(FileMode mode, std::istream& file) :
        Container<Mem_, DT_, IT_>(0)
      {
        read_from(mode, file);
      }

      /**
       * \brief Constructor
       *
       * \param[in] std::vector<char> A std::vector, containing the byte.
       *
       * Creates a vector from the given byte array.
       */
      template <typename DT2_ = DT_, typename IT2_ = IT_>
      explicit DenseVector(std::vector<char> input) :
        Container<Mem_, DT_, IT_>(0)
      {
        deserialise<DT2_, IT2_>(input);
      }

      /**
       * \brief Move Constructor
       *
       * \param[in] other The source vector.
       *
       * Moves another vector to this vector.
       */
      DenseVector(DenseVector && other) :
        Container<Mem_, DT_, IT_>(std::forward<DenseVector>(other))
      {
      }

      /**
       * \brief Destructor
       *
       * Destroys the DenseVector and releases all of its used arrays if its not marked a range vector.
       */
      virtual ~DenseVector()
      {
        // avoid releasing memory by base class destructor, because we do not own the referenced memory
        if (this->_scalar_index.size() > 0 && this->_scalar_index.at(1) == 1)
        {
          for (Index i(0) ; i < this->_elements.size() ; ++i)
            this->_elements.at(i) = nullptr;
          for (Index i(0) ; i < this->_indices.size() ; ++i)
            this->_indices.at(i) = nullptr;
        }
      }

      /**
       * \brief Assignment move operator
       *
       * \param[in] other The source vector.
       *
       * Moves another vector to the target vector.
       */
      DenseVector & operator= (DenseVector && other)
      {
        this->move(std::forward<DenseVector>(other));

        return *this;
      }

      InsertWeakClone( DenseVector )

      /** \brief Shallow copy operation
       *
       * Create a shallow copy of itself.
       *
       */
      DenseVector shared() const
      {
        DenseVector r;
        r.assign(*this);
        return r;
      }

      /**
       * \brief Conversion method
       *
       * \param[in] other The source vector.
       *
       * Use source vector content as content of current vector
       */
      template <typename Mem2_, typename DT2_, typename IT2_>
      void convert(const DenseVector<Mem2_, DT2_, IT2_> & other)
      {
        this->assign(other);
      }

      /**
       * \brief Conversion method
       *
       * \param[in] other The source vector.
       *
       * Use source vector content as content of current vector
       */
      template <typename Mem2_, typename DT2_, typename IT2_, int BS2_>
      void convert(const DenseVectorBlocked<Mem2_, DT2_, IT2_, BS2_> & other)
      {
        this->clear();

        this->_scalar_index.push_back(other.template size<Perspective::pod>());
        this->_scalar_index.push_back(0);
        this->_elements.push_back(other.get_elements().at(0));
        this->_elements_size.push_back(this->size());

        for (Index i(0) ; i < this->_elements.size() ; ++i)
          MemoryPool<Mem_>::increase_memory(this->_elements.at(i));
        for (Index i(0) ; i < this->_indices.size() ; ++i)
          MemoryPool<Mem_>::increase_memory(this->_indices.at(i));
      }

      /**
       * \brief Conversion method
       *
       * \param[in] a The input vector.
       *
       * Converts any vector to DenseVector-format
       */
      template <typename VT_>
      void convert(const VT_ & a)
      {
        this->template _convert<VT_>(a);
      }

      /**
       * \brief Deserialisation of complete container entity.
       *
       * \param[in] std::vector<char> A std::vector, containing the byte array.
       *
       * Recreate a complete container entity by a single binary array.
       */
      template <typename DT2_ = DT_, typename IT2_ = IT_>
      void deserialise(std::vector<char> input)
      {
        this->template _deserialise<DT2_, IT2_>(FileMode::fm_dv, input);
      }

      /**
       * \brief Serialisation of complete container entity.
       *
       * \param[in] mode FileMode enum, describing the actual container specialisation.
       * \param[out] std::vector<char> A std::vector, containing the byte array.
       *
       * Serialize a complete container entity into a single binary array.
       *
       * See \ref FEAT::LAFEM::Container::_serialise for details.
       */
      template <typename DT2_ = DT_, typename IT2_ = IT_>
      std::vector<char> serialise()
      {
        return this->template _serialise<DT2_, IT2_>(FileMode::fm_dv);
      }

      /**
       * \brief Performs \f$this \leftarrow x\f$.
       *
       * \param[in] x The vector to be copied (could be of any format; must have same size).
       */
      template<typename VT_>
      void copy(const VT_ & a)
      {
        if (this->size() != a.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vectors have not the same size!");

        this->template _copy<VT_>(a);
      }

      /**
       * \brief Performs \f$x \leftarrow this\f$.
       *
       * \param[in] x The target-vector to be copied to (could be of any format; must have same size).
       */
      template<typename VT_>
      void copy_inv(VT_ & a) const
      {
        if (this->size() != a.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vectors have not the same size!");

        this->template _copy_inv<VT_>(a);
      }

      /**
       * \brief Read in vector from file.
       *
       * \param[in] mode The used file format.
       * \param[in] filename The file that shall be read in.
       */
      void read_from(FileMode mode, String filename)
      {
        switch(mode)
        {
        case FileMode::fm_mtx:
          read_from_mtx(filename);
          break;
        case FileMode::fm_exp:
          read_from_exp(filename);
          break;
        case FileMode::fm_dv:
          read_from_dv(filename);
          break;
        case FileMode::fm_binary:
          read_from_dv(filename);
          break;
        default:
          throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
        }
      }

      /**
       * \brief Read in vector from stream.
       *
       * \param[in] mode The used file format.
       * \param[in] file The stream that shall be read in.
       */
      void read_from(FileMode mode, std::istream& file)
      {
        switch(mode)
        {
        case FileMode::fm_mtx:
          read_from_mtx(file);
          break;
        case FileMode::fm_exp:
          read_from_exp(file);
          break;
        case FileMode::fm_dv:
          read_from_dv(file);
          break;
        case FileMode::fm_binary:
          read_from_dv(file);
          break;
        default:
          throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
        }
      }

      /**
       * \brief Read in vector from MatrixMarket mtx file.
       *
       * \param[in] filename The file that shall be read in.
       */
      void read_from_mtx(String filename)
      {
        std::ifstream file(filename.c_str(), std::ifstream::in);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        read_from_mtx(file);
        file.close();
      }

      /**
       * \brief Read in vector from MatrixMarket mtx stream.
       *
       * \param[in] file The stream that shall be read in.
       */
      void read_from_mtx(std::istream& file)
      {
        this->clear();
        this->_scalar_index.push_back(0);
        this->_scalar_index.push_back(0);

        Index rows;
        String line;
        std::getline(file, line);
        if (line.find("%%MatrixMarket matrix array real general") == String::npos)
        {
          throw InternalError(__func__, __FILE__, __LINE__, "Input-file is not a compatible mtx-vector-file");
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
          String srows(line, 0, end);
          rows = (Index)atol(srows.c_str());
          line.erase(0, end);

          begin = line.find_first_not_of(" ");
          line.erase(0, begin);
          end = line.find_first_of(" ");
          String scols(line, 0, end);
          Index cols((Index)atol(scols.c_str()));
          line.erase(0, end);
          if (cols != 1)
            throw InternalError(__func__, __FILE__, __LINE__, "Input-file is no dense-vector-file");
        }

        DenseVector<Mem::Main, DT_, IT_> tmp(rows);
        DT_ * pval(tmp.elements());

        while(!file.eof())
        {
          std::getline(file, line);
          if (file.eof())
            break;

          String::size_type begin(line.find_first_not_of(" "));
          line.erase(0, begin);
          String::size_type end(line.find_first_of(" "));
          String sval(line, 0, end);
          DT_ tval((DT_)atof(sval.c_str()));

          *pval = tval;
          ++pval;
        }
        this->assign(tmp);
      }

      /**
       * \brief Read in vector from ASCII file.
       *
       * \param[in] filename The file that shall be read in.
       */
      void read_from_exp(String filename)
      {
        std::ifstream file(filename.c_str(), std::ifstream::in);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        read_from_exp(file);
        file.close();
      }

      /**
       * \brief Read in vector from ASCII stream.
       *
       * \param[in] file The stream that shall be read in.
       */
      void read_from_exp(std::istream& file)
      {
        this->clear();
        this->_scalar_index.push_back(0);
        this->_scalar_index.push_back(0);

        std::vector<DT_> data;

        while(!file.eof())
        {
          std::string line;
          std::getline(file, line);
          if(line.find("#", 0) < line.npos)
            continue;
          if(file.eof())
            break;

          std::string n_z_s;

          std::string::size_type first_digit(line.find_first_not_of(" "));
          line.erase(0, first_digit);
          std::string::size_type eol(line.length());
          for(unsigned long i(0) ; i < eol ; ++i)
          {
            n_z_s.append(1, line[i]);
          }

          DT_ n_z((DT_)atof(n_z_s.c_str()));

          data.push_back(n_z);

        }

        _size() = Index(data.size());
        this->_elements.push_back(MemoryPool<Mem_>::template allocate_memory<DT_>(Index(data.size())));
        this->_elements_size.push_back(Index(data.size()));
        MemoryPool<Mem_>::template upload<DT_>(this->_elements.at(0), &data[0], Index(data.size()));
      }

      /**
       * \brief Read in vector from binary file.
       *
       * \param[in] filename The file that shall be read in.
       */
      void read_from_dv(String filename)
      {
        std::ifstream file(filename.c_str(), std::ifstream::in | std::ifstream::binary);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        read_from_dv(file);
        file.close();
      }

      /**
       * \brief Read in vector from binary stream.
       *
       * \param[in] file The stream that shall be read in.
       */
      void read_from_dv(std::istream& file)
      {
        this->template _deserialise<double, uint64_t>(FileMode::fm_dv, file);
      }


      /**
       * \brief Write out vector to file.
       *
       * \param[in] mode The used file format.
       * \param[in] filename The file where the vector shall be stored.
       */
      void write_out(FileMode mode, String filename) const
      {
        switch(mode)
        {
        case FileMode::fm_mtx:
          write_out_mtx(filename);
          break;
        case FileMode::fm_exp:
          write_out_exp(filename);
          break;
        case FileMode::fm_dv:
          write_out_dv(filename);
          break;
        case FileMode::fm_binary:
          write_out_dv(filename);
          break;
        default:
          throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
        }
      }

      /**
       * \brief Write out vector to file.
       *
       * \param[in] mode The used file format.
       * \param[in] file The stream that shall be written to.
       */
      void write_out(FileMode mode, std::ostream& file) const
      {
        switch(mode)
        {
        case FileMode::fm_mtx:
          write_out_mtx(file);
          break;
        case FileMode::fm_exp:
          write_out_exp(file);
          break;
        case FileMode::fm_dv:
          write_out_dv(file);
          break;
        case FileMode::fm_binary:
          write_out_dv(file);
          break;
        default:
          throw InternalError(__func__, __FILE__, __LINE__, "Filemode not supported!");
        }
      }

      /**
       * \brief Write out vector to MatrixMarket mtx file.
       *
       * \param[in] filename The file where the vector shall be stored.
       */
      void write_out_mtx(String filename) const
      {
        std::ofstream file(filename.c_str(), std::ofstream::out);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        write_out_mtx(file);
        file.close();
      }

      /**
       * \brief Write out vector to MatrixMarket mtx file.
       *
       * \param[in] file The stream that shall be written to.
       */
      void write_out_mtx(std::ostream& file) const
      {
        DenseVector<Mem::Main, DT_, IT_> temp;
        temp.convert(*this);

        const Index tsize(temp.size());
        file << "%%MatrixMarket matrix array real general" << std::endl;
        file << tsize << " " << 1 << std::endl;

        const DT_ * pval(temp.elements());
        for (Index i(0) ; i < tsize ; ++i, ++pval)
        {
          file << std::scientific << *pval << std::endl;
        }
      }

      /**
       * \brief Write out vector to file.
       *
       * \param[in] filename The file where the vector shall be stored.
       */
      void write_out_exp(String filename) const
      {
        std::ofstream file(filename.c_str(), std::ofstream::out);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        write_out_exp(file);
        file.close();
      }

      /**
       * \brief Write out vector to file.
       *
       * \param[in] file The stream that shall be written to.
       */
      void write_out_exp(std::ostream& file) const
      {
        DT_ * temp = MemoryPool<Mem::Main>::template allocate_memory<DT_>((this->size()));
        MemoryPool<Mem_>::template download<DT_>(temp, this->_elements.at(0), this->size());

        for (Index i(0) ; i < this->size() ; ++i)
        {
          file << std::scientific << temp[i] << std::endl;
        }

        MemoryPool<Mem::Main>::release_memory(temp);
      }

      /**
       * \brief Write out vector to file.
       *
       * \param[in] filename The file where the vector shall be stored.
       */
      void write_out_dv(String filename) const
      {
        std::ofstream file(filename.c_str(), std::ofstream::out | std::ofstream::binary);
        if (! file.is_open())
          throw InternalError(__func__, __FILE__, __LINE__, "Unable to open Vector file " + filename);
        write_out_dv(file);
        file.close();
      }

      /**
       * \brief Write out vector to file.
       *
       * \param[in] file The stream that shall be written to.
       */
      void write_out_dv(std::ostream& file) const
      {
        if (! std::is_same<DT_, double>::value)
          std::cout<<"Warning: You are writing out a dense vector that is not double precision!"<<std::endl;

        this->template _serialise<double, uint64_t>(FileMode::fm_dv, file);
      }

      /**
       * \brief Get a pointer to the data array.
       *
       * \returns Pointer to the data array.
       */
      template <Perspective = Perspective::native>
      DT_ * elements()
      {
        if (this->_elements.size() == 0)
          return nullptr;

        return this->_elements.at(0);
      }

      template <Perspective = Perspective::native>
      DT_ const * elements() const
      {
        if (this->_elements.size() == 0)
          return nullptr;

        return this->_elements.at(0);
      }

      /**
       * \brief Retrieve specific vector element.
       *
       * \param[in] index The index of the vector element.
       *
       * \returns Specific vector element.
       */
      const DT_ operator()(Index index) const
      {
        ASSERT(index < this->size(), "Error: " + stringify(index) + " exceeds dense vector size " + stringify(this->size()) + " !");
        return MemoryPool<Mem_>::get_element(this->_elements.at(0), index);
      }

      /**
       * \brief Set specific vector element.
       *
       * \param[in] index The index of the vector element.
       * \param[in] value The value to be set.
       */
      void operator()(Index index, DT_ value)
      {
        ASSERT(index < this->size(), "Error: " + stringify(index) + " exceeds dense vector size " + stringify(this->size()) + " !");
        MemoryPool<Mem_>::set_memory(this->_elements.at(0) + index, value);
      }

      /**
       * \brief Create temporary object for direct data manipulation.
       * \warning Be aware, that any synchronisation only takes place, when the object is destroyed!
       *
       * \param[in] index The index of the vector element.
       */
      EDI<Mem_, DT_> edi(Index index)
      {
        EDI<Mem_, DT_> t(MemoryPool<Mem_>::get_element(this->_elements.at(0), index), this->_elements.at(0) + index);
        return t;
      }

      /**
       * \brief Returns a descriptive string.
       *
       * \returns A string describing the container.
       */
      static String name()
      {
        return "DenseVector";
      }

      /**
       * \brief Performs \f$this \leftarrow x\f$.
       *
       * \param[in] x The vector to be copied.
       * \param[in] full Shall we create a full copy, including scalars and index arrays?
       */
      void copy(const DenseVector & x, bool full = false)
      {
        this->_copy_content(x, full);
      }

      /**
       * \brief Performs \f$this \leftarrow x\f$.
       *
       * \param[in] x The vector to be copied.
       * \param[in] full Shall we create a full copy, including scalars and index arrays?
       */
      template <typename Mem2_>
      void copy(const DenseVector<Mem2_, DT_, IT_> & x, bool full = false)
      {
        this->_copy_content(x, full);
      }

      ///@name Linear algebra operations
      ///@{
      /**
       * \brief Calculate \f$this \leftarrow \alpha~ x + y\f$
       *
       * \param[in] x The first summand vector to be scaled.
       * \param[in] y The second summand vector
       * \param[in] alpha A scalar to multiply x with.
       */
      void axpy(
                const DenseVector & x,
                const DenseVector & y,
                const DT_ alpha = DT_(1))
      {
        if (x.size() != y.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
        if (x.size() != this->size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

        TimeStamp ts_start;

        // check for special cases
        // r <- x + y
        if(Math::abs(alpha - DT_(1)) < Math::eps<DT_>())
        {
          Statistics::add_flops(this->size());
          Arch::Sum<Mem_>::value(this->elements(), x.elements(), y.elements(), this->size());
        }
        // r <- y - x
        else if(Math::abs(alpha + DT_(1)) < Math::eps<DT_>())
        {
          Statistics::add_flops(this->size());
          Arch::Difference<Mem_>::value(this->elements(), y.elements(), x.elements(), this->size());
        }
        // r <- y
        else if(Math::abs(alpha) < Math::eps<DT_>())
          this->copy(y);
        // r <- y + alpha*x
        else
        {
          Statistics::add_flops(this->size() * 2);
          Arch::Axpy<Mem_>::dv(this->elements(), alpha, x.elements(), y.elements(), this->size());
        }

        TimeStamp ts_stop;
        Statistics::add_time_axpy(ts_stop.elapsed(ts_start));
      }

      /**
       * \brief Calculate \f$this_i \leftarrow x_i \cdot y_i\f$
       *
       * \param[in] x The first factor.
       * \param[in] y The second factor.
       */
      void component_product(const DenseVector & x, const DenseVector & y)
      {
        if (this->size() != x.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");
        if (this->size() != y.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

        TimeStamp ts_start;

        Statistics::add_flops(this->size());
        Arch::ComponentProduct<Mem_>::value(this->elements(), x.elements(), y.elements(), this->size());

        TimeStamp ts_stop;
        Statistics::add_time_axpy(ts_stop.elapsed(ts_start));
      }

      /**
       * \brief Performs \f$ this_i \leftarrow \alpha / x_i \f$
       *
       * \param[in] x
       * The vector whose components serve as denominators.
       *
       * \param[in] alpha
       * The nominator.
       */
      void component_invert(const DenseVector & x, const DT_ alpha = DT_(1))
      {
        if (this->size() != x.size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

        TimeStamp ts_start;

        Statistics::add_flops(this->size());
        Arch::ComponentInvert<Mem_>::value(this->elements(), x.elements(), alpha, this->size());

        TimeStamp ts_stop;
        Statistics::add_time_axpy(ts_stop.elapsed(ts_start));
      }

      /**
       * \brief Calculate \f$this \leftarrow \alpha~ x \f$
       *
       * \param[in] x The vector to be scaled.
       * \param[in] alpha A scalar to scale x with.
       */
      void scale(const DenseVector & x, const DT_ alpha)
      {
        if (x.size() != this->size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

        TimeStamp ts_start;

        Statistics::add_flops(this->size());
        Arch::Scale<Mem_>::value(this->elements(), x.elements(), alpha, this->size());

        TimeStamp ts_stop;
        Statistics::add_time_axpy(ts_stop.elapsed(ts_start));
      }

      /**
       * \brief Calculate \f$result \leftarrow x^T \mathrm{diag}(this) y \f$
       *
       * \param[in] x The first vector.
       *
       * \param[in] y The second vector.
       *
       * \return The computed triple dot product.
       */
      DataType triple_dot(const DenseVector & x, const DenseVector & y) const
      {
        if (x.size() != this->size() || y.size() != this->size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector sizes does not match!");

        TimeStamp ts_start;

        Statistics::add_flops(this->size() * 3);
        DataType result = Arch::TripleDotProduct<Mem_>::value(this->elements(), x.elements(), y.elements(), this->size());

        TimeStamp ts_stop;
        Statistics::add_time_reduction(ts_stop.elapsed(ts_start));

        return result;
      }

      /**
       * \brief Calculate \f$result \leftarrow this \cdot x\f$
       *
       * \param[in] x The other vector.
       *
       * \return The computed dot product.
       */
      DataType dot(const DenseVector & x) const
      {
        if (x.size() != this->size())
          throw InternalError(__func__, __FILE__, __LINE__, "Vector size does not match!");

        TimeStamp ts_start;

        Statistics::add_flops(this->size() * 2);
        DataType result = Arch::DotProduct<Mem_>::value(this->elements(), x.elements(), this->size());

        TimeStamp ts_stop;
        Statistics::add_time_reduction(ts_stop.elapsed(ts_start));

        return result;
      }

      /**
       * \brief Calculates and returns the euclid norm of this vector.
       *
       */
      DT_ norm2() const
      {
        TimeStamp ts_start;

        DT_ result = Arch::Norm2<Mem_>::value(this->elements(), this->size());

        TimeStamp ts_stop;
        Statistics::add_time_reduction(ts_stop.elapsed(ts_start));

        return result;
      }

      /**
       * \brief Calculates and returns the squared euclid norm of this vector.
       *
       * \return The computed norm.
       *
       */
      DT_ norm2sqr() const
      {
        // fallback
        return Math::sqr(this->norm2());
      }

      ///@}

      /// Permutate vector according to the given Permutation
      void permute(Adjacency::Permutation & perm)
      {
        if (perm.size() == 0)
          return;

        ASSERT(perm.size() == this->size(), "Error: Container size " + stringify(this->size()) + " does not match permutation size " + stringify(perm.size()) + " !");

        DenseVector<Mem::Main, DT_, IT_> local;
        local.convert(*this);
        perm(local.elements());
        this->assign(local);
      }

      /// \cond internal
      /// Writes the vector-entries in an allocated array
      void set_vec(DT_ * const pval_set) const
      {
        MemoryPool<Mem_>::copy(pval_set, this->elements(), this->size());
      }

      /// Writes data of an array in the vector
      void set_vec_inv(const DT_ * const pval_set)
      {
        MemoryPool<Mem_>::copy(this->elements(), pval_set, this->size());
      }
      /// \endcond

      /**
       * \brief DenseVector comparison operator
       *
       * \param[in] a A vector to compare with.
       * \param[in] b A vector to compare with.
       */
      template <typename Mem2_> friend bool operator== (const DenseVector & a, const DenseVector<Mem2_, DT_, IT_> & b)
      {
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
          ta = new DT_[a.size()];
          MemoryPool<Mem_>::template download<DT_>(ta, a.elements(), a.size());
        }
        if(std::is_same<Mem::Main, Mem2_>::value)
          tb = (DT_*)b.elements();
        else
        {
          tb = new DT_[b.size()];
          MemoryPool<Mem2_>::template download<DT_>(tb, b.elements(), b.size());
        }

        for (Index i(0) ; i < a.size() ; ++i)
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
       * \brief DenseVector streaming operator
       *
       * \param[in] lhs The target stream.
       * \param[in] b The vector to be streamed.
       */
      friend std::ostream & operator<< (std::ostream & lhs, const DenseVector & b)
      {
        lhs << "[";
        for (Index i(0) ; i < b.size() ; ++i)
        {
          lhs << "  " << b(i);
        }
        lhs << "]";

        return lhs;
      }
    }; // class DenseVector<...>
  } // namespace LAFEM
} // namespace FEAT

#endif // KERNEL_LAFEM_DENSE_VECTOR_HPP
