#pragma once
#ifndef KERNEL_GEOMETRY_TARGET_SET_HPP
#define KERNEL_GEOMETRY_TARGET_SET_HPP 1

// includes, FEAT
#include <kernel/shape.hpp>

namespace FEAT
{
  namespace Geometry
  {
    /**
     * \brief Target set class
     *
     * A MeshPart refers to its parent mesh through several TargetSets.
     *
     * \author Peter Zajac
     */
    class TargetSet
    {
    protected:
      /// number of entities
      Index _num_entities;

      /// Index array. _indices[i] = j means that entity i represents entity j in the parent
      Index* _indices;

    private:
      /// \brief Prevent the compiler from generating a copy assignment operator by declaring without implementing
      TargetSet& operator=(const TargetSet&);

    public:
      /**
       * \brief Constructor
       *
       * \param[in] num_entities
       * The number of entities that are to be indexed.
       */
      explicit TargetSet(Index num_entities = 0) :
        _num_entities(num_entities),
        _indices(nullptr)
      {
        if(num_entities > 0)
        {
          _indices = new Index[num_entities];
        }
      }

      /**
       * \brief Copy Constructor
       *
       * \param[in] other
       * The target set that is to be copied.
       */
      TargetSet(const TargetSet& other) :
        _num_entities(other._num_entities),
        _indices(nullptr)
      {
        if(_num_entities > 0)
        {
          _indices = new Index[_num_entities];
          for(Index i(0); i < _num_entities; ++i)
          {
            _indices[i] = other._indices[i];
          }
        }
      }

      /// virtual destructor
      virtual ~TargetSet()
      {
        if(_indices != nullptr)
        {
          delete [] _indices;
        }
        _indices = nullptr;
      }

      /**
       * \brief Move assignment operator
       *
       * \param[in] other
       * Other target set that gets moved to this.
       *
       * \returns
       * A reference to the object other was moved to.
       */
      TargetSet& operator=(TargetSet&& other)
      {
        if(this != &other)
        {
          _num_entities = other._num_entities;

          if(_indices != nullptr)
            delete[] _indices;

          _indices = other._indices;

          other._num_entities = 0;
          other._indices = nullptr;
        }
        return *this;
      }

      /// \returns The size of dynamically allocated memory in bytes.
      std::size_t bytes() const
      {
        return std::size_t(_num_entities) * sizeof(Index);
      }

      /// Returns the number of entities.
      Index get_num_entities() const
      {
        return _num_entities;
      }

      /**
       * \returns A pointer to the target index array.
       */
      Index* get_indices()
      {
        return _indices;
      }

      /** \copydoc get_indices() */
      const Index* get_indices() const
      {
        return _indices;
      }
      /*
      Index& get_index(Index i)
      {
        return _indices[i];
      }

      const Index& get_index(Index i) const
      {
        return _indices[i];
      }*/

      /**
       * \brief Returns a target index.
       *
       * \param[in] i
       * The index of the entity whose target index is to be returned.
       *
       * \returns
       * A reference to the target index of entity \p i.
       */
      Index& operator[](Index i)
      {
        return _indices[i];
      }

      /** \copydoc operator[]() */
      const Index& operator[](Index i) const
      {
        return _indices[i];
      }
    }; // class TargetSet

    /// \cond internal
    /**
     * \brief Template recursive array of TargetSets
     *
     * A MeshPart refering to a mesh of Shape_ can have Shape_::dimension+1 TargetSets and this class provides the
     * means of accessing them. It inherits from the TargetSetHolder class wrt. the same shape of one dimension less.
     *
     * \tparam Shape_
     * Shape type this class refers to.
     *
     */
    template<typename Shape_>
    class TargetSetHolder :
      public TargetSetHolder<typename Shape::FaceTraits<Shape_, Shape_::dimension - 1>::ShapeType>
    {
    public:
      typedef Shape_ ShapeType;
      static constexpr int shape_dim = ShapeType::dimension;

    protected:
      typedef TargetSetHolder<typename Shape::FaceTraits<ShapeType, shape_dim - 1>::ShapeType> BaseClass;

      TargetSet _target_set;

    public:
      explicit TargetSetHolder(const Index num_entities[]) :
        BaseClass(num_entities),
        _target_set(num_entities[shape_dim])
      {
      }

      TargetSetHolder(const TargetSetHolder& other) :
        BaseClass(other),
        _target_set(other._target_set)
      {
      }

      virtual ~TargetSetHolder()
      {
      }

      std::size_t bytes() const
      {
        return BaseClass::bytes() + _target_set.bytes();
      }

      template<int dim_>
      TargetSet& get_target_set()
      {
        static_assert(dim_ >= 0, "invalid dimension");
        static_assert(dim_ <= shape_dim, "invalid dimension");
        typedef typename Shape::FaceTraits<Shape_, dim_>::ShapeType CellType;
        return TargetSetHolder<CellType>::_target_set;
      }

      template<int dim_>
      const TargetSet& get_target_set() const
      {
        static_assert(dim_ >= 0, "invalid dimension");
        static_assert(dim_ <= shape_dim, "invalid dimension");
        typedef typename Shape::FaceTraits<Shape_, dim_>::ShapeType CellType;
        return TargetSetHolder<CellType>::_target_set;
      }

      Index get_num_entities(int dim) const
      {
        ASSERT(dim <= shape_dim, "invalid dimension parameter");
        if(dim == shape_dim)
        {
          return _target_set.get_num_entities();
        }
        return BaseClass::get_num_entities(dim);
      }

      static String name()
      {
        return "TargetSetHolder<" + Shape_::name() + ">";
      }
    };

    /**
     * \brief TargetSetHolder for lowest possible dimension as end of the recursive inheritance
     */
    template<>
    class TargetSetHolder<Shape::Vertex>
    {
    public:
      typedef Shape::Vertex ShapeType;
      static constexpr int shape_dim = ShapeType::dimension;

    protected:
      TargetSet _target_set;

    public:
      TargetSetHolder() :
        _target_set()
      {
      }

      explicit TargetSetHolder(const Index num_entities[]) :
        _target_set(num_entities[0])
      {
      }

      TargetSetHolder(const TargetSetHolder& other) :
        _target_set(other._target_set)
      {
      }

      virtual ~TargetSetHolder()
      {
      }

      std::size_t bytes() const
      {
        return _target_set.bytes();
      }

      template<int dim_>
      TargetSet& get_target_set()
      {
        static_assert(dim_ == 0, "invalid dimension");
        return _target_set;
      }

      template<int dim_>
      const TargetSet& get_target_set() const
      {
        static_assert(dim_ == 0, "invalid dimension");
        return _target_set;
      }

      Index get_num_entities(int dim) const
      {
#if defined DEBUG
        ASSERT(dim == 0, "invalid dimension parameter");
#else
        (void)dim;
#endif
        return _target_set.get_num_entities();
      }

      static String name()
      {
        return "TargetSetHolder<Vertex>";
      }
    };
    /// \endcond
  } // namespace Geometry
} // namespace FEAT

#endif // KERNEL_GEOMETRY_TARGET_SET_HPP
