add_llvm_library(LLVMDeadCodeEliminationPass MODULE
    CFG.cpp
    DeadCodeElimination.cpp

    PLUGIN_TOOL
    opt
)