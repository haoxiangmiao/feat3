#pragma once
#ifndef KERNEL_GRAPH_HPP
#define KERNEL_GRAPH_HPP 1

// includes, system
#include <iostream>
#include <stdlib.h>

namespace FEAST
{

  /**
  * \brief class providing a graph data structure for defining connectivity of subdomains / matrix patches / processes
  *
  * This data structure is very similar to that described in the MPI-2.2 standard (p. 250ff) for creating process
  * topologies (for the difference, see description of member #_index).
  *
  * In the case this data structure is used for storing the MPI communication graph, it is a \em global representation,
  * i.e. a process storing this graph knows the \em complete communication graph. (For a distributed representation, see
  * data structure GraphDistributed.) To construct a corresponding MPI communicator, one can use the function
  *   int MPI_Graph_create(MPI_Comm comm_old, int num_nodes, int *_index, int *_neighbours, int reorder,
  *                        MPI_Comm *comm_graph)
  * (see MPI-2.2 standard, p. 250). When passing the array #_index to this routine, remember to omit the first entry
  * (see description of the array #_index).
  *
  * The data structure can also be used to create distributed graph data structures via the MPI routine
  * MPI_Dist_graph_create(...), (see MPI-2.2 standard, example 7.3 on page 256).
  *
  * COMMENT_HILMAR: This is only a very rough first version, which will be surely adapted to our needs...
  *
  * COMMENT_HILMAR: This graph structure is the most general one. When it comes to MPI communication, we surely have to
  *   distinguish edge neighbours and diagonal neighbours.
  *
  * \author Hilmar Wobker
  */
  class Graph
  {

  private:

    /* *****************
    * member variables *
    *******************/
    /// number of nodes in the graph, which are numbered from 0 to num_nodes-1
    unsigned int const _num_nodes;

    /**
    * \brief access information for the array #_neighbours
    *
    * The i-th entry of this array stores the total number of neighbours of the graph nodes 0, ..., i-1, where
    * index[0] = 0. So, the i-th entry gives the position in the array #_neighbours, in which the subarray storing the
    * node neighbours of node i starts. The degree (=number of neighbours) of node i is given by
    * _index[i+1] - _index[i]. (Difference to the data structure described in the MPI-2.2 standard: There, the 0-th
    * entry is omitted. However, adding this entry eases array access since the first node does not have to be handled
    * in a special way.) For an example see #_neighbours.
    *
    * Dimension: [#_num_nodes+1]
    */
    unsigned int* _index;

    /**
    * \brief node neighbours within the graph (i.e. edges of the graph), represented as a list of node numbers
    *
    * The neighbours of node i are stored in the subarray _neighbours[#_index[i]], ..., _neighbours[#_index[i+1]-1].
    * The order of the neighbours within the subarrays is arbitrary.
    *
    * Example: domain consisting of 7 subdomains, each subdomain is a node in the graph. Graph edges represent
    * neighbouring subdomains, including diagonal neighbours.
    *   -----------------
    *   | 0 | 1 | 2 | 3 |
    *   -----------------
    *   | 4 | 5 |
    *   ---------
    *   | 6 |
    *   -----
    *   nodes     neighbours
    *   0         4,5,1
    *   1         0,4,5,2
    *   2         1,5,3
    *   3         2
    *   4         0,1,5,6
    *   5         0,1,2,4,6
    *   6         4,5
    * _num_nodes = 7
    * _index      = [0,        3,           7,      10, 11,          15,          20,    22]
    * _neighbours = [4, 5, 1,  0, 4, 5, 2,  1, 5, 3, 2,  0, 1, 5, 6,  0, 1, 2, 4,  6, 4,  5]
    *
    * Dimension: [total number of neighbours] = [number of edges] = [#_index[#_num_nodes]]
    */
    unsigned int* _neighbours;


  public:

    /* *****************
    * member variables *
    *******************/

    /* *************************
    * constructor & destructor *
    ***************************/
    /// CTOR
    Graph(
      unsigned int const num_nodes,
      unsigned int* const index,
      unsigned int* const neighbours
      )
      : _num_nodes(num_nodes),
        _index(nullptr),
        _neighbours(nullptr)
    {
      CONTEXT("Graph::Graph()");
      // copy index array
      _index = new unsigned int[num_nodes+1];
      for(unsigned int i(0) ; i < num_nodes+1 ; ++i)
      {
        _index[i] = index[i];
      }

      // copy neighbour array (its size is given by _index[_num_nodes])
      _neighbours = new unsigned int[_index[_num_nodes]];
      for(unsigned int i(0) ; i < _index[_num_nodes] ; ++i)
      {
        _neighbours[i] = neighbours[i];
      }
    }

    /// DTOR
    ~Graph()
    {
      CONTEXT("Graph::~Graph()");
      delete [] _index;
      _index = nullptr;
      delete [] _neighbours;
      _neighbours = nullptr;
    }

    /* ******************
    * getters & setters *
    ********************/
    /**
    * \brief getter for the number of nodes
    *
    * \return number of nodes #_num_nodes
    */
    inline unsigned int num_nodes() const
    {
      CONTEXT("Graph::num_nodes()");
      return _num_nodes;
    }

    /**
    * \brief getter for the index array
    *
    * \return pointer to the index array #_index
    */
    inline unsigned int* index() const
    {
      CONTEXT("Graph::index()");
      return _index;
    }

    /**
    * \brief getter for the neighbours array
    *
    * \return pointer to the edge array #_neighbours
    */
    inline unsigned int* neighbours() const
    {
      CONTEXT("Graph::neighbours()");
      return _neighbours;
    }

    /* *****************
    * member functions *
    *******************/
    /// print the graph to the given stream
    void print(std::ostream& stream) const
    {
      CONTEXT("Graph::print()");
      stream << "number of nodes: " << _num_nodes << std::endl;
      if (_num_nodes > 0)
      {
        stream << "node | degree | neighbours: " << std::endl;
        for(unsigned int i(0) ; i < _num_nodes ; ++i)
        {
          stream << i << " | " << _index[i+1] - _index[i];
          if (_index[i+1] - _index[i] > 0)
          {
            stream << " | " << _neighbours[_index[i]];
            for(unsigned int j(_index[i]+1) ; j < _index[i+1] ; ++j)
            {
              stream << ", " << _neighbours[j];
            }
          }
          stream << std::endl;
        }
      }
    }

    /// returns the graph print as string
    inline std::string print() const
    {
      CONTEXT("Graph::print()");
      std::ostringstream oss;
      print(oss);
      return oss.str();
    }
  }; // class Graph



  /**
  * \brief class providing a distributed graph data structure for defining connectivity of subdomains / matrix patches /
  *        processes
  *
  * This data structure represents the part of the global graph this process is associated with. It basically stores the
  * number of neighbours and the ranks of the neighbours. The data structure can be constructed from a global graph with
  * the help of the MPI functions
  *   MPI_Dist_graph_neighbors_count(...)
  * to get the number of neighbours and
  *   MPI_Dist_graph_neighbors(...)
  * to get the ranks of the neighbours. With the help of this data structure the global MPI topology graph can be
  * created via the function MPI_Dist_graph_create(...).
  *
  * COMMENT_HILMAR: This is only a very rough first version, which will be surely adapted to our needs...
  *
  * \author Hilmar Wobker
  */
  class GraphDistributed
  {

  private:

    /* *****************
    * member variables *
    *******************/
    /// number of neighbours
    unsigned int _num_neighbours;

    /**
    * \brief ranks of the neighbours
    *
    * Dimension: [#_num_neighbours]
    */
    unsigned int* _neighbours;


  public:

    /* *************************
    * constructor & destructor *
    ***************************/
    /// CTOR
    GraphDistributed(
      unsigned int const num_neighbours,
      unsigned int* const neighbours
      )
      : _num_neighbours(num_neighbours),
        _neighbours(nullptr)
    {
      CONTEXT("GraphDistributed::GraphDistributed()");
      // copy array of neighbours
      _neighbours = new unsigned int[num_neighbours];
      for(unsigned int i(0) ; i < num_neighbours ; ++i)
      {
        _neighbours[i] = neighbours[i];
      }
    }

    /// DTOR
    ~GraphDistributed()
    {
      CONTEXT("GraphDistributed::~GraphDistributed()");
      delete [] _neighbours;
      _neighbours = nullptr;
    }

    /* ******************
    * getters & setters *
    ********************/
    /**
    * \brief getter for the number of neighbours
    *
    * \return number of neighbours #_num_neighbours
    */
    inline unsigned int num_neighbours() const
    {
      CONTEXT("GraphDistributed::num_neighbours()");
      return _num_neighbours;
    }

    /**
    * \brief getter for the neighbour array
    *
    * \return pointer to the neibhbour array #_neighbours
    */
    inline unsigned int* neighbours() const
    {
      CONTEXT("GraphDistributed::neighbours()");
      return _neighbours;
    }

    /* *****************
    * member functions *
    *******************/
    /// prints the distributed graph to the given stream
    void print(std::ostream& stream) const
    {
      CONTEXT("GraphDistributed::print()");
      stream << "distributed graph: ";
      for(unsigned int i(0) ; i < _num_neighbours ; ++i)
      {
        stream << _neighbours[i] << " ";
      }
      stream << std::endl;
    }

    /// returns the distributed graph print as string
    inline std::string print() const
    {
      CONTEXT("GraphDistributed::print()");
      std::ostringstream oss;
      print(oss);
      return oss.str();
    }
  }; // class GraphDistributed
} // namespace FEAST

#endif // guard KERNEL_GRAPH_HPP
