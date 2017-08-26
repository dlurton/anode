
#pragma once

#include <unordered_map>

namespace lwnn { namespace runtime {

std::unordered_map<std::string, void*> getBuiltins();


}}