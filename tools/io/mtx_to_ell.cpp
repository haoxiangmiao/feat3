#include <kernel/lafem/sparse_matrix_ell.hpp>
#include <kernel/lafem/sparse_matrix_coo.hpp>
#include <iostream>

using namespace FEAST;
using namespace FEAST::LAFEM;

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        std::cout<<"Usage 'mtx2ell mtx-file ell-file'"<<std::endl;
        exit(EXIT_FAILURE);
    }

    String input(argv[1]);
    String output(argv[2]);

    SparseMatrixCOO<Mem::Main, double> coo(fm_mtx, input);
    SparseMatrixELL<Mem::Main, double> ell(coo);
    ell.write_out(fm_ell, output);
}
