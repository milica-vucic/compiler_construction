add_llvm_library(LLVMConstantPropagationPass MODULE
    ConstantPropagationInstruction.cpp
    ConstantPropagationPass.cpp

    PLUGIN_TOOL
    opt
)