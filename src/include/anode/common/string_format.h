
#pragma once
#include <string>
#include <memory>

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
}}