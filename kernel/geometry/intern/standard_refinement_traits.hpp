#pragma once
#ifndef KERNEL_GEOMETRY_INTERN_STANDARD_REFINEMENT_TRAITS_HPP
#define KERNEL_GEOMETRY_INTERN_STANDARD_REFINEMENT_TRAITS_HPP 1

// includes, FEAST
#include <kernel/geometry/shape.hpp>

namespace FEAST
{
  namespace Geometry
  {
    namespace Intern
    {
      /**
       * \brief Standard Refinement traits class template
       *
       * This class template specifies how many \e interiour faces of dimension \p face_dim_ are generated
       * upon refinement of a cell of a specified shape type.
       *
       * \tparam Shape_
       * The shape tag class of the cell that is to be refined.
       *
       * \tparam face_dim_
       * The dimension of the faces whose count is to be determined. Must be 0 <= \p face_dim_ <=  \p Shape_::dimension.
       *
       * \author Peter Zajac.
       */
      template<
        typename Shape_,
        int face_dim_>
  #ifndef DOXYGEN
      struct StandardRefinementTraits;
  #else
      struct StandardRefinementTraits
      {
        /// Shape type
        typedef Shape_ ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = ShapeType::dimension,

          /// face dimension
          face_dim = face_dim_,

          /// Number of faces generated upon refinement; depends on Shape_
          count = ...
        };
      };
  #endif // DOXYGEN

      /// \cond internal
      /**
       * \brief StandardRefinementTraits implementation for Vertex shape.
       *
       * \author Peter Zajac
       */
      template<int face_dim_>
      struct StandardRefinementTraits<Shape::Vertex, face_dim_>
      {
        // validate dimensions
        static_assert(face_dim_ == 0, "invalid face dimension");

        /// Shape type
        typedef Shape::Vertex ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension (=0)
          cell_dim = ShapeType::dimension, // = 0

          /// face dimension (=0)
          face_dim = face_dim_, // = 0

          /// Number of faces generated upon refinement
          count = 1
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<Vertex," + stringify(face_dim_) + ">";
        }
      }; // struct StandardRefinementTraits<...,Vertex,...>

      /**
       * \brief StandardRefinementTraits implementation for Hypercube<...> shape.
       *
       * \author Peter Zajac
       */
      template<
        int cell_dim_,
        int face_dim_>
      struct StandardRefinementTraits<Shape::Hypercube<cell_dim_>, face_dim_>
      {
        // validate dimensions
        static_assert(face_dim_ >= 0, "invalid face dimension");
        static_assert(cell_dim_ >= face_dim_, "invalid cell dimension");

        /// Shape type
        typedef Shape::Hypercube<cell_dim_> ShapeType;

        /// Face type
        typedef typename Shape::FaceTraits<ShapeType, face_dim_>::ShapeType FaceType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = cell_dim_,

          /// face dimension
          face_dim = face_dim_,

          /**
           * Number of faces generated upon refinement
           *
           * The total number of inner <em>m</em>-faces generated upon refinement of a single <em>n</em>-hypercubic
           * cell is equal to the number of <em>(n-m)</em>-faces of the cell.
           */
          count = Shape::FaceTraits<ShapeType, cell_dim - face_dim>::count
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + "," + stringify(face_dim_) + ">";
        }
      }; // struct StandardRefinementTraits<Hypercube<...>,...>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<1> shape.
       *
       * \author Constantin Christof
       */
      template<
        int face_dim_>
      struct StandardRefinementTraits<Shape::Simplex<1>, face_dim_>
      {
        // validate dimensions
        static_assert(face_dim_ >= 0, "invalid face dimension");
        static_assert(1 >= face_dim_, "invalid cell dimension");

        /// Shape type
        typedef Shape::Simplex<1> ShapeType;

        /// Face type
        typedef typename Shape::FaceTraits<ShapeType, face_dim_>::ShapeType FaceType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 1,

          /// face dimension
          face_dim = face_dim_,

          /**
           * Number of faces generated upon refinement
           */
          count = Shape::FaceTraits<ShapeType, cell_dim - face_dim>::count
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + "," + stringify(face_dim_) + ">";
        }
      }; // struct StandardRefinementTraits<Simplex<1>,...>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<2> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<2>, 0>
      {

        /// Shape type
        typedef Shape::Simplex<2> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 2,

          /// face dimension
          face_dim = 0,

          /**
           * Number of faces generated upon refinement
           */
          count = 0
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",0>";
        }
      }; // struct StandardRefinementTraits<Simplex<2>,0>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<2> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<2>, 1>
      {

        /// Shape type
        typedef Shape::Simplex<2> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 2,

          /// face dimension
          face_dim = 1,

          /**
           * Number of faces generated upon refinement
           */
          count = 3
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",1>";
        }
      }; // struct StandardRefinementTraits<Simplex<2>,1>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<2> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<2>, 2>
      {

        /// Shape type
        typedef Shape::Simplex<2> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 2,

          /// face dimension
          face_dim = 2,

          /**
           * Number of faces generated upon refinement
           */
          count = 4
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",2>";
        }
      }; // struct StandardRefinementTraits<Simplex<2>,2>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<3> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<3>, 0>
      {

        /// Shape type
        typedef Shape::Simplex<3> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 3,

          /// face dimension
          face_dim = 0,

          /**
           * Number of faces generated upon refinement
           */
          count = 1
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",0>";
        }
      }; // struct StandardRefinementTraits<Simplex<3>,0>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<3> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<3>, 1>
      {

        /// Shape type
        typedef Shape::Simplex<3> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 3,

          /// face dimension
          face_dim = 1,

          /**
           * Number of faces generated upon refinement
           */
          count = 6
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",1>";
        }
      }; // struct StandardRefinementTraits<Simplex<3>,1>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<3> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<3>, 2>
      {

        /// Shape type
        typedef Shape::Simplex<3> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 3,

          /// face dimension
          face_dim = 2,

          /**
           * Number of faces generated upon refinement
           */
          count = 16
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",2>";
        }
      }; // struct StandardRefinementTraits<Simplex<3>,2>

      /**
       * \brief StandardRefinementTraits implementation for Simplex<3> shape.
       *
       * \author Constantin Christof
       */
      template<>
      struct StandardRefinementTraits<Shape::Simplex<3>, 3>
      {

        /// Shape type
        typedef Shape::Simplex<3> ShapeType;

        /// dummy enumeration
        enum
        {
          /// cell dimension
          cell_dim = 3,

          /// face dimension
          face_dim = 3,

          /**
           * Number of faces generated upon refinement
           */
          count = 12
        };

        /// \brief Returns the name of the class.
        static inline String name()
        {
          return "StandardRefinementTraits<" + ShapeType::name() + ",3>";
        }
      }; // struct StandardRefinementTraits<Simplex<3>,3>

      /// \endcond
    } // namespace Intern
  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_INTERN_STANDARD_REFINEMENT_TRAITS_HPP
