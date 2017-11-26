
#include "front/unique_id.h"

#include <atomic>

namespace anode { namespace front {

namespace {
std::atomic<UniqueId> nextUniqueId{1};
}

UniqueId GetNextUniqueId() {
    return nextUniqueId++;
}

}}
