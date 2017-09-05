

#include <uuid/uuid.h>
#include <string>


namespace anode { namespace scope {

std::string createRandomUniqueName() {
    uuid_t uuid;
    uuid_generate(uuid);
    // unparse (to string)
    char uuid_str[37];      // ex. "1b4e28ba-2fa1-11d2-883f-0016d3cca427" + "\0"
    uuid_unparse_lower(uuid, uuid_str);

    std::string str{uuid_str};
    str.reserve(32);

    for(int i = 0; uuid_str[i] != 0; ++i) {
        if(uuid_str[i] != '-') {
            str.push_back(uuid_str[i]);
        }
    }
    return str;
}

}}