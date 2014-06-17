#pragma once
#ifndef KERNEL_UTIL_COMPILER_ORACLE_HPP
#define KERNEL_UTIL_COMPILER_ORACLE_HPP 1

/**
 * \file compiler_oracle.hpp
 *
 * \brief Compiler detection header for SunStudio/OracleStudio compilers.
 *
 * \author Dominik Goeddeke
 */

#if !defined(FEAST_COMPILER) && defined(__SUNPRO_CC)

// define FEAST_COMPILER_ORACLE macro
#  define FEAST_COMPILER_ORACLE __SUNPRO_CC

// map version to human-readable string and add a few
// C++0x-specific details.
#  if (__SUNPRO_CC == 0x5110)
//   SunStudio does not support nullptr, do not define HAVE_CPP0X_NULLPTR 1
#    define FEAST_COMPILER "SunStudio/OracleStudio C/C++ compiler 12.2"
#  else
#    define FEAST_COMPILER "SunStudio/OracleStudio C/C++ compiler (unknown version)"
#  endif

#  define FEAST_IVDEP

#endif // !defined(FEAST_COMPILER) && defined(__SUNPRO_CC)

#endif // KERNEL_UTIL_COMPILER_ORACLE_HPP
