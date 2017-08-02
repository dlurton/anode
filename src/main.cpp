#include "parse.h"
#include "ast_passes.h"
#include "execute.h"
#include "visualize.h"
#include "error.h"
#include "string.h"

#include <unistd.h>

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
    void evaluateLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName, bool shouldExecute);
}

std::string getHistoryFilePath() {
    std::string home{getenv("HOME")};
    return home + "/.lwnn_history";
}

std::string readLineOfCode() {
    char const* prompt = "\x1b[1;32mlwnn\x1b[0m> ";
    char* lineOfCodeChar = linenoise(prompt);
    if (lineOfCodeChar == NULL) {
        return "";
    }
    std::string lineOfCode{lineOfCodeChar};
    free(lineOfCodeChar);
    return lineOfCode;
}

void help() {
    std::cout << "Meta-Command      Description\n";
    std::cout << "/help             Displays this text.\n";
    std::cout << "/compile          Toggles compilation.  When disabled, the LLVM IR will not be generated.\n";
    std::cout << "/history          Displays command history.\n";
    std::cout << "/exit             Exits the lwnn REPL.\n\n";
    std::cout << "Valid lwnn statements may also be entered.\n";
}
using namespace lwnn;

int main(int argc, char **argv) {

    if(!isatty(fileno(stdin))) {
        std::cout << "stdin is not a terminal\n";
        return -1;
    }
        
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

    const std::string historyFilename = getHistoryFilePath();

    linenoiseHistoryLoad(historyFilename.c_str());

    //linenoiseSetCompletionCallback(completionHook);

    std::shared_ptr<lwnn::execute::ExecutionContext> executionContext = lwnn::execute::createExecutionContext();

    executionContext->setResultCallback(
        [](execute::ExecutionContext*, type::PrimitiveType primitiveType, uint64_t valuePtr) {
        const char *resultPrefix = "result: ";
        switch(primitiveType) {
            case type::PrimitiveType::NotAPrimitive:
                std::cout << "<result was not a primitive>";
                break;
            case type::PrimitiveType::Void:
                break;
            case type::PrimitiveType::Bool:
                std::cout << resultPrefix << (*reinterpret_cast<bool*>(valuePtr) ? "true" : "false") << std::endl;
                break;
            case type::PrimitiveType::Int32:
                std::cout << resultPrefix << *reinterpret_cast<int*>(valuePtr) << std::endl;
                break;
            case type::PrimitiveType::Float:
                std::cout << resultPrefix << *reinterpret_cast<float*>(valuePtr) << std::endl;
                break;
            case type::PrimitiveType::Double:
                std::cout << resultPrefix << *reinterpret_cast<double*>(valuePtr) << std::endl;
                break;
        }
    });


    const char* NUDGE = "Type '/help' for help or '/exit' to exit.";
    std::cout << "Welcome to the lwnn REPL. " << NUDGE << "\n";

    bool keepGoing = true;
    bool shouldCompile = true;
    int commandCount = 0;

    while (keepGoing) {
        std::string lineOfCode{readLineOfCode()};
        linenoiseHistoryAdd(lineOfCode.c_str());
        if(lineOfCode[0] == '/') {
            std::string command = lineOfCode.substr(1);
            if (command == "compile") {
                shouldCompile = !shouldCompile;
//             std::cout << "Compilation " << (shouldCompile ? "enabled" : "disabled") << "\n";
            } else if (command == "help") {
                help();
            } else if (command == "exit") {
                keepGoing = false;
            } else if (command == "history") {
                /* Display the current history. */
                for (int index = 0;; ++index) {
                    char *hist = linenoiseHistoryLine(index);
                    if (hist == NULL) break;
                    printf("%4d: %s\n", index, hist);
                    free(hist);
                }
            } else {
                std::cerr << "Unknown meta-command \"" << command << "\". " << NUDGE << "\n";
            }
        }
        else {
            std::string moduleName = lwnn::string::format("repl_line_%d", ++commandCount);
            lwnn::evaluateLine(executionContext, lineOfCode, moduleName, shouldCompile);
        }

        linenoiseHistorySave(historyFilename.c_str());
    }

    linenoiseHistoryFree();
    return 0;
}

namespace lwnn {
    void runModule(std::shared_ptr<execute::ExecutionContext> executionContext, std::unique_ptr<ast::Module> lwnnModule) {
        ASSERT(executionContext);
        ASSERT(lwnnModule);
        executionContext->setPrettyPrintAst(true);
        executionContext->setDumpIROnLoad(true);

        try {
            executionContext->prepareModule(lwnnModule.get());
        } catch (execute::ExecutionException &e) {
            return; //Don't try to compile a module that doesn't even pass semantics checks.
        }
        executionContext->executeModule(std::move(lwnnModule));
    }

    void evaluateLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName, bool shouldExecute) {
        std::unique_ptr<lwnn::ast::Module> module = lwnn::parse::parseModule(lineOfCode, moduleName);

        //If no Module returned, parsing failed.
        if(!module) {
            return;
        }
//        std::cout << "Before ";
//        visualize::prettyPrint(module.get());

        if(shouldExecute) {
            runModule(executionContext, std::move(module));
        }
    } // evaluateLine
} //namespace lwnn
