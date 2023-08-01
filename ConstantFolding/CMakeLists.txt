add_llvm_library(LLVMConstantFoldingPass MODULE
    ConstantFoldingPass.cpp

    PLUGIN_TOOL
    opt
)