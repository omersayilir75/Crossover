#include <iostream>
#include <csignal>
#include "src/visitor/Visitor.h"
#include "include/utils/utils.h"
#include "include/antlr/BabyCobolParser.h"
#include "include/antlr/BabyCobolLexer.h"
#include "antlr4-runtime.h"
#include <llvm/IR/BasicBlock.h>
#include "include/ir/bcmodule.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Function.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/GlobalVariable.h>
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

#include "llvm/ADT/Optional.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include <map>
#include <memory>
#include <system_error>
#include <vector>

using namespace std;
using namespace antlr4;
using namespace llvm;
using namespace llvm::sys;
using namespace utils;

// TODO: pls move me to somewhere sensible




int main(int argc, char **argv) {

    cout << "Starting Compiler..." << endl;

    // build external symbol map
    vector<string> externalFiles = utils::getArgumentParams(argc, argv, "--external");
    map<string,vector<string>> extTable;
    for (auto & element : externalFiles) {
        string execCommand = "nm --demangle ";
        string nmOutput = exec(execCommand.append(element));
        vector<string> textSymbols = utils::extractTextSymbols(nmOutput); // gather all T and t
        string programName = extractProgramNameFromPath(element); // get program name from the input
        extTable.insert({programName, textSymbols});
    }
    // end build external symbol map


    ifstream stream;
    stream.open("../test/call_datadiv_ints.bc");
    ANTLRInputStream input(stream);
    BabyCobolLexer lexer(&input);
    CommonTokenStream tokens(&lexer);
    BabyCobolParser parser(&tokens);

    BabyCobolParser::ProgramContext* tree = parser.program();

    // init  llvm
    llvm::LLVMContext* llvmContext = new llvm::LLVMContext();
    BCModule* module = new BCModule("module", *llvmContext);

    llvm::FunctionType* FT = llvm::FunctionType::get(llvm::Type::getVoidTy(*llvmContext), false);
    llvm::Function* F;
    if(presentInArgs(argc, argv, "-not-main")){
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "procedureDivision", module);
    } else{
        F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, "main", module);
    }

    // Run compiler
    llvm::BasicBlock* block = llvm::BasicBlock::Create(*llvmContext, "root_block", F);

    BCBuilder builder(module, block);

    Visitor visitor(module, &builder, &extTable);
    visitor.visitProgram(tree);


    builder.CreateRetVoid();
    cout << "Finished Compiling!" << endl;

    module->print(llvm::outs(), nullptr);

    // The following code allows us to compile the IR into C object files

    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    auto TargetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.

    if (!Target) {
        errs() << Error;
        return 1;
    }
    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>(Reloc::PIC_);
    auto TheTargetMachine =
            Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    module->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return 1;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }

    pass.run(*module);
    dest.flush();

    llvm::outs() << "Wrote " << Filename << "\n";

    if (presentInArgs(argc, argv,"-generate-structs")) {
        generateStructs(visitor.dataStructures);
    }

    cout << "Compiling BabyCobol Standard Library" << endl;
    // TODO: these relative paths are going to break if not run in the right working directory with access to the library source files...
    exec("mkdir -p out/lib");
    exec("cd out/lib && "
         "clang -c --include-directory ../../../Crossover_bstd_lib/include/ ../../../Crossover_bstd_lib/src/*.c && "
         "ar cr libbstd.a *.o");

    const string executableName = "exec";
    string linkCommand = "clang output.o out/lib/libbstd.a -lm -o " + executableName;

    cout << "Linking objects and creating executable" << endl;
    for (auto & element : externalFiles) {
        linkCommand.append(" ");
        linkCommand.append(element);
    }

    auto result = exec(linkCommand);

    std::cout << "Done." << std::endl << "Wrote executable to file: " << executableName << std::endl;

    return 0;
}

