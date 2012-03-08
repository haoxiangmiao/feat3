#pragma once
#ifndef KERNEL_GEOMETRY_EXPORT_VTK_HPP
#define KERNEL_GEOMETRY_EXPORT_VTK_HPP 1

// includes, FEAST
#include <kernel/geometry/shape.hpp>

// includes, STL
#include <fstream>

namespace FEAST
{
  namespace Geometry
  {
    /// \cond internal
    namespace Intern
    {
      template<typename Shape_>
      struct VTKHelper;

      template<>
      struct VTKHelper< Shape::Simplex<1> >
      {
        enum
        {
          type = 3 // VTK_LINE
        };
        static inline int map(int i)
        {
          return i;
        }
      };

      template<>
      struct VTKHelper< Shape::Simplex<2> >
      {
        enum
        {
          type = 5 // VTK_TRIANGLE
        };
        static inline int map(int i)
        {
          return i;
        }
      };

      template<>
      struct VTKHelper< Shape::Simplex<3> >
      {
        enum
        {
          type = 10 // VTK_TETRA
        };
        static inline int map(int i)
        {
          return i;
        }
      };

      template<>
      struct VTKHelper< Shape::Hypercube<1> >
      {
        enum
        {
          type = 3 // VTK_LINE
        };
        static inline int map(int i)
        {
          return i;
        }
      };

      template<>
      struct VTKHelper< Shape::Hypercube<2> >
      {
        enum
        {
          type = 9 // VTK_QUAD
        };
        static inline int map(int i)
        {
          static int v[] = {0, 1, 3, 2};
          return v[i];
        }
      };

      template<>
      struct VTKHelper< Shape::Hypercube<3> >
      {
        enum
        {
          type = 12 // VTK_HEXAHEDRON
        };
        static inline int map(int i)
        {
          static int v[] = {0, 1, 3, 2, 4, 5, 7, 6};
          return v[i];
        }
      };
    } // namespace Intern
    /// \endcond

    /**
     * \brief Provisional VTK exporter class template
     *
     * This class template is a provisional VTK exporter which will be replaced by a more mature one later.
     * \author Peter Zajac
     */
    template<typename Mesh_>
    class ExportVTK
    {
    public:
      /// mesh type
      typedef Mesh_ MeshType;
      /// vertex set type
      typedef typename MeshType::VertexSetType VertexSetType;

    protected:
      /// reference to mesh to be exported
      const MeshType& _mesh;

      /// \cond internal
      void write_vtx_1(std::ofstream& ofs, const VertexSetType& vtx) const
      {
        CONTEXT("ExportVTK::write_vtx_1()");
        Index n = vtx.get_num_vertices();
        for(Index i(0); i < n; ++i)
        {
          ofs << vtx[i][0] << " 0.0 0.0" << std::endl;
        }
      }

      void write_vtx_2(std::ofstream& ofs, const VertexSetType& vtx) const
      {
        CONTEXT("ExportVTK::write_vtx_2()");
        Index n = vtx.get_num_vertices();
        for(Index i(0); i < n; ++i)
        {
          ofs << vtx[i][0] << " " << vtx[i][1] << " 0.0" << std::endl;
        }
      }

      void write_vtx_3(std::ofstream& ofs, const VertexSetType& vtx) const
      {
        CONTEXT("ExportVTK::write_vtx_3()");
        Index n = vtx.get_num_vertices();
        for(Index i(0); i < n; ++i)
        {
          ofs << vtx[i][0] << " " << vtx[i][1] << " " << vtx[i][2] << std::endl;
        }
      }
      /// \endcond

    public:
      explicit ExportVTK(const MeshType& mesh) :
        _mesh(mesh)
      {
        CONTEXT("ExportVTK::ExportVTK()");
      }

      virtual ~ExportVTK()
      {
        CONTEXT("ExportVTK::~ExportVTK()");
      }

      void write(const String& filename)
      {
        CONTEXT("ExportVTK::write()");
        using std::endl;

        // try to open a file
        std::ofstream ofs(filename.c_str());
        ASSERT_(ofs.is_open());
        ASSERT_(ofs.good());

        // write VTK header
        ofs << "# vtk DataFile Version 2.0" << endl;
        ofs << "Generated by FEAST v" << version_major << "." << version_minor << "." << version_patch << endl;
        ofs << "ASCII" << endl;
        ofs << "DATASET UNSTRUCTURED_GRID" << endl;
        ofs << "POINTS " << _mesh.get_num_entities(0) << " double" << endl;

        // write vertex coordinates
        const typename MeshType::VertexSetType& vtx = _mesh.get_vertex_set();
        Index num_verts = vtx.get_num_vertices();
        switch(vtx.get_num_coords())
        {
        case 1:
          for(Index i(0); i < num_verts; ++i)
          {
            ofs << vtx[i][0] <<  " 0.0 0.0" << std::endl;
          }
          break;

        case 2:
          for(Index i(0); i < num_verts; ++i)
          {
            ofs << vtx[i][0] << " " << vtx[i][1] << " 0.0" << std::endl;
          }
          break;

        case 3:
          for(Index i(0); i < num_verts; ++i)
          {
            ofs << vtx[i][0] << " " << vtx[i][1] << " " << vtx[i][2] << std::endl;
          }
          break;

        default:
          ASSERT(false, "invalid coordinate count");
        }

        typedef Intern::VTKHelper<typename MeshType::ShapeType> VTKHelperType;

        // fetch index set
        const typename MeshType::template IndexSet<MeshType::shape_dim,0>::Type& idx =
          _mesh.template get_index_set<MeshType::shape_dim, 0>();
        Index num_cells = _mesh.get_num_entities(MeshType::shape_dim);
        int num_idx = idx.get_num_indices();

        // write cells
        ofs << "CELLS " << num_cells << " " << ((num_idx+1)*num_cells) << endl;
        for(Index i(0); i < num_cells; ++i)
        {
          ofs << num_idx;
          for(int j(0); j < num_idx; ++j)
          {
            ofs << " " << idx[i][VTKHelperType::map(j)];
          }
          ofs << endl;
        }

        // write cell types
        ofs << "CELL_TYPES " << num_cells << endl;
        for(Index i(0); i < num_cells; ++i)
        {
          ofs << VTKHelperType::type << endl;
        }

        // okay
        ofs.close();
      }
    }; // class ExportVTK
  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_EXPORT_VTK_HPP
