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
        typename ShapeType_,
        typename DataType_>
      class NodeFunctional :
        public NodeFunctionalNull<Space_, DataType_>
      {
      public:
        explicit NodeFunctional(const Space_& space) :
          NodeFunctionalNull<Space_, DataType_>(space)
        {
        }
      };

      template<
        typename Space_,
        typename DataType_>
      class NodeFunctional<Space_, Shape::Vertex, DataType_> :
        public NodeFunctionalBase<Space_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, DataType_> BaseClass;
        static constexpr int max_assigned_dofs = 1;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Vertex, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          static constexpr bool need_value = true;
          static constexpr bool need_grad = false;
          static constexpr bool need_hess = false;
        };

        TrafoEvalType _trafo_eval;

      public:
        explicit NodeFunctional(const Space_& space) :
          BaseClass(space),
          _trafo_eval(space.get_trafo())
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
        }

        void finish()
        {
          _trafo_eval.finish();
          BaseClass::finish();
        }

        int get_num_assigned_dofs() const
        {
          return max_assigned_dofs;
        }

        template<
          typename NodeData_,
          typename Function_>
        void operator()(NodeData_& node_data, const Function_& function) const
        {
          // fetch the function's trafo config
          typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

          // declare trafo evaluation data
          typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

          // declare evaluation traits
          typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;

          // declare function evaluator
          typename Function_::template Evaluator<AnalyticEvalTraits> func_eval(function);

          // prepare function evaluator
          func_eval.prepare(_trafo_eval);

          // compute trafo data
          DomainPointType dom_point;
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);

          // evaluate function
          node_data[0] = func_eval.value(trafo_data);

          // finish function evaluator
          func_eval.finish();
        }
      };

      template<
        typename Space_,
        int shape_dim_,
        typename DataType_>
      class NodeFunctional<Space_, Shape::Hypercube<shape_dim_>, DataType_> :
        public NodeFunctionalBase<Space_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, DataType_> BaseClass;
        static constexpr int max_assigned_dofs = 1;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Hypercube<shape_dim_>, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          static constexpr bool need_value = true;
          static constexpr bool need_grad = false;
          static constexpr bool need_hess = false;
        };

        TrafoEvalType _trafo_eval;

      public:
        explicit NodeFunctional(const Space_& space) :
          BaseClass(space),
          _trafo_eval(space.get_trafo())
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
        }

        void finish()
        {
          _trafo_eval.finish();
          BaseClass::finish();
        }

        int get_num_assigned_dofs() const
        {
          return max_assigned_dofs;
        }

        template<
          typename NodeData_,
          typename Function_>
        void operator()(NodeData_& node_data, const Function_& function) const
        {
          // fetch the function's trafo config
          typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

          // declare trafo evaluation data
          typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

          // declare evaluation traits
          typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;

          // declare function evaluator
          typename Function_::template Evaluator<AnalyticEvalTraits> func_eval(function);

          // prepare function evaluator
          func_eval.prepare(_trafo_eval);

          // compute trafo data
          DomainPointType dom_point(DataType_(0));
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);

          // evaluate function
          node_data[0] = func_eval.value(trafo_data);

          // finish function evaluator
          func_eval.finish();
        }
      };

      template<
        typename Space_,
        typename DataType_>
      class NodeFunctional<Space_, Shape::Simplex<1>, DataType_> :
        public NodeFunctionalBase<Space_, DataType_>
      {
      public:
        typedef NodeFunctionalBase<Space_, DataType_> BaseClass;
        static constexpr int max_assigned_dofs = 1;

      protected:
        typedef typename Space_::TrafoType TrafoType;
        typedef typename TrafoType::template Evaluator<Shape::Simplex<1>, DataType_>::Type TrafoEvalType;
        typedef typename TrafoEvalType::EvalTraits TrafoEvalTraits;
        typedef typename TrafoEvalTraits::DomainPointType DomainPointType;

        struct FunctionConfig :
          public Trafo::AnalyticConfigBase
        {
          static constexpr bool need_value = true;
          static constexpr bool need_grad = false;
          static constexpr bool need_hess = false;
        };

        TrafoEvalType _trafo_eval;

      public:
        explicit NodeFunctional(const Space_& space) :
          BaseClass(space),
          _trafo_eval(space.get_trafo())
        {
        }

        void prepare(Index cell_index)
        {
          BaseClass::prepare(cell_index);
          _trafo_eval.prepare(cell_index);
        }

        void finish()
        {
          _trafo_eval.finish();
          BaseClass::finish();
        }

        int get_num_assigned_dofs() const
        {
          return max_assigned_dofs;
        }

        template<
          typename NodeData_,
          typename Function_>
        void operator()(NodeData_& node_data, const Function_& function) const
        {
          // fetch the function's trafo config
          typedef typename Function_::template ConfigTraits<FunctionConfig>::TrafoConfig TrafoConfig;

          // declare trafo evaluation data
          typedef typename TrafoEvalType::template ConfigTraits<TrafoConfig>::EvalDataType TrafoEvalData;

          // declare evaluation traits
          typedef Trafo::AnalyticEvalTraits<TrafoEvalType, TrafoEvalData> AnalyticEvalTraits;

          // declare function evaluator
          typename Function_::template Evaluator<AnalyticEvalTraits> func_eval(function);

          // prepare function evaluator
          func_eval.prepare(_trafo_eval);

          // compute trafo data
          DomainPointType dom_point(DataType_(0.5));
          TrafoEvalData trafo_data;
          _trafo_eval(trafo_data, dom_point);

          // evaluate function
          node_data[0] = func_eval.value(trafo_data);

          // finish function evaluator
          func_eval.finish();
        }
      };
    } // namespace Lagrange1
  } // namespace Space
} // namespace FEAST

#endif // KERNEL_SPACE_LAGRANGE2_NODE_FUNCTIONAL_HPP
