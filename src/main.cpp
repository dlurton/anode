#include "parse.h"
#include "ast_passes.h"
#include "execute.h"
#include "visualize.h"
#include "error.h"
#include "backtrace.h"

#include <linenoise.h>
#include <cstring>


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

namespace lwnn {
    void evaluateLine(std::string lineOfCode, bool shouldExecute);
}

int main(int argc, char **argv) {
    //lwnn::backtrace::initBacktraceDumper();
    linenoiseInstallWindowChangeHandler();

    while(argc > 1) {
        argc--;
        argv++;
        if (!strcmp(*argv, "--keycodes")) {
            linenoisePrintKeyCodes();
            exit(0);
        }
    }

    const char* historyFilename = "~/.lwnn_history";

    linenoiseHistoryLoad(historyFilename);
    //linenoiseSetCompletionCallback(completionHook);

    printf("starting...\n");

    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";

    bool keepGoing = true;
    bool shouldCompile = true;

    while (keepGoing) {
        char* lineOfCode = linenoise(prompt);

        if (lineOfCode == NULL) {
            break;
        } else if (!strncmp(lineOfCode, "/c", 8)) {
            shouldCompile = !shouldCompile;
            std::cout << "Compilation " << (shouldCompile ? "enabled" : "disabled") << "\n";
        } else if (!strncmp(lineOfCode, "/history", 8)) {
            /* Display the current history. */
            for (int index = 0; ; ++index) {
                char* hist = linenoiseHistoryLine(index);
                if (hist == NULL) break;
                printf("%4d: %s\n", index, hist);
                free(hist);
            }
        } else if (*lineOfCode == '\0') {
            keepGoing = false;
        } else {
            linenoiseHistoryAdd(lineOfCode);

            lwnn::evaluateLine(lineOfCode, shouldCompile);
        }

        free(lineOfCode);
        linenoiseHistorySave(historyFilename);
    }

    printf("Saving history...");
    linenoiseHistoryFree();

    return 0;
}

namespace lwnn {
    bool runModule(std::unique_ptr<ast::Module> lwnnModule, std::string &resultAsString) {
        ASSERT(lwnnModule);
        auto ec = execute::createExecutionContext();
        ec->setDumpIROnLoad(true);
        auto bodyExpr = dynamic_cast<ast::ExprStmt*>(lwnnModule->body());
        if(!bodyExpr) {
            ec->executeModule(std::move(lwnnModule));
            return false;
        } else {
            auto moduleInitResultType = bodyExpr->type();
            ASSERT(moduleInitResultType->isPrimitive() && "Non-primitive types not yet supported here");
            switch (moduleInitResultType->primitiveType()) {
                case type::PrimitiveType::Int32: {
                    int result = ec->executeModuleWithResult<int>(std::move(lwnnModule));
                    resultAsString = std::to_string(result);
                    return true;
                }
                case type::PrimitiveType::Float: {
                    float result = ec->executeModuleWithResult<float>(std::move(lwnnModule));
                    resultAsString = std::to_string(result);
                    return true;
                }
                default:
                    ASSERT_FAIL("Unhandled PrimitiveType");
            }
        }
    }

    void evaluateLine(std::string lineOfCode, bool shouldExecute) {
        std::unique_ptr<lwnn::ast::Module> module = lwnn::parse::parseModule(lineOfCode, "<input string>");
        //If no Module returned, parsing failed.
        if(!module) {
            return;
        }
        visualize::prettyPrint(module.get());

        error::ErrorStream es{std::cerr};

        ast_passes::populateSymbolTables(module.get(), es);
        if(es.errorCount() > 0)
            return;

        ast_passes::resolveTypes(module.get(), es);
        if(es.errorCount() > 0)
            return;

        ast_passes::resolveSymbols(module.get(), es);
        if(es.errorCount() > 0)
            return;

        if(shouldExecute) {
            std::string resultAsString;
            bool hasResult = runModule(std::move(module), resultAsString);
            std::cout << (hasResult ? resultAsString : "<no result>") << "\n";
        }
    } // evaluateLine
} //namespace lwnn
