#include <test_system/test_system.hpp>
#include <test_system/cuda.hpp>

#ifdef FEAST_TESTING_VC
extern "C" {
unsigned int __stdcall GetErrorMode(void);
unsigned int __stdcall SetErrorMode(unsigned int);
}
#endif // FEAST_TESTING_VC

using namespace FEAST;
using namespace FEAST::TestSystem;

int main(int argc, char** argv)
{
#ifdef FEAST_TESTING_VC
  // disable any error prompts for testing
  SetErrorMode(GetErrorMode() | 0x8003);
  // disable handling of abort function
  _set_abort_behavior(0, 0x1);
#else
  std::cout << "CTEST_FULL_OUTPUT" << std::endl;
#endif

  int result(EXIT_SUCCESS);
#ifdef FEAST_BACKENDS_CUDA
  bool cudadevicereset(false);
#endif

  if(argc > 1)
  {
    std::list<String> labels;
    for(int i(1) ; i < argc ; ++i)
    {
      labels.push_back(argv[i]);
    }

#ifdef FEAST_BACKENDS_CUDA
    if (find(labels.begin(), labels.end(), "cudadevicereset") != labels.end())
      cudadevicereset = true;
#endif
    for(TestList::Iterator i(TestList::instance()->begin_tests()), i_end(TestList::instance()->end_tests()) ;
        i != i_end ; )
    {
      if((find(labels.begin(), labels.end(), (*i)->get_memory_name()) == labels.end()) &&
          (find(labels.begin(), labels.end(), (*i)->get_algo_name()) == labels.end()) &&
          (find(labels.begin(), labels.end(), (*i)->get_prec_name()) == labels.end()) &&
          (find(labels.begin(), labels.end(), (*i)->get_index_name()) == labels.end()) )
      {
        i = TestList::instance()->erase(i);
        continue;
      }
      ++i;
    }
  }

  size_t list_size(TestList::instance()->size());
  size_t tests_passed(0u);
  size_t tests_failed(0u);
  unsigned long iterator_index(1);
  for(TestList::Iterator i(TestList::instance()->begin_tests()), i_end(TestList::instance()->end_tests()) ;
      i != i_end ; )
  {
    CONTEXT("When running test case '" + (*i)->id() + ":");
    try
    {
      std::cout << "(" << iterator_index << "/" << list_size << ") " << (*i)->id()
        << " [Memory: " << (*i)->get_memory_name() << "]"
        << " [Algo: " << (*i)->get_algo_name() << "]"
        << " [Precision: "<< (*i)->get_prec_name() << "]"
        << " [Indexing: "<< (*i)->get_index_name() << "]"
        << std::endl;
      (*i)->run();
      std::cout << "PASSED" << std::endl;
      ++tests_passed;
    }
    catch (TestFailedException & e)
    {
      std::cout << "FAILED: " << (*i)->id() << std::endl << stringify(e.what()) << std::endl;
      result = EXIT_FAILURE;
      ++tests_failed;
    }
    i = TestList::instance()->erase(i);
    iterator_index++;
  }

  for(TestList::Iterator i(TestList::instance()->begin_tests()), i_end(TestList::instance()->end_tests()) ;
      i != i_end ; )
  {
    i = TestList::instance()->erase(i);
  }

  if(result == EXIT_SUCCESS)
  {
    std::cout << "All " << list_size << " tests PASSED!" << std::endl;
  }
  else
  {
    std::cout << tests_passed << " of " << list_size << " tests PASSED, "
      << tests_failed << " tests FAILED!" << std::endl;
  }

#ifdef FEAST_BACKENDS_CUDA
  if (cudadevicereset)
    reset_device();
#endif

  return result;
}
