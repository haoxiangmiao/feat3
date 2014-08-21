#pragma once
#ifndef FOUNDATION_GUARD_GLOBAL_DEFECT_HPP
#define FOUNDATION_GUARD_GLOBAL_DEFECT_HPP 1

#include<kernel/foundation/comm_base.hpp>
#include<kernel/foundation/communication.hpp>
#include<kernel/foundation/global_product_mat_vec.hpp>
#include<kernel/lafem/arch/difference.hpp>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::LAFEM::Arch;

///TODO add communicators
namespace FEAST
{
  namespace Foundation
  {
      template <typename Mem_, typename Algo_>
      struct GlobalDefect
      {
        public:
#ifndef SERIAL
          template<typename MatrixT_, typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_& target,
                           const VectorT_& b,
                           const MatrixT_& A,
                           const VectorT_& x,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >& mirrors,
                           StorageT_<Index, std::allocator<Index> >& other_ranks,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& sendbufs,
                           StorageT_<VectorT_, std::allocator<VectorT_> >& recvbufs,
                           Index tag = 0,
                           Communicator communicator = Communicator(MPI_COMM_WORLD)
                           )
          {
            ///assumes type-1 vector (full entries at inner boundaries)
            ///assumes type-0 matrix (entry fractions at inner boundaries)

            GlobalProductMat0Vec1<Mem::Main, Algo::Generic>::exec(
                                                                  target,
                                                                  A,
                                                                  x,
                                                                  mirrors,
                                                                  other_ranks,
                                                                  sendbufs,
                                                                  recvbufs,
                                                                  tag,
                                                                  communicator);

            Difference<Mem_, Algo_>::value(target.elements(), b.elements(), target.elements(), target.size());
          }
#else
          template<typename MatrixT_, typename VectorT_, typename VectorMirrorT_, template<typename, typename> class StorageT_>
          static void exec(
                           VectorT_& target,
                           const VectorT_& b,
                           const MatrixT_& A,
                           const VectorT_& x,
                           const StorageT_<VectorMirrorT_, std::allocator<VectorMirrorT_> >&,
                           StorageT_<Index, std::allocator<Index> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           StorageT_<VectorT_, std::allocator<VectorT_> >&,
                           Index = 0,
                           Communicator = Communicator(0)
                           )
          {
            A.template apply<Algo_>(target, x);
            Difference<Mem_, Algo_>::value(target.elements(), b.elements(), target.elements(), target.size());
          }
#endif
      };
  }
}


#endif