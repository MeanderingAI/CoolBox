#include "../headers/bloom_filter.h"

#include <string>

namespace data_structures {

template class BloomFilter<int>;
template class BloomFilter<std::string>;

template class BloomFilter<std::uint64_t>;

} // namespace data_structures
