#include <kernel/foundation/functor_error.hpp>
#include <string>

using namespace FEAST;
using namespace Foundation;

FunctorError::FunctorError(const std::string & message_in) :
    Exception(message_in)
{
}
