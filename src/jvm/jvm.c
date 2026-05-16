#include "common/classfile.h"
#include "jvm/jvmtypes.h"
#include <stdio.h>
#include <string.h>

method_info* find_method(ClassFile *cf, const char* name, 
    const char* descriptor) 
{
  if (cf == NULL) return NULL;

  method_info* m = NULL;
  for (u2 method_idx = 0; method_idx < cf->methods_count; method_idx++)
  {
    m = &cf->methods[method_idx];
    if (
        strcmp(name, cp_get_utf8(cf, m->name_index)) == 0 && 
        strcmp(descriptor, cp_get_utf8(cf, m->descriptor_index)) == 0
        ) 
    {
      return m; 
    }
  }

  return NULL;
}

JVM_Context* jvm_init(ClassFile* main_class)
{
  method_info* main = find_method(main_class, "main", "([Ljava/lang/String;)V");
  if (main == NULL || 
      !(main->access_flags & ACC_PUBLIC) || 
      !(main->access_flags & ACC_STATIC)) 
  {
    printf("ERROR: method main (public void main(String[] args))" 
        "not found in class \"%s\"\n", 
        cp_class_name(main_class, main_class->this_class));
    return NULL;
  }
  return NULL;
}
