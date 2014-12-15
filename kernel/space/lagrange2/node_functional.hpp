#pragma once
#ifndef KERNEL_SPACE_LAGRANGE2_NODE_FUNCTIONAL_HPP
#define KERNEL_SPACE_LAGRANGE2_NODE_FUNCTIONAL_HPP 1

// includes, FEAST
#include <kernel/space/node_functional_base.hpp>
#include <kernel/trafo/eval_data.hpp>

namespace FEAST
{
  namespace Space
  {
    namespace Lagrange2
    {
      template<
        typename Space_,
        typename Function_,
        typename ShapeType_,
        typename DataType_>
      class NodeFunctional :
        public NodeFunctionalNull<Space_, Function_, DataType_>
      {
      public:
        explicit NodeFunctional(const Space_& space, const Function_& function) :
          NodeFunctionalNull<Space_, Function_, DataType_>(space, function)
        {
        }
      };

      template<
        typename Space_,
        typename Function_,
        typename DataType_>
      class NodeFunctional<Space_, Function_, Shape::Vertex, DataType_> :
        public NodeFunctionalBase<Space_, Function_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, Function_, DataType_> BaseClass;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Vertex, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          enum
          {
            need_value = 1,
            need_grad = 0,
            need_hess = 0
          };
        };

        typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

        typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

        typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;
        typedef typename Function_::template Evaluator<AnalyticEvalTraits> FuncEval;

        TrafoEvalType _trafo_eval;
        FuncEval _func_eval;

      public:
        explicit NodeFunctional(const Space_& space, const Function_& function) :
          BaseClass(space, function),
          _trafo_eval(space.get_trafo()),
          _func_eval(function)
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
          _func_eval.prepare(_trafo_eval);
        }

        void finish()
        {
          _func_eval.finish();
          _trafo_eval.finish();
          BaseClass::finish();
        }

        Index get_max_assigned_dofs() const
        {
          return 1;
        }

        Index get_num_assigned_dofs() const
        {
          return 1;
        }

        DataType_ operator()(Index /*assign_idx*/) const
        {
          DomainPointType dom_point;
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);
          return _func_eval.value(trafo_data);
        }
      };

      template<
        typename Space_,
        typename Function_,
        int shape_dim_,
        typename DataType_>
      class NodeFunctional<Space_, Function_, Shape::Hypercube<shape_dim_>, DataType_> :
        public NodeFunctionalBase<Space_, Function_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, Function_, DataType_> BaseClass;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Hypercube<shape_dim_>, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          enum
          {
            need_value = 1,
            need_grad = 0,
            need_hess = 0
          };
        };

        typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

        typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

        typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;
        typedef typename Function_::template Evaluator<AnalyticEvalTraits> FuncEval;

        TrafoEvalType _trafo_eval;
        FuncEval _func_eval;

      public:
        explicit NodeFunctional(const Space_& space, const Function_& function) :
          BaseClass(space, function),
          _trafo_eval(space.get_trafo()),
          _func_eval(function)
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
          _func_eval.prepare(_trafo_eval);
        }

        void finish()
        {
          _func_eval.finish();
          _trafo_eval.finish();
          BaseClass::finish();
        }

        Index get_max_assigned_dofs() const
        {
          return 1;
        }

        Index get_num_assigned_dofs() const
        {
          return 1;
        }

        DataType_ operator()(Index /*assign_idx*/) const
        {
          DomainPointType dom_point(DataType_(0));
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);
          return _func_eval.value(trafo_data);
        }
      };


      template<
        typename Space_,
        typename Function_,
        typename DataType_>
      class NodeFunctional<Space_, Function_, Shape::Simplex<1>, DataType_> :
        public NodeFunctionalBase<Space_, Function_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, Function_, DataType_> BaseClass;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Simplex<1>, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          enum
          {
            need_value = 1,
            need_grad = 0,
            need_hess = 0
          };
        };

        typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

        typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

        typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;
        typedef typename Function_::template Evaluator<AnalyticEvalTraits> FuncEval;

        TrafoEvalType _trafo_eval;
        FuncEval _func_eval;

      public:
        explicit NodeFunctional(const Space_& space, const Function_& function) :
          BaseClass(space, function),
          _trafo_eval(space.get_trafo()),
          _func_eval(function)
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
          _func_eval.prepare(_trafo_eval);
        }

        void finish()
        {
          _func_eval.finish();
          _trafo_eval.finish();
          BaseClass::finish();
        }

        Index get_max_assigned_dofs() const
        {
          return 1;
        }

        Index get_num_assigned_dofs() const
        {
          return 1;
        }

        DataType_ operator()(Index /*assign_idx*/) const
        {
          DomainPointType dom_point(DataType_(0.5));
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);
          return _func_eval.value(trafo_data);
        }
      };
    } // namespace Lagrange1
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_LAGRANGE2_NODE_FUNCTIONAL_HPP