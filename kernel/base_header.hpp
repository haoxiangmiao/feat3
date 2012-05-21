#pragma once
#ifndef KERNEL_BASE_HEADER_HPP
#define KERNEL_BASE_HEADER_HPP 1

/**
 * \file
 * \brief FEAST Kernel base header.
 *
 * This file is the base header for the FEAST kernel, which is included by all other FEAST header and source files.
 * It defines macros and data types which are frequently used in other files.
 */

// The CMake build system automatically generates a FEAST configuration header, which is included here.
// If we use the *hand-made* Visual Studio project files, which define the VISUAL_STUDIO macro, we'll have to
// include the hand-made configuration header from the 'visual_studio' directory.
#ifdef VISUAL_STUDIO
#  include <visual_studio/feast_config.hpp>   // hand-made for Visual Studio
#else
#  include <feast_config.hpp>                 // automatically generated by CMake
#endif

// Make sure the DOXYGEN macro is not defined at compile-time;
// it is reserved for doxygen's preprocessor.
#ifdef DOXYGEN
#  error The DOXYGEN macro must not be defined at compile-time
#endif

/// \cond nodoxy
// Activate DEBUG macro if the build system tells us to do so.
#if defined(FEAST_DEBUG_MODE) && !defined(DEBUG)
#  define DEBUG 1
#endif

// The DEBUG and NDEBUG macros are mutually exclusive
#if defined(DEBUG) && defined(NDEBUG)
#  error The DEBUG and NDEBUG macros must not be defined at the same time.
#endif

// Assure that either DEBUG or NDEBUG is defined
// In consequence, if DEBUG and NDEBUG are both undefined, NDEBUG is defined.
#if !defined (NDEBUG) && !defined(DEBUG)
#  define NDEBUG 1
#endif

// Activate SERIAL macro if the build system tells us to do so.
#if defined(FEAST_SERIAL_MODE) && !defined(SERIAL)
#  define SERIAL 1
#endif

// The SERIAL and PARALLEL macros are mutually exclusive
#if defined(SERIAL) && defined(PARALLEL)
#  error The SERIAL and PARALLEL macros must not be defined at the same time.
#endif

// Assure that either SERIAL or PARALLEL is defined
// In consequence, if SERIAL and PARALLEL are both undefined, PARALLEL is defined.
#if !defined(SERIAL) && !defined(PARALLEL)
#  define PARALLEL 1
#endif
/// \endcond

// include compiler detection headers
#include <kernel/util/compiler_intel.hpp>      // Intel(R) C/C++ compiler
#include <kernel/util/compiler_microsoft.hpp>  // Microsoft(R) (Visual) C/C++ compiler
#include <kernel/util/compiler_oracle.hpp>     // SunStudio/OracleStudio C/C++ compiler
#include <kernel/util/compiler_open64.hpp>     // Open64 C/C++ compiler
#include <kernel/util/compiler_pgi.hpp>        // PGI C/C++ compiler
// The GNU compiler must be the last one in this list, because other compilers (e.g. Intel and Open64)
// also define the __GNUC__ macro used to identify the GNU C/C++ compiler, thus leading to incorrect
// compiler detection.
#include <kernel/util/compiler_gnu.hpp>        // GNU C/C++ compiler

// hide the following block from doxygen
/// \cond nodoxy
// If the compiler doesn't support the C++11 nullptr, we have to define it via pre-processor.
// Be warned that this definition is not 100% compatible, as the "real" nullptr is of type "nullptr_t",
// whereas this macro definition is of type "int", thus leading to potentially unwanted behaviour,
// especially when function overloads for int and pointer types are present!
#ifndef HAVE_CPP11_NULLPTR
#  define nullptr 0
#endif

// If the compiler doesn't support the C++11 static_assert, we'll define it as an empty macro.
// We do not use any hand-made emulations in this case, as there is no way to implement a both
// fully working and portable solution for this feature.
#ifndef HAVE_CPP11_STATIC_ASSERT
#  define static_assert(expr, msg)
#endif

// If the compiler doesn't support the C99/C++11 __func__ built-in variable, we'll define it as the
// commonly used __FUNCTION__ macro. Unfortunately, this macro is not part of any C or C++ standard,
// so there's no guarantee that the following macro will do its job. Also, we cannot check
// "defined(__FUNCTION__)" here, because the __FUNCTION__ macro (if the compiler supports it at all)
// is only defined within a function body.
#ifndef HAVE_CPP11_FUNC
#  define __func__ __FUNCTION__
#endif


// TODO_PZAJAC: remove the following #define and adapt all necessary files using it.
//
// In order to increase performance one can ignore the status object in some MPI functions
// (see Section 3.2.6 in the MPI 2.2 standard). In debug mode, we use a real status object, otherwise
// the special MPI status MPI_STATUS_IGNORE. Requires that the MPI_Status object *always* has the name 'status'.
// How to use this feature?
// Whenever a status object is required, define it like this:
//   #ifndef NDEBUG
//     MPI_Status status;
//   #endif
// Within the corresponding MPI routines, then use "MPI_STATUS_MACRO" instead of "&status", e.g.
//   MPI_Recv(..., some_source, some_tag, some_comm, MPI_STATUS_MACRO);
#ifdef NDEBUG
#  define MPI_STATUS_MACRO MPI_STATUS_IGNORE
#else
#  define MPI_STATUS_MACRO &status
#endif

///\endcond
// end of block hidden from doxygen

/// FEAST namespace
namespace FEAST
{
  /// FEAST version
  enum Version
  {
    /// FEAST major version number
    version_major = 1,
    /// FEAST minor version number
    version_minor = 0,
    /// FEAST patch version number
    version_patch = 0
  };

  /**
   * \brief Index data type.
   */
  typedef unsigned long Index;

  /**
   * \brief Real data type.
   */
  typedef double Real;

} // namespace FEAST

#endif // KERNEL_BASE_HEADER_HPP
