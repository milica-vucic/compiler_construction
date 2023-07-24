add_llvm_library( LLVMOurCallGraphPass MODULE
  ourcallgraph.cpp
  OurCallGraphPass.cpp

  PLUGIN_TOOL
  opt
  )
