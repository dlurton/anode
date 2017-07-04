#include <linenoise.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

extern void do_antlr4_demo();
extern void do_llvm_demo();



static const char* examples[] = {
        "db", "hello", "hallo", "hans", "hansekogge", "seamann", "quetzalcoatl", "quit", "power", NULL
};

void completionHook (char const* prefix, linenoiseCompletions* lc) {
    size_t i;

    for (i = 0;  examples[i] != NULL; ++i) {
        if (strncmp(prefix, examples[i], strlen(prefix)) == 0) {
            linenoiseAddCompletion(lc, examples[i]);
        }
    }
}


int main(int argc, char **argv) {
    do_antlr4_demo();
    do_llvm_demo();


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
    linenoiseSetCompletionCallback(completionHook);

    printf("starting...\n");

    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";

    bool keepGoing = true;
    while (keepGoing) {
        char* result = linenoise(prompt);

        if (result == NULL) {
            break;
        } else if (!strncmp(result, "/history", 8)) {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        }
        if (*result == '\0') {
            keepGoing = false;
        } else {
            printf("You said: %s\n", result);
            linenoiseHistoryAdd(result);
        }

        free(result);
        linenoiseHistorySave(file);
    }

    printf("Saving history...");
    linenoiseHistoryFree();

    return 0;
}

