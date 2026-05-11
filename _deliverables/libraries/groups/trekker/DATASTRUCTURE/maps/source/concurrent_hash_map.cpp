#include "../headers/concurrent_hash_map.h"

#include <string>

namespace data_structures {

template class ConcurrentHashMap<int, int>;
template class ConcurrentHashMap<int, std::string>;
template class ConcurrentHashMap<std::string, int>;
template class ConcurrentHashMap<std::string, std::string>;

} // namespace data_structures
