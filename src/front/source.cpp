

#include "../include/lwnn/front/source.h"

namespace lwnn {
    namespace source {
        SourceSpan SourceSpan::Any("?", SourceLocation(-1, -1), SourceLocation(-1, -1));

        //SourceLocation equality operators
        bool operator== (SourceLocation a, SourceLocation b) {
            return a.line() == b.line() && a.position() == b.position();
        }

        bool operator!= (SourceLocation a, SourceLocation b) {
            return !(a == b);
        }
        //SourceSpan equality operators
        bool operator==(SourceSpan &a, SourceSpan &b) {
            return a.name() == b.name() && a.start() == b.start() && a.end() == b.end();
        }

        bool operator!=(SourceSpan &a, SourceSpan &b) {
            return !(a == b);
        }
    }
}