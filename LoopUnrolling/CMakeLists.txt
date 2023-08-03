add_llvm_library( LLVMLoopUnrollingPass MODULE
  LoopUnrollingPass.cpp

  PLUGIN_TOOL
  opt
  )

set(CMAKE_CXX_STANDARD  17)
