#pragma once
#ifndef KERNEL_UTIL_COMPILER_MICROSOFT_HPP
#define KERNEL_UTIL_COMPILER_MICROSOFT_HPP 1

/**
 * \file compiler_microsoft.hpp
 *
 * \brief Compiler detection header for Microsoft Visual C++ compiler.
 *
 * \author Peter Zajac
 */
#if !defined(FEAST_COMPILER) && defined(_MSC_VER)

// define FEAST_COMPILER_MICROSOFT macro
#  define FEAST_COMPILER_MICROSOFT _MSC_VER

// detect the compiler verson and define the FEAST_COMPILER macro
#  if (_MSC_VER >= 1800)
#    define FEAST_COMPILER "Microsoft Visual C++ 2013 (or newer)"
#  elif (_MSC_VER >= 1700)
#    define FEAST_COMPILER "Microsoft Visual C++ 2012"
#  elif (_MSC_VER >= 1600)
#    define FEAST_COMPILER "Microsoft Visual C++ 2010"
#  elif (_MSC_VER >= 1500)
#    define FEAST_COMPILER "Microsoft Visual C++ 2008"
#  elif (_MSC_VER >= 1400)
#    define FEAST_COMPILER "Microsoft Visual C++ 2005"
#  elif (_MSC_VER >= 1310)
#    define FEAST_COMPILER "Microsoft Visual C++ .NET 2003"
#  elif (_MSC_VER >= 1300)
#    define FEAST_COMPILER "Microsoft Visual C++ .NET 2002"
#  elif (_MSC_VER >= 1200)
#    define FEAST_COMPILER "Microsoft Visual C++ 6"
#  else
  // old MS C/C++ compiler, the time before Visual Studio,
  // this compiler version won't be able to compile Feast anyway...
#    define FEAST_COMPILER "Microsoft C/C++ compiler"
#  endif

#  define FEAST_IVDEP __pragma(loop(ivdep))

#define FEAST_DISABLE_WARNINGS __pragma(warning(push, 0))
#define FEAST_RESTORE_WARNINGS __pragma(warning(pop))

// define the noinline specifier
#define NOINLINE __declspec(noinline)

// C4061: enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
// This warning is emitted when a 'switch' handles one or more cases using a 'default' block.
// Note: If there are unhandled cases and there is no default block, the compiler emits a C4062 warning.
#  pragma warning(disable: 4061)

// C4127: conditional expression is constant
// This warning arises for instance in an expression like 'if(true)'.
#  pragma warning(disable: 4127)

// C4180: qualifier applied to function type has no meaning; ignored
// This warning arises when a non-pointer return type of a function is declared as 'const'.
#  pragma warning(disable: 4180)

// C4503: 'identifier': decorated name length exceeded, name was truncated
// This warning arises from heavy template nesting, blowing the compiler's limit on maximal name lengths.
// Running into this warning does not affect the correctness of the program, however, it might confuse
// the debugger.
#  pragma warning(disable: 4503)

// C4512: 'class': assignment operator could not be generated
#  pragma warning(disable: 4512)

// C4514: 'function': unreferenced inline function has been removed
// This is an annoying optimisation information.
#  pragma warning(disable: 4514)

// C4555: expression has no effect; expected expression with side-effect
#  pragma warning(disable: 4555)

// C4571: Informational: catch(...) semantics changed since Visual C++ 7.1;
//        structured exceptions (SEH) are no longer caught
#  pragma warning(disable: 4571)

// C4625: 'derived class': copy constructor could not be generated because
//        base class copy constructor is inaccessible
// This warning arises from our non-copyable instantiation policy.
#  pragma warning(disable: 4625)

// C4626: 'derived class': assignment operator could not be generated because
//        base class assignment operator is inaccessible
// This warning arises from our non-copyable instantiation policy.
#  pragma warning(disable: 4626)

// C4702: unreachable code
// This warning is emitted in any case where the compiler detects a statement which will never be executed.
// This happens e.g. in for-loops which always perform zero iterations due to the chosen template parameters,
// buzzword 'dead code elimination'.
#  pragma warning(disable: 4702)

// C4710: 'function': function not inlined
// This is an annoying optimisation information.
#  pragma warning(disable: 4710)

// C4711: function 'function' selected for inline expansion
// This is an annoying optimisation information.
#  pragma warning(disable: 4711)

// C4738: storing 32-bit float result in memory, possible loss of performance
// This is an optimisation warning, which arises from the strict fp-model.
#  pragma warning(disable: 4738)

// C4820: 'bytes' bytes padding added after construct 'member_name'
// This warning is disabled as it is mass-produced when compiling standard libraries.
#  pragma warning(disable: 4820)

// C5024: 'class' : move constructor was implicitly defined as deleted
// C5025: 'class' : move assignment operator was implicitly defined as deleted
// C5026: 'class' : move constructor was implicitly defined as deleted because
//                  a base class move constructor is inaccessible or deleted
// C5027: 'class' : move assignment operator was implicitly defined as deleted because
//                  a base class move assignment operator is inaccessible or deleted
#  pragma warning(disable: 5024)
#  pragma warning(disable: 5025)
#  pragma warning(disable: 5026)
#  pragma warning(disable: 5027)

// disable CRT security warnings for standard C/C++ library functions
#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS 1
#endif

#endif // !defined(FEAST_COMPILER) && defined(_MSC_VER)

#endif // KERNEL_UTIL_COMPILER_MICROSOFT_HPP
