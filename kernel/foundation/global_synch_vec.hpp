#pragma once
#ifndef FOUNDATION_GUARD_GLOBAL_SYNCH_VEC_HPP
#define FOUNDATION_GUARD_GLOBAL_SYNCH_VEC_HPP 1

#include<kernel/foundation/comm_base.hpp>
#include<kernel/foundation/communication.hpp>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::LAFEM::Arch;

///TODO add communicators
namespace FEAST
{
  namespace Foundation
  {
      template <typename Mem_, typename Algo_>
      struct GlobalSynchVec0
      {
      };

      template <>
      struct GlobalSynchVec0<Mem::Main, Algo::Generic>
      {
        public:

#ifndef SERIAL
          ///TODO implement version with active queue of requests
          template<typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_& target,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >& mirrors,
                           StorageT_<Index, std::allocator<Index> >& other_ranks,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& sendbufs,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& recvbufs,
                           Index tag = 0,
                           Communicator communicator = Communicator(MPI_COMM_WORLD)
                           )
          {
            if(mirrors.size() == 0)
              return;

            ///assumes type-0 vector (entry fractions at inner boundaries)

            ///start recv
            StorageT_<Request, std::allocator<Request> > recvrequests;
            StorageT_<Status, std::allocator<Status> > recvstatus;
            for(Index i(0) ; i < mirrors.size() ; ++i)
            {
              Request rr;
              Status rs;

              recvrequests.push_back(std::move(rr));
              recvstatus.push_back(std::move(rs));

              Comm::irecv(recvbufs.at(i).elements(),
                          recvbufs.at(i).size(),
                          other_ranks.at(i),
                          recvrequests.at(i),
                          tag + Comm::rank(communicator),
                          communicator
                         );

            }

            ///gather and start send
            StorageT_<Request, std::allocator<Request> > sendrequests;
            StorageT_<Status, std::allocator<Status> > sendstatus;
            for(Index i(0) ; i < mirrors.size() ; ++i)
            {
              mirrors.at(i).gather_dual(sendbufs.at(i), target);

              Request sr;
              Status ss;

              sendrequests.push_back(std::move(sr));
              sendstatus.push_back(std::move(ss));

              Comm::isend(sendbufs.at(i).elements(),
                          sendbufs.at(i).size(),
                          other_ranks.at(i),
                          sendrequests.at(i),
                          tag + other_ranks.at(i),
                          communicator
                          );
            }

            int* recvflags = new int[recvrequests.size()];
            int* taskflags = new int[recvrequests.size()];
            for(Index i(0) ; i < recvrequests.size() ; ++i)
            {
              recvflags[i] = 0;
              taskflags[i] = 0;
            }

            Index count(0);
            while(count != recvrequests.size())
            {
              //go through all requests round robin
              for(Index i(0) ; i < recvrequests.size() ; ++i)
              {
                if(taskflags[i] == 0)
                {
                  Comm::test(recvrequests.at(i), recvflags[i], recvstatus.at(i));
                  if(recvflags[i] != 0)
                  {
                    VectorT_ temp(target.size(), typename VectorT_::DataType(0));
                    mirrors.at(i).scatter_dual(temp, recvbufs.at(i));
                    target.template axpy<Algo::Generic>(target, temp);
                    ++count;
                    taskflags[i] = 1;
                  }
                }
              }
            }

            for(Index i(0) ; i < sendrequests.size() ; ++i)
            {
              Status ws;
              Comm::wait(sendrequests.at(i), ws);
            }

            delete[] recvflags;
            delete[] taskflags;
          }
#else
          template<typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_&,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >&,
                           StorageT_<Index, std::allocator<Index> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           Index = 0,
                           Communicator = Communicator(0)
                           )
          {
          }
#endif
      };

      template <typename Mem_, typename Algo_>
      struct GlobalSynchVec1
      {
      };

      template <>
      struct GlobalSynchVec1<Mem::Main, Algo::Generic>
      {
        public:

#ifndef SERIAL
          ///TODO start all recvs in one sweep first
          template<typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_& target,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >& mirrors,
                           const VectorT_& frequencies,
                           StorageT_<Index, std::allocator<Index> > other_ranks,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& sendbufs,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& recvbufs,
                           Index tag = 0,
                           Communicator communicator = Communicator(MPI_COMM_WORLD)
                           )
          {
            if(mirrors.size() == 0)
              return;

            ///assumes type-1 vector (full entries at inner boundaries)

            ///start recv
            StorageT_<Request, std::allocator<Request> > recvrequests;
            StorageT_<Status, std::allocator<Status> > recvstatus;
            for(Index i(0) ; i < mirrors.size() ; ++i)
            {
              Request rr;
              Status rs;

              recvrequests.push_back(std::move(rr));
              recvstatus.push_back(std::move(rs));

              Comm::irecv(recvbufs.at(i).elements(),
                          recvbufs.at(i).size(),
                          other_ranks.at(i),
                          recvrequests.at(i),
                          tag + Comm::rank(communicator),
                          communicator
                         );
            }

            ///gather and start send
            StorageT_<Request, std::allocator<Request> > sendrequests;
            StorageT_<Status, std::allocator<Status> > sendstatus;
            for(Index i(0) ; i < mirrors.size() ; ++i)
            {
              mirrors.at(i).gather_dual(sendbufs.at(i), target);

              Request sr;
              Status ss;

              sendrequests.push_back(std::move(sr));
              sendstatus.push_back(std::move(ss));

              Comm::isend(sendbufs.at(i).elements(),
                          sendbufs.at(i).size(),
                          other_ranks.at(i),
                          sendrequests.at(i),
                          tag + other_ranks.at(i),
                          communicator
                          );
            }

            int* recvflags = new int[recvrequests.size()];
            int* taskflags = new int[recvrequests.size()];
            for(Index i(0) ; i < recvrequests.size() ; ++i)
            {
              recvflags[i] = 0;
              taskflags[i] = 0;
            }


            Index count(0);

            ///handle receives
            VectorT_ sum(target.size(), typename VectorT_::DataType(0));
            while(count != recvrequests.size())
            {
              //go through all requests round robin
              for(Index i(0) ; i < recvrequests.size() ; ++i)
              {
                if(taskflags[i] == 0)
                {
                  Comm::test(recvrequests.at(i), recvflags[i], recvstatus.at(i));
                  if(recvflags[i] != 0)
                  {
                    VectorT_ temp(target.size(), typename VectorT_::DataType(0));
                    mirrors.at(i).scatter_dual(temp, recvbufs.at(i));
                    sum.template axpy<Algo::Generic>(sum, temp);
                    ++count;
                    taskflags[i] = 1;
                  }
                }
              }
            }
            target.template axpy<Algo::Generic>(target, sum);

            typename VectorT_::DataType* target_d(target.elements());
            const typename VectorT_::DataType* freq_d(frequencies.elements());
            for(Index i(0) ; i < sum.size() ; ++i)
              target_d[i] /= freq_d[i];


            for(Index i(0) ; i < sendrequests.size() ; ++i)
            {
              Status ws;
              Comm::wait(sendrequests.at(i), ws);
            }

            delete[] recvflags;
            delete[] taskflags;
          }
#else
          template<typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_&,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >&,
                           const VectorT_&,
                           StorageT_<Index, std::allocator<Index> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           Index = 0,
                           Communicator = Communicator(0)
                           )
          {
          }
#endif
      };
  }
}


#endif