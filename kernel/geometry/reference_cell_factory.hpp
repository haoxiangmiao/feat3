#pragma once
#ifndef KERNEL_GEOMETRY_REFERENCE_FACTORY_HPP
#define KERNEL_GEOMETRY_REFERENCE_FACTORY_HPP 1

// includes, FEAST
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/intern/macro_index_mapping.hpp>

namespace FEAST
{
  namespace Geometry
  {
    /// \cond internal
    namespace Intern
    {
      template<typename Shape_>
      struct RefCellVertexer;
    }
    /// \endcond

    /**
     * \brief Reference Cell Mesh factory
     *
     * This factory creates a ConformalMesh representing the reference cell of the underlying shape.
     *
     * \author Peter Zajac
     */
    template<typename Shape_, typename CoordType_ = Real>
    class ReferenceCellFactory :
      public Factory< ConformalMesh<Shape_, Shape_::dimension, Shape_::dimension, CoordType_> >
    {
    public:
      /// mesh type
      typedef ConformalMesh<Shape_, Shape_::dimension, Shape_::dimension, CoordType_> MeshType;
      /// shape type
      typedef typename MeshType::ShapeType ShapeType;
      /// vertex set type
      typedef typename MeshType::VertexSetType VertexSetType;
      /// index holder type
      typedef typename MeshType::IndexSetHolderType IndexSetHolderType;

      /// dummy enum
      enum
      {
        /// shape dimension
        shape_dim = ShapeType::dimension
      };

    public:
      virtual Index get_num_entities(int dim)
      {
        return Index(Intern::DynamicNumFaces<Shape_>::value(dim));
      }

      virtual void fill_vertex_set(VertexSetType& vertex_set)
      {
        Intern::RefCellVertexer<ShapeType>::fill(vertex_set);
      }

      virtual void fill_index_sets(IndexSetHolderType& index_set_holder)
      {
        Intern::MacroIndexWrapper<Shape_>::build(index_set_holder);
      }
    }; // class ReferenceCellFactory<...>

    /// \cond internal
    namespace Intern
    {
      template<int dim_>
      struct RefCellVertexer< Shape::Simplex<dim_> >
      {
        template<typename VertexSet_>
        static void fill(VertexSet_& vtx)
        {
          typedef typename VertexSet_::CoordType CoordType;
          // loop over all vertices
          for(Index i(0); i < Index(dim_+1); ++i)
          {
            // loop over all coords
            for(Index j(0); j < Index(dim_); ++j)
            {
              vtx[i][j] = CoordType(j+1 == i ? 1 : 0);
            }
          }
        }
      };

      template<int dim_>
      struct RefCellVertexer< Shape::Hypercube<dim_> >
      {
        template<typename VertexSet_>
        static void fill(VertexSet_& vtx)
        {
          typedef typename VertexSet_::CoordType CoordType;
          // loop over all vertices
          for(Index i(0); i < Index(1 << dim_); ++i)
          {
            // loop over all coords
            for(Index j(0); j < Index(dim_); ++j)
            {
              vtx[i][j] = CoordType((((i >> j) & 1) << 1) - 1);
            }
          }
        }
      };
    }
    /// \endcond
  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_REFERENCE_FACTORY_HPP
