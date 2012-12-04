#pragma once
#ifndef SCARC_GUARD_SOLVER_PATTERN_HH
#define SCARC_GUARD_SOLVER_PATTERN_HH 1

#include<kernel/scarc/solver_functor.hpp>
#include<kernel/scarc/solver_data.hpp>

using namespace FEAST::Foundation;
using namespace FEAST;

namespace FEAST
{
  namespace ScaRC
  {
    ///tag definitions

    ///full local solver
    class Richardson
    {
    };

    ///local solver with preconditioner undefined
    class RichardsonProxy
    {
    };

    ///solver layer with application argument undefined
    class RichardsonLayer
    {
    };

    ///solver layer with both, application argument and preconditioner undefined
    class RichardsonProxyLayer
    {
    };

    ///full preconditioner application as SpMV
    class SpMVPreconApply
    {
    };

    ///...and its anonymous version
    class SpMVPreconApplyLayer
    {
    };

    ///pattern generation templates
    template<typename Pattern_, typename Algo_>
    struct SolverPatternGeneration
    {
    };

    template<typename Algo_>
    struct SolverPatternGeneration<Richardson, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 1;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               typename PT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(PreconditionedSolverData<DataType_, Tag_, VT_, MT_, PT_, StoreT_>& data,
                                                                                Index max_iter = 100,
                                                                                DataType_ eps = 1e-8)
      {
        ///take over 'logically constant' values
        data.max_iters() = max_iter;
        data.eps() = eps;

        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///add functors to the solver program:
        //defect(x, b, A, x)
        cf.add_functor(new DefectFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.rhs(), data.sys(), data.sol()));

        //norm2(norm_0, x)
        cf.add_functor(new NormFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm_0(), data.sol()));

        //iterate until s < eps: [product(x, P, x), sum(x, x, x), norm(norm, x), div(s, norm, norm_0)]
        ///TODO assumes scaled precon matrix
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > cfiterateptr(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cfiterate(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(cfiterateptr.get())));
        cfiterate.add_functor(new ProductFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.stored_prec, data.sol()));
        cfiterate.add_functor(new DefectFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.rhs(), data.sys(), data.sol()));
        cfiterate.add_functor(new NormFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm(), data.sol()));
        cfiterate.add_functor(new DivFunctor<VT_<Tag_, DataType_>, DataType_>(data.scalars().at(0), data.norm(), data.norm_0()));

        cf.add_functor(new IterateFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(cfiterateptr, data.scalars().at(0), data.eps(), data.used_iters(), data.max_iters(), coc_less));

        return result;
      }
    };

    template<typename Algo_>
    struct SolverPatternGeneration<RichardsonLayer, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 1;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               typename PT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(PreconditionedSolverData<DataType_, Tag_, VT_, MT_, PT_, StoreT_>& data,
                                                                                Index max_iter = 100,
                                                                                DataType_ eps = 1e-8)
      {
        ///define dummy
        VT_<Tag_, DataType_> dummy;

        ///take over 'logically constant' values
        data.max_iters() = max_iter;
        data.eps() = eps;

        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///add functors to the solver program:
        //defect(x, b, A, x)
        cf.add_functor(new DefectFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.rhs(), data.sys(), dummy));

        //norm2(norm_0, x)
        cf.add_functor(new NormFunctorProxyRight<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm_0(), dummy));

        //iterate until s < eps: [product(x, P, x), sum(x, x, x), norm(norm, x), div(s, norm, norm_0)]
        ///TODO assumes scaled precon matrix
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > cfiterateptr(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cfiterate(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(cfiterateptr.get())));
        cfiterate.add_functor(new ProductFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.stored_prec, dummy));
        cfiterate.add_functor(new DefectFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.rhs(), data.sys(), dummy));
        cfiterate.add_functor(new NormFunctorProxyRight<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm(), dummy));
        cfiterate.add_functor(new DivFunctor<VT_<Tag_, DataType_>, DataType_>(data.scalars().at(0), data.norm(), data.norm_0()));

        cf.add_functor(new IterateFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(cfiterateptr, data.scalars().at(0), data.eps(), data.used_iters(), data.max_iters(), coc_less));
        return result;
      }
    };

    template<typename Algo_>
    struct SolverPatternGeneration<RichardsonProxy, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 1;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(SolverDataBase<DataType_, Tag_, VT_, MT_, StoreT_>& data,
                                                                                Index max_iter = 100,
                                                                                DataType_ eps = 1e-8)
      {
        ///take over 'logically constant' values
        data.max_iters() = max_iter;
        data.eps() = eps;

        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///add functors to the solver program:
        //defect(x, b, A, x)
        cf.add_functor(new DefectFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.rhs(), data.sys(), data.sol()));

        //norm2(norm_0, x)
        cf.add_functor(new NormFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm_0(), data.sol()));

        //iterate until s < eps: [proxyprecon(x), sum(x, x, x), norm(norm, x), div(s, norm, norm_0)]
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > cfiterateptr(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cfiterate(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(cfiterateptr.get())));
        cfiterate.add_functor(new PreconFunctor<Algo_, VT_<Tag_, DataType_> >(data.sol()));
        cfiterate.add_functor(new DefectFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.rhs(), data.sys(), data.sol()));
        cfiterate.add_functor(new NormFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm(), data.sol()));
        cfiterate.add_functor(new DivFunctor<VT_<Tag_, DataType_>, DataType_>(data.scalars().at(0), data.norm(), data.norm_0()));

        cf.add_functor(new IterateFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(cfiterateptr, data.scalars().at(0), data.eps(), data.used_iters(), data.max_iters(), coc_less));

        return result;
      }
    };

    template<typename Algo_>
    struct SolverPatternGeneration<RichardsonProxyLayer, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 1;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(SolverDataBase<DataType_, Tag_, VT_, MT_, StoreT_>& data,
                                                                                Index max_iter = 100,
                                                                                DataType_ eps = 1e-8)
      {
        ///define dummy
        VT_<Tag_, DataType_> dummy;

        ///take over 'logically constant' values
        data.max_iters() = max_iter;
        data.eps() = eps;

        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///add functors to the solver program:
        //defect(x, b, A, x)
        cf.add_functor(new DefectFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.rhs(), data.sys(), dummy));

        //norm2(norm_0, x)
        cf.add_functor(new NormFunctorProxyRight<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm_0(), dummy));

        //iterate until s < eps: [proxyprecon(x), sum(x, x, x), norm(norm, x), div(s, norm, norm_0)]
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > cfiterateptr(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cfiterate(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(cfiterateptr.get())));
        cfiterate.add_functor(new PreconFunctorProxy<Algo_, VT_<Tag_, DataType_> >(dummy));
        cfiterate.add_functor(new DefectFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.rhs(), data.sys(), dummy));
        cfiterate.add_functor(new NormFunctorProxyRight<Algo_, VT_<Tag_, DataType_>, DataType_ >(data.norm(), dummy));
        cfiterate.add_functor(new DivFunctor<VT_<Tag_, DataType_>, DataType_>(data.scalars().at(0), data.norm(), data.norm_0()));

        cf.add_functor(new IterateFunctor<Algo_, VT_<Tag_, DataType_>, DataType_ >(cfiterateptr, data.scalars().at(0), data.eps(), data.used_iters(), data.max_iters(), coc_less));

        return result;
      }
    };

    template<typename Algo_>
    struct SolverPatternGeneration<SpMVPreconApply, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 0;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               typename PT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(PreconditionedSolverData<DataType_, Tag_, VT_, MT_, PT_, StoreT_>& data)
      {
        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///add functors to the solver program:
        //product(x, P, x)
        cf.add_functor(new ProductFunctor<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(data.sol(), data.stored_prec, data.sol()));

        return result;
      }
    };

    template<typename Algo_>
    struct SolverPatternGeneration<SpMVPreconApplyLayer, Algo_>
    {
      static Index min_num_temp_scalars()
      {
        return 0;
      }

      static Index min_num_temp_vectors()
      {
        return 0;
      }

      template<typename Tag_,
               typename DataType_,
               template<typename, typename> class VT_,
               template<typename, typename> class MT_,
               typename PT_,
               template<typename, typename> class StoreT_>
      static std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > execute(PreconditionedSolverData<DataType_, Tag_, VT_, MT_, PT_, StoreT_>& data)
      {
        ///create compound functor
        std::shared_ptr<SolverFunctorBase<VT_<Tag_, DataType_> > > result(new CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >());
        ///get reference to functor (in order to cast only once)
        CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >& cf(*((CompoundSolverFunctor<Algo_, VT_<Tag_, DataType_> >*)(result.get())));

        ///define dummy
        VT_<Tag_, DataType_> dummy;

        ///add functors to the solver program:
        //product(dummy, P, dummy)
        cf.add_functor(new ProductFunctorProxyResultRight<Algo_, VT_<Tag_, DataType_>, MT_<Tag_, DataType_> >(dummy, data.stored_prec, dummy));

        return result;
      }
    };
  }
}

#endif
