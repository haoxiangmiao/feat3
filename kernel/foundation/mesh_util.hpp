#pragma once
#ifndef KERNEL_FOUNDATION_MESH_UTIL_HPP
#define KERNEL_FOUNDATION_MESH_UTIL_HPP 1

#include<kernel/foundation/mesh.hpp>
#include<kernel/foundation/stl_util.hpp>
#include<cmath>

namespace FEAST
{
  namespace Foundation
  {
    enum EdgeTypes
    {
      et_iz_x = 0,
      et_c_x,
      et_iz_y,
      et_c_y
    };

    struct MeshUtil
    {
      ///for QUAD 2D meshes
      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static bool iz_property_quad(const Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y)
      {
        for(Index i(0) ; i < m.num_polytopes(pl_face) ; ++i)
        {
          auto v_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, i));

          if(v_fi.size() != 4)
          {
            std::cout << "WARNING: not a pure quad mesh!" << std::endl;
          }

          typename AT_::data_type_ e0_x(x.at(v_fi.at(1)) - x.at(v_fi.at(0)));
          typename AT_::data_type_ e0_y(y.at(v_fi.at(1)) - y.at(v_fi.at(0)));
          typename AT_::data_type_ ez_x(x.at(v_fi.at(2)) - x.at(v_fi.at(1)));
          typename AT_::data_type_ ez_y(y.at(v_fi.at(2)) - y.at(v_fi.at(1)));

          ///check sanity of vertex-based iz-curve (cross-edges)
          if(e0_x > 0. && e0_y > 0)
          {
            if(!(ez_x < 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x > 0 AND e0_y > 0 => ez_x < 0, but ez_x is " << ez_x <<  "!" << std::endl;
              return false;
            }
          }
          else if(e0_x > 0. && e0_y <= 0)
          {
            if(!(ez_y > 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x > 0 AND e0_y <= 0 => ez_y > 0, but ez_y is " << ez_y << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x <= 0. && e0_y > 0.)
          {
            if(!(ez_y < 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y > 0 => ez_y < 0, but ez_y is " << ez_y << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x <= 0. && e0_y <= 0)
          {
            if(!(ez_x > 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y <= 0 => ez_x > 0, but ez_x is " << ez_x << "!" << std::endl;
              return false;
            }
          }

          ///check existence of iz-edges
          ///check existence of completion edges
          auto e_fi(m.get_adjacent_polytopes(pl_face, pl_edge, i));
          bool found_e0(false);
          bool found_e1(false);
          bool found_c0(false);
          bool found_c1(false);
          for(auto e_fi_j : e_fi)
          {
            auto v_e_fi_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fi_j));
            found_e0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(1) ? true : found_e0;
            found_e1 = v_e_fi_j.at(0) == v_fi.at(2) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_e1;
            found_c0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(2) ? true : found_c0;
            found_c1 = v_e_fi_j.at(0) == v_fi.at(1) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_c1;
          }
          if(!(found_e0 && found_e1 && found_c0 && found_c1))
          {
            if(!found_e0)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e0) at face " << i << "!" << std::endl;
            if(!found_e1)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e1) at face " << i << "!" << std::endl;
            if(!found_c0)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c0) at face " << i << "!" << std::endl;
            if(!found_c1)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c1) at face " << i << "!" << std::endl;

            return false;
          }
        }
        return true;
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static void establish_iz_property_quad(Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y)
      {
        if(iz_property_quad(m, x, y))
          return;

        std::cout << "Establishing iz-property..." << std::endl;

        OuterStorageType_<Index, std::allocator<Index> > faces_processed;
        OuterStorageType_<Index, std::allocator<Index> > edges_processed;
        OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> > edge_types;

        ///start with face 0
        auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, 0));
        ///pick edge 0 and find e1 = E_f0 - E_V_e0
        auto e0 = E_f0.at(0);
        std::sort(E_f0.begin(), E_f0.end());
        auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
        decltype(V_e0) E_V_e0;
        for(auto V_e0_j : V_e0)
        {
          auto E_V_e0_j(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0_j));
          std::sort(E_V_e0_j.begin(), E_V_e0_j.end());

          decltype(E_V_e0) tmp(E_V_e0.size() + E_V_e0_j.size());
          auto iter(std::set_union(E_V_e0.begin(), E_V_e0.end(), E_V_e0_j.begin(), E_V_e0_j.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));
          E_V_e0 = tmp;
        }
        decltype(V_e0) E_f0_minus_E_V_e0(E_f0.size());
        std::set_difference(E_f0.begin(), E_f0.end(), E_V_e0.begin(), E_V_e0.end(), E_f0_minus_E_V_e0.begin());
        auto e1(E_f0_minus_E_V_e0.at(0));
        auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));


        ///direct e0, e1 to positive x direction (if possible), heuristics: take smallest y-coord-sum-edge for real ez0
        auto x_diff_ez0(x.at(V_e0.at(1)) - x.at(V_e0.at(0)));
        auto x_diff_ez1(x.at(V_e1.at(1)) - x.at(V_e1.at(0)));
        auto y_diff_ez0(y.at(V_e0.at(1)) - y.at(V_e0.at(0)));
        auto y_diff_ez1(y.at(V_e1.at(1)) - y.at(V_e1.at(0)));
        if(x_diff_ez0 != 0) //x pos mode
        {
          auto y_sum_e0(y.at(V_e0.at(0)) + y.at(V_e0.at(1)));
          auto y_sum_e1(y.at(V_e1.at(0)) + y.at(V_e1.at(1)));
          auto ez0(e0);
          auto V_ez0(V_e0);
          auto ez1(e1);
          auto V_ez1(V_e1);
          if(y_sum_e0 > y_sum_e1)
          {
            ez0=e1;
            V_ez0=V_e1;
            ez1=e0;
            V_ez1=V_e0;
          }
          edges_processed.push_back(ez0);
          edge_types.push_back(et_iz_x);

          //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
          edges_processed.push_back(ez1);
          edge_types.push_back(et_iz_x);

          x_diff_ez0 = x.at(V_ez0.at(1)) - x.at(V_ez0.at(0));
          x_diff_ez1 = x.at(V_ez1.at(1)) - x.at(V_ez1.at(0));

          if(x_diff_ez0 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
          }

          if(x_diff_ez1 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
          }

          ///find completion edges
          auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
          auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
          auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
          auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

          for(auto E_f0_j : E_f0)
          {
            auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

            auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
            auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

            auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
            auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

            if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) > x.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }

              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }

            if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) > x.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }
          }

          ///set iz-curve
          //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
          //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = V_ez0.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = V_ez0.at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = V_ez1.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(3) = V_ez1.at(1);
        }
        else //y pos mode
        {
          auto x_sum_e0(x.at(V_e0.at(0)) + x.at(V_e0.at(1)));
          auto x_sum_e1(x.at(V_e1.at(0)) + x.at(V_e1.at(1)));
          auto ez0(e0);
          auto V_ez0(V_e0);
          auto ez1(e1);
          auto V_ez1(V_e1);
          if(x_sum_e0 > x_sum_e1)
          {
            ez0=e1;
            V_ez0=V_e1;
            ez1=e0;
            V_ez1=V_e0;
          }
          edges_processed.push_back(ez0);
          edge_types.push_back(et_iz_x);

          //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
          edges_processed.push_back(ez1);
          edge_types.push_back(et_iz_x);

          y_diff_ez0 = y.at(V_ez0.at(1)) - y.at(V_ez0.at(0));
          y_diff_ez1 = y.at(V_ez1.at(1)) - y.at(V_ez1.at(0));

          if(y_diff_ez0 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
          }

          if(y_diff_ez1 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
          }

          ///find completion edges
          auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
          auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
          auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
          auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

          for(auto E_f0_j : E_f0)
          {
            auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

            auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
            auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

            auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
            auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

            if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
            {
              if(y.at(V_E_f0_j.at(0)) < y.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_x);
            }

            if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
            {
              if(y.at(V_E_f0_j.at(0)) < y.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_x);
            }
          }
          ///set iz-curve
          //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
          //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = V_ez0.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = V_ez0.at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = V_ez1.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(3) = V_ez1.at(1);
        }

        faces_processed.push_back(0);

        ///start on all adjacent faces
        auto E_fi(m.get_adjacent_polytopes(pl_face, pl_edge, 0));
        decltype(E_fi) F_E_fi;
        for(auto E_fi_j : E_fi)
        {
          auto F_E_fi_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_fi_j));
          std::sort(F_E_fi_j.begin(), F_E_fi_j.end());

          decltype(F_E_fi_j) tmp(F_E_fi.size() + F_E_fi_j.size());
          auto iter(std::set_union(F_E_fi.begin(), F_E_fi.end(), F_E_fi_j.begin(), F_E_fi_j.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));
          F_E_fi = tmp;
        }

        for(auto F_E_fi_j : F_E_fi)
          _establish_iz_property_quad(m, x, y, faces_processed, edges_processed, edge_types, 0, F_E_fi_j);
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static bool ccw_property_triangle(const Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y)
      {
        for(Index i(0) ; i < m.num_polytopes(pl_face) ; ++i)
        {
          auto v_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, i));
          auto e_fi(m.get_adjacent_polytopes(pl_face, pl_edge, i));

          if(v_fi.size() != 3)
          {
            std::cout << "WARNING: not a pure triangle mesh!" << std::endl;
          }

          auto v0_in(0);
          auto v1_in(0);
          auto v2_in(0);
          for(auto e_fi_j : e_fi)
          {
            auto V_e_fi_j(m.get_adjacent_polytopes(pl_edge,pl_vertex,e_fi_j));
            v0_in += V_e_fi_j.at(0) == v_fi.at(0) ? 1 : 0 ;
            v1_in += V_e_fi_j.at(0) == v_fi.at(1) ? 1 : 0 ;
            v2_in += V_e_fi_j.at(0) == v_fi.at(2) ? 1 : 0 ;
          }

          if( v0_in != 1 || v1_in != 1 || v2_in != 1)
          {
            std::cout << "WARNING: Face " << i << " not all edges are directed equivalent" << std::endl;
            return false;
          }

          typename AT_::data_type_ ccw_vert(0);
          ccw_vert = (x.at(v_fi.at(1)) - x.at(v_fi.at(0))) * (y.at(v_fi.at(1)) + y.at(v_fi.at(0)))
                     + (x.at(v_fi.at(2)) - x.at(v_fi.at(1))) * (y.at(v_fi.at(2)) + y.at(v_fi.at(1)))
                     + (x.at(v_fi.at(0)) - x.at(v_fi.at(2))) * (y.at(v_fi.at(0)) + y.at(v_fi.at(2)));
          if( ccw_vert > 0 )
          {
            std::cout << "WARNING: Face " << i << " Vertices are not drawn counterclockwise" << std::endl;
            return false;
          }

        }
        return true;
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static void establish_ccw_property_triangle(Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y)
      {
        if(ccw_property_triangle(m, x, y))
          return;

        OuterStorageType_<Index, std::allocator<Index> > faces_processed;
        OuterStorageType_<Index, std::allocator<Index> > edges_processed;

        ///start with face 0
        auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, 0));
        ///pick edge 0
        auto e0 = E_f0.at(0);
        auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));

        auto E_V_e0_1(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(1)));
        std::sort(E_V_e0_1.begin(), E_V_e0_1.end());
        std::sort(E_f0.begin(), E_f0.end());
        decltype(E_f0) e1(E_V_e0_1);
        auto iter(std::set_intersection(E_f0.begin(), E_f0.end(), E_V_e0_1.begin(), E_V_e0_1.end(), e1.begin()));
        e1.resize(Index(iter - e1.begin()));
        std::remove(e1.begin(), e1.end(), e0);
        auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1.at(0)));

        if( V_e1.at(0) != V_e0.at(1) )
        {
          m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1) = V_e1.at(0);
          m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(0) = V_e0.at(1);
        }
        V_e1 = m.get_adjacent_polytopes(pl_edge, pl_vertex, e1.at(0));

        auto E_V_e1_1(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e1.at(1)));
        std::sort(E_V_e1_1.begin(), E_V_e1_1.end());
        decltype(E_f0) e2(E_V_e0_1);
        iter = std::set_intersection(E_f0.begin(), E_f0.end(), E_V_e1_1.begin(), E_V_e1_1.end(), e2.begin());
        e2.resize(Index(iter - e2.begin()));
        std::remove(e2.begin(), e2.end(), e1.at(0));
        auto V_e2(m.get_adjacent_polytopes(pl_edge, pl_vertex, e2.at(0)));

        if( V_e2.at(0) != V_e1.at(1) )
        {
          m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(1) = V_e2.at(0);
          m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(0) = V_e1.at(1);
        }
        V_e2 = m.get_adjacent_polytopes(pl_edge, pl_vertex, e2.at(0));

        auto cclockwise ((x.at(V_e0.at(1)) - x.at(V_e0.at(0))) * (y.at(V_e0.at(1)) + y.at(V_e0.at(0)))
                       + (x.at(V_e1.at(1)) - x.at(V_e1.at(0))) * (y.at(V_e1.at(1)) + y.at(V_e1.at(0)))
                       + (x.at(V_e2.at(1)) - x.at(V_e2.at(0))) * (y.at(V_e2.at(1)) + y.at(V_e2.at(0))));
        if( cclockwise > 0 )
        {
          m.get_topologies().at(ipi_edge_vertex).at(e0).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_edge_vertex).at(e0).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(1);
          m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(0);
          m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(0);
          m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(0);

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(1);
        }
        else
        {
          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1);
        }

        edges_processed.push_back(e0);
        edges_processed.push_back(e1.at(0));
        edges_processed.push_back(e2.at(0));
        faces_processed.push_back(0);

        ///start on all adjacent faces
        decltype(E_f0) F_E_f0;
        for(auto E_f0_j : E_f0)
        {
          auto F_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_f0_j));
          std::sort(F_E_f0_j.begin(), F_E_f0_j.end());

          decltype(F_E_f0_j) tmp(F_E_f0.size() + F_E_f0_j.size());
          iter = std::set_union(F_E_f0.begin(), F_E_f0.end(), F_E_f0_j.begin(), F_E_f0_j.end(), tmp.begin());
          tmp.resize(Index(iter - tmp.begin()));
          F_E_f0 = tmp;
        }

        for(auto F_E_fi_j : F_E_f0)
          _establish_ccw_property_triangle(m, x, y, faces_processed, edges_processed, F_E_fi_j);
      }


      ///for QUAD 3D meshes
      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static bool iz_property_quad(const Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                              const AT_& z)
      {
        for(Index i(0) ; i < m.num_polytopes(pl_face) ; ++i)
        {
          auto v_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, i));

          if(v_fi.size() != 4)
          {
            std::cout << "WARNING: not a pure quad mesh!" << std::endl;
          }

          typename AT_::data_type_ e0_x(x.at(v_fi.at(1)) - x.at(v_fi.at(0)));
          typename AT_::data_type_ e0_y(y.at(v_fi.at(1)) - y.at(v_fi.at(0)));
          typename AT_::data_type_ e0_z(z.at(v_fi.at(1)) - z.at(v_fi.at(0)));
          typename AT_::data_type_ ez_x(x.at(v_fi.at(2)) - x.at(v_fi.at(1)));
          typename AT_::data_type_ ez_y(y.at(v_fi.at(2)) - y.at(v_fi.at(1)));
          typename AT_::data_type_ ez_z(z.at(v_fi.at(2)) - z.at(v_fi.at(1)));

          ///check sanity of vertex-based iz-curve (cross-edges)
          if(e0_x > 0. && e0_y > 0.)
          {
            if(!(e0_z > 0. && ez_x < 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x >= 0 AND e0_y >= 0 => ez_x < 0 AND e0_z > 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x < 0. && e0_y < 0. )
          {
            if(!(e0_z < 0. && ez_x > 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x < 0 AND e0_y < 0 => ez_x > 0 AND e0_z < 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x <= 0. && e0_y > 0. )
          {
            if(!(ez_y < 0. && ez_z < 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y > 0 => ez_y < 0 AND ez_z < 0, but ez_x is " << ez_x << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x > 0. && e0_y <= 0. )
          {
            if(!(ez_y >= 0. && ez_z >= 0.))
            {
              std::cout << "WARNING: malformed cross-edge in iz-curve! e0_x > 0 AND e0_y <= 0 => ez_y >= 0 AND ez_z >= 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
            }
          }


          ///check existence of iz-edges
          ///check existence of completion edges
          auto e_fi(m.get_adjacent_polytopes(pl_face, pl_edge, i));
          bool found_e0(false);
          bool found_e1(false);
          bool found_c0(false);
          bool found_c1(false);
          for(auto e_fi_j : e_fi)
          {
            auto v_e_fi_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fi_j));
            found_e0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(1) ? true : found_e0;
            found_e1 = v_e_fi_j.at(0) == v_fi.at(2) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_e1;
            found_c0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(2) ? true : found_c0;
            found_c1 = v_e_fi_j.at(0) == v_fi.at(1) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_c1;
          }
          if(!(found_e0 && found_e1 && found_c0 && found_c1))
          {
            if(!found_e0)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e0) at face " << i << "!" << std::endl;
            if(!found_e1)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e1) at face " << i << "!" << std::endl;
            if(!found_c0)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c0) at face " << i << "!" << std::endl;
            if(!found_c1)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c1) at face " << i << "!" << std::endl;

            return false;
          }
        }
        return true;
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static void establish_iz_property_quad(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                        const AT_& z)
      {
        if(iz_property_quad<>(m, x, y, z))
          return;

        OuterStorageType_<Index, std::allocator<Index> > faces_processed;
        OuterStorageType_<Index, std::allocator<Index> > edges_processed;
        OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> > edge_types;

        ///start with face 0
        auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, 0));
        ///pick edge 0 and find e1 = E_f0 - E_V_e0
        auto e0 = E_f0.at(0);
        std::sort(E_f0.begin(), E_f0.end());
        auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
        decltype(V_e0) E_V_e0;
        for(auto V_e0_j : V_e0)
        {
          auto E_V_e0_j(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0_j));
          std::sort(E_V_e0_j.begin(), E_V_e0_j.end());

          decltype(E_V_e0) tmp(E_V_e0.size() + E_V_e0_j.size());
          auto iter(std::set_union(E_V_e0.begin(), E_V_e0.end(), E_V_e0_j.begin(), E_V_e0_j.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));
          E_V_e0 = tmp;
        }
        decltype(V_e0) E_f0_minus_E_V_e0(E_f0.size());
        std::set_difference(E_f0.begin(), E_f0.end(), E_V_e0.begin(), E_V_e0.end(), E_f0_minus_E_V_e0.begin());
        auto e1(E_f0_minus_E_V_e0.at(0));
        auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));


        ///direct e0, e1 to positive x direction (if possible), heuristics: take smallest y-coord-sum-edge for real ez0
        auto x_diff_ez0(x.at(V_e0.at(1)) - x.at(V_e0.at(0)));
        auto x_diff_ez1(x.at(V_e1.at(1)) - x.at(V_e1.at(0)));
        auto y_diff_ez0(y.at(V_e0.at(1)) - y.at(V_e0.at(0)));
        auto y_diff_ez1(y.at(V_e1.at(1)) - y.at(V_e1.at(0)));
        if(x_diff_ez0 != 0 && y_diff_ez0 != y_diff_ez1) //x pos mode
        {
          auto y_sum_e0(y.at(V_e0.at(0)) + y.at(V_e0.at(1)));
          auto y_sum_e1(y.at(V_e1.at(0)) + y.at(V_e1.at(1)));

          auto ez0(e0);
          auto V_ez0(V_e0);
          auto ez1(e1);
          auto V_ez1(V_e1);
          if(y_sum_e0 > y_sum_e1)
          {
            ez0=e1;
            V_ez0=V_e1;
            ez1=e0;
            V_ez1=V_e0;
          }
          edges_processed.push_back(ez0);
          edge_types.push_back(et_iz_x);

          //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
          edges_processed.push_back(ez1);
          edge_types.push_back(et_iz_x);

          x_diff_ez0 = x.at(V_ez0.at(1)) - x.at(V_ez0.at(0));
          x_diff_ez1 = x.at(V_ez1.at(1)) - x.at(V_ez1.at(0));

          if(x_diff_ez0 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
          }

          if(x_diff_ez1 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
          }

          ///find completion edges
          auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
          auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
          auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
          auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

          for(auto E_f0_j : E_f0)
          {
            auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

            auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
            auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

            auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
            auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

            if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))//hier ggf. z
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }

              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }

            if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))//hier ggf. z
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }
          }

          ///set iz-curve
          //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
          //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = V_ez0.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = V_ez0.at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = V_ez1.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(3) = V_ez1.at(1);
        }
        else if(y_diff_ez0 != 0 )//y pos mode
        {
          auto x_sum_e0(x.at(V_e0.at(0)) + x.at(V_e0.at(1)));
          auto x_sum_e1(x.at(V_e1.at(0)) + x.at(V_e1.at(1)));
          //auto ez0(x_sum_e0 < x_sum_e1 ? e0 : e1);

          auto ez0(e0);
          auto V_ez0(V_e0);
          auto ez1(e1);
          auto V_ez1(V_e1);
          if(x_sum_e0 > x_sum_e1)
          {
            ez0=e1;
            V_ez0=V_e1;
            ez1=e0;
            V_ez1=V_e0;
          }
          edges_processed.push_back(ez0);
          edge_types.push_back(et_iz_y);

          //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
          edges_processed.push_back(ez1);
          edge_types.push_back(et_iz_y);

          y_diff_ez0 = y.at(V_ez0.at(1)) - y.at(V_ez0.at(0));
          y_diff_ez1 = y.at(V_ez1.at(1)) - y.at(V_ez1.at(0));

          if(y_diff_ez0 > 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
          }

          if(y_diff_ez1 > 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
          }

          ///find completion edges
          auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
          auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
          auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
          auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

          for(auto E_f0_j : E_f0)
          {
            auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

            auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
            auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

            auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
            auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

            if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
            {
              if(y.at(V_E_f0_j.at(0)) > y.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_x);
            }

            if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
            {
              if(y.at(V_E_f0_j.at(0)) > y.at(V_E_f0_j.at(1)))
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_x);
            }
          }
          ///set iz-curve
          //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
          //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = V_ez0.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = V_ez0.at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = V_ez1.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(3) = V_ez1.at(1);
        }
        else // z-pos mode
        {
          auto y_sum_e0(y.at(V_e0.at(0)) + y.at(V_e0.at(1)));
          auto y_sum_e1(y.at(V_e1.at(0)) + y.at(V_e1.at(1)));

          auto ez0(e0);
          auto V_ez0(V_e0);
          auto ez1(e1);
          auto V_ez1(V_e1);
          if(y_sum_e0 > y_sum_e1)
          {
            ez0=e1;
            V_ez0=V_e1;
            ez1=e0;
            V_ez1=V_e0;
          }
          edges_processed.push_back(ez0);
          edge_types.push_back(et_iz_x);

          //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
          edges_processed.push_back(ez1);
          edge_types.push_back(et_iz_x);

          x_diff_ez0 = x.at(V_ez0.at(1)) - x.at(V_ez0.at(0));
          x_diff_ez1 = x.at(V_ez1.at(1)) - x.at(V_ez1.at(0));

          if(x_diff_ez0 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
          }

          if(x_diff_ez1 < 0)
          {
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
          }

          ///find completion edges
          auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
          auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
          auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
          auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

          for(auto E_f0_j : E_f0)
          {
            auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

            auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
            auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

            auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
            auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

            if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))//hier ggf. z
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }

              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }

            if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
            {
              if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))//hier ggf. z
              {
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
              }
              edges_processed.push_back(E_f0_j);
              edge_types.push_back(et_c_y);
            }
          }

          ///set iz-curve
          //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
          //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

          m.get_topologies().at(ipi_face_vertex).at(0).at(0) = V_ez0.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(1) = V_ez0.at(1);
          m.get_topologies().at(ipi_face_vertex).at(0).at(2) = V_ez1.at(0);
          m.get_topologies().at(ipi_face_vertex).at(0).at(3) = V_ez1.at(1);
        }

        faces_processed.push_back(0);

        ///start on all adjacent faces
        auto E_fi(m.get_adjacent_polytopes(pl_face, pl_edge, 0));
        decltype(E_fi) F_E_fi;
        for(auto E_fi_j : E_fi)
        {
          auto F_E_fi_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_fi_j));
          std::sort(F_E_fi_j.begin(), F_E_fi_j.end());

          decltype(F_E_fi_j) tmp(F_E_fi.size() + F_E_fi_j.size());
          auto iter(std::set_union(F_E_fi.begin(), F_E_fi.end(), F_E_fi_j.begin(), F_E_fi_j.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));
          F_E_fi = tmp;
        }

        for(auto F_E_fi_j : F_E_fi)
          _establish_iz_property_quad(m, x, y, z, faces_processed, edges_processed, edge_types, 0, F_E_fi_j);
      }

      ///for HEXA 3D meshes
      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static bool iz_property_hexa(const Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                              const AT_& z)
      {
        for(Index i(0) ; i < m.num_polytopes(pl_polyhedron) ; ++i)
        {
          auto v_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_vertex, i));

          if(v_pi.size() != 8)
          {
            std::cout << "WARNING: not a pure hexa mesh!" << std::endl;
          }

          //check iz-property for all faces
          auto f_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_face, i));
          for(Index k(0); k < f_pi.size(); ++k)
          {
            auto v_fk(m.get_adjacent_polytopes(pl_face, pl_vertex, k));

            if(v_fk.size() != 4)
            {
              std::cout << "WARNING: not a pure quad mesh!" << std::endl;
            }

            typename AT_::data_type_ e0_x(x.at(v_fk.at(1)) - x.at(v_fk.at(0)));
            typename AT_::data_type_ e0_y(y.at(v_fk.at(1)) - y.at(v_fk.at(0)));
            typename AT_::data_type_ e0_z(z.at(v_fk.at(1)) - z.at(v_fk.at(0)));
            typename AT_::data_type_ ez_x(x.at(v_fk.at(2)) - x.at(v_fk.at(1)));
            typename AT_::data_type_ ez_y(y.at(v_fk.at(2)) - y.at(v_fk.at(1)));
            typename AT_::data_type_ ez_z(z.at(v_fk.at(2)) - z.at(v_fk.at(1)));

            ///check sanity of vertex-based iz-curve (cross-edges)
            if(e0_x > 0. && e0_y > 0.)
            {
              if(!(e0_z > 0. && ez_x < 0.))
              {
                std::cout << "WARNING: Polyhedron" << i << " Face " << k << " malformed cross-edge in iz-curve! e0_x >= 0 AND e0_y >= 0 => ez_x < 0 AND e0_z > 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
                return false;
              }
            }
            else if(e0_x < 0. && e0_y < 0. )
            {
              if(!(e0_z < 0. && ez_x > 0.))
              {
                std::cout << "WARNING: Polyhedron" << i << " Face " << k << " malformed cross-edge in iz-curve! e0_x < 0 AND e0_y < 0 => ez_x > 0 AND e0_z < 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
                return false;
              }
            }
            else if(e0_x <= 0. && e0_y > 0. )
            {
              if(!(ez_y < 0. && ez_z <= 0.))
              {
                std::cout << "WARNING: Polyhedron" << i << " Face " << k << " malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y > 0 => ez_y < 0 AND ez_z < 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
                return false;
              }
            }
            else if(e0_x > 0. && e0_y <= 0. )
            {
              if(!(ez_y >= 0. && ez_z >= 0.))
              {
                std::cout << "WARNING: Polyhedron" << i << " Face " << k << ": malformed cross-edge in iz-curve! e0_x > 0 AND e0_y <= 0 => ez_y >= 0 AND ez_z >= 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
                return false;
              }
            }


            ///check existence of iz-edges
            ///check existence of completion edges
            auto e_fk(m.get_adjacent_polytopes(pl_face, pl_edge, k));
            bool found_e0(false);
            bool found_e1(false);
            bool found_c0(false);
            bool found_c1(false);
            for(auto e_fk_j : e_fk)
            {
              auto v_e_fk_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fk_j));
              found_e0 = v_e_fk_j.at(0) == v_fk.at(0) && v_e_fk_j.at(1) == v_fk.at(1) ? true : found_e0;
              found_e1 = v_e_fk_j.at(0) == v_fk.at(2) && v_e_fk_j.at(1) == v_fk.at(3) ? true : found_e1;
              found_c0 = v_e_fk_j.at(0) == v_fk.at(0) && v_e_fk_j.at(1) == v_fk.at(2) ? true : found_c0;
              found_c1 = v_e_fk_j.at(0) == v_fk.at(1) && v_e_fk_j.at(1) == v_fk.at(3) ? true : found_c1;
            }
            if(!(found_e0 && found_e1 && found_c0 && found_c1))
            {
              if(!found_e0)
                std::cout << "WARNING: Polyhedron " << i <<" no matching iz-edge to iz-curve (e0) at face " << k << "!" << std::endl;
              if(!found_e1)
                std::cout << "WARNING: Polyhedron " << i <<" no matching iz-edge to iz-curve (e1) at face " << k << "!" << std::endl;
              if(!found_c0)
                std::cout << "WARNING: Polyhedron " << i <<" no matching completion-edge to iz-curve (c0) at face " << k << "!" << std::endl;
              if(!found_c1)
                std::cout << "WARNING: Polyhedron " << i <<" no matching completion-edge to iz-curve (c1) at face " << k << "!" << std::endl;

              return false;
            }

          }

          // check iz-property for the vertices of the hexa, so vertices 0,1,2 and 3 (an 4,5,6 and 7) have to satisfy the iz-property
          typename AT_::data_type_ e0_x(x.at(v_pi.at(1)) - x.at(v_pi.at(0)));
          typename AT_::data_type_ e0_y(y.at(v_pi.at(1)) - y.at(v_pi.at(0)));
          typename AT_::data_type_ e0_z(z.at(v_pi.at(1)) - z.at(v_pi.at(0)));
          typename AT_::data_type_ ez_x(x.at(v_pi.at(2)) - x.at(v_pi.at(1)));
          typename AT_::data_type_ ez_y(y.at(v_pi.at(2)) - y.at(v_pi.at(1)));
          typename AT_::data_type_ ez_z(z.at(v_pi.at(2)) - z.at(v_pi.at(1)));

          ///check sanity of vertex-based iz-curve (cross-edges)
          if(e0_x > 0. && e0_y > 0.)
          {
            if(!(e0_z > 0. && ez_x < 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x >= 0 AND e0_y >= 0 => ez_x < 0 AND e0_z > 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x < 0. && e0_y < 0. )
          {
            if(!(e0_z < 0. && ez_x > 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x < 0 AND e0_y < 0 => ez_x > 0 AND e0_z < 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x <= 0. && e0_y > 0. )
          {
            if(!(ez_y < 0. && ez_z <= 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y > 0 => ez_y < 0 AND ez_z < 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x > 0. && e0_y <= 0. )
          {
            if(!(ez_y >= 0. && ez_z >= 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << ": malformed cross-edge in iz-curve! e0_x > 0 AND e0_y <= 0 => ez_y >= 0 AND ez_z >= 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
             }
           }

          ///check existence of iz-edges
          ///check existence of completion edges
          auto e_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_edge, i));
          bool found_e0(false);
          bool found_e1(false);
          bool found_c0(false);
          bool found_c1(false);
          for(auto e_pi_j : e_pi)
          {
            auto v_e_pi_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_pi_j));
            found_e0 = v_e_pi_j.at(0) == v_pi.at(0) && v_e_pi_j.at(1) == v_pi.at(1) ? true : found_e0;
            found_e1 = v_e_pi_j.at(0) == v_pi.at(2) && v_e_pi_j.at(1) == v_pi.at(3) ? true : found_e1;
            found_c0 = v_e_pi_j.at(0) == v_pi.at(0) && v_e_pi_j.at(1) == v_pi.at(2) ? true : found_c0;
            found_c1 = v_e_pi_j.at(0) == v_pi.at(1) && v_e_pi_j.at(1) == v_pi.at(3) ? true : found_c1;
          }
          if(!(found_e0 && found_e1 && found_c0 && found_c1))
          {
            if(!found_e0)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e0) at polyhedron " << i << "!" << std::endl;
            if(!found_e1)
              std::cout << "WARNING: no matching iz-edge to iz-curve (e1) at polyhedron " << i << "!" << std::endl;
            if(!found_c0)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c0) at polyhedron " << i << "!" << std::endl;
            if(!found_c1)
              std::cout << "WARNING: no matching completion-edge to iz-curve (c1) at polyhedron " << i << "!" << std::endl;
            return false;
          }

          e0_x = (x.at(v_pi.at(5)) - x.at(v_pi.at(4)));
          e0_y = (y.at(v_pi.at(5)) - y.at(v_pi.at(4)));
          e0_z = (z.at(v_pi.at(5)) - z.at(v_pi.at(4)));
          ez_x = (x.at(v_pi.at(6)) - x.at(v_pi.at(5)));
          ez_y = (y.at(v_pi.at(6)) - y.at(v_pi.at(5)));
          ez_z = (z.at(v_pi.at(6)) - z.at(v_pi.at(5)));

          ///check sanity of vertex-based iz-curve (cross-edges)
          if(e0_x > 0. && e0_y > 0.)
          {
            if(!(e0_z > 0. && ez_x < 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x >= 0 AND e0_y >= 0 => ez_x < 0 AND e0_z > 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x < 0. && e0_y < 0. )
          {
            if(!(e0_z < 0. && ez_x > 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x < 0 AND e0_y < 0 => ez_x > 0 AND e0_z < 0, but ez_x is " << ez_x << " and e0_z is " << e0_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x <= 0. && e0_y > 0. )
          {
            if(!(ez_y < 0. && ez_z <= 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << " malformed cross-edge in iz-curve! e0_x <= 0 AND e0_y > 0 => ez_y < 0 AND ez_z < 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
            }
          }
          else if(e0_x > 0. && e0_y <= 0. )
          {
            if(!(ez_y >= 0. && ez_z >= 0.))
            {
              std::cout << "WARNING: Polyhedron " << i << ": malformed cross-edge in iz-curve! e0_x > 0 AND e0_y <= 0 => ez_y >= 0 AND ez_z >= 0, but ez_y is " << ez_y << " and ez_z is " << ez_z << "!" << std::endl;
              return false;
             }
           }

           ///check existence of iz-edges
           ///check existence of completion edges
           found_e0 = false;
           found_e1 = false;
           found_c0 = false;
           found_c1 = false;
           for(auto e_pi_j : e_pi)
           {
             auto v_e_pi_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_pi_j));
             found_e0 = v_e_pi_j.at(0) == v_pi.at(0) && v_e_pi_j.at(1) == v_pi.at(1) ? true : found_e0;
             found_e1 = v_e_pi_j.at(0) == v_pi.at(2) && v_e_pi_j.at(1) == v_pi.at(3) ? true : found_e1;
             found_c0 = v_e_pi_j.at(0) == v_pi.at(0) && v_e_pi_j.at(1) == v_pi.at(2) ? true : found_c0;
             found_c1 = v_e_pi_j.at(0) == v_pi.at(1) && v_e_pi_j.at(1) == v_pi.at(3) ? true : found_c1;
           }
           if(!(found_e0 && found_e1 && found_c0 && found_c1))
           {
             if(!found_e0)
               std::cout << "WARNING: no matching iz-edge to iz-curve (e0) at polyhedron " << i << "!" << std::endl;
             if(!found_e1)
               std::cout << "WARNING: no matching iz-edge to iz-curve (e1) at polyhedron " << i << "!" << std::endl;
             if(!found_c0)
               std::cout << "WARNING: no matching completion-edge to iz-curve (c0) at polyhedron " << i << "!" << std::endl;
             if(!found_c1)
               std::cout << "WARNING: no matching completion-edge to iz-curve (c1) at polyhedron " << i << "!" << std::endl;
             return false;
           }

           // vertex 0 must be opposite vertex 4
           auto E_v0(m.get_adjacent_polytopes(pl_vertex, pl_edge, v_pi.at(0)));
           auto v4(v_pi.at(4));
           decltype(E_v0) V_E_v0;
           std::sort(v_pi.begin(), v_pi.end());
           for(Index k(0); k < E_v0.size(); k++)
           {
             auto V_E_v0_k(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_v0.at(k)));
             std::sort(V_E_v0_k.begin(), V_E_v0_k.end());

             decltype(V_E_v0) tmp(V_E_v0.size() + V_E_v0_k.size());
             auto iter  (std::set_union(V_E_v0.begin(), V_E_v0.end(), V_E_v0_k.begin(), V_E_v0_k.end(), tmp.begin()));
             tmp.resize(Index(iter - tmp.begin()));
             V_E_v0 = tmp;
           }

           if(std::find(V_E_v0.begin(), V_E_v0.end(), v4) == V_E_v0.end())
           {
             std::cout << "WARNING: Polyhedron " << i << " no diagonal iz-edge" << std::endl;
             return false;
           }
        }
        return true;
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
      static void establish_iz_property_hexa(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                        const AT_& z)
      {
        if(iz_property_hexa(m, x, y, z))
          return;

        OuterStorageType_<Index, std::allocator<Index> > polyhedron_processed;
        OuterStorageType_<Index, std::allocator<Index> > faces_processed;
        OuterStorageType_<Index, std::allocator<Index> > edges_processed;
        OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> > edge_types;

        auto F_p0(m.get_adjacent_polytopes(pl_polyhedron, pl_face, 0));
        auto V_p0(m.get_adjacent_polytopes(pl_polyhedron, pl_vertex, 0));

        // establish iz_property for the first face
        auto f0(F_p0.at(0));
        _establish_iz_property_quadface(m, x, y, z, faces_processed, edges_processed, f0, true);
        auto V_f0(m.get_adjacent_polytopes(pl_face, pl_vertex, f0));
        decltype(V_f0) V_fa;//vertices of opposite face

        auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, f0));
        decltype(F_p0) F_E_f0;
        for(Index i(0); i < E_f0.size(); i++)
        {
          auto F_e_f0_i(m.get_adjacent_polytopes(pl_edge, pl_face, E_f0.at(i)));
          decltype (F_E_f0) tmp(F_E_f0.size() + F_e_f0_i.size());
          std::sort(F_e_f0_i.begin(), F_e_f0_i.end());
          auto iter(std::set_union(F_e_f0_i.begin(),F_e_f0_i.end(), F_E_f0.begin(), F_E_f0.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));
          F_E_f0 = tmp;
        }

        //search opposite face
        std::sort(F_p0.begin(), F_p0.end());
        std::sort(F_E_f0.begin(), F_E_f0.end());
        decltype (F_E_f0) tmp(F_E_f0.size());
        auto iter(std::set_difference(F_p0.begin(), F_p0.end(), F_E_f0.begin(), F_E_f0.end(), tmp.begin()));
        tmp.resize(Index(iter - tmp.begin()));
        auto fa(tmp.at(0));
        for(Index j(0) ; j < V_f0.size() ; ++j)//search right vertices order
        {
          auto vj(V_f0.at(j));
          auto E_vj(m.get_adjacent_polytopes(pl_vertex, pl_edge, vj));

          decltype(E_vj) V_E_vj;
          for(Index k(0) ; k < E_vj.size() ; ++k)
          {

            auto V_E_vj_k(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_vj.at(k)));
            std::sort(V_E_vj_k.begin(), V_E_vj_k.end());

            decltype(V_E_vj) tmp2(V_E_vj.size() + V_E_vj_k.size());
            iter = std::set_union(V_E_vj.begin(), V_E_vj.end(), V_E_vj_k.begin(), V_E_vj_k.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            V_E_vj = tmp2;
          }
          auto V_f0_sort(V_f0);
          std::sort(V_f0_sort.begin(),V_f0_sort.end());
          std::sort(V_p0.begin(), V_p0.end());
          //search point "opposite" v0
          decltype(V_E_vj) V_vj_a(V_E_vj.size());
          iter = std::set_intersection(V_E_vj.begin(), V_E_vj.end(), V_p0.begin(), V_p0.end(), V_vj_a.begin());
          V_vj_a.resize(Index(iter - V_vj_a.begin()));
          iter = std::set_difference(V_E_vj.begin(), V_E_vj.end(), V_f0_sort.begin(), V_f0_sort.end(), V_vj_a.begin());
          V_vj_a.resize(Index(iter - V_vj_a.begin()));
          iter = std::set_intersection(V_vj_a.begin(), V_vj_a.end(), V_p0.begin(), V_p0.end(), V_vj_a.begin());
          V_vj_a.resize(Index(iter - V_vj_a.begin()));
          V_fa.push_back(V_vj_a.at(0));
        }

        auto e_fa(m.get_adjacent_polytopes(pl_face, pl_edge, fa));
        for(auto e_fa_j : e_fa)//process edges
        {
          auto v_e_fa_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fa_j));
          if( (v_e_fa_j.at(0) == V_fa.at(0) && v_e_fa_j.at(1) == V_fa.at(1)) || (v_e_fa_j.at(0) == V_fa.at(1) && v_e_fa_j.at(1) == V_fa.at(0)) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(0);
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(1);
            edges_processed.push_back(e_fa_j);
          }
          if( (v_e_fa_j.at(0) == V_fa.at(2) && v_e_fa_j.at(1) == V_fa.at(3)) || (v_e_fa_j.at(0) == V_fa.at(3) && v_e_fa_j.at(1) == V_fa.at(2)) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(2);
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(3);
            edges_processed.push_back(e_fa_j);
          }
          if( (v_e_fa_j.at(0) == V_fa.at(0) && v_e_fa_j.at(1) == V_fa.at(2)) || (v_e_fa_j.at(0) == V_fa.at(2) && v_e_fa_j.at(1) == V_fa.at(0)) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(0);
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(2);
            edges_processed.push_back(e_fa_j);
          }
          if( (v_e_fa_j.at(0) == V_fa.at(1) && v_e_fa_j.at(1) == V_fa.at(3)) || (v_e_fa_j.at(0) == V_fa.at(3) && v_e_fa_j.at(1) == V_fa.at(1)) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(3);
            edges_processed.push_back(e_fa_j);
          }
        }

        //set iz-curve
        m.get_topologies().at(ipi_face_vertex).at(fa).at(0) = V_fa.at(0);
        m.get_topologies().at(ipi_face_vertex).at(fa).at(1) = V_fa.at(1);
        m.get_topologies().at(ipi_face_vertex).at(fa).at(2) = V_fa.at(2);
        m.get_topologies().at(ipi_face_vertex).at(fa).at(3) = V_fa.at(3);
        faces_processed.push_back(fa);

        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(0) = V_f0.at(0);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(1) = V_f0.at(1);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(2) = V_f0.at(2);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(3) = V_f0.at(3);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(4) = V_fa.at(0);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(5) = V_fa.at(1);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(6) = V_fa.at(2);
        m.get_topologies().at(ipi_polyhedron_vertex).at(0).at(7) = V_fa.at(3);


        // establish iz_property for all faces of p0
        for(Index i(1); i < F_p0.size(); ++i)
        {
          _complete_iz_property_quadface(m, x, y, z, faces_processed, edges_processed, F_p0.at(i));
        }

        polyhedron_processed.push_back(0);

        ///start on all adjacent polyhedrons
        auto P_p0(m.get_adjacent_polytopes(pl_polyhedron, pl_polyhedron, 0));
        for(auto P_p0_j : P_p0)
          _establish_iz_property_hexa(m, x, y, z, polyhedron_processed, faces_processed, edges_processed, P_p0_j);

      }

      ///for tetra 3D meshes
      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_>
      static bool property_tetra(const Mesh<Dim3D, TopologyType_, OuterStorageType_>& m)
      {
        for(Index i(0); i < m.num_polytopes(pl_polyhedron); i++)
        {
          auto v_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_vertex, i));

          if(v_pi.size() != 4)
          {
            std::cout << "WARNING: not a pure tetra mesh!" << std::endl;
            return false;
          }
        }
        //check property for all edges
        auto num_edges(m.get_topologies().at(ipi_edge_vertex).size());
        for(Index i(0) ; i < num_edges; ++i)
        {
          auto V_e_i(m.get_adjacent_polytopes(pl_edge, pl_vertex, i));
          if(V_e_i.at(0) > V_e_i.at(1))
          {
            std::cout << "WARNING: edge " << i << " is directed incorretly" << std::endl;
            return false;
          }
        }

        //check property for all faces
        auto num_faces(m.get_topologies().at(ipi_face_vertex).size());
        for(Index i(0); i < num_faces; ++i)
        {
          auto v_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, i));

          if(v_fi.size() != 3)
          {
            std::cout << "WARNING: not a pure tetra mesh!"<< "Face " << i << " has not 3 vertices!" << std::endl;
            return false;
          }

          if(v_fi.at(0) > v_fi.at(1))
          {
            std::cout << "WARNING: face " << i << " is directed incorretly" << std::endl;
            return false;
          }
          else if(v_fi.at(1) > v_fi.at(2))
          {
            std::cout << "WARNING: face " << i << " is directed incorretly" << std::endl;
            return false;
          }
        }

        return true;
      }

      template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_>
      static void establish_property_tetra(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m)
      {
        if(property_tetra(m))
          return;

        //direct all edges
        auto num_edges(m.get_topologies().at(ipi_edge_vertex).size());
        for(Index i(0); i < num_edges; i++)
        {
          auto V_ei(m.get_adjacent_polytopes(pl_edge, pl_vertex, i));
          if(V_ei.at(0) > V_ei.at(1))
          {
            m.get_topologies().at(ipi_edge_vertex).at(i).at(0) = V_ei.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(i).at(1) = V_ei.at(0);
          }
        }

        //direct all faces
        auto num_faces(m.get_topologies().at(ipi_face_vertex).size());
        for(Index i(0); i < num_faces; i++)
        {
          auto V_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, i));
          if(V_fi.at(0) > V_fi.at(1) || V_fi.at(1) > V_fi.at(2))
          {
            std::sort(V_fi.begin(), V_fi.end());
            m.get_topologies().at(ipi_face_vertex).at(i).at(0) = V_fi.at(0);
            m.get_topologies().at(ipi_face_vertex).at(i).at(1) = V_fi.at(1);
            m.get_topologies().at(ipi_face_vertex).at(i).at(2) = V_fi.at(2);
          }
        }
      }


      private:
        template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
        static void _establish_iz_property_quad(Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> >& et,
                                          Index face_from,
                                          Index face_num)
        {
          ///face already processed -> recursion end
          if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
            return;

          ///retrieve information about already processed edges
          auto E_fi(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          OuterStorageType_<Index, std::allocator<Index> > local_ep;
          OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> > local_et;

          for(auto E_fi_j : E_fi)
          {
            auto iter(std::find(ep.begin(), ep.end(), E_fi_j));

            if(iter != ep.end())
            {
              local_et.push_back(et.at(Index(iter - ep.begin())));
              local_ep.push_back(ep.at(Index(iter - ep.begin())));
            }
          }

          ///all edges processed -> recursion end
          if(local_et.size() >= 4)
          {
            fp.push_back(face_num);
            return;
          }

          ///pull directions from origin face
          auto E0(m.get_comm_intersection(pl_face, pl_edge, face_from, face_num));

          if(E0.size() == 0) ///coming from diagonal face
            return;

          auto e0(E0.at(0));

          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
          decltype(V_e0) E_V_e0;
          for(auto V_e0_i : V_e0)
          {
            auto E_V_e0_i(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0_i));
            E_V_e0 = STLUtil::set_union(E_V_e0, E_V_e0_i);
          }
          auto E_f1(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          auto E1(STLUtil::set_difference(E_f1, E_V_e0));
          auto e1(E1.at(0));

          auto iter0(std::find(ep.begin(), ep.end(), e0));
          Index access0(Index(iter0 - ep.begin()));
          _direct(m, et.at(access0) == et_iz_x || et.at(access0) == et_c_x ? x : y, e0, e1); //capture x_diff == 0 case
          ep.push_back(e1);
          et.push_back(et.at(access0));

          //et1 = E_f0 - E_V_e0
          auto Ef0(m.get_adjacent_polytopes(pl_face, pl_edge, face_from));
          auto et1(STLUtil::set_difference(Ef0, E_V_e0).at(0));

          //et2 = any of E_f0 - {e0, et1}
          decltype(Ef0) e0et1;
          e0et1.push_back(e0);
          e0et1.push_back(et1);
          auto et2(STLUtil::set_difference(Ef0, e0et1).at(0));

          //{e2, e3} = E_f1 - {e0, e1}
          auto E23(STLUtil::set_difference(E_f1, E0));
          E23 = STLUtil::set_difference(E23, E1);
          auto e2(E23.at(0));
          auto e3(E23.at(1));

          //_direct(et2, {e2, e3})
          auto iter1(std::find(ep.begin(), ep.end(), et2));
          Index access1(Index(iter1 - ep.begin()));
          _direct(m, et.at(access1) == et_iz_x || et.at(access1) == et_c_x ? x : y, et2, e2); //capture x_diff == 0 case
          ep.push_back(e2);
          et.push_back(et.at(access1));
          _direct(m, et.at(access1) == et_iz_x || et.at(access1) == et_c_x ? x : y, et2, e3); //capture x_diff == 0 case
          ep.push_back(e3);
          et.push_back(et.at(access1));

          if(et.at(access0) == et_iz_x || et.at(access0) == et_iz_y) //e0, e1 is iz curve
          {
            auto V_e0_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
            auto V_e1_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));
            if(et.at(access0) == et_iz_x)
            {
              auto y_sum_e0(y.at(V_e0_new.at(0)) + y.at(V_e0_new.at(1)));
              auto y_sum_e1(y.at(V_e1_new.at(0)) + y.at(V_e1_new.at(1)));

              if(y_sum_e0 < y_sum_e1)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e0_new.at(1);
              }
            }
            else
            {
              auto x_sum_e0(x.at(V_e0_new.at(0)) + x.at(V_e0_new.at(1)));
              auto x_sum_e1(x.at(V_e1_new.at(0)) + x.at(V_e1_new.at(1)));

              if(x_sum_e0 > x_sum_e1)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e0_new.at(1);
              }
            }
          }
          else if(et.at(access1) == et_iz_x || et.at(access1) == et_iz_y) //e2, e3 is iz curve
          {
            auto V_e2_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e2));
            auto V_e3_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e3));
            if(et.at(access1) == et_iz_x)
            {
              auto y_sum_e2(y.at(V_e2_new.at(0)) + y.at(V_e2_new.at(1)));
              auto y_sum_e3(y.at(V_e3_new.at(0)) + y.at(V_e3_new.at(1)));

              if(y_sum_e2 < y_sum_e3)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e2_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e3_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e3_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e2_new.at(1);
              }
            }
            else
            {
              auto x_sum_e2(x.at(V_e2_new.at(0)) + x.at(V_e2_new.at(1)));
              auto x_sum_e3(x.at(V_e3_new.at(0)) + x.at(V_e3_new.at(1)));

              if(x_sum_e2 > x_sum_e3)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e2_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e3_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e3_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e2_new.at(1);
              }
            }
          }

          //end and recursion
          decltype(E_fi) F_E_fi;
          for(auto E_fi_j : E_fi)
          {
            auto F_E_fi_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_fi_j));
            std::sort(F_E_fi_j.begin(), F_E_fi_j.end());

            decltype(F_E_fi_j) tmp(F_E_fi.size() + F_E_fi_j.size());
            auto iter3(std::set_union(F_E_fi.begin(), F_E_fi.end(), F_E_fi_j.begin(), F_E_fi_j.end(), tmp.begin()));
            tmp.resize(Index(iter3 - tmp.begin()));
            F_E_fi = tmp;
          }
          fp.push_back(face_num);
          ///do recursion
          for(auto F_E_fi_j : F_E_fi)
            _establish_iz_property_quad(m, x, y, fp, ep, et, face_num, F_E_fi_j);
        }

        template<typename TopologyType_,
                 template <typename, typename> class OuterStorageType_,
                 typename AT_>
        static void _direct(Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& xy, Index e0, Index e1)
        {
          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
          auto xy_diff_e0(xy.at(V_e0.at(1)) - xy.at(V_e0.at(0)));

          if(xy_diff_e0 == typename AT_::data_type_(0))
            throw MeshError("Edge cannot be directed like this!");

          auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));
          auto xy_diff_e1(xy.at(V_e1.at(1)) - xy.at(V_e1.at(0)));

          if((xy_diff_e0 > 0 && xy_diff_e1 > 0) || (xy_diff_e0 < 0 && xy_diff_e1 < 0))
            return;
          else
          {
            m.get_topologies().at(ipi_edge_vertex).at(e1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(e1).at(1) = V_e1.at(0);
          }
        }

        template<typename TopologyType_,
                template <typename, typename> class OuterStorageType_,
                typename AT_>
        static void _establish_ccw_property_triangle(Mesh<Dim2D, TopologyType_, OuterStorageType_>& m, const AT_& x,
                                                    const AT_& y,
                                                    OuterStorageType_<Index, std::allocator<Index> >& fp,
                                                    OuterStorageType_<Index, std::allocator<Index> >& ep,
                                                    Index face_num)
        {
          std::sort(fp.begin(), fp.end());
          ///face already processed -> recursion end
          if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
            return;
          std::sort(ep.begin(), ep.end());

          ///retrieve information about already processed edges
          auto E_fi(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          ///search already processed edge
          Index ep_num(0);
          bool exit(false);
          while(exit != true)
          {
            auto iter(std::find(ep.begin(), ep.end(), E_fi.at(ep_num)));

            iter != ep.end() ? exit = true : ep_num++;
          }

          auto e0 = E_fi.at(ep_num);
          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));

          auto E_V_e0_1(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(1)));
          std::sort(E_V_e0_1.begin(), E_V_e0_1.end());
          std::sort(E_fi.begin(), E_fi.end());
          decltype(E_fi) e1(E_V_e0_1);
          auto iter(std::set_intersection(E_fi.begin(), E_fi.end(), E_V_e0_1.begin(), E_V_e0_1.end(), e1.begin()));
          e1.resize(Index(iter - e1.begin()));
          std::remove(e1.begin(), e1.end(), e0);
          auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1.at(0)));

          if( V_e1.at(0) != V_e0.at(1) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1) = V_e1.at(0);
            m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(0) = V_e0.at(1);
            V_e1.at(0) = m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(0);
            V_e1.at(1) = m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1);
          }
          V_e1 = m.get_adjacent_polytopes(pl_edge, pl_vertex, e1.at(0));

          auto E_V_e1_1(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e1.at(1)));
          std::sort(E_V_e1_1.begin(), E_V_e1_1.end());
          decltype(E_fi) e2(E_V_e0_1);
          iter = std::set_intersection(E_fi.begin(), E_fi.end(), E_V_e1_1.begin(), E_V_e1_1.end(), e2.begin());
          e2.resize(Index(iter - e2.begin()));
          std::remove(e2.begin(), e2.end(), e1.at(0));
          auto V_e2(m.get_adjacent_polytopes(pl_edge, pl_vertex, e2.at(0)));

          if( V_e2.at(0) != V_e1.at(1) )
          {
            m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(1) = V_e2.at(0);
            m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(0) = V_e1.at(1);
          }

        auto cclockwise ((x.at(V_e0.at(1)) - x.at(V_e0.at(0))) * (y.at(V_e0.at(1)) + y.at(V_e0.at(0)))
                       + (x.at(V_e1.at(1)) - x.at(V_e1.at(0))) * (y.at(V_e1.at(1)) + y.at(V_e1.at(0)))
                       + (x.at(V_e2.at(1)) - x.at(V_e2.at(0))) * (y.at(V_e2.at(1)) + y.at(V_e2.at(0))));
        if( cclockwise > 0 )
        {
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(0);
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = m.get_topologies().at(ipi_edge_vertex).at(e2.at(0)).at(0);
        }
        else
        {
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(0);
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = m.get_topologies().at(ipi_edge_vertex).at(e0).at(1);
          m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = m.get_topologies().at(ipi_edge_vertex).at(e1.at(0)).at(1);
        }

          ep.push_back(e1.at(0));
          ep.push_back(e2.at(0));
          fp.push_back(face_num);

          ///start on all adjacent faces
          decltype(E_fi) F_E_fi;
          for(auto E_fi_j : E_fi)
          {
            auto F_E_fi_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_fi_j));
            std::sort(F_E_fi_j.begin(), F_E_fi_j.end());

            decltype(F_E_fi_j) tmp(F_E_fi.size() + F_E_fi_j.size());
            iter = std::set_union(F_E_fi.begin(), F_E_fi.end(), F_E_fi_j.begin(), F_E_fi_j.end(), tmp.begin());
            tmp.resize(Index(iter - tmp.begin()));
            F_E_fi = tmp;
          }

          for(auto F_E_fi_j : F_E_fi)
            _establish_ccw_property_triangle(m, x, y, fp, ep, F_E_fi_j);
        }

        template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
        static void _establish_iz_property_quad(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          const AT_& z,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> >& et,
                                          Index face_from,
                                          Index face_num)
        {
          ///face already processed -> recursion end
          if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
            return;

          ///retrieve information about already processed edges
          auto E_fi(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          OuterStorageType_<Index, std::allocator<Index> > local_ep;
          OuterStorageType_<EdgeTypes, std::allocator<EdgeTypes> > local_et;

          for(auto E_fi_j : E_fi)
          {
            auto iter(std::find(ep.begin(), ep.end(), E_fi_j));

            if(iter != ep.end())
            {
              local_et.push_back(et.at(Index(iter - ep.begin())));
              local_ep.push_back(ep.at(Index(iter - ep.begin())));
            }
          }

          ///all edges processed -> recursion end
          if(local_et.size() >= 4)
          {
            fp.push_back(face_num);
            return;
          }

          ///pull directions from origin face
          auto E0(m.get_comm_intersection(pl_face, pl_edge, face_from, face_num));

          if(E0.size() == 0) ///coming from diagonal face
            return;

          auto e0(E0.at(0));

          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
          decltype(V_e0) E_V_e0;
          for(auto V_e0_i : V_e0)
          {
            auto E_V_e0_i(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0_i));
            E_V_e0 = STLUtil::set_union(E_V_e0, E_V_e0_i);
          }
          auto E_f1(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          auto E1(STLUtil::set_difference(E_f1, E_V_e0));
          auto e1(E1.at(0));

          auto iter0(std::find(ep.begin(), ep.end(), e0));
          Index access0(Index(iter0 - ep.begin()));
          _direct(m, et.at(access0) == et_iz_x || et.at(access0) == et_c_x ? x : y, e0, e1); //capture x_diff == 0 case
          ep.push_back(e1);
          et.push_back(et.at(access0));

          //et1 = E_f0 - E_V_e0
          auto Ef0(m.get_adjacent_polytopes(pl_face, pl_edge, face_from));
          auto et1(STLUtil::set_difference(Ef0, E_V_e0).at(0));

          //et2 = any of E_f0 - {e0, et1}
          decltype(Ef0) e0et1;
          e0et1.push_back(e0);
          e0et1.push_back(et1);
          auto et2(STLUtil::set_difference(Ef0, e0et1).at(0));

          //{e2, e3} = E_f1 - {e0, e1}
          auto E23(STLUtil::set_difference(E_f1, E0));
          E23 = STLUtil::set_difference(E23, E1);
          auto e2(E23.at(0));
          auto e3(E23.at(1));

          //_direct(et2, {e2, e3})
          auto iter1(std::find(ep.begin(), ep.end(), et2));
          Index access1(Index(iter1 - ep.begin()));
          _direct(m, et.at(access1) == et_iz_x || et.at(access1) == et_c_x ? x : y, et2, e2); //capture x_diff == 0 case
          ep.push_back(e2);
          et.push_back(et.at(access1));
          _direct(m, et.at(access1) == et_iz_x || et.at(access1) == et_c_x ? x : y, et2, e3); //capture x_diff == 0 case
          ep.push_back(e3);
          et.push_back(et.at(access1));

          if(et.at(access0) == et_iz_x || et.at(access0) == et_iz_y) //e0, e1 is iz curve
          {
            auto V_e0_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
            auto V_e1_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));
            if(et.at(access0) == et_iz_x)
            {
              auto y_sum_e0(y.at(V_e0_new.at(0)) + y.at(V_e0_new.at(1)));
              auto y_sum_e1(y.at(V_e1_new.at(0)) + y.at(V_e1_new.at(1)));

              if(y_sum_e0 < y_sum_e1)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e0_new.at(1);
              }
            }
            else
            {
              auto x_sum_e0(x.at(V_e0_new.at(0)) + x.at(V_e0_new.at(1)));
              auto x_sum_e1(x.at(V_e1_new.at(0)) + x.at(V_e1_new.at(1)));

              if(x_sum_e0 > x_sum_e1)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e1_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e0_new.at(1);
              }
            }
          }
          else if(et.at(access1) == et_iz_x || et.at(access1) == et_iz_y) //e2, e3 is iz curve
          {
            auto V_e2_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e2));
            auto V_e3_new(m.get_adjacent_polytopes(pl_edge, pl_vertex, e3));
            if(et.at(access1) == et_iz_x)
            {
              auto y_sum_e2(y.at(V_e2_new.at(0)) + y.at(V_e2_new.at(1)));
              auto y_sum_e3(y.at(V_e3_new.at(0)) + y.at(V_e3_new.at(1)));

              if(y_sum_e2 < y_sum_e3)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e2_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e3_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e3_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e2_new.at(1);
              }
            }
            else
            {
              auto x_sum_e2(x.at(V_e2_new.at(0)) + x.at(V_e2_new.at(1)));
              auto x_sum_e3(x.at(V_e3_new.at(0)) + x.at(V_e3_new.at(1)));

              if(x_sum_e2 > x_sum_e3)
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e2_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e3_new.at(1);
              }
              else
              {
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e3_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e3_new.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e2_new.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e2_new.at(1);
              }
            }
          }

          //end and recursion
          decltype(E_fi) F_E_fi;
          for(auto E_fi_j : E_fi)
          {
            auto F_E_fi_j(m.get_adjacent_polytopes(pl_edge, pl_face, E_fi_j));
            std::sort(F_E_fi_j.begin(), F_E_fi_j.end());

            decltype(F_E_fi_j) tmp(F_E_fi.size() + F_E_fi_j.size());
            auto iter3(std::set_union(F_E_fi.begin(), F_E_fi.end(), F_E_fi_j.begin(), F_E_fi_j.end(), tmp.begin()));
            tmp.resize(Index(iter3 - tmp.begin()));
            F_E_fi = tmp;
          }
          fp.push_back(face_num);
          ///do recursion
          for(auto F_E_fi_j : F_E_fi)
            _establish_iz_property_quad(m, x, y, z, fp, ep, et, face_num, F_E_fi_j);
        }

        template<typename TopologyType_,
                 template <typename, typename> class OuterStorageType_,
                 typename AT_>
        static void _direct(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& xy, Index e0, Index e1)
        {
          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
          auto xy_diff_e0(xy.at(V_e0.at(1)) - xy.at(V_e0.at(0)));

          if(xy_diff_e0 == typename AT_::data_type_(0))
            throw MeshError("Edge cannot be directed like this!");

          auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));
          auto xy_diff_e1(xy.at(V_e1.at(1)) - xy.at(V_e1.at(0)));

          if((xy_diff_e0 > 0 && xy_diff_e1 > 0) || (xy_diff_e0 < 0 && xy_diff_e1 < 0))
            return;
          else
          {
            m.get_topologies().at(ipi_edge_vertex).at(e1).at(0) = V_e1.at(1);
            m.get_topologies().at(ipi_edge_vertex).at(e1).at(1) = V_e1.at(0);
          }
        }



        template<typename TopologyType_,
                template <typename, typename> class OuterStorageType_,
                typename AT_>
        static void _establish_iz_property_quadface(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          const AT_& z,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          Index face_num,
                                          bool first_face)
        {
          std::sort(fp.begin(), fp.end());
          std::sort(ep.begin(), ep.end());
          //face already processed -> recursion end
            if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
              return;

          ///start with face 0
          auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          ///if already processed a face start with this, otherwise take first face
          Index ep_num(0);
          bool exit(false);
          first_face == true ? exit = true : exit = false;
          while(exit != true)
          {
            auto iter(std::find(ep.begin(), ep.end(), E_f0.at(ep_num)));

            iter != ep.end() ? exit = true : ep_num++;
          }

          //pick edge 0, search edge 1
          auto e0 = E_f0.at(ep_num);
          std::sort(E_f0.begin(), E_f0.end());
          auto V_e0(m.get_adjacent_polytopes(pl_edge, pl_vertex, e0));
          decltype(V_e0) E_V_e0;
          for(auto V_e0_j : V_e0)
          {
            auto E_V_e0_j(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0_j));
            std::sort(E_V_e0_j.begin(), E_V_e0_j.end());

            decltype(E_V_e0) tmp(E_V_e0.size() + E_V_e0_j.size());
            auto iter(std::set_union(E_V_e0.begin(), E_V_e0.end(), E_V_e0_j.begin(), E_V_e0_j.end(), tmp.begin()));
            tmp.resize(Index(iter - tmp.begin()));
            E_V_e0 = tmp;
          }
          decltype(V_e0) E_f0_minus_E_V_e0(E_f0.size());
          std::sort(E_V_e0.begin(), E_V_e0.end());
          auto iter (std::set_difference(E_f0.begin(), E_f0.end(), E_V_e0.begin(), E_V_e0.end(), E_f0_minus_E_V_e0.begin()));
          E_f0_minus_E_V_e0.resize(Index(iter - E_f0_minus_E_V_e0.begin()));
          auto e1(E_f0_minus_E_V_e0.at(0));
          auto V_e1(m.get_adjacent_polytopes(pl_edge, pl_vertex, e1));


          ///direct e0, e1 to positive x direction (if possible), heuristics: take smallest y-coord-sum-edge for real ez0
          auto x_diff_ez0(x.at(V_e0.at(1)) - x.at(V_e0.at(0)));
          auto x_diff_ez1(x.at(V_e1.at(1)) - x.at(V_e1.at(0)));
          auto y_diff_ez0(y.at(V_e0.at(1)) - y.at(V_e0.at(0)));
          auto y_diff_ez1(y.at(V_e1.at(1)) - y.at(V_e1.at(0)));
          auto z_diff_ez0(z.at(V_e0.at(1)) - z.at(V_e0.at(0)));

          if(x_diff_ez0 != 0 && y_diff_ez0 >= z_diff_ez0) //x pos mode
          {
            auto y_sum_e0(y.at(V_e0.at(0)) + y.at(V_e0.at(1)));
            auto y_sum_e1(y.at(V_e1.at(0)) + y.at(V_e1.at(1)));

            auto ez0(e0);
            auto V_ez0(V_e0);
            auto ez1(e1);
            auto V_ez1(V_e1);
            if(y_sum_e0 > y_sum_e1)
            {
              ez0=e1;
              V_ez0=V_e1;
              ez1=e0;
              V_ez1=V_e0;
            }
            ep.push_back(ez0);

            //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
            ep.push_back(ez1);

            x_diff_ez0 = x.at(V_ez0.at(1)) - x.at(V_ez0.at(0));
            x_diff_ez1 = x.at(V_ez1.at(1)) - x.at(V_ez1.at(0));

            if(x_diff_ez0 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
            }

            if(x_diff_ez1 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
            }

            ///find completion edges
            auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
            auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
            auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
            auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

            for(auto E_f0_j : E_f0)
            {
              auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

              auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
              auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

              auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
              auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

              if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
              {
                if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }

                ep.push_back(E_f0_j);
              }

              if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
              {
                if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))//hier ggf. z
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }
            }

            ///set iz-curve
            //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
            //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_ez0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_ez0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_ez1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_ez1.at(1);

            fp.push_back(face_num);
          }
          else if(x_diff_ez0 != 0 && y_diff_ez0 < z_diff_ez0) //x pos mode
          {
            auto z_sum_e0(y.at(V_e0.at(0)) + y.at(V_e0.at(1)));
            auto z_sum_e1(y.at(V_e1.at(0)) + y.at(V_e1.at(1)));

            auto ez0(e0);
            auto V_ez0(V_e0);
            auto ez1(e1);
            auto V_ez1(V_e1);
            if(z_sum_e0 > z_sum_e1)
            {
              ez0=e1;
              V_ez0=V_e1;
              ez1=e0;
              V_ez1=V_e0;
            }
            ep.push_back(ez0);

            //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
            ep.push_back(ez1);

            x_diff_ez0 = x.at(V_ez0.at(1)) - x.at(V_ez0.at(0));
            x_diff_ez1 = x.at(V_ez1.at(1)) - x.at(V_ez1.at(0));

            if(x_diff_ez0 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_e0.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_e0.at(0);
            }

            if(x_diff_ez1 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_e1.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_e1.at(0);
            }

            ///find completion edges
            auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
            auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
            auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
            auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

            for(auto E_f0_j : E_f0)
            {
              auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

              auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
              auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

              auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
              auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

              if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
              {
                if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }

                ep.push_back(E_f0_j);
              }

              if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
              {
                if(x.at(V_E_f0_j.at(0)) < x.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }
            }

            ///set iz-curve
            //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
            //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_ez0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_ez0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_ez1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_ez1.at(1);

            fp.push_back(face_num);
          }
          else if( y_diff_ez0 >= z_diff_ez0 )//y pos mode
          {
            auto z_sum_e0(z.at(V_e0.at(0)) + z.at(V_e0.at(1)));
            auto z_sum_e1(z.at(V_e1.at(0)) + z.at(V_e1.at(1)));
            //auto ez0(x_sum_e0 < x_sum_e1 ? e0 : e1);

            auto ez0(e0);
            auto V_ez0(V_e0);
            auto ez1(e1);
            auto V_ez1(V_e1);
            if(z_sum_e0 < z_sum_e1)
            {
              ez0=e1;
              V_ez0=V_e1;
              ez1=e0;
              V_ez1=V_e0;
            }
            ep.push_back(ez0);

            //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
            ep.push_back(ez1);

            y_diff_ez0 = y.at(V_ez0.at(1)) - y.at(V_ez0.at(0));
            y_diff_ez1 = y.at(V_ez1.at(1)) - y.at(V_ez1.at(0));

            if(y_diff_ez0 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_ez0.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_ez0.at(0);
            }

            if(y_diff_ez1 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_ez1.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_ez1.at(0);
            }

            ///find completion edges
            auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
            auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
            auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
            auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

            for(auto E_f0_j : E_f0)
            {
              auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

              auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
              auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

              auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
              auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

              if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
              {
                if(z.at(V_E_f0_j.at(0)) < z.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }

              if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
              {
                if(z.at(V_E_f0_j.at(0)) < z.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }
            }
            ///set iz-curve
            //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
            //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_ez0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_ez0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_ez1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_ez1.at(1);

            fp.push_back(face_num);
          }
          else // z-pos mode
          {
            auto x_sum_e0(x.at(V_e0.at(0)) + x.at(V_e0.at(1)));
            auto x_sum_e1(x.at(V_e1.at(0)) + x.at(V_e1.at(1)));
            //auto ez0(x_sum_e0 < x_sum_e1 ? e0 : e1);

            auto ez0(e0);
            auto V_ez0(V_e0);
            auto ez1(e1);
            auto V_ez1(V_e1);
            if(x_sum_e0 > x_sum_e1)
            {
              ez0=e1;
              V_ez0=V_e1;
              ez1=e0;
              V_ez1=V_e0;
            }
            ep.push_back(ez0);

            //auto ez1(x_sum_e0 < x_sum_e1 ? e1 : e0);
            ep.push_back(ez1);

            y_diff_ez0 = y.at(V_ez0.at(1)) - y.at(V_ez0.at(0));
            y_diff_ez1 = y.at(V_ez1.at(1)) - y.at(V_ez1.at(0));

            if(y_diff_ez0 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(0) = V_ez0.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez0).at(1) = V_ez0.at(0);
            }

            if(y_diff_ez1 < 0)
            {
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(0) = V_ez1.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(ez1).at(1) = V_ez1.at(0);
            }

            ///find completion edges
            auto v_ec0_0(ez0 == e0 ? V_e0.at(0) : V_e1.at(0));
            auto v_ec0_1(ez0 == e0 ? V_e1.at(0) : V_e0.at(0));
            auto v_ec1_0(ez0 == e0 ? V_e0.at(1) : V_e1.at(1));
            auto v_ec1_1(ez0 == e0 ? V_e1.at(1) : V_e0.at(1));

            for(auto E_f0_j : E_f0)
            {
              auto V_E_f0_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_f0_j));

              auto iter00(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_0));
              auto iter01(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec0_1));

              auto iter10(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_0));
              auto iter11(std::find(V_E_f0_j.begin(), V_E_f0_j.end(), v_ec1_1));

              if(iter00 != V_E_f0_j.end() && iter01 != V_E_f0_j.end())
              {
                if(y.at(V_E_f0_j.at(0)) < y.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }

              if(iter10 != V_E_f0_j.end() && iter11 != V_E_f0_j.end())
              {
                if(y.at(V_E_f0_j.at(0)) < y.at(V_E_f0_j.at(1)))
                {
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(0) = V_E_f0_j.at(1);
                  m.get_topologies().at(ipi_edge_vertex).at(E_f0_j).at(1) = V_E_f0_j.at(0);
                }
                ep.push_back(E_f0_j);
              }
            }
            ///set iz-curve
            //auto V_ez0(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez0));
            //auto V_ez1(m.get_adjacent_polytopes(pl_edge, pl_vertex, ez1));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_ez0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_ez0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_ez1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_ez1.at(1);

            fp.push_back(face_num);
          }
        }


        template<typename TopologyType_,
               template <typename, typename> class OuterStorageType_,
               typename AT_>
        static void _establish_iz_property_hexa(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          const AT_& z,
                                          OuterStorageType_<Index, std::allocator<Index> >& pp,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          Index polyhedron_num)
        {
          std::sort(pp.begin(),pp.end());
          std::sort(fp.begin(),fp.end());
          std::sort(ep.begin(), ep.end());
          ///face already processed -> recursion end
          if(std::find(pp.begin(), pp.end(), polyhedron_num) != pp.end())
            return;

          ///retrieve information about already processed edges
          auto F_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_face, polyhedron_num));
          ///search already processed face
          Index fp_num(0);
          bool exit(false);
          while(exit != true)
          {
            auto iter(std::find(fp.begin(), fp.end(), F_pi.at(fp_num)));

            iter != fp.end() ? exit = true : fp_num++;
          }

          auto V_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_vertex, polyhedron_num));

          ///search opposite face of processed f0
          auto f0(F_pi.at(fp_num));
          auto V_f0(m.get_adjacent_polytopes(pl_face, pl_vertex, f0));
          decltype(V_f0) V_fa;

          auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, f0));
          decltype(F_pi) F_E_f0;
          for(Index i(0); i < E_f0.size(); i++)
          {
            auto F_e_f0_i(m.get_adjacent_polytopes(pl_edge, pl_face, E_f0.at(i)));
            decltype (F_E_f0) tmp(F_E_f0.size() + F_e_f0_i.size());
            std::sort(F_e_f0_i.begin(), F_e_f0_i.end());
            auto iter(std::set_union(F_e_f0_i.begin(),F_e_f0_i.end(), F_E_f0.begin(), F_E_f0.end(), tmp.begin()));
            tmp.resize(Index(iter - tmp.begin()));
            F_E_f0 = tmp;
          }

          std::sort(F_pi.begin(),F_pi.end());
          std::sort(F_E_f0.begin(),F_E_f0.end());

          decltype (F_E_f0) tmp(F_pi.size());
          auto iter(std::set_difference(F_pi.begin(), F_pi.end(), F_E_f0.begin(), F_E_f0.end(), tmp.begin()));
          tmp.resize(Index(iter - tmp.begin()));

          auto fa(tmp.at(0));
          for(Index j(0) ; j < V_f0.size() ; ++j)//search right order of vertices from fa
          {
            auto vj(V_f0.at(j));
            auto E_vj(m.get_adjacent_polytopes(pl_vertex, pl_edge, vj));

            decltype(E_vj) V_E_vj;
            for(Index k(0) ; k < E_vj.size() ; ++k)
            {

              auto V_E_vj_k(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_vj.at(k)));
              std::sort(V_E_vj_k.begin(), V_E_vj_k.end());

              decltype(V_E_vj) tmp2(V_E_vj.size() + V_E_vj_k.size());
              iter = std::set_union(V_E_vj.begin(), V_E_vj.end(), V_E_vj_k.begin(), V_E_vj_k.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              V_E_vj = tmp2;
            }

            auto V_f0_sort(V_f0);
            std::sort(V_f0_sort.begin(),V_f0_sort.end());
            std::sort(V_pi.begin(), V_pi.end());
            //search point "above" v0
            decltype(V_E_vj) V_vj_a(V_E_vj.size());
            iter = std::set_intersection(V_E_vj.begin(), V_E_vj.end(), V_pi.begin(), V_pi.end(), V_vj_a.begin());
            V_vj_a.resize(Index(iter - V_vj_a.begin()));
            iter = std::set_difference(V_E_vj.begin(), V_E_vj.end(), V_f0_sort.begin(), V_f0_sort.end(), V_vj_a.begin());
            V_vj_a.resize(Index(iter - V_vj_a.begin()));
            iter = std::set_intersection(V_vj_a.begin(), V_vj_a.end(), V_pi.begin(), V_pi.end(), V_vj_a.begin());
            V_vj_a.resize(Index(iter - V_vj_a.begin()));
            V_fa.push_back(V_vj_a.at(0));
          }

          /// process edges of fa
          auto e_fa(m.get_adjacent_polytopes(pl_face, pl_edge, fa));
          std::sort(e_fa.begin(), e_fa.end());
          iter = std::set_difference(e_fa.begin(), e_fa.end(), ep.begin(), ep.end(), e_fa.begin());
          e_fa.resize(Index( iter - e_fa.begin()));
          for(auto e_fa_j : e_fa)
          {
            auto v_e_fa_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fa_j));

            if( (v_e_fa_j.at(0) == V_fa.at(0) && v_e_fa_j.at(1) == V_fa.at(1)) || (v_e_fa_j.at(0) == V_fa.at(1) && v_e_fa_j.at(1) == V_fa.at(0)) )
            {
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(0);
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(1);
              ep.push_back(e_fa_j);
            }
            if( (v_e_fa_j.at(0) == V_fa.at(2) && v_e_fa_j.at(1) == V_fa.at(3)) || (v_e_fa_j.at(0) == V_fa.at(3) && v_e_fa_j.at(1) == V_fa.at(2)) )
            {
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(2);
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(3);
              ep.push_back(e_fa_j);
            }
            if( (v_e_fa_j.at(0) == V_fa.at(0) && v_e_fa_j.at(1) == V_fa.at(2)) || (v_e_fa_j.at(0) == V_fa.at(2) && v_e_fa_j.at(1) == V_fa.at(0)) )
            {
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(0);
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(2);
              ep.push_back(e_fa_j);
            }
            if( (v_e_fa_j.at(0) == V_fa.at(1) && v_e_fa_j.at(1) == V_fa.at(3)) || (v_e_fa_j.at(0) == V_fa.at(3) && v_e_fa_j.at(1) == V_fa.at(1)) )
            {
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(0) = V_fa.at(1);
              m.get_topologies().at(ipi_edge_vertex).at(e_fa_j).at(1) = V_fa.at(3);
              ep.push_back(e_fa_j);
            }
          }
          /// set iz_property of fa
          m.get_topologies().at(ipi_face_vertex).at(fa).at(0) = V_fa.at(0);
          m.get_topologies().at(ipi_face_vertex).at(fa).at(1) = V_fa.at(1);
          m.get_topologies().at(ipi_face_vertex).at(fa).at(2) = V_fa.at(2);
          m.get_topologies().at(ipi_face_vertex).at(fa).at(3) = V_fa.at(3);

          fp.push_back(fa);

          //set iz_property of pi
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(0) = V_f0.at(0);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(1) = V_f0.at(1);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(2) = V_f0.at(2);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(3) = V_f0.at(3);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(4) = V_fa.at(0);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(5) = V_fa.at(1);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(6) = V_fa.at(2);
          m.get_topologies().at(ipi_polyhedron_vertex).at(polyhedron_num).at(7) = V_fa.at(3);

          ///establish iz_property from all faces of pi
          for(Index i(0); i < F_pi.size(); ++i)
          {
            _complete_iz_property_quadface(m, x, y, z, fp, ep, F_pi.at(i));
          }
          pp.push_back(polyhedron_num);

          ///start on all adjacent polyhedrons
          auto P_F_pi(m.get_adjacent_polytopes(pl_polyhedron, pl_polyhedron, polyhedron_num));
          std::sort(P_F_pi.begin(), P_F_pi.end());
          for(Index i(0); i < F_pi.size(); i++)
          {
            auto P_Fi_pi(m.get_adjacent_polytopes(pl_face, pl_polyhedron, F_pi.at(i)));
            std::sort(P_Fi_pi.begin(), P_Fi_pi.end());
            decltype(P_F_pi) tmp2(P_Fi_pi.size() + P_F_pi.size());
            iter = std::set_union(P_F_pi.begin(), P_F_pi.end(), P_Fi_pi.begin(), P_Fi_pi.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            P_F_pi = tmp2;
          }


          for(auto P_p0_j : P_F_pi)
            _establish_iz_property_hexa(m, x, y, z, pp, fp, ep, P_p0_j);

        }


        template<typename TopologyType_,
                template <typename, typename> class OuterStorageType_,
                typename AT_>
        static bool _iz_property_quadface(const Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                const AT_& z, Index face_num)
        {
          auto v_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, face_num));

          if(v_fi.size() != 4)
          {
              std::cout << "WARNING: not a pure quad mesh!" << std::endl;
          }

          typename AT_::data_type_ e0_x(x.at(v_fi.at(1)) - x.at(v_fi.at(0)));
          typename AT_::data_type_ e0_y(y.at(v_fi.at(1)) - y.at(v_fi.at(0)));
          typename AT_::data_type_ e0_z(z.at(v_fi.at(1)) - z.at(v_fi.at(0)));
          typename AT_::data_type_ ez_x(x.at(v_fi.at(2)) - x.at(v_fi.at(1)));
          typename AT_::data_type_ ez_y(y.at(v_fi.at(2)) - y.at(v_fi.at(1)));
          typename AT_::data_type_ ez_z(z.at(v_fi.at(2)) - z.at(v_fi.at(1)));

          ///check sanity of vertex-based iz-curve (cross-edges)
          if(e0_x > 0. && e0_y > 0.)
          {
              if(!(e0_z > 0. && ez_x < 0.))
              {
                  return false;
              }
          }
          else if(e0_x < 0. && e0_y < 0. )
          {
              if(!(e0_z < 0. && ez_x > 0.))
              {
                  return false;
              }
          }
          else if(e0_x <= 0. && e0_y > 0. )
          {
              if(!(ez_y < 0. && ez_z < 0.))
              {
                  return false;
              }
          }
          else if(e0_x > 0. && e0_y <= 0. )
          {
              if(!(ez_y >= 0. && ez_z >= 0.))
              {
                  return false;
              }
          }


          ///check existence of iz-edges
          ///check existence of completion edges
          auto e_fi(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          bool found_e0(false);
          bool found_e1(false);
          bool found_c0(false);
          bool found_c1(false);
          for(auto e_fi_j : e_fi)
          {
              auto v_e_fi_j(m.get_adjacent_polytopes(pl_edge, pl_vertex, e_fi_j));
              found_e0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(1) ? true : found_e0;
              found_e1 = v_e_fi_j.at(0) == v_fi.at(2) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_e1;
              found_c0 = v_e_fi_j.at(0) == v_fi.at(0) && v_e_fi_j.at(1) == v_fi.at(2) ? true : found_c0;
              found_c1 = v_e_fi_j.at(0) == v_fi.at(1) && v_e_fi_j.at(1) == v_fi.at(3) ? true : found_c1;
          }
          if(!(found_e0 && found_e1 && found_c0 && found_c1))
            return false;

          return true;
        }



        template<typename TopologyType_,
                template <typename, typename> class OuterStorageType_,
                typename AT_>
        static void _complete_iz_property_quadface(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          const AT_& z,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          Index face_num)
        {
          //face already processed -> recursion end
          std::sort(fp.begin(), fp.end());
          std::sort(ep.begin(), ep.end());
          if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
            return;

          //search how much edges already processed
          auto e_fi(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          std::sort(e_fi.begin(),e_fi.end());
          decltype(e_fi) ep_fi(e_fi.size());
          auto iter (std::set_intersection(ep.begin(), ep.end(), e_fi.begin(), e_fi.end(), ep_fi.begin()));
          ep_fi.resize(Index(iter - ep_fi.begin()));

          if (ep_fi.size() == 2)
          {
            _establish_iz_property_quadface_2edges_processed(m, x, y, z, fp, ep, face_num);
          }
          else if(ep_fi.size() == 3)
          {
            auto V_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, face_num));

            std::sort(V_fi.begin(),V_fi.end());

            decltype(e_fi) enp_fi(e_fi.size());//not processed edges
            iter = std::set_difference(e_fi.begin(), e_fi.end(), ep_fi.begin(), ep_fi.end(), enp_fi.begin());
            enp_fi.resize(Index(iter - enp_fi.begin()));

            auto V_enp_fi_0(m.get_adjacent_polytopes(pl_edge, pl_vertex, enp_fi.at(0)));
            decltype(e_fi) E_enp_fi_0;
            for(Index k(0); k < V_enp_fi_0.size(); k++)//adjacent edges of the not processed edge
            {
              auto E_enp_fi_0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_enp_fi_0.at(k)));
              decltype (E_enp_fi_0) tmp(E_enp_fi_0.size() + E_enp_fi_0_k.size());
              std::sort(E_enp_fi_0_k.begin(), E_enp_fi_0_k.end());
              iter = std::set_union(E_enp_fi_0_k.begin(),E_enp_fi_0_k.end(), E_enp_fi_0.begin(), E_enp_fi_0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              E_enp_fi_0 = tmp;
            }
            std::sort(E_enp_fi_0.begin(),E_enp_fi_0.end());
            iter = std::set_intersection(E_enp_fi_0.begin(), E_enp_fi_0.end(), e_fi.begin(), e_fi.end(), E_enp_fi_0.begin());
            E_enp_fi_0.resize(Index(iter - E_enp_fi_0.begin()));

            decltype(E_enp_fi_0) opp_edge(E_enp_fi_0.size());//opposite edge of the not processed edge
            iter = std::set_difference(e_fi.begin(), e_fi.end(), E_enp_fi_0.begin(), E_enp_fi_0.end(), opp_edge.begin());
            opp_edge.resize(Index(iter - opp_edge.begin()));

            auto V_oe(m.get_adjacent_polytopes(pl_edge, pl_vertex, opp_edge.at(0)));

            //process the 'not processed edge'
            auto E_v_e0_0(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_oe.at(0)));
            decltype(V_oe) V_v_e0_0;
            for( Index k(0); k < E_v_e0_0.size(); k++)
            {
              auto V_Ek_v_e0_0(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_v_e0_0.at(k)));
              decltype (V_v_e0_0) tmp(V_v_e0_0.size() + V_Ek_v_e0_0.size());
              std::sort(V_Ek_v_e0_0.begin(), V_Ek_v_e0_0.end());
              iter = std::set_union(V_Ek_v_e0_0.begin(),V_Ek_v_e0_0.end(), V_v_e0_0.begin(), V_v_e0_0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              V_v_e0_0 = tmp;
            }
            std::sort(V_v_e0_0.begin(),V_v_e0_0.end());
            decltype(V_fi) tmp(V_fi.size());
            iter = std::set_difference(V_fi.begin(), V_fi.end(), V_v_e0_0.begin(), V_v_e0_0.end(), tmp.begin());
            tmp.resize(Index(iter - tmp.begin()));
            m.get_topologies().at(ipi_edge_vertex).at(enp_fi.at(0)).at(1) = tmp.at(0);

            V_v_e0_0 = m.get_adjacent_polytopes(pl_vertex, pl_vertex, V_oe.at(1));
            std::sort(V_v_e0_0.begin(),V_v_e0_0.end());
            tmp.resize(V_fi.size());
            iter = std::set_difference(V_fi.begin(), V_fi.end(), V_v_e0_0.begin(), V_v_e0_0.end(), tmp.begin());
            tmp.resize(Index(iter - tmp.begin()));
            m.get_topologies().at(ipi_edge_vertex).at(enp_fi.at(0)).at(0) = tmp.at(0);
            ep.push_back(enp_fi.at(0));auto ve(m.get_adjacent_polytopes(pl_edge,pl_vertex,enp_fi.at(0)));

            //set iz_property of the face
            //search starting point of iz_property
            auto v0_in(0);
            auto v1_in(0);
            auto v2_in(0);
            auto v3_in(0);
            for(auto e_fi_j : e_fi)
            {
              auto V_e_fi_j(m.get_adjacent_polytopes(pl_edge,pl_vertex,e_fi_j));
              v0_in += V_e_fi_j.at(0) == V_fi.at(0) ? 1 : 0 ;
              v1_in += V_e_fi_j.at(0) == V_fi.at(1) ? 1 : 0 ;
              v2_in += V_e_fi_j.at(0) == V_fi.at(2) ? 1 : 0 ;
              v3_in += V_e_fi_j.at(0) == V_fi.at(3) ? 1 : 0 ;
            }

            if( v0_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(0)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());

              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));//set e0
              decltype(E_v0) E_e_v0;
              //search e1
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
                tmp2.resize(Index(iter - tmp2.begin()));
                E_e_v0 = tmp2;
              }

              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());

              decltype(E_e_v0) tmp2(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

              ///set iz_property
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else if( v1_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(1)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());

              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));///set e0
              decltype(E_v0) E_e_v0;
              //search e1
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
                tmp2.resize(Index(iter - tmp2.begin()));
                E_e_v0 = tmp2;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());

              decltype(E_e_v0) tmp2(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

              ///set iz-property
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else if( v2_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(2)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());

              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));///set e0
              decltype(E_v0) E_e_v0;
              ///search e1
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
                tmp2.resize(Index(iter - tmp2.begin()));
                E_e_v0 = tmp2;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());

              decltype(E_e_v0) tmp2(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

              ///set iz_property
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(3)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());

              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));///set e0
              decltype(E_v0) E_e_v0;
              ///search e1
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
                tmp2.resize(Index(iter - tmp2.begin()));
                E_e_v0 = tmp2;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());

              decltype(E_e_v0) tmp2(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

              ///set iz_property
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
          }
          else//all edges processed
          {
            auto V_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, face_num));
            std::sort(V_fi.begin(),V_fi.end());
            auto v0_in(0);
            auto v1_in(0);
            auto v2_in(0);
            auto v3_in(0);

            //search starting point of iz_property
            for(auto e_fi_j : e_fi)
            {
              auto V_e_fi_j(m.get_adjacent_polytopes(pl_edge,pl_vertex,e_fi_j));
              v0_in += V_e_fi_j.at(0) == V_fi.at(0) ? 1 : 0 ;
              v1_in += V_e_fi_j.at(0) == V_fi.at(1) ? 1 : 0 ;
              v2_in += V_e_fi_j.at(0) == V_fi.at(2) ? 1 : 0 ;
              v3_in += V_e_fi_j.at(0) == V_fi.at(3) ? 1 : 0 ;
            }

            if( v0_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(0)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());
              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
              decltype(E_v0) E_e_v0;
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
                tmp.resize(Index(iter - tmp.begin()));
                E_e_v0 = tmp;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());
              decltype(E_e_v0) tmp(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp.at(0)));

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else if( v1_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(1)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());
              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
              decltype(E_v0) E_e_v0;
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
                tmp.resize(Index(iter - tmp.begin()));
                E_e_v0 = tmp;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());
              decltype(E_e_v0) tmp(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp.at(0)));

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else if( v2_in == 2 )
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(2)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());
              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
              decltype(E_v0) E_e_v0;
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
                tmp.resize(Index(iter - tmp.begin()));
                E_e_v0 = tmp;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());
              decltype(E_e_v0) tmp(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp.at(0)));

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
            else
            {
              auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(3)));
              std::set_intersection(E_v0.begin(), E_v0.end(), e_fi.begin(), e_fi.end(), E_v0.begin());
              auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
              decltype(E_v0) E_e_v0;
              for(Index k(0); k < V_e0.size(); k++)
              {
                auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
                decltype (E_e_v0) tmp(E_e_v0.size() + E_e_v0_k.size());
                std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
                iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
                tmp.resize(Index(iter - tmp.begin()));
                E_e_v0 = tmp;
              }
              std::sort(E_v0.begin(),E_v0.end());
              std::sort(E_e_v0.begin(), E_e_v0.end());

              iter = std::set_intersection(E_e_v0.begin(), E_e_v0.end(), e_fi.begin(), e_fi.end(), E_e_v0.begin());
              E_e_v0.resize(Index(iter - E_e_v0.begin()));

              decltype(E_e_v0) tmp(E_e_v0.size());
              iter = std::set_difference(e_fi.begin(), e_fi.end(), E_e_v0.begin(), E_e_v0.end(), tmp.begin());
              tmp.resize(Index(iter - tmp.begin()));
              auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp.at(0)));

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

              if(!(_iz_property_quadface(m, x, y, z, face_num)))
              {

                m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
                m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
              }
              fp.push_back(face_num);
            }
          }

        }

        template<typename TopologyType_,
                template <typename, typename> class OuterStorageType_,
                typename AT_>
        static void _establish_iz_property_quadface_2edges_processed(Mesh<Dim3D, TopologyType_, OuterStorageType_>& m, const AT_& x, const AT_& y,
                                          const AT_& z,
                                          OuterStorageType_<Index, std::allocator<Index> >& fp,
                                          OuterStorageType_<Index, std::allocator<Index> >& ep,
                                          Index face_num)
        {
          std::sort(fp.begin(), fp.end());
          std::sort(ep.begin(), ep.end());
          //face already processed -> recursion end
          if(std::find(fp.begin(), fp.end(), face_num) != fp.end())
            return;

          ///start with face 0
          auto V_fi(m.get_adjacent_polytopes(pl_face, pl_vertex, face_num));
          auto E_f0(m.get_adjacent_polytopes(pl_face, pl_edge, face_num));
          ///pick processed edge "e0" and find e1 = E_f0 - E_V_e0
          decltype(E_f0) Ep_f0(E_f0.size());
          std::sort(E_f0.begin(), E_f0.end());
          std::sort(V_fi.begin(), V_fi.end());
          auto iter(std::set_intersection(ep.begin(), ep.end(), E_f0.begin(), E_f0.end(), Ep_f0.begin()));
          Ep_f0.resize(Index(iter - Ep_f0.begin()));

          decltype(E_f0) Enp_f0(E_f0.size());
          iter = std::set_difference(E_f0.begin(), E_f0.end(), Ep_f0.begin(), Ep_f0.end(), Enp_f0.begin());
          Enp_f0.resize(Index(iter - Enp_f0.begin()));

          //suche von enp 0 die parallelen knoten und richte enp 1
          ep.push_back(Enp_f0.at(0));
          auto V_enp_0(m.get_adjacent_polytopes(pl_edge, pl_vertex, Enp_f0.at(0)));
          auto E_v_enp0_0(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_enp_0.at(0)));
          decltype(V_enp_0) V_v_enp0_0;
          for( Index k(0); k < E_v_enp0_0.size(); k++)
          {
            auto V_Ek_v_enp0_0(m.get_adjacent_polytopes(pl_edge, pl_vertex, E_v_enp0_0.at(k)));
            decltype (V_v_enp0_0) tmp(V_v_enp0_0.size() + V_Ek_v_enp0_0.size());
            std::sort(V_Ek_v_enp0_0.begin(), V_Ek_v_enp0_0.end());
            iter = std::set_union(V_Ek_v_enp0_0.begin(),V_Ek_v_enp0_0.end(), V_v_enp0_0.begin(), V_v_enp0_0.end(), tmp.begin());
            tmp.resize(Index(iter - tmp.begin()));
            V_v_enp0_0 = tmp;
          }
          std::sort(V_v_enp0_0.begin(),V_v_enp0_0.end());
          decltype(V_fi) tmp(V_fi.size());
          iter = std::set_difference(V_fi.begin(), V_fi.end(), V_v_enp0_0.begin(), V_v_enp0_0.end(), tmp.begin());
          tmp.resize(Index(iter - tmp.begin()));
          m.get_topologies().at(ipi_edge_vertex).at(Enp_f0.at(1)).at(1) = tmp.at(0);

          V_v_enp0_0 = m.get_adjacent_polytopes(pl_vertex, pl_vertex, V_enp_0.at(1));
          std::sort(V_v_enp0_0.begin(),V_v_enp0_0.end());
          tmp.resize(V_fi.size());
          iter = std::set_difference(V_fi.begin(), V_fi.end(), V_v_enp0_0.begin(), V_v_enp0_0.end(), tmp.begin());
          tmp.resize(Index(iter - tmp.begin()));
          m.get_topologies().at(ipi_edge_vertex).at(Enp_f0.at(1)).at(0) = tmp.at(0);
          ep.push_back(Enp_f0.at(1));

          //search starting point of iz_property
          auto v0_in(0);
          auto v1_in(0);
          auto v2_in(0);
          auto v3_in(0);

          for(auto e_fi_j : E_f0)
          {
            auto V_e_fi_j(m.get_adjacent_polytopes(pl_edge,pl_vertex,e_fi_j));
            v0_in += V_e_fi_j.at(0) == V_fi.at(0) ? 1 : 0 ;
            v1_in += V_e_fi_j.at(0) == V_fi.at(1) ? 1 : 0 ;
            v2_in += V_e_fi_j.at(0) == V_fi.at(2) ? 1 : 0 ;
            v3_in += V_e_fi_j.at(0) == V_fi.at(3) ? 1 : 0 ;
          }

          if( v0_in == 2 )
          {
            auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(0)));
            std::set_intersection(E_v0.begin(), E_v0.end(), E_f0.begin(), E_f0.end(), E_v0.begin());
            auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
            decltype(E_v0) E_e_v0;
            for(Index k(0); k < V_e0.size(); k++)
            {
              auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
              decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
              std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
              iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              E_e_v0 = tmp2;
            }

            std::sort(E_v0.begin(),E_v0.end());
            std::sort(E_e_v0.begin(), E_e_v0.end());

            decltype(E_e_v0) tmp2(E_e_v0.size());
            iter = std::set_difference(E_f0.begin(), E_f0.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

            if(!(_iz_property_quadface(m, x, y, z, face_num)))
            {

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
            }
            fp.push_back(face_num);
          }
          else if( v1_in == 2 )
          {
            auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(1)));
            std::set_intersection(E_v0.begin(), E_v0.end(), E_f0.begin(), E_f0.end(), E_v0.begin());
            auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
            decltype(E_v0) E_e_v0;
            for(Index k(0); k < V_e0.size(); k++)
            {
              auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
              decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
              std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
              iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              E_e_v0 = tmp2;
            }
            std::sort(E_v0.begin(),E_v0.end());
            std::sort(E_e_v0.begin(), E_e_v0.end());

            decltype(E_e_v0) tmp2(E_e_v0.size());
            iter = std::set_difference(E_f0.begin(), E_f0.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

            if(!(_iz_property_quadface(m, x, y, z, face_num)))
            {

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
            }
            fp.push_back(face_num);
          }
          else if( v2_in == 2 )
          {
            auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(2)));
            std::set_intersection(E_v0.begin(), E_v0.end(), E_f0.begin(), E_f0.end(), E_v0.begin());
            auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
            decltype(E_v0) E_e_v0;
            for(Index k(0); k < V_e0.size(); k++)
            {
              auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
              decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
              std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
              iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              E_e_v0 = tmp2;
            }
            std::sort(E_v0.begin(),E_v0.end());
            std::sort(E_e_v0.begin(), E_e_v0.end());

            decltype(E_e_v0) tmp2(E_e_v0.size());
            iter = std::set_difference(E_f0.begin(), E_f0.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

            if(!(_iz_property_quadface(m, x, y, z, face_num)))
            {

              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
            }
            fp.push_back(face_num);
          }
          else
          {
            auto E_v0(m.get_adjacent_polytopes(pl_vertex,pl_edge, V_fi.at(3)));
            std::set_intersection(E_v0.begin(), E_v0.end(), E_f0.begin(), E_f0.end(), E_v0.begin());
            auto V_e0(m.get_adjacent_polytopes(pl_edge,pl_vertex, E_v0.at(0)));
            decltype(E_v0) E_e_v0;
            for(Index k(0); k < V_e0.size(); k++)
            {
              auto E_e_v0_k(m.get_adjacent_polytopes(pl_vertex, pl_edge, V_e0.at(k)));
              decltype (E_e_v0) tmp2(E_e_v0.size() + E_e_v0_k.size());
              std::sort(E_e_v0_k.begin(), E_e_v0_k.end());
              iter = std::set_union(E_e_v0_k.begin(),E_e_v0_k.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
              tmp2.resize(Index(iter - tmp2.begin()));
              E_e_v0 = tmp2;
            }
            std::sort(E_v0.begin(),E_v0.end());
            std::sort(E_e_v0.begin(), E_e_v0.end());

            decltype(E_e_v0) tmp2(E_e_v0.size());
            iter = std::set_difference(E_f0.begin(), E_f0.end(), E_e_v0.begin(), E_e_v0.end(), tmp2.begin());
            tmp2.resize(Index(iter - tmp2.begin()));
            auto V_e1(m.get_adjacent_polytopes(pl_edge,pl_vertex, tmp2.at(0)));

            m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e0.at(1);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e1.at(0);
            m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);

            if(!(_iz_property_quadface(m, x, y, z, face_num)))
            {
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(0) = V_e0.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(1) = V_e1.at(0);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(2) = V_e0.at(1);
              m.get_topologies().at(ipi_face_vertex).at(face_num).at(3) = V_e1.at(1);
            }
            fp.push_back(face_num);

          }

        }


    };

  }
}
#endif
