#pragma once
#ifndef KERNEL_UTIL_COMPILER_INTEL_HPP
#define KERNEL_UTIL_COMPILER_INTEL_HPP 1

/**
 * \file compiler_intel.hpp
 *
 * \brief Compiler detection header for Intel C++ compiler.
 *
 * \author Dominik Goeddeke
 */

#if !defined(FEAST_COMPILER) && defined(__INTEL_COMPILER)

// define FEAST_COMPILER_INTEL macro
// Note that __ICC is already linear sortable
#  define FEAST_COMPILER_INTEL __INTEL_COMPILER

// map version to human-readable string
#  if(__INTEL_COMPILER >= 1600)
#    define FEAST_COMPILER "Intel C/C++ compiler 16.0 (or newer)"
#  elif(__INTEL_COMPILER >= 1500)
#    define FEAST_COMPILER "Intel C/C++ compiler 15.0"
#  elif(__INTEL_COMPILER >= 1400)
#    define FEAST_COMPILER "Intel C/C++ compiler 14.0"
#  elif(__INTEL_COMPILER >= 1310)
#    define FEAST_COMPILER "Intel C/C++ compiler 13.1"
#  elif(__INTEL_COMPILER >= 1300)
#    define FEAST_COMPILER "Intel C/C++ compiler 13.0"
#  elif(__INTEL_COMPILER >= 1210)
#    define FEAST_COMPILER "Intel C/C++ compiler 12.1"
#  elif(__INTEL_COMPILER >= 1200)
#    define FEAST_COMPILER "Intel C/C++ compiler 12.0"
#  elif(__INTEL_COMPILER >= 1110)
#    define FEAST_COMPILER "Intel C/C++ compiler 11.1"
#  elif(__INTEL_COMPILER >= 1100)
#    define FEAST_COMPILER "Intel C/C++ compiler 11.0"
#  elif(__INTEL_COMPILER >= 1010)
#    define FEAST_COMPILER "Intel C/C++ compiler 10.1"
#  elif(__INTEL_COMPILER >= 1000)
#    define FEAST_COMPILER "Intel C/C++ compiler 10.0"
#  else
// too old to have a chance to support FEAST anyway
#    define FEAST_COMPILER "Intel C/C++ compiler"
#  endif

// Note: The ICC 14.0.x compilers have a bug which causes the compiler to choke
// on _Pragma statements in preprocessed include files; see
// https://software.intel.com/en-us/forums/topic/515154?language=en
// Therefore we skip any _Pragma definition for these versions, if any
// Compiler wrapper is active, causing these errors.
#if (__INTEL_COMPILER != 1400) || !defined(FEAST_USE_COMPILER_WRAPPER)

#  define FEAST_DISABLE_WARNINGS _Pragma("warning(push,0)") \
    _Pragma("warning(disable:177)") \
    _Pragma("warning(disable:858)")

#  define FEAST_RESTORE_WARNINGS _Pragma("warning(pop)")

#  define FEAST_IVDEP _Pragma("ivdep")

#endif //  (__INTEL_COMPILER != 1400) && defined(FEAST_USE_COMPILER_WRAPPER)

// disable warning #2196 (routine is both "inline" and "noinline") unconditionally
/// \todo evaluate, if the icc finally inlines or not the corresponding routines, marked by NOINLINE
_Pragma("warning(disable:2196)")

// define the noinline specifier
#define NOINLINE __attribute__((noinline))

#define FORCE_INLINE inline __forceinline

#endif // !defined(FEAST_COMPILER) && defined(__INTEL_COMPILER)

#endif // KERNEL_UTIL_COMPILER_INTEL_HPP
