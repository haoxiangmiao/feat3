#pragma once
#ifndef KERNEL_LOAD_BAL_HPP
#define KERNEL_LOAD_BAL_HPP 1

// includes, system
#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <vector>

// includes, Feast
#include <kernel/base_header.hpp>
#include <kernel/process.hpp>
#include <kernel/process_group.hpp>
#include <kernel/base_mesh.hpp>

/**
* \brief class defining a load balancer
*
* Each initial process group is organised by a load balancer. It runs on all processes of the process group, which
* eases work flow organisation significantly. There is one coordinator process which is the only one knowing the
* complete computational mesh. It is responsible for reading and distributing the mesh to the other processes, and for
* organising partitioning and load balancing (collect and process matrix patch statistics, ...). This coordinator is
* always the process with the largest rank within the process group.
*
* The user can choose if this coordinator process should also perform compute tasks (solving linear systems etc),
* or if it should be a dedicated load balancing / coordinator process doing nothing else. This means, the coordinator
* process and the dedicated load balancer process, if the latter exists, coincide.
*
* The user knows what each process group and its respective load balancer should do. He calls, e.g.,
* if(load_bal->group_id() == 0)
* {
*   load_bal->readMesh();
*   ...
* }
* else
* {
*   coffee_machine.start();
* }
*
* The load bal. with id 0 in the example above then
* 1) reads in the mesh (this is only done by the dedicated load balancer process or the coordinator, resp.)
* 2) builds the necessary WorkGroup objects (e.g., one WorkGroup for the fine mesh problems and one for the
*    coarse mesh problem) and creates corresponding MPI communicators. The worker with rank 0 in this communicator
*    is usually the coordinator which communicates with the master or the dedicated load balancer. The process
*    topologies of the work groups are then optimised by building corresponding graph structures. There are two cases:
*    a) There is a dedicated load balancer: The dedicated load balancer reads the mesh, creates work groups and and a
*       global graph structure for each work group. Then it distributes to each process of the work group the relevant
*       parts of the global graph. Each work group process then creates its local graph structure and calls
*       MPI_Dist_graph_create(...) to build the new MPI process topology in a distributed fashion.
*    b) There is no dedicated load balancer: Same as case a), but instead of the dedicated load balancer the coordinator
*       of the process group builds the global graph structure. In this case, it must be distinguished whether the
*       coordinator is part of the work group or not.
*    COMMENT_HILMAR: It might be more clever to also let the MPI implementation decide on which physical process the
*    dedicated load balancer should reside (instead of pinning it to the last rank in the process group). To improve
*    this is task of the ITMC.
*
* 3) tells each member of the work groups to create a Worker object (representing the Worker on this process itself)
*    and corresponding RemoteWorker objects (as members of the WorkGroup objects) representing the remote Worker
*    objects
* COMMENT_HILMAR: Probably, step 3) is skipped, i.e., there will be no extra Worker objects. Instead "the part of
* the WorkGroup living on this process" represents such a worker.
*
* 4) tells each work group which other work groups it has to communicate with via the communicator they all share
*    within the parent process group. E.g., the fine mesh work group has to send the restricted defect vector to the
*    coarse mesh work group, while the coarse mesh work group has to send the coarse mesh correction to the fine mesh
*    work group. Two such communicating work groups live either on the same process (internal communication = copy) or
*    on different processes (external communication = MPI send/recv). (See example below.)
*
* 5) sends corresponding parts of the mesh to the work groups
*
*     Example:
*
*     Distribution of submeshes to processes A-G on different levels (note that processes A-G are not necessarily
*     disjunct, i.e., several of them can refer to the same physical process, see cases a and b):
*
*     ---------------      ---------------      ---------------
*     |             |      |      |      |      |      |      |
*     |             |      |      |      |      |  D   |  G   |
*     |             |      |      |      |      |      |      |
*     |      A      |      |  B   |  C   |      ---------------
*     |             |      |      |      |      |      |      |
*     |             |      |      |      |      |  E   |  F   |
*     |             |      |      |      |      |      |      |
*     ---------------      ---------------      ---------------
*       level 0               level 1              levels 2-L
*
*     * case a, four physical processes:
*       process group rank:  0  1  2  3
*              WorkGroup 2:  D  E  F  G         (four WorkGroup processes for the problems on level 2-L)
*              WorkGroup 1:  B     C            (two WorkGroup processes for the problem on level 1)
*              WorkGroup 0:  A                  (one WorkGroup process for the coarse mesh problem on level 0)
*
*       Communication:
*       A <--> B (internal, rank 0) A <--> C (external, ranks 0+2)
*       B <--> D (internal, rank 0) B <--> E (external, ranks 0+1)
*       C <--> F (internal, rank 2) C <--> G (external, ranks 2+3)
*
*     * case b, five physical processes::
*       process group rank:  0  1  2  3  4
*              WorkGroup 2:     D  E  F  G
*              WorkGroup 1:     B     C
*              WorkGroup 0:  A
*
*       Communication:
*       A <--> B (external, ranks 0+1) A <--> C (external, ranks 0+3)
*       B <--> D (internal, rank 1) B <--> E (external, ranks 1+2)
*       C <--> F (internal, rank 3) C <--> G (external, ranks 3+4)
*
*     * case c, seven physical processes:
*       process group rank:  0  1  2  3  4  5  6
*              WorkGroup 2:           D  E  F  G
*              WorkGroup 1:     B  C
*              WorkGroup 0:  A
*
*       Communication:
*       A <--> B (external, ranks 0+1) A <--> C (external, ranks 0+2)
*       B <--> D (external, ranks 1+3) B <--> E (external, ranks 1+4)
*       C <--> F (external, ranks 2+5) C <--> G (external, ranks 2+6)
*
* \author Hilmar Wobker
* \author Dominik Goeddeke
*/
class LoadBalancer
{

private:

  /* *****************
  * member variables *
  *******************/
  /// pointer to the process group the load balancer manages
  ProcessGroup* _process_group;

  /// flag whether the load balancer's process group uses a dedicated load balancer process
  bool _group_has_dedicated_load_bal;

  /// vector of work groups the load balancer manages
  std::vector<WorkGroup*> _work_groups;

  /// vector of graph structures representing the process topology within the work groups
  std::vector<Graph*> _graphs;

  /// number of work groups
  int _num_work_groups;

  /**
  * \brief array of number of workers in each work group
  *
  * Dimension: [#_num_work_groups]
  */
  int* _num_proc_in_group;

  /**
  * \brief 2-dim. array for storing the process group ranks building the work groups
  *
  * Dimension: [#_num_work_groups][#_num_proc_in_group[\a group_id]]
  */
  int** _work_group_ranks;

  /**
  * \brief base mesh the load balancer works with
  *
  * bla bla
  */
  BaseMesh* _base_mesh;

public:

  /* *************************
  * constructor & destructor *
  ***************************/
  /// constructor
  LoadBalancer(
    ProcessGroup* process_group,
    bool group_has_dedicated_load_bal)
    : _process_group(process_group),
      _group_has_dedicated_load_bal(group_has_dedicated_load_bal),
      _num_proc_in_group(nullptr),
      _work_group_ranks(nullptr),
      _base_mesh(nullptr)
  {
  }

  /// destructor
  ~LoadBalancer()
  {
    if (_work_group_ranks != nullptr)
    {
      for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
      {
        delete [] _work_group_ranks[igroup];
      }
      delete [] _work_group_ranks;
      _work_group_ranks = nullptr;
    }

    for(unsigned int igroup(0) ; igroup < _work_groups.size() ; ++igroup)
    {
      delete _work_groups[igroup];
    }

    if (_num_proc_in_group != nullptr)
    {
      delete [] _num_proc_in_group;
    }

    if (_base_mesh != nullptr)
    {
      delete _base_mesh;
    }
  }

  /* ******************
  * getters & setters *
  ********************/
  /**
  * \brief getter for the process group
  *
  * \return ProcessGroup pointer #_process_group
  */
  inline ProcessGroup* process_group() const
  {
    return _process_group;
  }


  /* *****************
  * member functions *
  *******************/
  /// dummy function in preparation of a function reading in a mesh file
  void read_mesh()
  {
    // the mesh is read by the process group coordinator
    if(_process_group->is_coordinator())
    {
      _base_mesh = new BaseMesh();
      _base_mesh->read_mesh();
    }
  }

  /**
  * \brief dummy function in preparation of a function for managing work groups
  *
  * This dummy function creates two work groups: one consisting of two workers responsible for the coarse grid
  * problem and one consisting of all the other workers responsible for the fine grid problem. Currently, everything
  * is hard-coded. Later, the user must be able to control the creation of work groups and even later the load
  * balancer has to apply clever strategies to create these work groups automatically so that the user doesn't have
  * to do anything.
  *
  * To optimise the communication between the coordinator of the main process group and the work groups, we add the
  * this coordinator to a work group if it is not a compute process of this work group anyway. Hence, for each work
  * group, there are three different possibilities:
  * 1) There is a dedicated load balancer process, which is automatically the coordinator of the main process group
  *    and belongs to no work group:
  *    --> work group adds the coordinator as extra process
  * 2) There is no dedicated load balancer process, and the coordinator of the main process group ...
  *   a) ... is not part of the work group:
  *     --> work group adds the coordinator as extra process
  *   b) ... is part of the work group:
  *     --> work group does not have to add the coordinator
  * Thus the 1-to-n or n-to-1 communication between coordinator and n work group processes can be performed via
  * MPI_Scatter() and MPI_Gather() (which always have to be called by all members of an MPI process group).
  * This is more efficient then using n calls of MPI_send() / MPI_recv() via the communicator of the main process group.
  */
  void create_work_groups()
  {
    // shortcut to the number of processes in the load balancer's process group
    int num_processes = _process_group->num_processes();

/* **************************************************************************************
* The following code is completely hard-wired with respect to one special example mesh. *
* Later, this has to be done in some auto-magically way.                                *
****************************************************************************************/

    // two tests:
    // 1) with dedicated load balancer process
    //    - work group for coarse grid: 2 processes: {0, 1}
    //    - work group for fine grid: 15 processes: {1, ..., 16}
    //    - i.e. process 1 is in both work groups
    //    - dedicated load balancer and coordinator process: 17
    // 2) without dedicated load balancer process
    //    - work group for coarse grid: 2 processes: {0, 1}
    //    - work group for fine grid: 15 processes: {2, ..., 17}
    //    - i.e. the two work groups are disjunct
    //    - coordinator process: 17
    // both tests need 18 processes in total
    // assert that the number of processes is 18
    assert(num_processes == 18);


    // set up the two test cases

    // number of work groups, manually set to 2
    _num_work_groups = 2;
    // array of numbers of processes per work group
    _num_proc_in_group = new int[2];

    // Boolean array indicating whether the work groups contain an extra process for the coordinator (which will then
    // not be a compute process in this work group)
    bool group_contains_extra_coord[_num_work_groups];

    // allocate first dimension of the array for rank partitioning
    _work_group_ranks = new int*[_num_work_groups];

    if(_group_has_dedicated_load_bal)
    {
      // test case 1
      // with dedicated load balancer process
      //  - work group for coarse grid: 2 processes: {0, 1}
      //  - work group for fine grid: 15 processes: {1, ..., 16}
      //  - i.e. process 1 is in both work groups
      //  - dedicated load balancer and coordinator process: 17

      // since there is a dedicated load balancer process, this has to be added to both work groups as extra coordinator
      // process
      group_contains_extra_coord[0] = true;
      group_contains_extra_coord[1] = true;

      // set number of processes per group
      _num_proc_in_group[0] = 2 + 1;
      _num_proc_in_group[1] = 16 + 1;

      // partition the process group ranks into work groups
      _work_group_ranks[0] = new int[_num_proc_in_group[0]];
      _work_group_ranks[0][0] = 0;
      _work_group_ranks[0][1] = 1;
      _work_group_ranks[0][2] = 17;

      _work_group_ranks[1] = new int[_num_proc_in_group[1]];
      // set entries to {1, ..., 16}
      for(int i(0) ; i < _num_proc_in_group[1] ; ++i)
      {
        _work_group_ranks[1][i] = i+1;
      }
    }
    else
    {
      // test case 2
      // without dedicated load balancer process
      //  - work group for coarse grid: 2 processes: {0, 1}
      //  - work group for fine grid: 15 processes: {2, ..., 17}
      //  - i.e. the two work groups are disjunct
      //  - coordinator process: 17

      // the coordinator is at the same time a compute process of the second work group, so only the first work group
      // has to add an extra process
      group_contains_extra_coord[0] = true;
      group_contains_extra_coord[1] = false;

      // set number of processes per group
      _num_proc_in_group[0] = 2 + 1;
      _num_proc_in_group[1] = 16;

      // partition the process group ranks into work groups
      _work_group_ranks[0] = new int[_num_proc_in_group[0]];
      _work_group_ranks[0][0] = 0;
      _work_group_ranks[0][1] = 1;
      _work_group_ranks[0][2] = 17;
      _work_group_ranks[1] = new int[_num_proc_in_group[1]];
      // set entries to {2, ..., 17}
      for(int i(0) ; i < _num_proc_in_group[1] ; ++i)
      {
        _work_group_ranks[1][i] = i+2;
      }
    }

// COMMENT_HILMAR: Old code that performs the stuff above at least partially automatically (however, not considering
// the group_contains_extra_coord array)
//
//    // Partition the ranks of the process group communicator into groups, by simply enumerating the process
//    // group ranks and assigning them consecutively to the requested number of processes.
//
//    // iterator for process group ranks, used to split them among the groups
//    int iter_group_rank(-1);
//    // now partition the ranks
//    for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
//    {
//      _work_group_ranks[igroup] = new int[_num_proc_in_group[igroup]];
//      for(int j(0) ; j < _num_proc_in_group[igroup] ; ++j)
//      {
//        // increase group rank
//        ++iter_group_rank;
//        // set group rank
//        _work_group_ranks[igroup][j] = iter_group_rank;
//      }
//    } // for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
// COMMENT_HILMAR: end of old code

/* ************************
* End of hard-wired code. *
**************************/

    // boolean array indicating to which work groups this process belongs
    bool belongs_to_group[_num_work_groups];
    for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
    {
      // intialise with false
      belongs_to_group[igroup] = false;
      for(int j(0) ; j < _num_proc_in_group[igroup] ; ++j)
      {
        if(_process_group->rank() == _work_group_ranks[igroup][j])
        {
          belongs_to_group[igroup] = true;
        }
      }
    } // for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)

    // create WorkGroup objects including MPI groups and MPI communicators
    // It is not possible to set up all WorkGroups in one call, since the processes building the WorkGroups are
    // not necessarily disjunct. Hence, there are as many calls as there are WorkGroups. All processes not belonging
    // to the WorkGroup currently created call the MPI_Comm_create() function with a dummy communicator and the
    // special group MPI_GROUP_EMPTY.

    _work_groups.resize(_num_work_groups, nullptr);
    for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
    {
      if(belongs_to_group[igroup])
      {
        _work_groups[igroup] = new WorkGroup(_num_proc_in_group[igroup], _work_group_ranks[igroup],
                                             _process_group, igroup, group_contains_extra_coord[igroup]);
      }
      else
      {
        // *All* processes of the parent MPI group have to call the MPI_Comm_create() routine (otherwise the forking
        // will deadlock), so let all processes that are not part of the current work group call it with special
        // MPI_GROUP_EMPTY and dummy communicator.
        MPI_Comm dummy_comm;
        int mpi_error_code = MPI_Comm_create(_process_group->comm(), MPI_GROUP_EMPTY, &dummy_comm);
        MPIUtils::validate_mpi_error_code(mpi_error_code, "MPI_Comm_create");
      }
    } // for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)


    /* *********************************************************
    * create graph structures corresponding to the work groups *
    ***********************************************************/

    // let the coordinator create the process topology
    if(_process_group->is_coordinator())
    {
      _graphs.resize(_num_work_groups, nullptr);

      // build an artificial graph mimicing the distribution of the 16 base mesh cells to two processors
      // (e.g. BMCs 0-7 on proc 1 and BMCs 8-15 on proc 2) which start an imagined coarse grid solver; this graph will
      // be used for the coarse grid work group
      int* index = new int[3];
      int* edges = new int[2];
      index[0] = 0;
      index[1] = 1;
      index[2] = 2;
      edges[0] = 1;
      edges[1] = 0;
      _graphs[0] = new Graph(2, index, edges);
      _graphs[0]->print();

      // get connectivity graph of the base mesh; this one will be used for the fine grid work group
      _graphs[1] = _base_mesh->graph();
    }

//    /* ***************************************************************************
//    * now let the coordinator send the relevant parts of the global graph to the *
//    * corresponding work group members                                           *
//    *****************************************************************************/
//
// COMMENT_HILMAR: TODO
//
//    for(int igroup(0) ; igroup < _num_work_groups ; ++igroup)
//    {
//
////      int root = _work_groups[igroup]->rank_coord();
//      if(_process_group->is_coordinator())
//      {
//        /* **********************************
//        * code for the sending root process *
//        ************************************/
//
//        // send the graph index to the non-root processes
//        MPI_Scatter(graph[igroup]->index(), graph[igroup]->num_nodes, MPI_INT, void* recvbuf, int recvcount,
//                    MPI_Datatype recvtype, root, _extended_work_groups[igroup]->comm())
// at root, use MPI_IN_PLACE instead of recvbuf --> recvbuf and recvcount ignored, root doesn't send data to itself
// still, the scattered vector has to contain n segments of data, where n is the number processes in the group
//
//      }
//      else
//      {
//        /* **************************************
//        * code for receiving non-root processes *
//        ****************************************/
//
//// at receiver only the last five are significant
//
////int MPI_Scatter(void* sendbuf, int sendcount, MPI_Datatype sendtype,
////void* recvbuf, int recvcount, MPI_Datatype recvtype, int root,
////MPI_Comm comm)
//
//      }
//    }

  } // create_work_groups()
};

#endif // guard KERNEL_LOAD_BAL_HPP
