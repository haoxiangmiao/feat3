#pragma once
#ifndef KERNEL_FOUNDATION_FUNCTOR_HH
#define KERNEL_FOUNDATION_FUNCTOR_HH 1

#include <vector>
#include<kernel/base_header.hpp>
#include<kernel/foundation/functor_error.hpp>

namespace FEAST
{
  namespace Foundation
  {
    /**
     * \brief FunctorBase wraps Foundation operation functors
     *
     * \author Markus Geveler
     */
    class FunctorBase
    {
      public:
        virtual void execute() = 0;
        virtual void undo() = 0;

        std::string name()
        {
          return _name;
        }

      protected:
        bool _executed;
        bool _undone;
        std::string _name;
    };


    /**
     * \brief STL conformal push_back(i) functor
     *
     * \author Markus Geveler
     */
    template<
      typename ContainerType_,
      typename IndexType_,
      typename ValueType_>
    class PushBackFunctor : public FunctorBase
    {
      public:
        PushBackFunctor(ContainerType_& target, IndexType_ position, ValueType_ value) :
          _target(target),
          _position(position),
          _value(value)
        {
          this->_executed = true;
          this->_undone = false;

          this->_name = "push_back(i)";
        }

        virtual void execute()
        {
          if(this->_executed)
            throw FunctorError("Already executed!");

          _target.push_back(_value);
          this->_executed = true;
          this->_undone = false;
        }

        virtual void undo()
        {
          if(this->_undone)
            throw FunctorError("Already undone!");

          _target.erase(_target.begin() + _position);
          this->_undone = true;
          this->_executed = false;
        }

        ContainerType_& get_target()
        {
          return _target;
        }

        IndexType_ get_position()
        {
          return _position;
        }

        ValueType_ get_value()
        {
          return _value;
        }

      private:
        ContainerType_& _target;
        IndexType_ _position;
        ValueType_ _value;
    };

    /**
     * \brief push_back() functor
     *
     * \author Markus Geveler
     */
    template<typename ContainerType_, typename IndexType_, typename ValueType_>
    class EmptyPushBackFunctor : public FunctorBase
    {
      public:
        EmptyPushBackFunctor(ContainerType_ target, IndexType_ position, ValueType_ value) :
          _target(target),
          _position(position),
          _value(value)
        {
          this->_executed = true;
          this->_undone = false;
          this->_name = "push_back()";
        }

        virtual void execute()
        {
          if(this->_executed)
            throw FunctorError("Already executed!");

          _target.push_back(_value);
          this->_executed = true;
          this->_undone = false;
        }

        virtual void undo()
        {
          if(this->_undone)
            throw FunctorError("Already undone!");

          _target.erase(_target.begin() + _position);
          this->_undone = true;
          this->_executed = false;
        }

      private:
        ContainerType_& _target;
        IndexType_ _position;
        ValueType_ _value;
    };

    /**
     * \brief STL conformal erase(i) functor
     *
     * \author Markus Geveler
     */
    template<
      typename ContainerType_,
      typename IndexType_,
      typename ValueType_>
    class EraseFunctor : public FunctorBase
    {
      public:
        EraseFunctor(ContainerType_& target, IndexType_ position, ValueType_ value) :
          _target(target),
          _position(position),
          _value(value)
        {
          this->_executed = true;
          this->_undone = false;

          this->_name = "erase(i)";
        }

        virtual void execute()
        {
          if(this->_executed)
            throw FunctorError("Already executed!");

          _target.erase(_target.begin() + _position);
          this->_executed = true;
          this->_undone = false;
        }

        virtual void undo()
        {
          if(this->_undone)
            throw FunctorError("Already undone!");

          _target.insert(_target.begin() + _position, _value);
          this->_undone = true;
          this->_executed = false;
        }

        ContainerType_& get_target()
        {
          return _target;
        }

        IndexType_ get_position()
        {
          return _position;
        }

        ValueType_ get_value()
        {
          return _value;
        }

      private:
        ContainerType_& _target;
        IndexType_ _position;
        ValueType_ _value;
    };

    /**
     * \brief push_back() functor
     *
     * \author Markus Geveler
     */
    template<typename ContainerType_, typename IndexType_, typename ValueType_>
    class EmptyEraseFunctor : public FunctorBase
    {
      public:
        EmptyEraseFunctor(ContainerType_ target, IndexType_ position, ValueType_ value) :
          _target(target),
          _position(position),
          _value(value)
        {
          std::cout << "EmptErase" << std::endl;
          this->_executed = true;
          this->_undone = false;
          this->_name = "erase()";
        }

        virtual void execute()
        {
          if(this->_executed)
            throw FunctorError("Already executed!");

          _target.erase(_target.end() - 1);
          this->_executed = true;
          this->_undone = false;
        }

        virtual void undo()
        {
          if(this->_undone)
            throw FunctorError("Already undone!");

          _target.push_back(_value);
          this->_undone = true;
          this->_executed = false;
        }

      private:
        ContainerType_& _target;
        IndexType_ _position;
        ValueType_ _value;
    };
  }
}
#endif
