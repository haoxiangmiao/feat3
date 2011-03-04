#pragma once
#ifndef UTIL_ASSERTION_HH
/// Header guard
#define UTIL_ASSERTION_HH 1

#include <kernel/util/exception.hpp>
#include <string>
#include <iostream>

namespace FEAST
{
  /**
  * \brief defining assertion
  *
  * An assertion is thrown when a critical condition is not fulfilled. Together with the macro defined below, it
  * replaces the standard C assert(...).
  *
  * \author Dirk Ribbrock
  */
  class Assertion
    : public Exception
  {

  public:

    /**
    * \brief CTOR
    *
    * \param[in] function
    * name of the function in which the assertion failed
    *
    * \param[in] file
    * name of the source file that contains the failed assertion
    *
    * \param[in] line
    * line number of the failed assertion
    *
    * \param[in] message
    * message that shall be displayed
    */
    Assertion(
      const char * const function,
      const char * const file,
      const long line,
      const std::string & message)
      : Exception(StringUtils::stringify(file) + ":" + StringUtils::stringify(line) + ": in "
                  + StringUtils::stringify(function) + ": " + message)
    {
      std::cout << backtrace("\n") << this->message() << std::endl;
    }
  };




  /**
  * \brief Simple struct to catch compile time assertions
  *
  * \author Dirk Ribbrock
  */
  template<bool>
  struct CompileTimeChecker
  {
    /// Constructor accepting any kind of arguments:
    CompileTimeChecker(...){};
  };




  /**
  * \brief 'False' version of CompileTimeChecker struct to catch compile time assertions.
  *
  * \author Dirk Ribbrock
  */
  template<>
  struct CompileTimeChecker<false>
  {
  };




/**
* \def ASSERT
*
* \brief Convenience definition that provides a way to throw Assertion exceptions.
*
* The thrown Assertion will be automatically provided with the correct filename,
* line number and function name.
*
* \param expr Boolean expression that shall be asserted.
* \param msg Error message that will be display in case that expr evaluates to false.
*
* \warning Will only be compiled in when debug support is enabled.
*/
#if defined (DEBUG)
#define ASSERT(expr, msg) \
    do { \
        if (! (expr)) \
            throw FEAST::Assertion(__PRETTY_FUNCTION__, __FILE__, __LINE__, msg); \
    } while (false)
#else
#define ASSERT(expr, msg)
#endif




/**
* \def STATIC_ASSERT
*
* \brief Convenience definition that provides a way to throw compile-time errors.
*
* The thrown Assertion will be automatically provided with the correct filename,
* line number and function name.
*
* \param[in] expr
* Boolean expression that shall be asserted.
* \param[in] msg
* Error message that will be display in case that expr evaluates to false.
*
* \note Will only be compiled in when debug support is enabled.
*/
#if defined (DEBUG)
#define STATIC_ASSERT(const_expr, msg) \
    {\
      class ERROR_##msg {}; \
      (void) (new FEAST::CompileTimeChecker<\
        (const_expr) != 0>((ERROR_##msg())));\
    }
#else
#define STATIC_ASSERT(const_expr, msg)
#endif
}

#endif //UTIL_ASSERTION_HPP
