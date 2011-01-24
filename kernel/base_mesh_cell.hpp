#pragma once
#ifndef KERNEL_BASE_MESH_CELL_HPP
#define KERNEL_BASE_MESH_CELL_HPP 1

// includes, system
#include <iostream> // for std::ostream
#include <cassert>  // for assert()
#include <vector>   // for std::vector

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/base_mesh_cell_data.hpp>
#include <kernel/base_mesh_cell_subdivision.hpp>
#include <kernel/base_mesh_vertex.hpp>

namespace FEAST
{
  namespace BaseMesh
  {

    /// stores the fixed numbering schemes
    struct Numbering
    {
      /// indices of start and end vertex of the four edges in a quad
      static unsigned char quad_edge_vertices[][2];

      /**
      * \brief index of the next vertex in a quad w.r.t. ccw ordering ([1,3,0,2])
      *
      * Due to this numbering scheme of the quad
      *   2---1---3
      *   |       |
      *   2       3
      *   |       |
      *   0---0---1
      * the respectively next vertex w.r.t. ccw ordering is given by the mapping [1,3,0,2]
      */
      static unsigned char quad_next_vertex_ccw[];

      /**
      * \brief  index of the previous vertex in a quad w.r.t. ccw ordering ([2,0,3,1])
      *
      * See the detailed description of #quad_next_vertex_ccw.
      */
      static unsigned char quad_previous_vertex_ccw[];

      /**
      * \brief  index of the next edge in a quad w.r.t. ccw ordering ([3,2,0,1])
      *
      * See the detailed description of #quad_next_vertex_ccw.
      */
      static unsigned char quad_next_edge_ccw[];

      /**
      * \brief  index of the previous edge in a quad w.r.t. ccw ordering ([2,3,1,0])
      *
      * See the detailed description of #quad_next_vertex_ccw.
      */
      static unsigned char quad_previous_edge_ccw[];

      /// indices of start and end vertex of the twelve edges in a hexa
      static unsigned char hexa_edge_vertices[][2];

      /// indices of the four vertices of the six faces in a hexa
      static unsigned char hexa_face_vertices[][4];

      /// indices of the four edges of the six faces in a hexa
      static unsigned char hexa_face_edges[][4];

      /**
      * \brief quad-to-quad mappings of vertices
      *
      * On the one hand a quad face in a 3D cell has a certain numbering w.r.t. the numbering of the 3D cell, i.e.
      * the numbering of the 3D cell determines which are first, second, ... vertex/edge of the face. On the
      * other hand the quad is stored as a 2D cell with a certain numbering. These two numberings usually do not
      * coincide. There are eight possibilities how the two numberings can be related. The first four have the same
      * orientation as the reference numeration, i.e. they are only rotated, the last four have opposite orientation.
      * The eight possibilites lead to eight different mappings of vertices and edges, resp., which we denote by, e.g.,
      *   V1:2031
      *   (relation 2: vertex 0 (of the reference numbering) equals vert. 2 in the different numbering,
      *                vert. 1 equals vert. 0, ...)
      *   E7:3210
      *   (relation 7: edge 0 (of the reference numbering) equals edge 3 in the different numbering, ...).
      *
      * same orientation as reference                          opposite orientation
      *
      * relation 0   relation 1   relation 2   relation 3      relation 4   relation 5   relation 6   relation 7
      *
      * 2---1---3    0---2---2    3---3---1    1---0---0       1---3---3    3---1---2    0---0---1    2---2---0
      * |       |    |       |    |       |    |       |       |       |    |       |    |       |    |       |
      * 2       3    0       1    1       0    3       2       0       1    3       2    2       3    1       0
      * |       |    |       |    |       |    |       |       |       |    |       |    |       |    |       |
      * 0---0---1    1---3---3    2---2---0    3---1---2       0---2---2    1---0---0    2---1---3    3---3---1
      *  V0:0123      V1:1302      V2:2031      V3:3210         V4:0213      V5:1032      V6:2301      V7:3120
      *  E0:0123      E1:3201      E2:2310      E3:1032         E4:2301      E5:0132      E6:1023      E7:3210
      * (reference)
      *
      * When vertex i of the given numbering equals vertex 0 in the reference numbering, then we have either
      * relation i or relation i+4 (depending on the orientation).
      */
// COMMENT_HILMAR: Sieht hier jemand eine Moeglichkeit, dieses Mapping ohne explizites Abspeichern zu erhalten? Also
// durch Berechnung in Abhaengigkeit vom Knoten- bzw. Kantenindex? Ich seh's nicht...
// COMMENT_HILMAR: Mit dem Standard-ccw-Mapping waere das alles einfacher... da koennte man mit sowas �hnlichem wie
// (ivertex + ishift)%4 arbeiten.
      static unsigned char quad_to_quad_mappings_vertices[][4];

      /**
      * \brief quad-to-quad mappings of edges
      *
      * See the description of #quad_to_quad_mappings_vertices.
      */
      static unsigned char quad_to_quad_mappings_edges[][4];
    };

    // indices of start and end vertex of the four edges in a quad
    unsigned char Numbering::quad_edge_vertices[4][2] = {{0,1}, {2,3}, {0,2}, {1,3}};

    // index of the next vertex in a quad w.r.t. ccw ordering ([1,3,0,2])
    unsigned char Numbering::quad_next_vertex_ccw[4] = {1,3,0,2};

    // index of the previous vertex in a quad w.r.t. ccw ordering ([2,0,3,1])
    unsigned char Numbering::quad_previous_vertex_ccw[4] = {2,0,3,1};

    // index of the next edge in a quad w.r.t. ccw ordering ([3,2,0,1])
    unsigned char Numbering::quad_next_edge_ccw[4] = {3,2,0,1};

    // index of the previous edge in a quad w.r.t. ccw ordering ([2,3,1,0])
    unsigned char Numbering::quad_previous_edge_ccw[4] = {2,3,1,0};

    // indices of start and end vertex of the twelve edges in a hexa
    unsigned char Numbering::hexa_edge_vertices[12][2]
      = {{0,1}, {2,3}, {4,5}, {6,7},   {0,2}, {1,3}, {4,6}, {5,7},   {0,4}, {1,5}, {2,6}, {3,7}};

    // indices of the four vertices of the six faces in a hexa
    unsigned char Numbering::hexa_face_vertices[6][4]
      = {{0,1,2,3}, {4,5,6,7}, {0,1,4,5}, {2,3,6,7}, {0,2,4,6}, {1,3,5,7}};

    // indices of the four edges of the six faces in a hexa
    unsigned char Numbering::hexa_face_edges[6][4]
      = {{0,1,4,5}, {2,3,6,7}, {0,2,8,9}, {1,3,10,11}, {4,6,8,10}, {5,7,9,11}};

    // quad-to-quad mappings for vertices
    // V0:0123    V1:1302    V2:2031    V3:3210    V4:0213    V5:1032    V6:2301    V7:3120
    unsigned char Numbering::quad_to_quad_mappings_vertices[8][4] =
      {{0,1,2,3}, {1,3,0,2}, {2,0,3,1}, {3,2,1,0}, {0,2,1,3}, {1,0,3,2}, {2,3,0,1}, {3,1,2,0}};

    // quad-to-quad mappings for edges
    // E0:0123    E1:3201    E2:2310    E3:1032    E4:2301    E5:0132    E6:1023     E7:3210
    unsigned char Numbering::quad_to_quad_mappings_edges[8][4] =
      {{0,1,2,3}, {3,2,0,1}, {2,3,1,0}, {1,0,3,2}, {2,3,0,1}, {0,1,3,2}, {1,0,2,3}, {3,2,1,0}};



    /**
    * \brief dimension specific function interface for cell class (empty definition to be specialised by cell dimension)
    *
    * While vertices are common to all cell dimensions, edges and faces do not exist in all dimensions. So,
    * getter and setter routines for these entities cannot be provided in the general Cell class.
    * The alternative would be to specialise the complete Cell class by cell dimension.
    * Why do we need this interface already in the Cell class? Since we must be able to do something like
    *   face(iface)->child(0)->edge(1)
    * where child(0) is of type Cell<2, ...> (and not, e.g., of type Quad<...>!).
    */
    template<
      unsigned char cell_dim_,
      unsigned char space_dim_,
      unsigned char world_dim_>
    class CellInterface
    {
    };


    /// function interface for 1D cells
    template<
      unsigned char space_dim_,
      unsigned char world_dim_>
    class CellInterface<1, space_dim_, world_dim_>
    {
    public:
      /// returns number of vertices
      virtual unsigned char num_vertices() const = 0;

      /// returns vertex at given index
// TODO: pointer oder referenz auf pointer zurueckgeben? (ueberall sonst dann ggfls. auch anpassen)
      virtual Vertex<world_dim_>* vertex(unsigned char const index) const = 0;
    };


    /// function interface for 2D cells
    template<
      unsigned char space_dim_,
      unsigned char world_dim_>
    class CellInterface<2, space_dim_, world_dim_>
    {
    public:
      /// returns number of vertices
      virtual unsigned char num_vertices() const = 0;

      /// returns vertex at given index
      virtual Vertex<world_dim_>* vertex(unsigned char const index) const = 0;

      /// returns number of edges
      virtual unsigned char num_edges() const = 0;

      /// returns edge at given index
      virtual Cell<1, space_dim_, world_dim_>* edge(unsigned char const index) const = 0;

      /// returns next vertex of vertex with given index w.r.t. to ccw ordering
      virtual Vertex<world_dim_>* next_vertex_ccw(unsigned char const index) const = 0;

      /// returns previous vertex of vertex with given index w.r.t. to ccw ordering
      virtual Vertex<world_dim_>* previous_vertex_ccw(unsigned char const index) const = 0;

      /// returns next edge of edge with given index w.r.t. to ccw ordering
      virtual Cell<1, space_dim_, world_dim_>* next_edge_ccw(unsigned char const index) const = 0;

      /// returns previous edge of edge with given index w.r.t. to ccw ordering
      virtual Cell<1, space_dim_, world_dim_>* previous_edge_ccw(unsigned char const index) const = 0;
    };



    /// function interface for 3D cells
    template<
      unsigned char space_dim_,
      unsigned char world_dim_>
    class CellInterface<3, space_dim_, world_dim_>
    {
    public:
      /// returns number of vertices
      virtual unsigned char num_vertices() const = 0;

      /// returns vertex at given index
      virtual Vertex<world_dim_>* vertex(unsigned char const index) const = 0;

      /// returns number of edges
      virtual unsigned char num_edges() const = 0;

      /// returns edge at given index
      virtual Cell<1, space_dim_, world_dim_>* edge(unsigned char const index) const = 0;

      /// returns number of faces
      virtual unsigned char num_faces() const = 0;

      /// returns face at given index
      virtual Cell<2, space_dim_, world_dim_>* face(unsigned char const index) const = 0;
    };





    /**
    * \brief general base mesh cell class containing parent/child information
    *
    * Used for cells of maximum dimension (e.g., quads in a 2D world), but also for those of lower dimension
    * (e.g., edges in a 2D world). For the latter, however, the CellData class is empty such that no unnecessary
    * neighbourhood information etc. included.
    *
    * \author Hilmar Wobker
    * \author Dominik Goeddeke
    * \author Peter Zajac
    */
    template<
      unsigned char cell_dim_,
      unsigned char space_dim_,
      unsigned char world_dim_>
    class Cell
      : public CellData<cell_dim_, space_dim_, world_dim_>,
    // COMMENT_HILMAR: muss "virtual public" sein, falls z.B. Quad direkt den CellData-Konstruktor aufrufen muss
        public CellInterface<cell_dim_, space_dim_, world_dim_>
    {
    private:

      /// parent of this cell
      Cell* _parent;
      /// number of children (when zero, then the cell is called active)
      unsigned char _num_children;
      /// array of children of this cell
      Cell** _children;




// COMMENT_HILMAR: Das ist 2D-spezifischer Code! Der muss woanders hin!
// Wohin?
// Routinen:
//    void update_edge_neighbours()
//       --> Cell2D spezifisch, erfordert neighbour-Zugriff
//
//    void set_edge_neighbours(
//      Edge* shared_edge,
//      Cell* neighbour)
//       --> Cell2D spezifisch, erfordert neighbour-Zugriff
//
//    void get_cells_along_edge_ccw(vector<Cell*>, Edge shared_edge)
//       --> Cell2D spezifisch, kein neighbour-Zugriff
//
//    void get_children_at_edge_ccw(
//      unsigned char num_cells_at_edge&,
//      std::vector<Cell*>& cell_children_at_edge,
//      std::vector<Edge*>& edge_children_at_edge,
//      Edge* edge)
//       --> Cell2D spezifisch, kein neighbour-Zugriff

//    void get_edge_children_ccw(Edge* edge, Edge* children_ordered[])
//       --> Cell2D-spezifisch, kein neighbour-Zugriff


//    void update_edge_neighbours()
//    {
//      for(int iedge(0) ; iedge < num_edges() ; ++iedge)
//      {
//        if (num_edge_neighbours(iedge) > 0)
//        {
//          BaseMeshItem1D<2>* shared_edge = edge(iedge);
//          // get the parent cell of the neighbour cells that has the same refinement level as this cell
//          // TODO: get rid of dynamic_cast!
//          BaseMeshCell* neighbour =
//            dynamic_cast<BaseMeshCell*>(edge_neighbour(iedge, 0)->parent(refinement_level()));
//
//          set_edge_neighbours(shared_edge, neighbour);
//        }
//      }
//    }
//
//    void set_edge_neighbours(
//      BaseMeshItem1D<2>* shared_edge,
//      BaseMeshCell* neighbour)
//    {
//      if (active() && neighbour->active())
//      {
//        // none of the two cells has children
//        // set neighbourhood directly
//        unsigned char local_index = get_local_edge_index(shared_edge);
//        edge_neighbours(local_index).clear();
//        edge_neighbours(local_index).push_back(neighbour);
//        local_index = neighbour->get_local_edge_index(shared_edge);
//        neighbour->edge_neighbours(local_index).clear();
//        neighbour->edge_neighbours(local_index).push_back(this);
//      }
//      else if (active())
//      {
//        // this cell has no children, the neighbour has children
//        std::vector<BaseMeshCell*> cells_at_edge;
//        // Set in all neighbour cells along shared edge this cell as neighbour. While doing this, store all these
//        // cells in the vector cells_at_edge. This is done in ccw-fashion w.r.t. the orientation of the neighbour cell.
//        neighbour->set_edge_neighbour(shared_edge, this, cells_at_edge);
//        // get local index of the edge in this cell
//        unsigned char local_index = get_local_edge_index(shared_edge);
//        edge_neighbours(local_index).clear();
//        // Now store all neighbour cells (stored in cells_at_edge) along shared edge as neighbours. Since they have
//        // been stored in ccw-fashion (w.r.t. the neigbour cell), the order has to be reversed.
//        for(int i(cells_at_edge.size()) ; i >= 0 ; --i)
//        {
//          edge_neighbours(local_index).push_back(cells_at_edge[i]);
//        }
//      }
//      else if (neighbour->active())
//      {
//        // this cell has children, the neighbour has no children
//      }
//      else
//      {
//        // both cells have children
//      }
//    }


//    inline void set_edge_neighbour(
//      std::vector<BaseMeshCell*>& cells_at_edge,
//      BaseMeshItem1D<2>* shared_edge,
//      BaseMeshCell* neighbour)
//    {
//
//    }



//    inline void get_cells_along_edge_ccw(
//      std::vector<Cell*>& cells_at_edge,
//      BaseMeshItem1D<2>* shared_edge)
//    {
//      assert(contains(shared_edge));
//      if (active())
//      {
//        cells_at_edge.push_back(this);
//      }
//      else
//      {
////        BaseMeshItem1D<2>* edge_children_ordered[2];
////        get_edge_children_ccw(cshared_edge, edge_children_ordered);
//
//
//        int num_cells_at_edge;
//        std::vector<BaseMeshCell*> cell_children_at_edge;
//        std::vector<BaseMeshItem1D*> edge_children_at_edge;
//
//        get_children_at_edge_ccw(num_cells_at_edge, cell_children_at_edge, edge_children_at_edge, shared_edge);
//        ???.get_cells_along_edge_ccw(edge_children_ordered[0], cells_at_edge);
//        ???.get_cells_along_edge_ccw(edge_children_ordered[1], cells_at_edge);
//      }
//    }


//    // collects the direct cell and edge children of the cell along the given edge
//    // one-to-one correspondence:
//    // The local edge of i-th cell child, that is a child of the given edge, is stored at the i-th
//    // position of edge_children_at_edge[]. I.e., in the ASCII art, the two vectors are:
//    //   cell_children_at_edge = [c0, c1]
//    //   edge_children_at_edge = [e0, e1]
//    // (and not, e.g., edge_children_at_edge = [e1, e0]).
//    // ---------
//    //  e0 |
//    //     | c0
//    // e   -----
//    //     | c1
//    //  e1 |
//    // --------
//    // In the special case of an edge neighbour that only shares a vertex with the edge
//    // -----------
//    //  e0 |c0 / |
//    //     | /   |
//    // e   v   c1|
//    //     | \   |
//    //  e1 |c2 \ |
//    // -----------
//    // a nullptr is stored in the edge array, i.e.
//    //   cell_children_at_edge = [c0, c1, c2]
//    //   edge_children_at_edge = [e0,  0, e1]
//    void get_children_at_edge_ccw(
//      short num_cells_at_edge&,
//      std::vector<BaseMeshCell*>& cell_children_at_edge,
//      std::vector<BaseMeshItem1D*>& edge_children_at_edge,
//      BaseMeshItem1D<2>* edge)
//    {
//      // may only be called when the cell actually has children
//      assert(!active());
//
//    }

//    void get_edge_children_ccw(BaseMeshItem1D<2>* edge, BaseMeshItem1D<2>* children_ordered[])
//    {
//      // assumption: edge->child(i) is incident with edge->vertex(i), i = 0,1
//      assert(!edge->active());
//      if (edge_orientation_ccw(edge))
//      {
//        children_ordered[0] = edge->child(0);
//        children_ordered[1] = edge->child(1);
//      }
//      else
//      {
//        children_ordered[0] = edge->child(1);
//        children_ordered[1] = edge->child(0);
//      }
//    }
//
//    inline bool edge_orientation_ccw(BaseMeshItem1D<2>* edge)
//    {
//      unsigned char local_index = get_local_edge_index(edge);
//      return (edge->vertex(0) == vertex(local_index));
//    }
//
//    inline bool edge_orientation_ccw(unsigned char iedge)
//    {
//      return (edge(iedge)->vertex(0) == vertex(iedge));
//    }
    protected:

      /**
      * \brief sets number of children and allocates the _children array
      *
      * This function may only be called, when new children are to be created. "Old" children have to be cleared before via
      * unset_children(...).
      * Rationale:
      * - It won't happen often that a cell creates and removes children over and over again.
      * - A situation like "I already have 4 children, and now I want to have 2 more" will not occur.
      * - Allocating/deallocating an array of less than 10 pointers does not really harm.
      */
      inline void _set_num_children(unsigned char const num)
      {
        // this function must not be called when there *are* already children
        assert(_children == nullptr && _num_children == 0);
        // and it must not be called to unset children (use unset_children() for that)
        assert(num > 0);
        _num_children = num;
        _children = new Cell*[_num_children];
        // nullify the new pointer array
        for (unsigned char i(0) ; i < _num_children ; ++i)
        {
          _children[i] = nullptr;
        }
      }

      /**
      * \brief sets child at given index
      *
      * The cell-type does not have to be the same (quads can have tris as children), so a general BaseMeshItem2D is
      * is passed to the function.
      */
      inline void _set_child(
        unsigned char const index,
        Cell* e)
      {
        assert(index < num_children());
        // ensure that this function is not used to unset children (use unset_children() for that)
        assert(e != nullptr);
        _children[index] = e;
      }

      /// unsets all children (which should not be done via set_child(i, nullptr))
      inline void _unset_children()
      {
        // COMMENT_HILMAR: For the time being, we enforce that this function may only be called when there actually *are*
        // valid children. (To discover eventual errors in apply/unapply actions.)
        assert(num_children() > 0);
        for (unsigned char i(0) ; i < num_children() ; ++i)
        {
          assert(_children[i] != nullptr);
          _children[i] = nullptr;
        }
        delete [] _children;
        _children = nullptr;
        _num_children = 0;
      }


    public:

      Cell()
        : _parent(nullptr),
          _num_children(0),
          _children(nullptr)
      {
        assert(world_dim_ >= space_dim_);
        assert(space_dim_ >= cell_dim_);
      }


      virtual ~Cell()
      {
        _parent = nullptr;
      }


      /// returns parent
      inline Cell* parent() const
      {
        return _parent;
      }


      /// sets parent
      inline void set_parent(Cell* const par)
      {
        _parent = par;
      }


      /// returns number of children
      inline unsigned char num_children() const
      {
        return _num_children;
      }


      /// returns child at given index
      inline Cell* child(unsigned char index) const
      {
        assert(index < num_children());
        return _children[index];
      }


      inline bool active() const
      {
        return (num_children() == 0);
      }


      virtual void subdivide(SubdivisionData<cell_dim_, space_dim_, world_dim_>& subdiv_data) = 0;


      virtual void print(std::ostream& stream) = 0;


      virtual void validate() const = 0;


      inline void print_history(std::ostream& stream)
      {
        stream << "[parent: ";
        if (parent() != nullptr)
        {
          parent()->print_index(stream);
        }
        else
        {
          stream << "-";
        }
        if (num_children() > 0)
        {
          stream << ", children: ";
          child(0)->print_index(stream);
          for(int i(1) ; i < num_children() ; ++i)
          {
            std:: cout << ", ";
            child(i)->print_index(stream);
          }
        }
        else
        {
          stream << ", children: -";
        }
        stream << "]";
      }
    };
  } // namespace BaseMesh
} // namespace FEAST

#endif // #define KERNEL_BASE_MESH_CELL_DATA_HPP
