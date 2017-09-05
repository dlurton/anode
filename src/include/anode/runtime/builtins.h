
#pragma once

#include <unordered_map>

namespace anode { namespace runtime {

std::unordered_map<std::string, void*> getBuiltins();

extern unsigned int AssertPassCount;

}}