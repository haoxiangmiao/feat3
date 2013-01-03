#include <kernel/util/mesh_reader.hpp>
#include <test_system/test_system.hpp>
#include <sstream>

using namespace FEAST;
using namespace FEAST::TestSystem;

/**
 * \brief Test class for the MeshReader class.
 *
 * \test Tests the MeshReader class.
 *
 * \author Constantin Christof
 */

class MeshReaderTest
  : public TaggedTest<Archs::None, Archs::None>
{
public:
  MeshReaderTest() :
    TaggedTest<Archs::None, Archs::None>("mesh_reader_test")
  {
  }

  void test_0() const
  {
    using namespace std;
    stringstream ioss;

    // let's write an (awfully ugly) mesh file into the stream
    ioss << "blabla nonsense" << endl;
    ioss << "in front of the file &%$&/%&" << endl;
    ioss << "should be ignored" << endl;
    ioss << "    " << endl;
    ioss << "<feast_mesh_file>" << endl;
    ioss << "<header>  " << endl;
    ioss << " version 1" << endl;
    ioss << " chart_file unit_quad_chart.txt" << endl;
    ioss << " submeshes 1" << endl;
    ioss << " cellsets 0" << endl;
    ioss << "</header>" << endl;
    ioss << "   <info>    " << endl;
    ioss << " This file contains a simple unit-square mesh with one quadrilateral" << endl;
    ioss << "and a 1D submesh for the parameterisation of the outer boundary." << endl;
    ioss << "</info>" << endl;
    ioss << "<mesh>" << endl;
    ioss << " <header>" << endl;
    ioss << "  type conformal" << endl;
    ioss << "  coords 2" << endl;
    ioss << "  shape quad" << endl;
    ioss << " </header>" << endl;
    ioss << " <info>" << endl;
    ioss << "Friss meine shorts!" << endl;
    ioss << " </info>" << endl;
    ioss << " <counts>" << endl;
    ioss << "  verts 4" << endl;
    ioss << " " << endl;
    ioss << "" << endl;
    ioss << "  quads 1" << endl;
    ioss << "  edges 4" << endl;
    ioss << " </counts>" << endl;
    ioss << " <coords>" << endl;
    ioss << " 0.0 0.0" << endl;
    ioss << "  1.0 0.0" << endl;
    ioss << "0.0 1.0" << endl;
    ioss << "  1.0 1.0       " << endl;
    ioss << " </coords>" << endl;
    ioss << " <vert@edge>" << endl;
    ioss << "  0 1" << endl;
    ioss << "2   3" << endl;
    ioss << "  0   2   " << endl;
    ioss << "1                    3" << endl;
    ioss << " </vert@edge>" << endl;
    ioss << " <vert@quad>" << endl;
    ioss << "  0 1 2 3" << endl;
    ioss << " </vert@quad>" << endl;
    ioss << "</mesh>" << endl;
    // submesh
    ioss << "<submesh>   " << endl;
    ioss << " <header>" << endl;
    ioss << "  name outer" << endl;
    ioss << "  parent root" << endl;
    ioss << "  type conformal  " << endl;
    ioss << "  shape edge " << endl;
    ioss << "  coords    1" << endl;
    ioss << "</header>" << endl;
    ioss << " <info>  " << endl;
    ioss << " This is a submesh that 42" << endl;
    ioss << " </info> " << endl;
    ioss << " <counts>" << endl;
    ioss << "   verts    5   " << endl;
    ioss << "  edges 4" << endl;
    ioss << "  </counts>" << endl;
    ioss << " <coords> " << endl;
    ioss << "  0.0" << endl;
    ioss << "  1.0" << endl;
    ioss << "  2.0" << endl;
    ioss << "3.0" << endl;
    ioss << "  4.0 " << endl;
    ioss << " </coords> " << endl;
    ioss << " <vert@edge>" << endl;
    ioss << " 0  1" << endl;
    ioss << "1  2   " << endl;
    ioss << "  2 3" << endl;
    ioss << "3 4" << endl;
    ioss << "</vert@edge>" << endl;
    ioss << " <vert_idx>" << endl;
    ioss << "0" << endl;
    ioss << "1" << endl;
    ioss << "2" << endl;
    ioss << "3" << endl;
    ioss << "0" << endl;
    ioss << " </vert_idx>" << endl;
    ioss << " <edge_idx>" << endl;
    ioss << "0" << endl;
    ioss << "3" << endl;
    ioss << "1" << endl;
    ioss << "2" << endl;
    ioss << " </edge_idx>" << endl;
    ioss << "</submesh>" << endl;
    ioss << "</feast_mesh_file>" << endl;
    ioss << "BleBlaBlu ignored too" << endl;

    // parse the stream
    MeshReader reader;
    reader.parse_mesh_file(ioss);

    // check members
    TEST_CHECK_EQUAL(reader.get_version(), "1");
    TEST_CHECK_EQUAL(reader.get_chart_path(), "unit_quad_chart.txt");
    TEST_CHECK_EQUAL(reader.get_number_of_submeshes(), Index(1));
    TEST_CHECK_EQUAL(reader.get_number_of_cellsets(), Index(0));

    // check the cellset stack
    TEST_CHECK_EQUAL(reader.no_cellsets(), true);

    // check the mesh stack
    TEST_CHECK_EQUAL(reader.no_meshes(), false);

    // test the get_mesh function
    std::pair<MeshReader::MeshDataContainer, bool> root_mesh_pair;
    root_mesh_pair = reader.get_mesh("rooot");
    TEST_CHECK_EQUAL(root_mesh_pair.second, false);

    root_mesh_pair = reader.get_mesh("root");
    TEST_CHECK_EQUAL(root_mesh_pair.second, true);

    MeshReader::MeshDataContainer root_mesh = root_mesh_pair.first;

    // check the root mesh data
    TEST_CHECK_EQUAL(root_mesh.name , "root");
    TEST_CHECK_EQUAL(root_mesh.parent , "none");
    TEST_CHECK_EQUAL(root_mesh.chart , "none");
    TEST_CHECK_EQUAL(root_mesh.coord_version , "");
    TEST_CHECK_EQUAL(root_mesh.adjacency_version , "");
    TEST_CHECK_EQUAL(root_mesh.mesh_type , "conformal");
    TEST_CHECK_EQUAL(root_mesh.shape_type , "quad");
    TEST_CHECK_EQUAL(root_mesh.coord_per_vertex , Index(2));
    TEST_CHECK_EQUAL(root_mesh.vertex_number , Index(4));
    TEST_CHECK_EQUAL(root_mesh.edge_number , Index(4));
    TEST_CHECK_EQUAL(root_mesh.tria_number , Index(0));
    TEST_CHECK_EQUAL(root_mesh.quad_number , Index(1));
    TEST_CHECK_EQUAL(root_mesh.tetra_number , Index(0));
    TEST_CHECK_EQUAL(root_mesh.hexa_number, Index(0));

    TEST_CHECK_EQUAL(root_mesh.coord_path , "");
    TEST_CHECK_EQUAL(root_mesh.adj_path, "");

    // check the root mesh coordinates
    std::vector<std::vector<double> > coords = root_mesh.coords;

    // reference coordinates
    double coords_ref[] =
    {
      0.0, 0.0,
      1.0, 0.0,
      0.0, 1.0,
      1.0, 1.0
    };

    // loop through the coordinates
    Index count(0);
    bool error = false;
    for(Index i(0); i < coords.size(); ++i)
    {
      for(Index j(0); j < root_mesh.coord_per_vertex; ++j)
      {
        if((coords[i])[j] != coords_ref[count])
        {
          error = true;
          break;
        }
        ++count;
      }
      if(error)
      {
        break;
      }
    }

    // check if an error occured
    TEST_CHECK_EQUAL(error, false);

    // check the adjacencies
    std::vector<std::vector<Index> > adj_stack;

    // reference adjacencies
    Index adj_ref_01[] =
    {
      0, 1,
      2, 3,
      0, 2,
      1, 3
    };

    Index adj_ref_02[] =
    {
      0, 1, 2, 3
    };

    // check vertex at edge adjacencies
    adj_stack = root_mesh.adjacencies[0][1];
    count = 0;
    for(Index i(0); i < 4; ++i)
    {
      for(Index j(0); j < 2; ++j)
      {
        if((adj_stack[i])[j] != adj_ref_01[count])
        {
          error = true;
          break;
        }
        ++count;
      }
      if(error)
      {
        break;
      }
    }
    TEST_CHECK_EQUAL(error, false);

    // check vertex at quad adjacencies
    adj_stack = root_mesh.adjacencies[0][2];
    count = 0;
    for(Index j(0); j < 4; ++j)
    {
      if((adj_stack[0])[j] != adj_ref_02[count])
      {
        error = true;
        break;
      }
      ++count;
    }

    // check if an error occured
    TEST_CHECK_EQUAL(error, false);

    // check if the rest is emtpy
    TEST_CHECK_EQUAL((root_mesh.adjacencies[1][2]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.adjacencies[0][3]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.adjacencies[1][3]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.adjacencies[2][3]).empty(), true);

    // check parent indices
    TEST_CHECK_EQUAL((root_mesh.parent_indices[0]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.parent_indices[1]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.parent_indices[2]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.parent_indices[3]).empty(), true);

    //
    // check the sub mesh data
    //

    std::pair<MeshReader::MeshDataContainer, bool> sub_mesh_pair;
    sub_mesh_pair = reader.get_mesh("outer");
    TEST_CHECK_EQUAL(sub_mesh_pair.second, true);

    MeshReader::MeshDataContainer sub_mesh = sub_mesh_pair.first;
    TEST_CHECK_EQUAL(sub_mesh.name , "outer");
    TEST_CHECK_EQUAL(sub_mesh.parent , "root");
    TEST_CHECK_EQUAL(sub_mesh.chart , "");
    TEST_CHECK_EQUAL(sub_mesh.coord_version , "");
    TEST_CHECK_EQUAL(sub_mesh.adjacency_version , "");
    TEST_CHECK_EQUAL(sub_mesh.mesh_type , "conformal");
    TEST_CHECK_EQUAL(sub_mesh.shape_type , "edge");
    TEST_CHECK_EQUAL(sub_mesh.coord_per_vertex , Index(1));
    TEST_CHECK_EQUAL(sub_mesh.vertex_number , Index(5));
    TEST_CHECK_EQUAL(sub_mesh.edge_number , Index(4));
    TEST_CHECK_EQUAL(sub_mesh.tria_number , Index(0));
    TEST_CHECK_EQUAL(sub_mesh.quad_number , Index(0));
    TEST_CHECK_EQUAL(sub_mesh.tetra_number , Index(0));
    TEST_CHECK_EQUAL(sub_mesh.hexa_number, Index(0));

    TEST_CHECK_EQUAL(sub_mesh.coord_path , "");
    TEST_CHECK_EQUAL(sub_mesh.adj_path, "");

    // check the sub mesh coordinates
    std::vector<std::vector<double> > coords_sub = sub_mesh.coords;

    // reference coordinates
    double coords_sub_ref[] =
    {
      0.0, 1.0, 2.0, 3.0, 4.0
    };

    // loop through the coordinates
    count = 0;
    error = false;
    for(Index i(0); i < coords_sub.size(); ++i)
    {
      for(Index j(0); j < sub_mesh.coord_per_vertex; ++j)
      {
        if((coords_sub[i])[j] != coords_sub_ref[count])
        {
          error = true;
          break;
        }
        ++count;
      }
      if(error)
      {
        break;
      }
    }

    // check if an error occured
    TEST_CHECK_EQUAL(error, false);

    // check the adjacencies
    std::vector<std::vector<Index> > adj_stack_sub;

    // reference adjacencies
    Index adj_sub_ref_01[] =
    {
      0, 1,
      1, 2,
      2, 3,
      3, 4
    };

    // check vertex at edge adjacencies
    adj_stack_sub = sub_mesh.adjacencies[0][1];
    count = 0;
    for(Index i(0); i < 4; ++i)
    {
      for(Index j(0); j < 2; ++j)
      {
        if((adj_stack_sub[i])[j] != adj_sub_ref_01[count])
        {
          error = true;
          break;
        }
        ++count;
      }
      if(error)
      {
        break;
      }
    }
    TEST_CHECK_EQUAL(error, false);

    // check if the rest is emtpy
    TEST_CHECK_EQUAL((sub_mesh.adjacencies[0][2]).empty(), true);
    TEST_CHECK_EQUAL((sub_mesh.adjacencies[1][2]).empty(), true);
    TEST_CHECK_EQUAL((sub_mesh.adjacencies[0][3]).empty(), true);
    TEST_CHECK_EQUAL((sub_mesh.adjacencies[1][3]).empty(), true);
    TEST_CHECK_EQUAL((sub_mesh.adjacencies[2][3]).empty(), true);

    // check parent indices of the sub mesh
    std::vector<Index> par0 = sub_mesh.parent_indices[0];
    std::vector<Index> par1 = sub_mesh.parent_indices[1];

    // reference
    Index par0_ref[] =
    {
      0, 1, 2, 3, 0
    };

    Index par1_ref[] =
    {
      0, 3, 1, 2
    };

    // check vertex parents
    error = false;
    for(Index j(0); j < 5; ++j)
    {
      if(par0_ref[j] != par0[j])
      {
        error = true;
        break;
      }
    }
    TEST_CHECK_EQUAL(error, false);

    // check edge parents
    for(Index j(0); j < 4; ++j)
    {
      if(par1_ref[j] != par1[j])
      {
        error = true;
        break;
      }
    }
    TEST_CHECK_EQUAL(error, false);

    // check if the rest is empty
    TEST_CHECK_EQUAL((root_mesh.parent_indices[2]).empty(), true);
    TEST_CHECK_EQUAL((root_mesh.parent_indices[3]).empty(), true);

    // ok, everything is right
  } // test_0

  void test_1() const
  {
    using namespace std;
    stringstream ioss1, ioss2, ioss3, ioss4;

    // ioss1: missing "</mesh>" flag
    ioss1 << "<feast_mesh_file>" << endl;
    ioss1 << "<header>  " << endl;
    ioss1 << "  version 1" << endl;
    ioss1 << "  submeshes 0" << endl;
    ioss1 << "  cellsets 0" << endl;
    ioss1 << "</header>" << endl;
    ioss1 << "<mesh>" << endl;
    ioss1 << "  <header>" << endl;
    ioss1 << "    type conformal" << endl;
    ioss1 << "    coords 2" << endl;
    ioss1 << "    shape quad" << endl;
    ioss1 << "  </header>" << endl;
    ioss1 << "  <counts>" << endl;
    ioss1 << "    verts 4" << endl;
    ioss1 << "    quads 1" << endl;
    ioss1 << "    edges 4" << endl;
    ioss1 << "  </counts>" << endl;
    ioss1 << "  <coords>" << endl;
    ioss1 << "    0.0 0.0" << endl;
    ioss1 << "    1.0 0.0" << endl;
    ioss1 << "    0.0 1.0" << endl;
    ioss1 << "    1.0 1.0" << endl;
    ioss1 << "  </coords>" << endl;
    ioss1 << "</feast_mesh_file>" << endl;

    // parse the stream and check if an exception is thrown
    MeshReader reader1;
    TEST_CHECK_THROWS(reader1.parse_mesh_file(ioss1), SyntaxError);

    // ioss2: wrong number of coordinates
    ioss2 << "<feast_mesh_file>" << endl;
    ioss2 << "<header>  " << endl;
    ioss2 << "  version 1" << endl;
    ioss2 << "  submeshes 0" << endl;
    ioss2 << "  cellsets 0" << endl;
    ioss2 << "</header>" << endl;
    ioss2 << "<mesh>" << endl;
    ioss2 << "  <header>" << endl;
    ioss2 << "    type conformal" << endl;
    ioss2 << "    coords 2" << endl;
    ioss2 << "    shape quad" << endl;
    ioss2 << "  </header>" << endl;
    ioss2 << "  <counts>" << endl;
    ioss2 << "    verts 4" << endl;
    ioss2 << "    quads 1" << endl;
    ioss2 << "    edges 4" << endl;
    ioss2 << "  </counts>" << endl;
    ioss2 << "  <coords>" << endl;
    ioss2 << "    0.0 0.0" << endl;
    ioss2 << "    1.0 0.0" << endl;
    ioss2 << "    0.0 1.0" << endl;
    ioss2 << "    1.0 1.0 42.23" << endl;
    ioss2 << "  </coords>" << endl;
    ioss2 << "</mesh>" << endl;
    ioss2 << "</feast_mesh_file>" << endl;

    // parse the stream and check if an exception is thrown
    MeshReader reader2;
    TEST_CHECK_THROWS(reader2.parse_mesh_file(ioss2), SyntaxError);

    // ioss3: missing version entry
    ioss3 << "<feast_mesh_file>" << endl;
    ioss3 << "<header>  " << endl;
    ioss3 << "  version   " << endl;
    ioss3 << "  submeshes 0" << endl;
    ioss3 << "  cellsets 0" << endl;
    ioss3 << "</header>" << endl;
    ioss3 << "<mesh>" << endl;
    ioss3 << "  <header>" << endl;
    ioss3 << "    type conformal" << endl;
    ioss3 << "    coords 2" << endl;
    ioss3 << "    shape quad" << endl;
    ioss3 << "  </header>" << endl;
    ioss3 << "  <counts>" << endl;
    ioss3 << "    verts 4" << endl;
    ioss3 << "    quads 1" << endl;
    ioss3 << "    edges 4" << endl;
    ioss3 << "  </counts>" << endl;
    ioss3 << "  <coords>" << endl;
    ioss3 << "    0.0 0.0" << endl;
    ioss3 << "    1.0 0.0" << endl;
    ioss3 << "    0.0 1.0" << endl;
    ioss3 << "    1.0 1.0" << endl;
    ioss3 << "  </coords>" << endl;
    ioss3 << "</mesh>" << endl;
    ioss3 << "</feast_mesh_file>" << endl;

    // parse the stream and check if an exception is thrown
    MeshReader reader3;
    TEST_CHECK_THROWS(reader3.parse_mesh_file(ioss3), SyntaxError);

    // ioss4: nonsense
    ioss4 << "<feast_mesh_file>" << endl;
    ioss4 << "<header>  " << endl;
    ioss4 << "  version 42" << endl;
    ioss4 << "  submeshes 0" << endl;
    ioss4 << "  cellsets 0" << endl;
    ioss4 << "</header>" << endl;
    ioss4 << "<mesh>" << endl;
    ioss4 << "  <header>" << endl;
    ioss4 << "    type conformal" << endl;
    ioss4 << "    coords 2" << endl;
    ioss4 << "    shape quad" << endl;
    ioss4 << "  </header>" << endl;
    ioss4 << "  <counts>" << endl;
    ioss4 << "    verts 4" << endl;
    ioss4 << "    quads 1" << endl;
    ioss4 << "    edges 4" << endl;
    ioss4 << "  </counts>" << endl;
    ioss4 << "  <coords>" << endl;
    ioss4 << "    0.0 0.0" << endl;
    ioss4 << "    1.0 0.0" << endl;
    ioss4 << "    0.0 1.0" << endl;
    ioss4 << "    1.0 1.0" << endl;
    ioss4 << "  </coords> blub" << endl;
    ioss4 << "</mesh>" << endl;
    ioss4 << "</feast_mesh_file>" << endl;

    // parse the stream and check if an exception is thrown
    MeshReader reader4;
    TEST_CHECK_THROWS(reader4.parse_mesh_file(ioss4), SyntaxError);

    // okay
  } // test_1

  virtual void run() const
  {
    // run test #0 (checks parsing)
    test_0();
    // run test #1 (checks if errors are found)
    test_1();
  }
} mesh_reader_test;
