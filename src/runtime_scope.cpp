#include <tang/runtime_scope.h>

namespace tang {

// Initialize static members
std::atomic<int> RuntimeScope::instance_count_{0};
std::atomic<bool> RuntimeScope::runtime_initialized_{false};
std::atomic<bool> RuntimeScope::runtime_running_{false};

} // namespace tang