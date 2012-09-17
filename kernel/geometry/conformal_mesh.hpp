#pragma once
#ifndef KERNEL_GEOMETRY_CONFORMAL_MESH_HPP
#define KERNEL_GEOMETRY_CONFORMAL_MESH_HPP 1

// includes, FEAST
#include <kernel/geometry/intern/standard_index_refiner.hpp>
#include <kernel/geometry/intern/standard_vertex_refiner.hpp>

namespace FEAST
{
  namespace Geometry
  {
    /**
     * \brief Standard conformal mesh policy
     *
     * This class defines a default policy for the ConformalMesh class template.
     *
     * \tparam Shape_
     * The shape that is to be used for the mesh. Must be either Shape::Simplex<n> or Shape::Hypercube<n>
     * for some \c n > 0.
     *
     * \tparam VertexSet_
     * The vertex set class to be used by the mesh. By default, VertexSetFixed is used.
     *
     * \author Peter Zajac
     */
    template<
      typename Shape_,
      typename VertexSet_ = VertexSetFixed<Shape_::dimension> >
    struct ConformalMeshPolicy
    {
      /// shape type
      typedef Shape_ ShapeType;

      /// Vertex set type
      typedef VertexSet_ VertexSetType;
    }; // struct ConformalMeshPolicy

    /**
     * \brief Conformal mesh class template
     *
     * \todo detailed documentation
     *
     * \author Peter Zajac
     */
    template<typename Policy_>
    class ConformalMesh
    {
    public:
      /// policy type
      typedef Policy_ PolicyType;

      /// Shape type
      typedef typename PolicyType::ShapeType ShapeType;

      /// Vertex set type
      typedef typename PolicyType::VertexSetType VertexSetType;

      /// index set holder type
      typedef IndexSetHolder<ShapeType> IndexSetHolderType;

      /// dummy enum
      enum
      {
        /// shape dimension
        shape_dim = ShapeType::dimension,
        /// world dimension
        world_dim = VertexSetType::num_coords
      };

      /**
       * \brief Index set type class template
       *
       * This nested class template is used to define the return type of the ConformalMesh::get_index_set()
       * function template.
       *
       * \tparam cell_dim_, face_dim_
       * The cell and face dimension parameters as passed to the ConformalMesh::get_index_set() function template.
       */
      template<
        int cell_dim_,
        int face_dim_>
      struct IndexSet
      {
        static_assert(cell_dim_ <= shape_dim, "invalid cell dimension");
        static_assert(face_dim_ < cell_dim_, "invalid face/cell dimension");
        static_assert(face_dim_ >= 0, "invalid face dimension");

        /// index set type
        typedef
          FEAST::Geometry::IndexSet<
            Shape::FaceTraits<
              typename Shape::FaceTraits<
                ShapeType,
                cell_dim_>
              ::ShapeType,
              face_dim_>
            ::count> Type;
      }; // struct IndexSet<...>


    protected:
      /// number of entities for each shape dimension
      Index _num_entities[shape_dim + 1];

      /// the vertex set of the mesh
      VertexSetType _vertex_set;

      /// the index sets of the mesh
      IndexSetHolderType _index_set_holder;

    private:
      ConformalMesh(const ConformalMesh&);
      ConformalMesh& operator=(const ConformalMesh&);

    public:
      /**
       * \brief Constructor.
       *
       * \param[in] num_entities
       * An array of length at least #shape_dim + 1 holding the number of entities for each shape dimension.
       * Must not be \c nullptr.
       *
       * \param[in] num_coords
       * The number of coordinates per vertex. This parameter is passed to the constructor of the vertex set.
       *
       * \param[in] vertex_stride
       * The vertex stride. This parameter is passed to the constructor of the vertex set.
       */
      explicit ConformalMesh(
        const Index num_entities[])
          :
        _vertex_set(num_entities[0]),
        _index_set_holder(num_entities)
      {
        CONTEXT(name() + "::ConformalMesh(const Index[])");
        for(int i(0); i <= shape_dim; ++i)
        {
          ASSERT(num_entities[i] > 0, "Number of entities must not be zero!");
          _num_entities[i] = num_entities[i];
        }
      }

      /// virtual destructor
      virtual ~ConformalMesh()
      {
        CONTEXT(name() + "::~ConformalMesh()");
      }

      /**
       * \brief Returns the number of entities.
       *
       * \param[in] dim
       * The dimension of the entity whose count is to be returned. Must be 0 <= \p dim <= #shape_dim.
       *
       * \returns
       * The number of entities of dimension \p dim.
       */
      Index get_num_entities(int dim) const
      {
        CONTEXT(name() + "::get_num_entities()");
        ASSERT_(dim >= 0);
        ASSERT_(dim <= shape_dim);
        return _num_entities[dim];
      }

      /// Returns a reference to the vertex set of the mesh.
      VertexSetType& get_vertex_set()
      {
        CONTEXT(name() + "::get_vertex_set()");
        return _vertex_set;
      }

      /** \copydoc get_vertex_set() */
      const VertexSetType& get_vertex_set() const
      {
        CONTEXT(name() + "::get_vertex_set() [const]");
        return _vertex_set;
      }

      /**
       * \brief Returns the reference to an index set.
       *
       * \tparam cell_dim_
       * The dimension of the entity whose index set is to be returned.
       *
       * \tparam face_dim_
       * The dimension of the face that the index set refers to.
       *
       * \returns
       * A reference to the index set.
       */
      template<
        int cell_dim_,
        int face_dim_>
      typename IndexSet<cell_dim_, face_dim_>::Type& get_index_set()
      {
        CONTEXT(name() + "::get_index_set<" + stringify(cell_dim_) + "," + stringify(face_dim_) + ">()");
        return _index_set_holder.template get_index_set_wrapper<cell_dim_>().get_index_set<face_dim_>();
      }

      /** \copydoc get_index_set() */
      template<
        int cell_dim_,
        int face_dim_>
      const typename IndexSet<cell_dim_, face_dim_>::Type& get_index_set() const
      {
        CONTEXT(name() + "::get_index_set<" + stringify(cell_dim_) + "," + stringify(face_dim_) + ">() [const]");
        return _index_set_holder.template get_index_set_wrapper<cell_dim_>().get_index_set<face_dim_>();
      }

      /// \cond internal
      IndexSetHolderType& get_index_set_holder()
      {
        CONTEXT(name() + "::get_index_set_holder()");
        return _index_set_holder;
      }

      const IndexSetHolderType& get_index_set_holder() const
      {
        CONTEXT(name() + "::get_index_set_holder() [const]");
        return _index_set_holder;
      }
      /// \endcond

      /**
       * \brief Refines the mesh.
       *
       * This function applies the standard refinement algorithm onto the mesh and returns the refined mesh.
       *
       * \returns
       * A pointer to the refined mesh.
       */
      ConformalMesh* refine() const
      {
        CONTEXT(name() + "::refine()");

        // get number of entities in coarse mesh
        Index num_entities_fine[shape_dim + 1];
        for(int i(0); i <= shape_dim; ++i)
        {
          num_entities_fine[i] = _num_entities[i];
        }

        // calculate number of entities in fine mesh
        Intern::EntityCountWrapper<ShapeType>::query(num_entities_fine);

        // allocate a fine mesh
        ConformalMesh* fine_mesh = new ConformalMesh(num_entities_fine);

        // refine vertices
        Intern::StandardVertexRefineWrapper<ShapeType, VertexSetType>
          ::refine(fine_mesh->_vertex_set, _vertex_set, _index_set_holder);

        // refine indices
        Intern::IndexRefineWrapper<ShapeType>
          ::refine(fine_mesh->_index_set_holder, _num_entities, _index_set_holder);

        // return fine mesh
        return fine_mesh;
      }

      /**
       * \brief Returns the name of the class.
       * \returns
       * The name of the class as a String.
       */
      static String name()
      {
        return "ConformalMesh<...>";
      }
    }; // class ConformalMesh<...>
  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_CONFORMAL_MESH_HPP
