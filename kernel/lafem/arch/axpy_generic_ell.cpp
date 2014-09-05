// includes, FEAST
#include <kernel/base_header.hpp>
#include <kernel/archs.hpp>
#include <kernel/lafem/arch/axpy.hpp>

#include <cstring>

using namespace FEAST;
using namespace FEAST::LAFEM;
using namespace FEAST::LAFEM::Arch;

template void Axpy<Mem::Main, Algo::Generic>::ell(float *, const float, const float * const, const float * const, const float * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(float *, const float, const float * const, const float * const, const float * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(double *, const double, const double * const, const double * const, const double * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(double *, const double, const double * const, const double * const, const double * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index);

template void Axpy<Mem::Main, Algo::Generic>::ell(float *, const float * const, const float * const, const float * const, const float * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(float *, const float * const, const float * const, const float * const, const float * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(double *, const double * const, const double * const, const double * const, const double * const, const unsigned long * const, const unsigned long * const, const unsigned long * const, const Index, const Index);
template void Axpy<Mem::Main, Algo::Generic>::ell(double *, const double * const, const double * const, const double * const, const double * const, const unsigned int * const, const unsigned int * const, const unsigned int * const, const Index, const Index);