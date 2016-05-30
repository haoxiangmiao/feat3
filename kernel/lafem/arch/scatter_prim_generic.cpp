// includes, FEAT
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/lafem/arch/scatter_prim.hpp>

#include <cstring>

using namespace FEAT;
using namespace FEAT::LAFEM;
using namespace FEAT::LAFEM::Arch;

template void ScatterPrim<Mem::Main>::dv_csr_generic(float*, const float*, const unsigned long*, const float*, const unsigned long*, const Index);
template void ScatterPrim<Mem::Main>::dv_csr_generic(double*, const double*, const unsigned long*, const double*, const unsigned long*, const Index);
template void ScatterPrim<Mem::Main>::dv_csr_generic(float*, const float*, const unsigned int*, const float*, const unsigned int*, const Index);
template void ScatterPrim<Mem::Main>::dv_csr_generic(double*, const double*, const unsigned int*, const double*, const unsigned int*, const Index);
