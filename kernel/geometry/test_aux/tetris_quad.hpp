#pragma once
#ifndef KERNEL_GEOMETRY_TEST_AUX_TETRIS_QUAD_HPP
#define KERNEL_GEOMETRY_TEST_AUX_TETRIS_QUAD_HPP 1

// includes, FEAST
#include <kernel/geometry/conformal_mesh.hpp>
#include <kernel/geometry/test_aux/copy_comp_set.hpp>

namespace FEAST
{
  namespace Geometry
  {
    /// \cond internal
    namespace TestAux
    {
      typedef ConformalMesh< ConformalMeshPolicy< Shape::Quadrilateral > > QuadMesh;
      typedef ConformalSubMesh< ConformalSubMeshPolicy< Shape::Quadrilateral > > QuadSubMesh;

      /**
       * \brief Creates the 2D tetris mesh.
       *
       * \verbatim
         7----L----8----M----9
         |         |         |
         I   Q_2   J   Q_3   K
         |         |         |
         3----F----4----G----5----H----6
                   |         |         |
                   C   Q_0   D   Q_1   E
                   |         |         |
                   0----A----1----B----2 \endverbatim
       * \note
       * - All horizontal edges are pointing from left to right.
       * - All vertical edges are pointing from down to up.
       * - For all quads
       *   - the lower left vertex is the first local vertex
       *   - the lower right vertex is the second local vertex
       *
       * \author Peter Zajac
       */
      QuadMesh* create_tetris_mesh_2d()
      {
        Index num_entities[] =
        {
          10, // vertices (0, ..., 9)
          13, // edges (A, ..., M)
          4   // quads (Q_0,..., Q_3)
        };

        // create mesh
        QuadMesh* mesh = new QuadMesh(num_entities);

        // set up vertex coordinates array
        static const Real vtx[10*2] =
        {
          1.0, 0.0,
          2.0, 0.0,
          3.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          2.0, 1.0,
          3.0, 1.0,
          0.0, 2.0,
          1.0, 2.0,
          2.0, 2.0
        };
        copy_vtx(mesh->get_vertex_set(), vtx);

        // set up vertices-at-edge array
        static const Index v_e[13*2] =
        {
          0, 1,
          1, 2,
          0, 4,
          1, 5,
          2, 6,
          3, 4,
          4, 5,
          5, 6,
          3, 7,
          4, 8,
          5, 9,
          7, 8,
          8, 9
        };
        copy_idx(mesh->get_index_set<1,0>(), v_e);

        // set up vertices-at-quad array
        static const Index v_q[4*4] =
        {
          0, 1, 4, 5,
          1, 2, 5, 6,
          3, 4, 7, 8,
          4, 5, 8, 9
        };
        copy_idx(mesh->get_index_set<2,0>(), v_q);

        // set up edges-at-quad array
        static const Index e_q[4*4] =
        {
          0,  6,  2,  3,
          1,  7,  3,  4,
          5, 11,  8,  9,
          6, 12,  9, 10
        };
        copy_idx(mesh->get_index_set<2,1>(), e_q);

        // okay
        return mesh;
      }

      /**
       * \brief Validates the refined 2d tetris mesh.
       *
       * \verbatim
           7----W---21----X----8----Y---22----Z----9
           |         |         |         |         |
          R   Q_10  J'  Q_11  T   Q_14  N'  Q_15  V
           |         |         |         |         |
          18---K'---25---L'---19---O'---26---P'---20
           |         |         |         |         |
           Q   Q_8   I'  Q_9   S   Q_12  M'  Q_13  U
           |         |         |         |         |
           3----K---15----L----4----M---16----N----5----O---17----P----6
                               |         |         |         |         |
                               F   Q_2   B'  Q_3   H   Q_6   F'  Q_7   J
                               |         |         |         |         |
                              12---C'---23---D'---13---G'---24---H'---14
                               |         |         |         |         |
                               E   Q_0   A'  Q_1   G   Q_4   E'  Q_5   I
                               |         |         |         |         |
                               0----A---10----B----1----C---11----D----2 \endverbatim
       *
       * \author Peter Zajac
       */
      void validate_refined_tetris_mesh_2d(const QuadMesh& mesh)
      {
        // validate sizes
        if(mesh.get_num_entities(0) != 27)
          throw String("Vertex count mismatch");
        if(mesh.get_num_entities(1) != 42)
          throw String("Edge count mismatch");
        if(mesh.get_num_entities(2) != 16)
          throw String("Quad count mismatch");

        // check vertex coordinates array
        static const Real vtx[] =
        {
          1.0, 0.0, // coarse mesh vertices (0,...,9)
          2.0, 0.0,
          3.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          2.0, 1.0,
          3.0, 1.0,
          0.0, 2.0,
          1.0, 2.0,
          2.0, 2.0,
          1.5, 0.0, // edge midpoints (10,...,22)
          2.5, 0.0,
          1.0, 0.5,
          2.0, 0.5,
          3.0, 0.5,
          0.5, 1.0,
          1.5, 1.0,
          2.5, 1.0,
          0.0, 1.5,
          1.0, 1.5,
          2.0, 1.5,
          0.5, 2.0,
          1.5, 2.0,
          1.5, 0.5, // quad midpoints (23,...,26)
          2.5, 0.5,
          0.5, 1.5,
          1.5, 1.5
        };
        if(!comp_vtx(mesh.get_vertex_set(), vtx))
          throw String("Vertex coordinate refinement failure");

        // check vertices-at-edge array
        static const Index v_e[] =
        {
          0, 10,
          10, 1,
          1, 11,
          11, 2,
          0, 12,
          12, 4,
          1, 13,
          13, 5,
          2, 14,
          14, 6,
          3, 15,
          15, 4,
          4, 16,
          16, 5,
          5, 17,
          17, 6,
          3, 18,
          18, 7,
          4, 19,
          19, 8,
          5, 20,
          20, 9,
          7, 21,
          21, 8,
          8, 22,
          22, 9,
          10, 23,
          23, 16,
          12, 23,
          23, 13,
          11, 24,
          24, 17,
          13, 24,
          24, 14,
          15, 25,
          25, 21,
          18, 25,
          25, 19,
          16, 26,
          26, 22,
          19, 26,
          26, 20
        };
        if(!comp_idx(mesh.get_index_set<1,0>(), v_e))
          throw String("Vertex-At-Edge index set refinement failure");

        // check vertices-at-quad
        static const Index v_q[] =
        {
          0, 10, 12, 23,
          10, 1, 23, 13,
          12, 23, 4, 16,
          23, 13, 16, 5,
          1, 11, 13, 24,
          11, 2, 24, 14,
          13, 24, 5, 17,
          24, 14, 17, 6,
          3, 15, 18, 25,
          15, 4, 25, 19,
          18, 25, 7, 21,
          25, 19, 21, 8,
          4, 16, 19, 26,
          16, 5, 26, 20,
          19, 26, 8, 22,
          26, 20, 22, 9
        };
        if(!comp_idx(mesh.get_index_set<2,0>(), v_q))
          throw String("Vertex-At-Quad index set refinement failure");

        // check edges-at-quad
        static const Index e_q[] =
        {
          0, 28, 4, 26,
          1, 29, 26, 6,
          28, 12, 5, 27,
          29, 13, 27, 7,
          2, 32, 6, 30,
          3, 33, 30, 8,
          32, 14, 7, 31,
          33, 15, 31, 9,
          10, 36, 16, 34,
          11, 37, 34, 18,
          36, 22, 17, 35,
          37, 23, 35, 19,
          12, 40, 18, 38,
          13, 41, 38, 20,
          40, 24, 19, 39,
          41, 25, 39, 21
        };
        if(!comp_idx(mesh.get_index_set<2,1>(), e_q))
          throw String("Edge-At-Quad index set refinement failure");
      }

      QuadSubMesh* create_tetris_edge_submesh_2d()
      {
        Index num_entities[] =
        {
          4, // vertices
          3, // edges
          0  // quads
        };

        // create mesh
        QuadSubMesh* mesh = new QuadSubMesh(num_entities, 1, 1);

        // set up vertex coordinates array
        Real vtx[] =
        {
          0.0,
          1.0,
          2.0,
          3.0
        };
        copy_vtx(mesh->get_vertex_set(), vtx);

        // set up vertices-at-edge array
        Index v_e[] =
        {
          0, 1,
          1, 2,
          2, 3
        };
        copy_idx(mesh->get_index_set<1,0>(), v_e);

        // set up vertex-target-indices
        Index vti[] =
        {
          1, 5, 4, 8
        };
        copy_trg(mesh->get_target_set<0>(), vti);

        // set up edge-target-indices
        Index eti[] =
        {
          3, 6, 9
        };
        copy_trg(mesh->get_target_set<1>(), eti);

        // okay
        return mesh;
      }

      void validate_refined_tetris_edge_submesh_2d(const QuadSubMesh& mesh)
      {
        // validate sizes
        if(mesh.get_num_entities(0) != 7)
          throw String("Vertex count mismatch");
        if(mesh.get_num_entities(1) != 6)
          throw String("Edge count mismatch");
        if(mesh.get_num_entities(2) != 0)
          throw String("Quad count mismatch");

        // check vertex coordinates array
        Real vtx[] =
        {
          0.0,
          1.0,
          2.0,
          3.0,
          0.5,
          1.5,
          2.5
        };
        if(!comp_vtx(mesh.get_vertex_set(), vtx))
          throw String("Vertex coordinate refinement failure");

        // check vertices-at-edge array
        Index v_e[] =
        {
          0, 4,
          4, 1,
          1, 5,
          5, 2,
          2, 6,
          6, 3
        };
        if(!comp_idx(mesh.get_index_set<1,0>(), v_e))
          throw String("Vertex-At-Edge index set refinement failure");

        // check vertex-target incides
        Index vti[] =
        {
          1, 5, 4, 8, 13, 16, 19
        };
        if(!comp_trg(mesh.get_target_set<0>(), vti))
          throw String("Vertex target set refinement failure");

        // check edge indices
        Index eti[] =
        {
          6, 7, 13, 12, 18, 19
        };
        if(!comp_trg(mesh.get_target_set<1>(), eti))
          throw String("Edge target set refinement failure");
      }

      QuadSubMesh* create_tetris_quad_submesh_2d()
      {
        //  2<---C----4
        //  ^        ||
        //  B   Q_0   F
        //  |         v
        //  3<---G----1
        //  ||        |
        //  E   Q_1   D
        //  v         v
        //  0----A--->5
        //
        // Note:
        // <, >, v and ^ describe the edge orientation

        Index num_entities[] =
        {
          6, // vertices
          7, // edges
          2  // quads
        };

        // create mesh
        QuadSubMesh* mesh = new QuadSubMesh(num_entities, 2);

        // set up vertex coordinates array
        Real vtx[] =
        {
          0.0, 0.0,
          1.0, 1.0,
          0.0, 2.0,
          0.0, 1.0,
          1.0, 2.0,
          1.0, 0.0
        };
        copy_vtx(mesh->get_vertex_set(), vtx);

        // set up vertices-at-edge array
        Index v_e[] =
        {
          0, 5,
          3, 2,
          4, 2,
          1, 5,
          3, 0,
          4, 1,
          1, 3
        };
        copy_idx(mesh->get_index_set<1,0>(), v_e);

        // set up vertices-at-quad array
        Index v_q[] =
        {
          4, 1, 2, 3,
          3, 0, 1, 5
        };
        copy_idx(mesh->get_index_set<2,0>(), v_q);

        // set up edges-at-quad array
        Index e_q[] =
        {
          5, 1, 2, 6,
          4, 3, 6, 0
        };
        copy_idx(mesh->get_index_set<2,1>(), e_q);

        // set up vertex-target indices
        Index vti[] =
        {
          0, 5, 8, 4, 9, 1
        };
        copy_trg(mesh->get_target_set<0>(), vti);

        // set up edge-target indices
        Index eti[] =
        {
          0, 9, 12, 3, 2, 10, 6,
        };
        copy_trg(mesh->get_target_set<1>(), eti);

        // set up quad-target indices
        Index qti[] =
        {
          3, 0
        };
        copy_trg(mesh->get_target_set<2>(), qti);

        // okay
        return mesh;
      }

      void validate_refined_tetris_quad_submesh_2d(const QuadSubMesh& mesh)
      {
        //  2<---F----8<---E----4
        //  ^        ||        ||
        //  D   Q_2   Q   Q_0   K
        //  |         v         v
        //  7<---P---13<---O---11
        //  ^        ||        ||
        //  C   Q_3   R   Q_1   L
        //  |         v         v
        //  3<---N---12<---M----1
        //  ||        ||        |
        //  I   Q_4   U   Q_6   G
        //  v         v         v
        // 10----S-->14----T--->9
        //  ||        ||        |
        //  J   Q_5   V   Q_7   H
        //  v         v         v
        //  0----A--->6----B--->5

        // validate sizes
        if(mesh.get_num_entities(0) != 15)
          throw String("Vertex count mismatch");
        if(mesh.get_num_entities(1) != 22)
          throw String("Edge count mismatch");
        if(mesh.get_num_entities(2) != 8)
          throw String("Quad count mismatch");

        // check vertex coordinates array
        Real vtx[] =
        {
          0.0, 0.0, // coarse mesh vertices
          1.0, 1.0,
          0.0, 2.0,
          0.0, 1.0,
          1.0, 2.0,
          1.0, 0.0,
          0.5, 0.0, // edge midpoints
          0.0, 1.5,
          0.5, 2.0,
          1.0, 0.5,
          0.0, 0.5,
          1.0, 1.5,
          0.5, 1.0,
          0.5, 1.5, // quad midpoints
          0.5, 0.5
        };
        if(!comp_vtx(mesh.get_vertex_set(), vtx))
          throw String("Vertex coordinate refinement failure");

        // check vertices-at-edge array
        Index v_e[] =
        {
          0, 6,
          6, 5,
          3, 7,
          7, 2,
          4, 8,
          8, 2,
          1, 9,
          9, 5,
          3, 10,
          10, 0,
          4, 11,
          11, 1,
          1, 12,
          12, 3,
          11, 13,
          13, 7,
          8, 13,
          13, 12,
          10, 14,
          14, 9,
          12, 14,
          14, 6,
        };
        if(!comp_idx(mesh.get_index_set<1,0>(), v_e))
          throw String("Vertex-At-Edge index set refinement failure");

        // check vertices-at-quad
        Index v_q[] =
        {
          4, 11, 8, 13,
          11, 1, 13, 12,
          8, 13, 2, 7,
          13, 12, 7, 3,
          3, 10, 12, 14,
          10, 0, 14, 6,
          12, 14, 1, 9,
          14, 6, 9, 5
        };
        if(!comp_idx(mesh.get_index_set<2,0>(), v_q))
          throw String("Vertex-At-Quad index set refinement failure");

        // check edges-at-quad
        Index e_q[] =
        {
          10, 16, 4, 14,
          11, 17, 14, 12,
          16, 3, 5, 15,
          17, 2, 15, 13,
          8, 20, 13, 18,
          9, 21, 18, 0,
          20, 6, 12, 19,
          21, 7, 19, 1
        };
        if(!comp_idx(mesh.get_index_set<2,1>(), e_q))
          throw String("Edges-At-Quad refinement failure");

        // check vertex-target indices
        Index vti[] =
        {
          0, 5, 8, 4, 9, 1, 10, 19, 22, 13, 12, 20, 16, 26, 23
        };
        if(!comp_trg(mesh.get_target_set<0>(), vti))
          throw String("Vertex-Target-Indices refinement failure");

        // check edge-target indices
        Index eti[] =
        {
          0, 1, 18, 19, 25, 24, 7, 6, 5, 4, 21, 20, 13, 12, 41, 40, 39, 38, 28, 29, 27, 26
        };
        if(!comp_trg(mesh.get_target_set<1>(), eti))
          throw String("Edge-Target-Indices refinement failure");

        // check quad-target indices
        Index qti[] =
        {
          15, 13, 14, 12, 2, 0, 3, 1
        };
        if(!comp_trg(mesh.get_target_set<2>(), qti))
          throw String("Quad-Target-Indices refinement failure");
      }

      QuadMesh* create_quadrefinement_mesh_2d(int orientation)
      {
        Index num_entities[] =
        {
          4, // vertices
          4, // edges
          1   // quads
        };

        // create mesh
        QuadMesh* mesh = new QuadMesh(num_entities);

        //// first possibility ////

        //             e_1
        //    v_2_______>_______v_3
        //     |                 |
        //     |     -->---      |
        //     |      \          |
        //     |       ^         |
        // e_2 ^        \        ^ e_3
        //     |         \       |
        //     |     -->---      |
        //     |________>________|
        //    v_0      e_0      v_1
        //

        // set up vertex coordinates array
        static const Real vtx0[4*2] =
        {
          0.0, 0.0,
          1.0, 0.0,
          0.0, 1.0,
          1.0, 1.0
        };

        // set up vertices-at-edge array
        static const Index v_e0[4*2] =
        {
          0, 1,
          2, 3,
          0, 2,
          1, 3
        };

        // set up vertices-at-quad array
        static const Index v_q0[1*4] =
        {
          0, 1, 2, 3
        };

        // set up edges-at-quad array
        static const Index e_q0[1*4] =
        {
          0,  1,  2,  3
        };

        ////second possibility ////

        //             e_0
        //    v_1_______<_______v_2
        //     |                 |
        //     |     |     /|    |
        //     |     |    / |    |
        //     |     |   /  |    |
        // e_2 v     ^  v   ^    v e_1
        //     |     | /    |    |
        //     |     |/     |    |
        //     |________<________|
        //    v_3      e_3      v_0
        //

        // set up vertex coordinates array
        static const Real vtx1[4*2] =
        {
          1.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          0.0, 0.0
        };

        // set up vertices-at-edge array
        static const Index v_e1[4*2] =
        {
          2, 1,
          2, 0,
          1, 3,
          0, 3
        };

        // set up vertices-at-quad array
        static const Index v_q1[1*4] =
        {
          0, 2, 3, 1
        };

        // set up edges-at-quad array
        static const Index e_q1[1*4] =
        {
          1,  2,  3,  0
        };

        ////third possibility ////

        //             e_2
        //    v_2_______>_______v_1
        //     |                 |
        //     |     --<---      |
        //     |      \          |
        //     |       v         |
        // e_3 ^        \        ^ e_0
        //     |         \       |
        //     |     --<---      |
        //     |________>________|
        //    v_0      e_1      v_3
        //

        // set up vertex coordinates array
        static const Real vtx2[4*2] =
        {
          0.0, 0.0,
          1.0, 1.0,
          0.0, 1.0,
          1.0, 0.0
        };

        // set up vertices-at-edge array
        static const Index v_e2[4*2] =
        {
          3, 1,
          0, 3,
          2, 1,
          0, 2
        };

        // set up vertices-at-quad array
        static const Index v_q2[1*4] =
        {
          1, 2, 3, 0
        };

        // set up edges-at-quad array
        static const Index e_q2[1*4] =
        {
          2,  1,  0,  3
        };

        ////fourth possibility ////

        //             e_2
        //    v_1_______>_______v_2
        //     |                 |
        //     |     |     /|    |
        //     |     |    / |    |
        //     |     |   /  |    |
        // e_1 ^     v  ^   v    v e_0
        //     |     | /    |    |
        //     |     |/     |    |
        //     |________<________|
        //    v_0      e_3      v_3
        //

        // set up vertex coordinates array
        static const Real vtx3[4*2] =
        {
          0.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          1.0, 0.0
        };

        // set up vertices-at-edge array
        static const Index v_e3[4*2] =
        {
          2, 3,
          0, 1,
          1, 2,
          3, 0
        };

        // set up vertices-at-quad array
        static const Index v_q3[1*4] =
        {
          1, 0, 2, 3
        };

        // set up edges-at-quad array
        static const Index e_q3[1*4] =
        {
          1,  0,  2,  3
        };

        switch(orientation)
        {
          case 0:
            copy_vtx(mesh->get_vertex_set(), vtx0);
            copy_idx(mesh->get_index_set<1,0>(), v_e0);
            copy_idx(mesh->get_index_set<2,0>(), v_q0);
            copy_idx(mesh->get_index_set<2,1>(), e_q0);
            break;
          case 1:
            copy_vtx(mesh->get_vertex_set(), vtx1);
            copy_idx(mesh->get_index_set<1,0>(), v_e1);
            copy_idx(mesh->get_index_set<2,0>(), v_q1);
            copy_idx(mesh->get_index_set<2,1>(), e_q1);
            break;
          case 2:
            copy_vtx(mesh->get_vertex_set(), vtx2);
            copy_idx(mesh->get_index_set<1,0>(), v_e2);
            copy_idx(mesh->get_index_set<2,0>(), v_q2);
            copy_idx(mesh->get_index_set<2,1>(), e_q2);
            break;
          case 3:
            copy_vtx(mesh->get_vertex_set(), vtx3);
            copy_idx(mesh->get_index_set<1,0>(), v_e3);
            copy_idx(mesh->get_index_set<2,0>(), v_q3);
            copy_idx(mesh->get_index_set<2,1>(), e_q3);
            break;
        }
        // okay
        return mesh;
      } //�create_quadrefinement_mesh_2d

      void validate_refined_quadrefinement_mesh_2d(const QuadMesh& mesh, int orientation)
      {
        // validate sizes
        if(mesh.get_num_entities(0) != 9)
          throw String("Vertex count mismatch");
        if(mesh.get_num_entities(1) != 12)
          throw String("Edge count mismatch");
        if(mesh.get_num_entities(2) != 4)
          throw String("Quad count mismatch");

        //// first possibility ////

        // vertex coordinates array
        static const Real vtx0[] =
        {
          0.0, 0.0, // coarse mesh vertices (0,...,3)
          1.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          0.5, 0.0,
          0.5, 1.0,
          0.0, 0.5,
          1.0, 0.5,
          0.5, 0.5
        };

        // vertices-at-edge array
        static const Index v_e0[] =
        {
          0, 4,
          4, 1,
          2, 5,
          5, 3,
          0, 6,
          6, 2,
          1, 7,
          7, 3,
          4, 8,
          8, 5,
          6, 8,
          8, 7
        };

        // vertices-at-quad
        static const Index v_q0[] =
        {
          0, 4, 6, 8,
          4, 1, 8, 7,
          6, 8, 2, 5,
          8, 7, 5, 3
        };

        // edges-at-quad
        static const Index e_q0[] =
        {
          0, 10, 4, 8,
          1, 11, 8, 6,
          10, 2, 5, 9,
          11, 3, 9, 7
        };

        //// second possibility ////

        // vertex coordinates array
        static const Real vtx1[] =
        {
          1.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          0.0, 0.0,
          0.5, 1.0,
          1.0, 0.5,
          0.0, 0.5,
          0.5, 0.0,
          0.5, 0.5
        };

        // vertices-at-edge array
        static const Index v_e1[] =
        {
          2, 4,
          4, 1,
          2, 5,
          5, 0,
          1, 6,
          6, 3,
          0, 7,
          7, 3,
          5, 8,
          8, 6,
          7, 8,
          8, 4
        };

        // vertices-at-quad
        static const Index v_q1[] =
        {
          0, 5, 7, 8,
          5, 2, 8, 4,
          7, 8, 3, 6,
          8, 4, 6, 1
        };

        // edges-at-quad
        static const Index e_q1[] =
        {
          3, 10, 6, 8,
          2, 11, 8, 0,
          10, 5, 7, 9,
          11, 4, 9, 1
        };

        //// third possibility ////

        // vertex coordinates array
        static const Real vtx2[] =
        {
          0.0, 0.0,
          1.0, 1.0,
          0.0, 1.0,
          1.0, 0.0,
          1.0, 0.5,
          0.5, 0.0,
          0.5, 1.0,
          0.0, 0.5,
          0.5, 0.5
        };

        // vertices-at-edge array
        static const Index v_e2[] =
        {
          3, 4,
          4, 1,
          0, 5,
          5, 3,
          2, 6,
          6, 1,
          0, 7,
          7, 2,
          6, 8,
          8, 5,
          4, 8,
          8, 7
        };

        // vertices-at-quad
        static const Index v_q2[] =
        {
          1, 6, 4, 8,
          6, 2, 8, 7,
          4, 8, 3, 5,
          8, 7, 5, 0
        };

        // edges-at-quad
        static const Index e_q2[] =
        {
          5, 10, 1, 8,
          4, 11, 8, 7,
          10, 3, 0, 9,
          11, 2, 9, 6
        };

        //// fourth possibility ////

        // vertex coordinates array
        static const Real vtx3[] =
        {
          0.0, 0.0,
          0.0, 1.0,
          1.0, 1.0,
          1.0, 0.0,
          1.0, 0.5,
          0.0, 0.5,
          0.5, 1.0,
          0.5, 0.0,
          0.5, 0.5
        };

        // vertices-at-edge array
        static const Index v_e3[] =
        {
          2, 4,
          4, 3,
          0, 5,
          5, 1,
          1, 6,
          6, 2,
          3, 7,
          7, 0,
          5, 8,
          8, 4,
          6, 8,
          8, 7
        };

        // vertices-at-quad
        static const Index v_q3[] =
        {
          1, 5, 6, 8,
          5, 0, 8, 7,
          6, 8, 2, 4,
          8, 7, 4, 3
        };

        // edges-at-quad
        static const Index e_q3[] =
        {
          3, 10, 4, 8,
          2, 11, 8, 7,
          10, 0, 5, 9,
          11, 1, 9, 6
        };


        switch(orientation)
        {
          case 0:
            // check vertex coordinates array
            if(!comp_vtx(mesh.get_vertex_set(), vtx0))
              throw String("Vertex coordinate refinement failure");

            // check vertices-at-edge array
            if(!comp_idx(mesh.get_index_set<1,0>(), v_e0))
              throw String("Vertex-At-Edge index set refinement failure");

            // check vertices-at-quad
            if(!comp_idx(mesh.get_index_set<2,0>(), v_q0))
              throw String("Vertex-At-Quad index set refinement failure");

            // check edges-at-quad
            if(!comp_idx(mesh.get_index_set<2,1>(), e_q0))
              throw String("Edge-At-Quad index set refinement failure");
            break;

          case 1:
            // check vertex coordinates array
            if(!comp_vtx(mesh.get_vertex_set(), vtx1))
              throw String("Vertex coordinate refinement failure");

            // check vertices-at-edge array
            if(!comp_idx(mesh.get_index_set<1,0>(), v_e1))
              throw String("Vertex-At-Edge index set refinement failure");

            // check vertices-at-quad
            if(!comp_idx(mesh.get_index_set<2,0>(), v_q1))
              throw String("Vertex-At-Quad index set refinement failure");

            // check edges-at-quad
            if(!comp_idx(mesh.get_index_set<2,1>(), e_q1))
              throw String("Edge-At-Quad index set refinement failure");
            break;

          case 2:
            // check vertex coordinates array
            if(!comp_vtx(mesh.get_vertex_set(), vtx2))
              throw String("Vertex coordinate refinement failure");

            // check vertices-at-edge array
            if(!comp_idx(mesh.get_index_set<1,0>(), v_e2))
              throw String("Vertex-At-Edge index set refinement failure");

            // check vertices-at-quad
            if(!comp_idx(mesh.get_index_set<2,0>(), v_q2))
              throw String("Vertex-At-Quad index set refinement failure");

            // check edges-at-quad
            if(!comp_idx(mesh.get_index_set<2,1>(), e_q2))
              throw String("Edge-At-Quad index set refinement failure");
            break;

          case 3:
            // check vertex coordinates array
            if(!comp_vtx(mesh.get_vertex_set(), vtx3))
              throw String("Vertex coordinate refinement failure");

            // check vertices-at-edge array
            if(!comp_idx(mesh.get_index_set<1,0>(), v_e3))
              throw String("Vertex-At-Edge index set refinement failure");

            // check vertices-at-quad
            if(!comp_idx(mesh.get_index_set<2,0>(), v_q3))
              throw String("Vertex-At-Quad index set refinement failure");

            // check edges-at-quad
            if(!comp_idx(mesh.get_index_set<2,1>(), e_q3))
              throw String("Edge-At-Quad index set refinement failure");
            break;
        } //switch
      } // validate_refined_quadrefinement_mesh_2d

    } // namespace TestAux
    /// \endcond
  } // namespace Geometry
} // namespace FEAST

#endif // KERNEL_GEOMETRY_TEST_AUX_TETRIS_QUAD_HPP
