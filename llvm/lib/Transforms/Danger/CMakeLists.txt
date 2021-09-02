add_llvm_library( LLVMDanger MODULE 
  Danger.cpp
  callee.cpp
  
  PLUGIN_TOOL
  opt
  )
