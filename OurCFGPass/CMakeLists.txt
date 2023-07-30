add_llvm_library(LLVMOurCFGPass MODULE
   OurCFG.cpp
   OurCFGPass.cpp

   PLUGIN_TOOL
   opt
)