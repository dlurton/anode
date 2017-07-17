
#include "backtrace.h"

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

namespace lwnn {
    namespace backtrace {

        void handler(int sig) {
            void *array[10];
            int size;

            // get void*'s for all entries on the stack
            size = ::backtrace(array, 10);

            // print out all the frames to stderr
            fprintf(stderr, "Error: signal %d:\n", sig);
            backtrace_symbols_fd(array, size, STDERR_FILENO);
            exit(1);
        }

        void initBacktraceDumper() {
            signal(SIGSEGV, handler);
            signal(SIGABRT, handler);
        }
    }
}