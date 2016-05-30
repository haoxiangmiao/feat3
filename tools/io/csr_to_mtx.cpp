#include <kernel/base_header.hpp>
#include <kernel/lafem/sparse_matrix_csr.hpp>
#include <iostream>

using namespace FEAT;
using namespace FEAT::LAFEM;

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        std::cout<<"Usage 'csr2mtx csr-file mtx-file'"<<std::endl;
        exit(EXIT_FAILURE);
    }

    String input(argv[1]);
    String output(argv[2]);

    SparseMatrixCSR<Mem::Main, double> csr(FileMode::fm_csr, input);
    csr.write_out(FileMode::fm_mtx, output);
}
