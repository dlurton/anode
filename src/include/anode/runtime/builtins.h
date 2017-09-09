
#pragma once

#include <unordered_map>

namespace anode { namespace runtime {

typedef uint64_t symbolptr_t;

std::unordered_map<std::string, symbolptr_t> getBuiltins();

extern unsigned int AssertPassCount;

}}