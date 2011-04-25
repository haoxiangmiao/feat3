#pragma once
#ifndef KERN_MANAG_COMP_COORD_HPP
#define KERN_MANAG_COMP_COORD_HPP 1

// includes, system
#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

// includes, Feast
#include <kernel/base_header.hpp>
#include <kernel/util/exception.hpp>
#include <kernel/util/assertion.hpp>
#include <kernel/error_handler.hpp>
#include <kernel/manager_comp.hpp>
#include <kernel/load_balancer.hpp>
#include <kernel/base_mesh/file_parser.hpp>
#include <kernel/base_mesh/bm.hpp>

/// FEAST namespace
namespace FEAST
{
  /**
  * \brief defines manager of a compute process group on the coordinator process
  *
  * \author Hilmar Wobker
  */
  template<
    unsigned char space_dim_,
    unsigned char world_dim_>
  class ManagerCompCoord
    : public ManagerComp<space_dim_, world_dim_>
  {

  private:

    /* *****************
    * member variables *
    *******************/
    /**
    * \brief vector of graph structures representing the process topology within the work groups
    *
    * These are only known to the coordinator process. The worker processes only get 'their' portions of the global
    * graph.
    */
    std::vector<Graph*> _graphs;

    /// base mesh the manager works with
    BaseMesh::BM<space_dim_, world_dim_>* _base_mesh;

    /// pointer to the load balancer of the process group
    LoadBalancer<space_dim_, world_dim_>* _load_balancer;

  public:

    /* *************************
    * constructor & destructor *
    ***************************/
    /// CTOR
    ManagerCompCoord(ProcessGroup* process_group)
      : ManagerComp<space_dim_, world_dim_>(process_group),
        _base_mesh(nullptr),
        _load_balancer(nullptr)
    {
      CONTEXT("ManagerCompCoord::ManagerCompCoord()");
    }

    /// DTOR
    ~ManagerCompCoord()
    {
      CONTEXT("ManagerCompCoord::~ManagerCompCoord()");
      if (_base_mesh != nullptr)
      {
        delete _base_mesh;
      }

      // assume that the _graphs vector only holds copies of the graph object pointers and that the graph objects
      // themselves are destroyed where they were created
      _graphs.clear();

    }


    /* ******************
    * getters & setters *
    ********************/
    /**
    * \brief getter for the vector of extended work groups
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return reference to the vector of extended work groups
    */
    inline const std::vector<WorkGroupExt*>& work_groups() const
    {
      CONTEXT("ManagerCompCoord::work_groups()");
      return ManagerComp<space_dim_, world_dim_>::_work_groups;
    }


    /**
    * \brief getter for the number of work groups
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return number of work groups
    */
    inline unsigned int num_work_groups() const
    {
      CONTEXT("ManagerCompCoord::num_work_groups()");
      return ManagerComp<space_dim_, world_dim_>::_num_work_groups;
    }


    /**
    * \brief setter for the number of work groups
    *
    * \param[in] number of subgroups
    */
    inline void set_num_work_groups(unsigned int num_work_groups)
    {
      CONTEXT("ManagerCompCoord::set_num_work_groups()");
      ManagerComp<space_dim_, world_dim_>::_num_work_groups = num_work_groups;
    }

    /**
    * \brief getter for the array of number of processes per extended work group
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return pointer to array of number of processes per work group
    */
    inline unsigned int* num_proc_in_work_group() const
    {
      CONTEXT("ManagerCompCoord::num_proc_in_work_group()");
      return ManagerComp<space_dim_, world_dim_>::_num_proc_in_work_group;
    }


    /**
    * \brief setter for the array of number of processes per extended work group
    *
    * \param[in] pointer to array of number of processes per extended work group
    */
    inline void set_num_proc_in_work_group(unsigned int* num_proc_in_work_group)
    {
      CONTEXT("ManagerCompCoord::set_num_proc_in_work_group()");
      ManagerComp<space_dim_, world_dim_>::_num_proc_in_work_group = num_proc_in_work_group;
    }


    /**
    * \brief getter for the array indicating whether the ext. work groups contain an extra process for the coordinator
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return pointer to array indicating whether the ext. work groups contain an extra process for the coordinator
    */
    inline unsigned char* group_contains_extra_coord() const
    {
      CONTEXT("ManagerCompCoord::group_contains_extra_coord()");
      return ManagerComp<space_dim_, world_dim_>::_group_contains_extra_coord;
    }


    /**
    * \brief setter for the array indicating whether the ext. work groups contain an extra process for the coordinator
    *
    * \param[in] pointer to array indicating whether the ext. work groups contain an extra process for the coordinator
    */
    inline void set_group_contains_extra_coord(unsigned char* group_contains_extra_coord)
    {
      CONTEXT("ManagerCompCoord::set_group_contains_extra_coord()");
      ManagerComp<space_dim_, world_dim_>::_group_contains_extra_coord = group_contains_extra_coord;
    }


    /**
    * \brief getter for the 2D array of ext. work group ranks
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return pointer to 2D array of ext. work group ranks
    */
    inline int** work_group_ranks() const
    {
      CONTEXT("ManagerCompCoord::work_group_ranks()");
      return ManagerComp<space_dim_, world_dim_>::_work_group_ranks;
    }


    /**
    * \brief setter for the 2D array of ext. work group ranks
    *
    * \param[in] pointer to 2D array of ext. work group ranks
    */
    inline void set_work_group_ranks(int** work_group_ranks)
    {
      CONTEXT("ManagerCompCoord::set_work_group_ranks()");
      ManagerComp<space_dim_, world_dim_>::_work_group_ranks = work_group_ranks;
    }


    /**
    * \brief getter for the array indicating to which work groups this process belongs
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return array indicating to which work groups this process belongs
    */
    inline bool* belongs_to_group() const
    {
      CONTEXT("ManagerCompCoord::belongs_to_group()");
      return ManagerComp<space_dim_, world_dim_>::_belongs_to_group;
    }


    /**
    * \brief getter for the process group
    *
    * Just a shortcut to save typing of 'ManagerComp<space_dim_, world_dim_>::'
    *
    * \return ProcessGroup pointer #_process_group
    */
    inline ProcessGroup* process_group() const
    {
      CONTEXT("ManagerCompCoord::process_group()");
      return ManagerComp<space_dim_, world_dim_>::_process_group;
    }


    /**
    * \brief getter for the base mesh
    *
    * \return BaseMesh::BM<space_dim_, world_dim_> pointer #_base_mesh
    */
    inline BaseMesh::BM<space_dim_, world_dim_>* base_mesh() const
    {
      CONTEXT("ManagerCompCoord::base_mesh()");
      return _base_mesh;
    }


    /**
    * \brief setter for the pointer to the load balancer
    *
    * \param[in] pointer to load balancer object
    */
    inline void set_load_balancer(LoadBalancer<space_dim_, world_dim_>* load_bal)
    {
      CONTEXT("ManagerCompCoord::set_load_balancer()");
      _load_balancer = load_bal;
    }


    /* *****************
    * member functions *
    *******************/
    /// read in a mesh file and set up base mesh
    void read_mesh(std::string const & mesh_file)
    {
      CONTEXT("ManagerCompCoord::read_mesh()");
      // the mesh is read by the process group coordinator

      _base_mesh = new BaseMesh::BM<space_dim_, world_dim_>();
      BaseMesh::FileParser<space_dim_, world_dim_> parser;
      Logger::log_master("Reading mesh file " + mesh_file + "...\n", Logger::SCREEN_FILE);
      try
      {
        parser.parse(mesh_file, _base_mesh);
      }
      catch(Exception& e)
      {
        // abort the program
        ErrorHandler::exception_occured(e);
      }
      // set cell numbers (equal to indices since all cells are active)
      _base_mesh->set_cell_numbers();
      // create base mesh's graph structure
      _base_mesh->create_graph();
      // print base mesh
      std::string s = _base_mesh->print();
      Logger::log_master(s, Logger::SCREEN);
      Logger::log(s);
      // validate base mesh
      _base_mesh->validate(Logger::file);
    }


    /**
    * \brief function that sets up (extended) work groups basing on the provided information
    *
    * This function is called on all processes of the manager's compute process group.
    *
    * \author Hilmar Wobker
    */
    void create_work_groups()
    {
      CONTEXT("ManagerCompCoord::create_work_groups()");

      // set data/pointers on the coordinator process (where the data is already available)
      set_num_work_groups(_load_balancer->num_work_groups());
      set_num_proc_in_work_group(_load_balancer->num_proc_in_work_group());
      set_group_contains_extra_coord(_load_balancer->group_contains_extra_coord());
      set_work_group_ranks(_load_balancer->work_group_ranks());

      // now the coordinator broadcasts the relevant data to the other processes, that is:
      //   - _num_work_groups
      //   - _num_proc_in_work_group
      //   - _group_contains_extra_coord

      unsigned int temp(num_work_groups());
      int mpi_error_code = MPI_Bcast(&temp, 1, MPI_UNSIGNED, process_group()->rank_coord(), process_group()->comm());
      validate_error_code_mpi(mpi_error_code, "MPI_Bcast");

      mpi_error_code = MPI_Bcast(num_proc_in_work_group(), num_work_groups(), MPI_UNSIGNED,
                                 process_group()->rank_coord(), process_group()->comm());
      validate_error_code_mpi(mpi_error_code, "MPI_Bcast");

      mpi_error_code = MPI_Bcast(group_contains_extra_coord(), num_work_groups(), MPI_UNSIGNED_CHAR,
                                 process_group()->rank_coord(), process_group()->comm());
      validate_error_code_mpi(mpi_error_code, "MPI_Bcast");

      // call routine for creating work groups (within this routine the array _work_group_ranks is transferred)
      ManagerComp<space_dim_, world_dim_>::_create_work_groups();
    } // create_work_groups()


    /**
    * \brief sends the relevant parts of the global graph to the corresponding workers of each work group
    *
    * The local graphs tell the workers with which workers of their work group they have to communicate.
    * This function must be called on the coordinator process of the process group, at the same time all
    * non-coordinator processes must call the function receive_and_set_graphs(). Before this function can be used, the
    * function create_work_groups() must have been called.
    *
    * \param[in] graphs
    * array of Graph pointers representing the connectivity of work group processes
    *
    * \sa receive_and_set_graphs()
    *
    * \author Hilmar Wobker
    */
    void transfer_graphs_to_workers(Graph** graphs)
    {
      CONTEXT("ManagerCompCoord::transfer_graphs_to_workers()");

      for(unsigned int igroup(0) ; igroup < num_work_groups() ; ++igroup)
      {
        if(belongs_to_group()[igroup])
        {
          ASSERT(work_groups()[igroup]->is_coordinator(), "Routine must be called on the coordinator process.");
          // number of neighbours of the graph node corresponding to this process (use unsigned int datatype here
          // instead of index_glob_t since MPI routines expect it)
          unsigned int num_neighbours_local;
          index_glob_t* neighbours_local(nullptr);
          int rank_coord = work_groups()[igroup]->rank_coord();

          // set the graph pointer for the current work group
          _graphs.push_back(graphs[igroup]);

          // since the MPI routines used below expect integer arrays, we have to copy two index_glob_t arrays
          // within the graph structures to corresponding int arrays
// COMMENT_HILMAR: Gibt es eine Moeglichkeit, das zu vermeiden? Ein reinterpret_cast<int*>(unsigned long) funzt nicht!
          unsigned int* num_neighbours_aux;
          unsigned int* index_aux;

          index_glob_t num_nodes;
          if(work_groups()[igroup]->contains_extra_coordinator())
          {
            // In case there is an extra coordinator process, we have to add one pseudo node to the graph and the
            // index array must be modified correspondingingly. This pseudo node corresponds to the coordinator
            // process itself which has to be included in the call of MPI_Scatterv(...). It has to appear at the
            // position in the arrays num_neighbours_aux[] and index_aux[] corresponding to the rank of the
            // coordinator. Although we know, that this is rank 0, we do not explicitly exploit this information here
            // since this might be changed in future.
            num_nodes = _graphs[igroup]->num_nodes() + 1;
            index_aux = new unsigned int[num_nodes + 1];
            // copy the first part of the graphs's index array to the aux array, performing implicit cast from
            // index_glob_t to unsigned int
            for(index_glob_t i(0) ; i < (index_glob_t)rank_coord+1 ; ++i)
            {
              index_aux[i] = _graphs[igroup]->index()[i];
            }
            // insert the pseudo node
            index_aux[rank_coord+1] = index_aux[rank_coord];
            // copy the remaining part of the graphs's index array to the aux array, performing implicit cast from
            // index_glob_t to unsigned int
            for(index_glob_t i(rank_coord+1) ; i < num_nodes ; ++i)
            {
              index_aux[i+1] = _graphs[igroup]->index()[i];
            }
          }
          else
          {
            // in case there is no extra coordinator process, the number of neighbours equals the number of nodes
            // in the graph and the index array does not have to be modified
            num_nodes = _graphs[igroup]->num_nodes();
            index_aux = new unsigned int[num_nodes + 1];
            // copy the graphs's index array to the aux array, performing implicit cast from index_glob_t to
            // unsigned int
            for(index_glob_t i(0) ; i < num_nodes+1 ; ++i)
            {
              index_aux[i] = _graphs[igroup]->index()[i];
            }
          }

          // now determine the number of neighbours per node (eventually including the pseudo node for the extra
          // coordinator process)
          num_neighbours_aux = new unsigned int[num_nodes];
          for(index_glob_t i(0) ; i < num_nodes ; ++i)
          {
            num_neighbours_aux[i] = index_aux[i+1] - index_aux[i];
          }

          if(work_groups()[igroup]->contains_extra_coordinator())
          {
            // send the number of neighbours to the non-coordinator processes (use MPI_IN_PLACE to indicate that the
            // coordinator does not receive/store any data)
            MPI_Scatter(num_neighbours_aux, 1, MPI_UNSIGNED, MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
                        rank_coord, work_groups()[igroup]->comm());
            // send the neighbours to the non-coordinator processes
            MPI_Scatterv(_graphs[igroup]->neighbours(), reinterpret_cast<int*>(num_neighbours_aux),
                         reinterpret_cast<int*>(index_aux), MPIType<index_glob_t>::value(), MPI_IN_PLACE, 0,
                         MPI_DATATYPE_NULL, rank_coord, work_groups()[igroup]->comm());
          }
          else
          {
            // When there is no extra coordinator process, then the coordinator is part of the compute work group and
            // also sends data to itself.

            // scatter the number of neighbours to the non-coordinator processes and to the coordinator process itself
            MPI_Scatter(reinterpret_cast<int*>(num_neighbours_aux), 1, MPI_UNSIGNED, &num_neighbours_local, 1,
                        MPI_INTEGER, rank_coord, work_groups()[igroup]->comm());
            neighbours_local = new index_glob_t[num_neighbours_local];
            // scatter the neighbours to the non-coordinator processes and to the coordinator process itself
            MPI_Scatterv(_graphs[igroup]->neighbours(), reinterpret_cast<int*>(num_neighbours_aux),
                         reinterpret_cast<int*>(index_aux), MPIType<index_glob_t>::value(), neighbours_local,
                         num_neighbours_local, MPIType<index_glob_t>::value(), rank_coord,
                         work_groups()[igroup]->comm());
          }
          // delete aux. arrays again
          delete [] num_neighbours_aux;
          delete [] index_aux;

          // now create distributed graph structure within the compute work groups. The coordinater only performs this
          // task when it is not an extra coordinator process, i.e., only if it actually is a worker process.
          if (!work_groups()[igroup]->contains_extra_coordinator())
          {
            work_groups()[igroup]->work_group()->set_graph_distributed(num_neighbours_local, neighbours_local);
            // The array neighbours_local is copied inside the constructor of the distributed graph object, hence it
            // can be deallocated again.
            delete [] neighbours_local;
          }
        } // if(belongs_to_group()[igroup])
      } // for(unsigned int igroup(0) ; igroup < num_work_groups() ; ++igroup)

// TODO: remove this test code here and call it from somewhere else
      // test local neighbourhood communication
      for(unsigned int igroup(0) ; igroup < num_work_groups() ; ++igroup)
      {
        if (belongs_to_group()[igroup] && !work_groups()[igroup]->contains_extra_coordinator())
        {
          work_groups()[igroup]->work_group()->do_exchange();
        }
      }
    } // transfer_graphs_to_workers()
  };
} // namespace FEAST

#endif // guard KERN_MANAG_COMP_COORD_HPP
