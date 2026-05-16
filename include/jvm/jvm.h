#ifndef JVM
#define JVM

#include "common/classfile.h"
#include "jvmtypes.h"

method_info* find_method(ClassFile *cf, const char* name, 
    const char* descriptor);
JVM_Context* jvm_init(ClassFile* main_class);


#endif
