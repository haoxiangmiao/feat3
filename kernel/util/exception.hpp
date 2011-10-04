#pragma once
#ifndef KERNEL_UTIL_EXCEPTION_HPP
#define KERNEL_UTIL_EXCEPTION_HPP 1

// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/util/instantiation_policy.hpp>
#include <kernel/util/string_utils.hpp>

// includes, system
#include <string>
#include <cstdlib>
#include <iostream>
#include <list>

namespace FEAST
{
#ifndef FEAST_NO_CONTEXT
  /// The global context stack.
  /// \todo Ist der stack global oder compile-unit lokal?
  std::list<std::string> * context_stack = 0;

  /**
  * \brief This structs holds the actual context history
  *
  * \author Dirk Ribbrock
  */
  struct ContextData
  {
    /// The local context stack
    std::list<std::string> local_context_stack;

    /// CTOR
    ContextData()
    {
      if (context_stack)
      {
        local_context_stack.assign(context_stack->begin(), context_stack->end());
      }
    }

    /// returns a full context stack aka backtrace
    std::string backtrace(const std::string& delimiter) const
    {
      if (context_stack)
      {
        return join_strings(local_context_stack.begin(), local_context_stack.end(), delimiter);
      }
      else return "";
    }
  };
#endif // FEAST_NO_CONTEXT

  /**
  * \brief Base exception class.
  *
  * \author Dirk Ribbrock
  */
  class Exception :
    public std::exception
  {
  private:
#ifndef FEAST_NO_CONTEXT
    /// Our (local) context data.
    ContextData * const _context_data;
#endif // FEAST_NO_CONTEXT

    /// descriptive error message
    const std::string _message;

    /// Our what string (for std::exception).
    mutable std::string _what_str;

  protected:
    /**
    * \brief CTOR
    *
    * \param message
    * the exception's message
    */
    Exception(const std::string & message) :
#ifndef FEAST_NO_CONTEXT
      _context_data(new ContextData),
#endif // FEAST_NO_CONTEXT
      _message(message)
    {
    }

    /// copy CTOR
    Exception(const Exception & other) :
      std::exception(other),
#ifndef FEAST_NO_CONTEXT
      _context_data(new ContextData(*other._context_data)),
#endif // FEAST_NO_CONTEXT
      _message(other._message)
    {
    }

  public:
    /// DTOR
    virtual ~Exception()
    {
#ifndef FEAST_NO_CONTEXT
      delete _context_data;
#endif // FEAST_NO_CONTEXT
    }

    /// returns error message
    const std::string message() const
    {
      return _message;
    }

#ifndef FEAST_NO_CONTEXT
    /// returns backtrace
    std::string backtrace(const std::string & delimiter) const
    {
      return _context_data->backtrace(delimiter);
    }
#endif // FEAST_NO_CONTEXT

    /// returns true if the backtrace is empty
    bool empty() const;

    /// return descriptive exception name
    const char * what() const
    {
      /// \todo Add a working win32 alternative (see http://www.int0x80.gr/papers/name_mangling.pdf)
      /*if (_what_str.empty())
      {
        int status(0);
        char * const name(abi::__cxa_demangle(("_Z" + stringify(std::exception::what())).c_str(), 0, 0, &status));
        if (0 == status)
        {
          _what_str = name;
          _what_str += " (" + message() + ")";
          std::free(name);
        }
      }*/
      if (_what_str.empty())
      {
        _what_str = stringify(std::exception::what());
        _what_str += " (" + message() + ")";
      }
      return _what_str.c_str();
    }
  };


  /**
  * \brief exception that is thrown if something that is never supposed to happen happens
  *
  * It simply prefixes the exception message with "Internal error: ", otherwise it does not differ from its
  * parent class Exception.
  *
  * \author Dirk Ribbrock
  */
  class InternalError :
    public Exception
  {
  public:
    /**
    * \brief Constructor.
    *
    * \param message
    * A short error message.
    */
    InternalError(const std::string & message) :
      Exception("Internal error: " + message)
    {
    }
  };

#ifndef FEAST_NO_CONTEXT
  /**
  * \brief Backtrace class context.
  *
  * \author Dirk Ribbrock
  */
  class Context
    : public InstantiationPolicy<Context, NonCopyable>
  {
  public:
    /**
    * \brief Constructor.
    *
    * \param file
    * name of the source file that contains the context
    * \param line
    * line number of the context
    * \param context
    * description of the context
    */
    Context(const char * const file, const long line, const std::string & context)
    {
      if (! context_stack)
      {
        context_stack = new std::list<std::string>;
      }

      context_stack->push_back(context + " (" + stringify(file) + ":" + stringify(line) +")");
    }

    /// DTOR
    ~Context()
    {
      if (! context_stack)
        throw InternalError("no context!");

      context_stack->pop_back();

      if (context_stack->empty())
      {
        delete context_stack;
        context_stack = 0;
      }
    }

    /**
    * \brief Current context
    *
    * \param[in] delimiter
    * A delimiter added between to context strings
    */
    static std::string backtrace(const std::string & delimiter)
    {
      if (! context_stack)
        return "";

      return join_strings(context_stack->begin(), context_stack->end(), delimiter);
    }
  };
#endif // FEAST_NO_CONTEXT

  /**
  * \def CONTEXT
  *
  * \brief Convenience definition that provides a way to declare uniquely-named instances of class Context.
  *
  * The created Context will be automatically provided with the correct filename and line number.
  *
  * \param s
  * Context message that can be display by an exception-triggered backtrace.
  *
  * \warning Will only be compiled in when debug support is enabled.
  */
#if defined (DEBUG) && !defined(FEAST_NO_CONTEXT)
  // C preprocessor abomination following...
#define CONTEXT_NAME_(x) ctx_##x
#define CONTEXT_NAME(x) CONTEXT_NAME_(x)
#define CONTEXT(s) \
  Context CONTEXT_NAME(__LINE__)(__FILE__, __LINE__, (s))
#else
#define CONTEXT(s)
#endif

} // namespace FEAST

#endif // KERNEL_UTIL_EXCEPTION_HPP
