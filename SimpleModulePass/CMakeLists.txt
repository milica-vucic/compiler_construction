add_llvm_library( LLVMOurModulePass MODULE
    OurModulePass.cpp

    PLUGIN_TOOL
    opt
)
