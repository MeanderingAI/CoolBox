#include "../headers/egh_filter.h"

#include <string>

namespace data_structures {

template class EGHFilter<int>;
template class EGHFilter<std::string>;

template class EGHFilter<std::uint64_t>;

} // namespace data_structures
