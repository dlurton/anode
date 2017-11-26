
#pragma once
#include <string>
#include <memory>
#include <vector>
#include <algorithm>

namespace anode { namespace string {

    template<typename ... Args>
    std::string format( const std::string& format, Args ... args )
    {
        // TODO:  would like to very much to be able to check for formatting error in errno as specified here:
        // http://en.cppreference.com/w/cpp/io/c/fprintf

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
        size_t size = (size_t)snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        std::unique_ptr<char[]> buf( new char[ size ] );
        snprintf( buf.get(), size, format.c_str(), args ... );
#pragma GCC diagnostic pop

        return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }

    inline std::vector<std::string> split(const std::string str, std::string delim) {
        std::vector<std::string> parts;

        auto start = 0U;
        auto end = str.find(delim);
        while (end != std::string::npos)
        {
            parts.push_back(str.substr(start, end - start));
            start = end + delim.length();
            end = str.find(delim, start);
        }

        parts.push_back(str.substr(start, end));

        return parts;
    }

    // trim from start (in place)
    static inline void ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
            return !std::isspace(ch);
        }));
    }

    // trim from end (in place)
    static inline void rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

    // trim from both ends (in place)
    static inline void trim(std::string &s) {
        ltrim(s);
        rtrim(s);
    }

    // trim from start (copying)
    static inline std::string ltrim_copy(std::string s) {
        ltrim(s);
        return s;
    }

    // trim from end (copying)
    static inline std::string rtrim_copy(std::string s) {
        rtrim(s);
        return s;
    }

    // trim from both ends (copying)
    static inline std::string trim_copy(std::string s) {
        trim(s);
        return s;
    }
}}