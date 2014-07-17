#pragma once
#ifndef KERNEL_ASSEMBLY_COMMON_OPERATORS_HPP
#define KERNEL_ASSEMBLY_COMMON_OPERATORS_HPP 1

// includes, FEAST
#include <kernel/assembly/bilinear_operator.hpp>

namespace FEAST
{
  namespace Assembly
  {
    namespace Common
    {
      /**
       * \brief -Laplace operator implementation
       *
       * This functor implements the weak formulation of the bilinear scalar Laplace operator, i.e.
       *   \f[ \nabla \varphi \cdot \nabla\psi \f]
       *
       * This functor can be used with the BilinearOperator assembly class template to assemble a
       * scalar Laplace/Stiffness matrix.
       *
       * \author Peter Zajac
       */
      class LaplaceOperator :
        public BilinearOperator
      {
      public:
        /// test space configuration
        struct TestConfig :
          public Space::ConfigBase
        {
          /// test-space requirement enumeration
          enum
          {
            /// this functor requires test-function gradients
            need_grad = 1
          };
        };

        /// trial space configuration
        struct TrialConfig :
          public Space::ConfigBase
        {
          /// trial-space requirement enumeration
          enum
          {
            /// this functor requires trial-function gradients
            need_grad = 1
          };
        };

        /**
         * \brief Laplace evaluator class template
         *
         * \tparam AsmTraits_
         * The assembly traits class.
         *
         * \author Peter Zajac
         */
        template<typename AsmTraits_>
        class Evaluator :
          public BilinearOperator::Evaluator<AsmTraits_>
        {
        public:
          /// the data type to be used
          typedef typename AsmTraits_::DataType DataType;
          /// the assembler's trafo data type
          typedef typename AsmTraits_::TrafoData TrafoData;
          /// the assembler's test-function data type
          typedef typename AsmTraits_::TestBasisData TestBasisData;
          /// the assembler's trial-function data type
          typedef typename AsmTraits_::TrialBasisData TrialBasisData;

        public:
          /**
           * \brief Constructor
           *
           * \param[in] operat
           * A reference to the Laplace operator object.
           */
          explicit Evaluator(const LaplaceOperator& DOXY(operat))
          {
          }

          /** \copydoc BilinearFunctorBase::Evaluator::operator() */
          DataType operator()(const TrialBasisData& phi, const TestBasisData& psi)
          {
            return dot(phi.grad, psi.grad);
          }
        }; // class LaplaceOperator::Evaluator<...>
      }; // class LaplaceOperator

      /**
       * \brief Identity operator implementation
       *
       * This functor implements the weak formulation of the bilinear scalar Identity operator, i.e.
       *   \f[ \varphi \cdot \psi \f]
       *
       * This functor can be used with the BilinearOperator assembler class template to assemble a
       * scalar mass matrix.
       *
       * \author Peter Zajac
       */
      class IdentityOperator :
        public BilinearOperator
      {
      public:
        /// test space configuration
        struct TestConfig :
          public Space::ConfigBase
        {
          /// test-space requirement enumeration
          enum
          {
            /// this functor requires test-function values
            need_value = 1
          };
        };

        /// trial space configuration
        struct TrialConfig :
          public Space::ConfigBase
        {
          /// trial-space requirement enumeration
          enum
          {
            /// this functor requires trial-function values
            need_value = 1
          };
        };

        /**
         * \brief Bilinear scalar Identity evaluator class template
         *
         * \tparam AsmTraits_
         * The assembly traits class.
         *
         * \author Peter Zajac
         */
        template<typename AsmTraits_>
        class Evaluator :
          public BilinearOperator::Evaluator<AsmTraits_>
        {
        public:
          /// the data type to be used
          typedef typename AsmTraits_::DataType DataType;
          /// the assembler's trafo data type
          typedef typename AsmTraits_::TrafoData TrafoData;
          /// the assembler's test-function data type
          typedef typename AsmTraits_::TestBasisData TestBasisData;
          /// the assembler's trial-function data type
          typedef typename AsmTraits_::TrialBasisData TrialBasisData;

        public:
          /**
           * \brief Constructor
           *
           * \param[in] operat
           * A reference to the Identity operator object.
           */
          explicit Evaluator(const IdentityOperator& DOXY(operat))
          {
          }

          /** \copydoc BilinearFunctorBase::Evaluator::operator() */
          DataType operator()(const TrialBasisData& phi, const TestBasisData& psi)
          {
            return phi.value * psi.value;
          }
        }; // class IdentityOperator::Evaluator<...>
      }; // class IdentityOperator

      /**
       * \brief Test-Derivative operator implementation
       *
       * This functor implements the weak formulation of the bilinear test-function derivative operator, i.e.
       *   \f[ \varphi \cdot \partial_i \psi \f]
       *
       * This functor can be used with the BilinearOperator assembly class template to assemble a
       * scalar matrix for the pressure-gradient operator of the Stokes equation.
       *
       * \tparam derivative_
       * The index of the derivative for this operator:
       *  - 0: X-derivative
       *  - 1: Y-derivative
       *  - 2: Z-derivative
       *  - ...
       *
       * \author Peter Zajac
       */
      template<Index derivative_>
      class TestDerivativeOperator :
        public BilinearOperator
      {
      public:
        /// test space configuration
        struct TestConfig :
          public Space::ConfigBase
        {
          /// test-space requirement enumeration
          enum
          {
            /// this functor requires test-function gradients
            need_grad = 1
          };
        };

        /// trial space configuration
        struct TrialConfig :
          public Space::ConfigBase
        {
          /// trial-space requirement enumeration
          enum
          {
            /// this functor requires trial-function values
            need_value = 1
          };
        };

        /**
         * \brief Test-Derivative evaluator class template
         *
         * \tparam AsmTraits_
         * The assembly traits class.
         *
         * \author Peter Zajac
         */
        template<typename AsmTraits_>
        class Evaluator :
            public BilinearOperator::Evaluator<AsmTraits_>
        {
        public:
          /// the data type to be used
          typedef typename AsmTraits_::DataType DataType;
          /// the assembler's trafo data type
          typedef typename AsmTraits_::TrafoData TrafoData;
          /// the assembler's test-function data type
          typedef typename AsmTraits_::TestBasisData TestBasisData;
          /// the assembler's trial-function data type
          typedef typename AsmTraits_::TrialBasisData TrialBasisData;

        public:
          /**
           * \brief Constructor
           *
           * \param[in] operat
           * A reference to the Laplace operator object.
           */
          explicit Evaluator(const TestDerivativeOperator<derivative_>& DOXY(operat))
          {
          }

          /** \copydoc BilinearFunctorBase::Evaluator::operator() */
          DataType operator()(const TrialBasisData& phi, const TestBasisData& psi)
          {
            return phi.value * psi.grad[derivative_];
          }
        }; // class TestDerivativeOperator::Evaluator<...>
      }; // class TestDerivativeOperator
    } // namespace Common
  } // namespace Assembly
} // namespace FEAST

#endif // KERNEL_ASSEMBLY_COMMON_OPERATORS_HPP
