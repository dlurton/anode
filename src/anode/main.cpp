#include "anode.h"
#include "front/parse.h"
#include "front/ast_passes.h"
#include "front/visualize.h"
#include "front/ErrorStream.h"
#include "common/string.h"
#include "common/stacktrace.h"
#include "execute/execute.h"
#include "runtime/builtins.h"
#include "cxxopts.h"


#include "gc/gc.h"

#include <unistd.h>

#include <linenoise.h>
#include <cstring>
#include <fstream>

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

using namespace anode::front;

namespace CmdLine {


enum class Action : unsigned char {
    JustExit,
    DumpAst,
    Execute,
    RunInteractive
};

Action DesiredAction = Action::RunInteractive;
std::string StartScriptFilename;

void parseCmdLine(int argc, char **argv) {
    cxxopts::Options options("anode", "Anode REPL and JIT compiler/runtime.");
    options.add_options("")
        ("h,help", "Display this text and exit", cxxopts::value<bool>(), "")
        ("e,execute", "Execute the specified file", cxxopts::value<std::string>(), "")
        ;
    options.add_options("diagnostics")
    //TODO:  the last argument to OptionsAdder doesn't seem to do anything and doesn't seem to be documented?
        ("a,dumpast", "Display the AST of the specified file", cxxopts::value<std::string>(), "")
        ;

    options.parse(argc, argv);

    if(options["help"].as<bool>()) {
        DesiredAction = Action::JustExit;
        std::cout << options.help(options.groups()) << "\n";
        return;
    }

    std::string temp = options["execute"].as<std::string>();
    if(!temp.empty()) {
        DesiredAction = Action::Execute;
        StartScriptFilename = temp;
    }

    temp = options["dumpast"].as<std::string>();
    if(!temp.empty()) {
         DesiredAction = Action::DumpAst;
        StartScriptFilename = temp;
    }
}
}

namespace anode {

void executeLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName,
                 bool shouldExecute);

bool executeScript(const std::string &startScriptFilename);

bool dumpAst(const std::string &startScriptFilename);

bool runInteractive();

std::string getHistoryFilePath() {
    std::string home{getenv("HOME")};
    return home + "/.anode_history";
}

std::string readLineOfCode() {
    char const *prompt = "\x1b[1;32manode\x1b[0m> ";
    char *lineOfCodeChar = linenoise(prompt);
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
    std::cout << "/exit             Exits the anode REPL.\n\n";
    std::cout << "Valid anode statements may also be entered.\n";
}

void resultCallback(execute::ExecutionContext *, type::PrimitiveType primitiveType, void *valuePtr) {
    const char *resultPrefix = "result: ";
    switch (primitiveType) {
        case type::PrimitiveType::NotAPrimitive:
            std::cout << "<result was not a primitive>";
            break;
        case type::PrimitiveType::Void:
            break;
        case type::PrimitiveType::Bool:
            std::cout << resultPrefix << (*reinterpret_cast<bool *>(valuePtr) ? "true" : "false") << std::endl;
            break;
        case type::PrimitiveType::Int32:
            std::cout << resultPrefix << *reinterpret_cast<int *>(valuePtr) << std::endl;
            break;
        case type::PrimitiveType::Float:
            std::cout << resultPrefix << *reinterpret_cast<float *>(valuePtr) << std::endl;
            break;
        case type::PrimitiveType::Double:
            std::cout << resultPrefix << *reinterpret_cast<double *>(valuePtr) << std::endl;
            break;
    }
}

bool runInteractive() {
    if (!isatty(fileno(stdin))) {
        std::cout << "stdin is not a terminal\n";
        return true;
    }

    const std::string historyFilename = getHistoryFilePath();

    linenoiseHistoryLoad(historyFilename.c_str());

    //linenoiseSetCompletionCallback(completionHook);

    std::shared_ptr<execute::ExecutionContext> executionContext = execute::createExecutionContext();
    executionContext->setPrettyPrintAst(true);
    executionContext->setDumpIROnLoad(true);

    executionContext->setResultCallback(resultCallback);


    const char *NUDGE = "Type '/help' for help or '/exit' to exit.";
    std::cout << "Welcome to the anode REPL. " << NUDGE << "\n";

    bool keepGoing = true;
    bool shouldCompile = true;
    int commandCount = 0;

    while (keepGoing) {
        std::string lineOfCode{readLineOfCode()};
        linenoiseHistoryAdd(lineOfCode.c_str());
        if (lineOfCode[0] == '/') {
            std::string command = lineOfCode.substr(1);
            if (command == "compile") {
                shouldCompile = !shouldCompile;
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
        } else {
            std::string moduleName = string::format("repl_line_%d", ++commandCount);
            executeLine(executionContext, lineOfCode, moduleName, shouldCompile);
        }

        linenoiseHistorySave(historyFilename.c_str());
    }
    linenoiseHistoryFree();
    return false;
}

bool runModule(std::shared_ptr<execute::ExecutionContext> executionContext, ast::Module *anodeModule) {
    ASSERT(executionContext);
    ASSERT(anodeModule);

    if(executionContext->prepareModule(anodeModule)) {
        return true; //Don't try to compile a module that doesn't even pass semantics checks.
    }

    executionContext->executeModule(anodeModule);
    return false;
}

void executeLine(std::shared_ptr<execute::ExecutionContext> executionContext, std::string lineOfCode, std::string moduleName,
                 bool shouldExecute) {

    ast::Module *module;
    try {
        module = anode::front::parseModule(lineOfCode, moduleName);
    } catch (anode::front::ParseAbortedException &e) {
        std::cerr << "Parse aborted!\n";
        std::cerr << e.what();
        return;
    }

    //If no Module returned, parsing failed.
    if (!module) {
        return;
    }

    if (shouldExecute) {
        runModule(executionContext, module);
    }
}

ast::Module *parseModule(const std::string &startScriptFilename) {
    try {
        return anode::front::parseModule(startScriptFilename);
    }
    catch (anode::front::ParseAbortedException &e) {
        std::cerr << "Parse aborted!\n";
        std::cerr << e.what();
    }
    catch (std::runtime_error &err) {
        std::cerr << err.what();
    }
    return nullptr;
}

bool executeScript(const std::string &startScriptFilename) {
    ast::Module *module = parseModule(startScriptFilename);
    if (!module) {
        return true;
    }

    std::shared_ptr<execute::ExecutionContext> executionContext = execute::createExecutionContext();
    executionContext->setResultCallback(resultCallback);

    bool failFlag = runModule(executionContext, module);
    if (!failFlag && anode::runtime::AssertPassCount > 0) {
        std::cerr << anode::runtime::AssertPassCount << " assertion(s) passed.\n";
    }

    return failFlag;
}

bool dumpAst(const std::string &startScriptFilename) {
    ast::Module *module = parseModule(startScriptFilename);
    if (!module) {
        return true;
    }

    std::shared_ptr<execute::ExecutionContext> executionContext = execute::createExecutionContext();

    if(executionContext->prepareModule(module)) {
        std::cerr << "Semantic module preparation failed.\n";
        return true;
    }

    anode::front::visualize::prettyPrint(module);
    return false;
}
} // namespace anode

volatile int destructionCount = 0;
class SomeGarbage : public gc_cleanup {
public:
    ~SomeGarbage() {
        destructionCount++;
    }
    int randomWahteverIdoncare = 0;

    void foo() { randomWahteverIdoncare++; }
};

void generateSomeGarbage() {

    for(int i = 0; i < 100000; ++i) {
        SomeGarbage *garbage = new SomeGarbage();
        garbage->foo();
    }
}

void sigsegv_handler(int) {
    std::cerr << "SIGSEGV!\n";
    print_stacktrace();
    exit(1);
}



void initializeGC() {

    //GC_set_all_interior_pointers(1);
    //GC_enable_incremental();

    GC_INIT();

    generateSomeGarbage();
    GC_gcollect();

    //I'm just gonna leave this here for a while until we're confident that libgc is in fact working reliably.
    std::cout << "destructionCount: " << destructionCount << std::endl;
    if (destructionCount == 0) {
        std::cout << "**************************************************************************\n";
        std::cout << "WARNING: libgc doesn't appear to be collecting anything.\n";
        std::cout << "**************************************************************************\n";
    }
}

int main(int argc, char **argv) {

#ifdef ANODE_DEBUG
    std::cout << "anode: this is a debug build.\n";
#endif

    initializeGC();

    try {
        CmdLine::parseCmdLine(argc, argv);
    } catch(cxxopts::OptionException &exception) {
        std::cerr << exception.what() << "\n";
        return -1;
    }

    switch(CmdLine::DesiredAction) {
        case CmdLine::Action::JustExit:
            return 0;
        case CmdLine::Action::DumpAst:
            if (anode::dumpAst(CmdLine::StartScriptFilename)) {
                return -1;
            }
            break;
        case CmdLine::Action::Execute:
            if (anode::executeScript(CmdLine::StartScriptFilename)) {
                return -1;
            }
            break;
        case CmdLine::Action::RunInteractive:
            if (anode::runInteractive()) {
                return -1;
            }
            break;
    }

    return 0;
}

