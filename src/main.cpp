#include <linenoise.h>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "lwnn.h"

//
//static const char* examples[] = {
//        "db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
//};
//
//void completionHook (char const* prefix, linenoiseCompletions* lc) {
//    size_t i;
//
//    for (i = 0;  examples[i] != NULL; ++i) {
//        if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
//            linenoiseAddCompletion(lc, examples[i]);
//        }
//    }
//}

int main(int argc, char **argv) {
    lwnn::do_llvm_demo();

    linenoiseInstallWindowChangeHandler();

    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        }
    }

    const char* file = "~/.lwnn_history";

    linenoiseHistoryLoad(file);
    //linenoiseSetCompletionCallback(completionHook);

    printf("starting...\n");

    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";

    bool keepGoing = true;
    while (keepGoing) {
        char* lineOfCode = linenoise(prompt);

        if (lineOfCode == NULL) {
            break;
        } else if (!strncmp(lineOfCode, "/history", 8)) {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        }
        if (*lineOfCode == '\0') {
            keepGoing = false;
        } else {
            lwnn::parse(lineOfCode);
            linenoiseHistoryAdd(lineOfCode);
        }

        free(lineOfCode);
        linenoiseHistorySave(file);
    }

    printf("Saving history...");
    linenoiseHistoryFree();

    return 0;
}

