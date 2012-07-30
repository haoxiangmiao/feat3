#pragma once
#ifndef KERNEL_FOUNDATION_HALO_DATA_HH
#define KERNEL_FOUNDATION_HALO_DATA_HH 1

#include <kernel/foundation/halo.hpp>
#include <iostream>
#include <cmath>

namespace FEAST
{
  namespace Foundation
  {
    template<
      typename HaloType_,
      template<typename> class VectorType_,
      typename IndexType_ = Index>
    class HaloData
    {
      public:
        HaloData(HaloType_ & halo) :
          _halo(halo),
          _halo_elements(VectorType_<IndexType_>(halo.size())),
          _halo_element_counterparts(VectorType_<IndexType_>(halo.size()))
        {
          for(IndexType_ i(0) ; i < halo.size() ; ++i)
          {
            _halo_element_counterparts[i] = halo.get_element_counterpart(i);
            _halo_elements[i] = halo.get_element(i);
          }
        }

        ~HaloData()
        {
        }

        HaloType_ & get_halo()
        {
          return _halo;
        }

        IndexType_ get_element_counterpart(IndexType_ index)
        {
          return _halo_element_counterparts[index];
        }

        IndexType_ get_element(IndexType_ index)
        {
          return _halo_elements[index];
        }

        IndexType_ size()
        {
          return _halo_elements.size();
        }

        typename HaloType_::mesh_type_ & get_mesh()
        {
          return _halo.get_mesh();
        }

        IndexType_ & get_other()
        {
          return _halo.get_other();
        }

        unsigned get_overlap()
        {
          return _halo.get_overlap();
        }


      private:
        HaloType_ & _halo;

        VectorType_<IndexType_> _halo_elements;
        VectorType_<IndexType_> _halo_element_counterparts;
    };
  }
}
#endif
