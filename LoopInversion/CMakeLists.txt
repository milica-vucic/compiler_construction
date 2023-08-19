add_llvm_library( LLVMLoopInversionPass MODULE
  LoopInversion.cpp

  PLUGIN_TOOL
  opt
  )