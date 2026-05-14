#include "common/classfile.h"

const char* cp_get_utf8(const ClassFile* cf, u2 utf8_index) {
  cp_info *utf8_entry = &cf->constant_pool[utf8_index];
  if (utf8_entry->tag != CONSTANT_Utf8) return "<invalid UTF-8>";

  return (const char*)utf8_entry->info.utf8_info.bytes;
}

const char* cp_class_name(const ClassFile *cf, u2 class_index) {
  cp_info *class_entry = &cf->constant_pool[class_index]; 
  if (class_entry->tag != CONSTANT_Class) return "<invalid class>";

  u2 name_index = class_entry->info.class_info.name_index;
  return cp_get_utf8(cf, name_index);
}

const char* cp_nameandtype_name(const ClassFile *cf, u2 nt_index) {
  cp_info *nt = &cf->constant_pool[nt_index];
  if (nt->tag != CONSTANT_NameAndType) return "<invalid nt>";

  u2 name_index = nt->info.name_and_type_info.name_index;
  return cp_get_utf8(cf, name_index);  
}
