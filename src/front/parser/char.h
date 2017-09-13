
#pragma once

namespace anode { namespace front { namespace parser {

// These types are used everywhere in the parser in place of char and std::string in the hopes that they
// will make our lives easier if we ever decide to use UTF-16 or UTF-32 for input files.

typedef char char_t;
const unsigned int MAX_CHAR = 255;
typedef std::string string_t;

inline string_t to_string(char_t c) {
    string_t str;
    str += c;
    return str;
}

}}}