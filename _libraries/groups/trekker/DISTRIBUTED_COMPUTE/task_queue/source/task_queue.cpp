// TaskQueue is a header-only template class.
// Explicit instantiations for common types are provided here so that the
// symbols are available without callers having to include the full template.

#include "../headers/task_queue.h"

#include <functional>
#include <string>

namespace distributed {

template class TaskQueue<int>;
template class TaskQueue<std::string>;
template class TaskQueue<std::function<void()>>;

} // namespace distributed
