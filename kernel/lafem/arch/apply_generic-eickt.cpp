// includes, FEAT
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/lafem/arch/apply.hpp>

#include <cstring>

using namespace FEAT;
using namespace FEAT::LAFEM;
using namespace FEAT::LAFEM::Arch;

template void Apply<Mem::Main>::csr_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::csr_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::csr_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::csr_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index, const bool);

template void Apply<Mem::Main>::cscr_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::cscr_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::cscr_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index, const Index, const bool);
template void Apply<Mem::Main>::cscr_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index, const Index, const bool);

template void Apply<Mem::Main>::coo_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index);
template void Apply<Mem::Main>::coo_generic(float *, const float, const float * const, const float, const float * const, const float * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index);
template void Apply<Mem::Main>::coo_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned long * const, const unsigned long * const, const Index, const Index, const Index);
template void Apply<Mem::Main>::coo_generic(double *, const double, const double * const, const double, const double * const, const double * const, const unsigned int * const, const unsigned int * const, const Index, const Index, const Index);

template void Apply<Mem::Main>::dense_generic(float *, const float, const float, const float * const, const float * const, const float * const, const Index, const Index);
template void Apply<Mem::Main>::dense_generic(double *, const double, const double, const double * const, const double * const, const double * const, const Index, const Index);
