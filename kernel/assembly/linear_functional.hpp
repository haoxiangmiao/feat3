// FEAT3: Finite Element Analysis Toolbox, Version 3
// Copyright (C) 2010 - 2020 by Stefan Turek & the FEAT group
// FEAT3 is released under the GNU General Public License version 3,
// see the file 'copyright.txt' in the top level directory for details.

#pragma once
#ifndef KERNEL_ASSEMBLY_LINEAR_FUNCTIONAL_HPP
#define KERNEL_ASSEMBLY_LINEAR_FUNCTIONAL_HPP 1

// includes, FEAT
#include <kernel/assembly/base.hpp>

namespace FEAT
{
  namespace Assembly
  {
    /**
     * \brief Base class for Linear Functionals
     *
     * This class acts as a base class and interface documentation for linear functionals which are used by
     * the LinearFunctionalAssembler assembly class template.
     *
     * \author Peter Zajac
     */
    class LinearFunctional
    {
    public:
#ifdef DOXYGEN
      static constexpr TrafoTags trafo_config = ...;
      static constexpr SpaceTags test_config = ...;
#endif // DOXYGEN

      /**
       * \brief Linear Functional Evaluator class template
       *
       * \tparam AsmTraits_
       * The assembly traits class.
       *
       * \author Peter Zajac
       */
      template<typename AsmTraits_>
      class Evaluator
      {
      public:
        /// trafo evaluator type
        typedef typename AsmTraits_::TrafoEvaluator TrafoEvaluator;
        /// data type
        typedef typename AsmTraits_::DataType DataType;
        /// trafo data type
        typedef typename AsmTraits_::TrafoData TrafoData;
        /// test function data type
        typedef typename AsmTraits_::TestBasisData TestBasisData;

      public:
        /**
         * \brief Prepares the evaluator for a given cell
         *
         * \param[in] trafo_eval
         * A reference to the trafo evaluator containing the cell information.
         */
        void prepare(const TrafoEvaluator& DOXY(trafo_eval))
        {
          // do nothing
        }

        /**
         * \brief Releases the evaluator from the current cell.
         */
        void finish()
        {
          // do nothing
        }

        /**
         * \brief Point initialisation function
         *
         * This function is called to initialise the evaluator for a new evaluation point.
         *
         * \param[in] tau
         * The transformation data in the current evaluation point. \see Trafo::EvalData
         */
        void set_point(const TrafoData& DOXY(tau))
        {
          // do nothing
        }

#ifdef DOXYGEN
        /**
         * \brief Evaluation operator
         *
         * This operator evaluates the linear functional for a given test function in a single point.
         *
         * \param[in] psi
         * The test function data in the current evaluation point. \see Space::EvalData
         *
         * \returns
         * The value of the linear functional.
         */
        DataType operator()(const TestBasisData& psi) const;
#endif // DOXYGEN
      }; // class LinearFunctional::Evaluator<...>
    }; // class LinearFunctional
  } // namespace Assembly
} // namespace FEAT

#endif // KERNEL_ASSEMBLY_LINEAR_FUNCTIONAL_HPP
