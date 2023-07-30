add_llvm_library(LLVMDominatorTree MODULE
    DominatorTree.cpp
    DominatorTreePass.cpp

    PLUGIN_TOOL
    opt
)