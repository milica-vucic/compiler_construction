add_llvm_library( LLVMOurCFGPass MODULE
    OurCFG.cpp
    CFGPass.cpp  

    PLUGIN_TOOL
    opt 
)