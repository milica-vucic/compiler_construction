add_llvm_library( LLVMOurFunctionPass MODULE
  OurFunctionPass.cpp
  1.c
  
  PLUGIN_TOOL
  opt
)
