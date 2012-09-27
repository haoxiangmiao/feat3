#pragma once
#ifndef KERNEL_CUBATURE_DRIVER_FACTORY_HPP
#define KERNEL_CUBATURE_DRIVER_FACTORY_HPP 1

// includes, FEAST
#include <kernel/cubature/rule.hpp>

namespace FEAST
{
  namespace Cubature
  {
    /// \cond internal
    namespace Intern
    {
      template<
        typename Driver_,
        bool variadic_ = (Driver_::variadic != 0)>
      class DriverFactoryAliasFunctor;
    } // namespace Intern
    /// \endcond

    template<
      template<typename,typename,typename,typename> class Driver_,
      typename Shape_,
      typename Weight_ = Real,
      typename Coord_ = Real,
      typename Point_ = Coord_[Shape_::dimension],
      bool variadic_ = (Driver_<Shape_, Weight_, Coord_, Point_>::variadic != 0)>
    class DriverFactory;

    template<
      template<typename,typename,typename,typename> class Driver_,
      typename Shape_,
      typename Weight_,
      typename Coord_,
      typename Point_>
    class DriverFactory<Driver_, Shape_, Weight_, Coord_, Point_, false> :
      public Rule<Shape_, Weight_, Coord_, Point_>::Factory
    {
    public:
      typedef Rule<Shape_, Weight_, Coord_, Point_> RuleType;
      typedef Driver_<Shape_, Weight_, Coord_, Point_> DriverType;
      typedef Shape_ ShapeType;
      typedef Weight_ WeightType;
      typedef Coord_ CoordType;
      typedef Point_ PointType;
      enum
      {
        variadic = 0,
        num_points = DriverType::num_points
      };

    public:
      DriverFactory()
      {
      }

      virtual RuleType produce() const
      {
        return create();
      }

      static RuleType create()
      {
        RuleType rule(DriverType::num_points, DriverType::name());
        DriverType::fill(rule);
        return rule;
      }

      static bool create(RuleType& rule, const String& name)
      {
        // map alias names
        Intern::DriverFactoryAliasFunctor<DriverType> functor(name);
        DriverType::alias(functor);
        String mapped_name(functor.name());

        // check mapped name
        if(mapped_name.trim().compare_no_case(DriverType::name()) != 0)
          return false;
        rule = create();
        return true;
      }

      static String name()
      {
        return DriverType::name();
      }

      template<typename Functor_>
      static void alias(Functor_& functor)
      {
        DriverType::alias(functor);
      }
    }; // class DriverFactory<...,false>

    template<
      template<typename,typename,typename,typename> class Driver_,
      typename Shape_,
      typename Weight_,
      typename Coord_,
      typename Point_>
    class DriverFactory<Driver_, Shape_, Weight_, Coord_, Point_, true> :
      public Rule<Shape_, Weight_, Coord_, Point_>::Factory
    {
    public:
      typedef Rule<Shape_, Weight_, Coord_, Point_> RuleType;
      typedef Driver_<Shape_, Weight_, Coord_, Point_> DriverType;
      typedef Shape_ ShapeType;
      typedef Weight_ WeightType;
      typedef Coord_ CoordType;
      typedef Point_ PointType;
      enum
      {
        variadic = 1,
        min_points = DriverType::min_points,
        max_points = DriverType::max_points
      };

    protected:
      Index _num_points;

    public:
      explicit DriverFactory(Index num_points) :
        _num_points(num_points)
      {
        ASSERT_(num_points >= DriverType::min_points);
        ASSERT_(num_points <= DriverType::max_points);
      }

      virtual RuleType produce() const
      {
        return create(_num_points);
      }

      static RuleType create(Index num_points)
      {
        ASSERT_(num_points >= DriverType::min_points);
        ASSERT_(num_points <= DriverType::max_points);

        RuleType rule(DriverType::count(num_points), (DriverType::name() + ":" + stringify(num_points)));
        DriverType::fill(num_points, rule);
        return rule;
      }

      static bool create(RuleType& rule, const String& name)
      {
        // map alias names
        Intern::DriverFactoryAliasFunctor<DriverType> functor(name);
        DriverType::alias(functor);
        String mapped_name(functor.name());

        // try to find a colon within the string
        String::size_type k = mapped_name.find_first_of(':');
        if(k == mapped_name.npos)
          return false;

        // extract substrings until the colon
        String head(mapped_name.substr(0, k));
        String tail(mapped_name.substr(k + 1));

        // check head - this is the name of the formula
        if(head.trim().compare_no_case(DriverType::name()) != 0)
          return false;

        // check substring
        Index num_points = 0;
        if(!tail.trim().parse(num_points))
          return false;

        // try to create the rule
        rule = create(num_points);
        return true;
      }

      static String name()
      {
        return DriverType::name();
      }

      template<typename Functor_>
      static void alias(Functor_& functor)
      {
        DriverType::alias(functor);
      }
    }; // class DriverFactory<...,true>

    /// \cond internal
    namespace Intern
    {
      template<typename Driver_>
      class DriverFactoryAliasFunctor<Driver_, false>
      {
      private:
        String _name;
        bool _mapped;

      public:
        explicit DriverFactoryAliasFunctor(const String& name) :
          _name(name),
          _mapped(false)
        {
        }

        void alias(const String& alias_name)
        {
          if(!_mapped)
          {
            if(_name.compare_no_case(alias_name) == 0)
            {
              _name = Driver_::name();
              _mapped = true;
            }
          }
        }

        String name()
        {
          return _name;
        }
      };

      template<typename Driver_>
      class DriverFactoryAliasFunctor<Driver_, true>
      {
      private:
        String _name;
        bool _mapped;

      public:
        explicit DriverFactoryAliasFunctor(const String& name) :
          _name(name),
          _mapped(false)
        {
        }

        void alias(const String& alias_name, Index num_points)
        {
          if(!_mapped)
          {
            if(_name.compare_no_case(alias_name) == 0)
            {
              _name = Driver_::name() + ":" + stringify(num_points);
              _mapped = true;
            }
          }
        }

        String name()
        {
          return _name;
        }
      };
    } // namespace Intern
    /// \endcond
  } // namespace Cubature
} // namespace FEAST

#endif // KERNEL_CUBATURE_DRIVER_FACTORY_HPP
