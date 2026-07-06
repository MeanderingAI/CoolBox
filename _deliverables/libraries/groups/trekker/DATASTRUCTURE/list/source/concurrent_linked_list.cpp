#include "../headers/concurrent_linked_list.h"

#include <string>

namespace data_structures {

template class ConcurrentLinkedList<int>;
template class ConcurrentLinkedList<double>;
template class ConcurrentLinkedList<std::string>;

} // namespace data_structures
