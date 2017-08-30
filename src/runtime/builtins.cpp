#include "runtime/builtins.h"

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <common/exception.h>

namespace lwnn { namespace runtime {


extern "C" {
    void assert_fail(char *filename, unsigned int lineNo) {
        std::stringstream out;
        out << filename << ":" << lineNo << ": " << "ASSERTION FAILED" << std::endl;
        throw exception::LwnnAssertionFailedException(out.str());
        //std::abort();
    }
}

std::unordered_map<std::string, void*> getBuiltins() {

    std::unordered_map<std::string, void*> builtins {
        { "__assert_failed__", (void*)assert_fail }
    };

    return builtins;
}

}}