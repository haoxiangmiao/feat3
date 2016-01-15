#pragma once
#ifndef KERNEL_CUBATURE_AUTO_ALIAS_HPP
#define KERNEL_CUBATURE_AUTO_ALIAS_HPP 1

// includes, FEAST
#include <kernel/shape.hpp>
#include <kernel/cubature/scalar/gauss_legendre_driver.hpp>
#include <kernel/cubature/lauffer_driver.hpp>
#include <kernel/cubature/hammer_stroud_driver.hpp>
#include <kernel/cubature/dunavant_driver.hpp>

// includes, STL
#include <vector>
#include <algorithm>

namespace FEAST
{
  namespace Cubature
  {
    /// \cond internal
    namespace Intern
    {
      template<typename Shape_>
      class AutoDegree;
    }
    /// \endcond

    template<typename Shape_>
    class AutoAlias
    {
    public:
      /// Maximum specialised auto-degree parameter.
      static constexpr int max_auto_degree = Intern::AutoDegree<Shape_>::max_degree;

      static String map(const String& name)
      {
        // split the name into its parts
        std::vector<String> parts;
        name.split_by_charset(parts, ":");

        // ensure that we have at least two parts
        if(parts.size() < std::size_t(2))
          return name;

        // fetch the last two parts
        String param(parts.back());
        parts.pop_back();
        String auto_part(parts.back());
        parts.pop_back();

        // try to split the auto-part
        std::vector<String> args;
        auto_part.split_by_charset(args, "-");

        // does this identify an auto-alias rule?
        if(args.front().compare_no_case("auto") != 0)
          return name;

        // auto-degree?
        if((args.size() == std::size_t(2)) && (args.back().compare_no_case("degree") == 0))
        {
          // try to parse the degree
          Index degree = 0;
          if(!param.parse(degree))
            return name; // failed to parse degree

          // map auto-degree rule alias
          String alias(Intern::AutoDegree<Shape_>::choose(degree));

          // join up with remaining prefix parts (if any)
          if(!parts.empty())
            return String().join(parts, ":").append(":").append(alias);
          else
            return alias;
        }

        // unknown auto-alias
        return name;
      }
    };

    // \cond internal
    namespace Intern
    {
      template<>
      class AutoDegree< Shape::Simplex<1> >
      {
      public:
        // We choose the Gauss-Legendre cubature rule, so our maximum degree is 2*n-1.
        static constexpr int max_degree = 2*Scalar::GaussLegendreDriver::max_points - 1;

        static String choose(Index degree)
        {
          // k-point Gauss-Legendre cubature is exact up to a degree of 2*k-1,
          // so for a degree of n we need k := (n+2)/2 = n/2 + 1
          Index k = degree/2 + 1;

          // Also, ensure that we respect the currently implemented borders...
          k = std::max(k, Index(Scalar::GaussLegendreDriver::min_points));
          k = std::min(k, Index(Scalar::GaussLegendreDriver::max_points));

          // and build the name
          return
#ifdef FEAST_CUBATURE_SCALAR_PREFIX
            "scalar:" +
#endif
            Scalar::GaussLegendreDriver::name() + ":" + stringify(k);
        }
      };

      template<>
      class AutoDegree< Shape::Simplex<2> >
      {
      public:
        static constexpr int max_degree = 19;

        static String choose(Index degree)
        {
          switch(int(degree))
          {
          case 0:
          case 1:
            return BarycentreDriver<Shape::Simplex<2> >::name();
          case 2:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":2";
          case 3: // dunavant:3 has negative weights
          case 4:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":4";
          case 5:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":5";
          case 6:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":6";
          case 7: // dunavant:7 has negative weights
          case 8:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":8";
          case 9:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":9";
          case 10:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":10";
          case 11: // dunavant:11 has points outside the element
          case 12:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":12";
          case 13:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":13";
          case 14:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":14";
          case 15: // dunavant:15 has points outside the element
          case 16: // dunavant:16 has points outside the element
          case 17:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":17";
          //case 18: // dunavant:18 has points outside the element and negative weights
          //case 19:
          default:
            return DunavantDriver<Shape::Simplex<2> >::name() + ":19";
          }
        }
      };

      template<>
      class AutoDegree< Shape::Simplex<3> >
      {
      public:
        static constexpr int max_degree = 5;

        static String choose(Index degree)
        {
          if(degree <= 1) // barycentre
          {
            return BarycentreDriver<Shape::Simplex<3> >::name();
          }
          else if(degree == 2) // Hammer-Stroud of degree 2
          {
            return HammerStroudD2Driver<Shape::Simplex<3> >::name();
          }
          else if(degree == 3) // Hammer-Stroud of degree 3
          {
            return HammerStroudD3Driver<Shape::Simplex<3> >::name();
          }
          else if(degree == 4) // Lauffer formula of degree 4 only for tetrahedron
          {
            return LaufferD4Driver<Shape::Simplex<3> >::name();
          }
          else // Hammer-Stroud of degree 5 only for tetrahedra
          {
            return HammerStroudD5Driver<Shape::Simplex<3> >::name();
          }
        }
      };

      template<int dim_>
      class AutoDegree< Shape::Hypercube<dim_> >
      {
      public:
        // We choose the Gauss-Legendre cubature rule, so our maximum degree is 2*n-1.
        static constexpr int max_degree = 2*Scalar::GaussLegendreDriver::max_points - 1;

        static String choose(Index degree)
        {
          // k-point Gauss-Legendre cubature is exact up to a degree of 2*k-1,
          // so for a degree of n we need k := (n+2)/2 = n/2 + 1
          Index k = degree/2 + 1;

          // Also, ensure that we respect the currently implemented borders...
          k = std::max(k, Index(Scalar::GaussLegendreDriver::min_points));
          k = std::min(k, Index(Scalar::GaussLegendreDriver::max_points));

          // and build the name
          return
#ifdef FEAST_CUBATURE_TENSOR_PREFIX
            "tensor:" +
#endif
            Scalar::GaussLegendreDriver::name() + ":" + stringify(k);
        }
      };
    } // namespace Intern
    /// \endcond
  } // namespace Cubature
} // namespace FEAST

#endif // KERNEL_CUBATURE_AUTO_ALIAS_HPP
