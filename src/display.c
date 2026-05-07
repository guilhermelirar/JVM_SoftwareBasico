// TODO printar significado de identificadores de field
#include "display.h"
#include "classfile.h"
#include "reader.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

static const access_flag_desc class_flags[] = {
  {ACC_PUBLIC,       "public",       "ACC_PUBLIC"},
  {ACC_FINAL,        "final",        "ACC_FINAL"},
  {ACC_SUPER,        "super",        "ACC_SUPER"},
  {ACC_INTERFACE,    "interface",    "ACC_INTERFACE"},
  {ACC_ABSTRACT,     "abstract",     "ACC_ABSTRACT"},
  {ACC_SYNTHETIC,    "synthetic",    "ACC_SYNTHETIC"},
  {ACC_ANNOTATION,   "annotation",   "ACC_ANNOTATION"},
  {ACC_ENUM,         "enum",         "ACC_ENUM"},
};

static const access_flag_desc method_flags[] = {
  {ACC_PUBLIC,        "public",        "ACC_PUBLIC"},
  {ACC_PRIVATE,       "private",       "ACC_PRIVATE"},
  {ACC_PROTECTED,     "protected",     "ACC_PROTECTED"},
  {ACC_STATIC,        "static",        "ACC_STATIC"},
  {ACC_FINAL,         "final",         "ACC_FINAL"},
  {ACC_SYNCHRONIZED,  "synchronized",  "ACC_SYNCHRONIZED"},
  {ACC_ABSTRACT,      "abstract",      "ACC_ABSTRACT"},
  {ACC_SYNTHETIC,     "synthetic",     "ACC_SYNTHETIC"},
};

static const access_flag_desc field_flags[] = {
  {ACC_PUBLIC,      "public",      "ACC_PUBLIC"},
  {ACC_PRIVATE,     "private",     "ACC_PRIVATE"},
  {ACC_PROTECTED,   "protected",   "ACC_PROTECTED"},
  {ACC_STATIC,      "static",      "ACC_STATIC"},
  {ACC_FINAL,       "final",       "ACC_FINAL"},
  {ACC_VOLATILE,    "volatile",    "ACC_VOLATILE"},
  {ACC_TRANSIENT,   "transient",   "ACC_TRANSIENT"},
  {ACC_SYNTHETIC,   "synthetic",   "ACC_SYNTHETIC"},
  {ACC_ENUM,        "enum",        "ACC_ENUM"},
};

void print_utf8(FILE* out, const char* str) {
  if (!str) return;
  for (int i = 0; str[i] != '\0'; i++) {
    switch (str[i]) {
      case '\n': 
        fprintf(out, "\\n"); 
          break;
      case '\r': 
          fprintf(out, "\\r"); 
          break;
      case '\t': 
          fprintf(out, "\\t"); 
          break;
      case '\\': 
          fprintf(out, "\\\\"); 
          break;
      default:
          fputc(str[i], out);
          break;
    }
  }
}

void print_access_flags(u2 bits,
                        access_context_t ctx,
                        access_format_t fmt,
                        FILE* file)
{
  const access_flag_desc* table = NULL;
  size_t count = 0;

  switch (ctx) {
    case ACCESS_CLASS:
      table = class_flags;
      count = sizeof(class_flags) / sizeof(class_flags[0]);
      break;
    case ACCESS_METHOD:
      table = method_flags;
      count = sizeof(method_flags) / sizeof(method_flags[0]);
      break;
    case ACCESS_FIELD:
      table = field_flags;
      count = sizeof(field_flags) / sizeof(field_flags[0]);
      break;
    default:
      return;
  }

  int first = 1;

  const char* sep = (fmt == ACCESS_FMT_JAVA) ? " " : ", ";

  for (size_t i = 0; i < count; i++) {
    if (bits & table[i].mask) {
      if (!first) fputs(sep, file);
      first = 0;
      fputs(
          fmt == ACCESS_FMT_DEBUG ? 
          table[i].debug_kw : table[i].java_kw, 
          file);
    }
  }
}

void print_cp_value(u2 idx, const ClassFile *cf, FILE *file) {
  if (idx == 0 || idx >= cf->constant_pool_count) return;

  cp_info* entry = &cf->constant_pool[idx];
  switch (entry->tag) {
    case CONSTANT_Class:
      fputs(cp_class_name(cf, idx), file);
      break;

    case CONSTANT_Fieldref:
      fprintf(file, "%s.", 
        cp_class_name(cf, entry->info.fieldref_info.class_index));
      print_cp_value(entry->info.fieldref_info.name_and_type_index, cf, file);
      break;

    case CONSTANT_Methodref:
      fprintf(file, "%s.", 
        cp_class_name(cf, entry->info.methodref_info.class_index));
      print_cp_value(entry->info.methodref_info.name_and_type_index, cf, file);
      break;

    case CONSTANT_InterfaceMethodref:
      fprintf(file, "%s.", 
        cp_class_name(cf, entry->info.interface_methodref_info.class_index));  
      print_cp_value(entry->info.interface_methodref_info
          .name_and_type_index, cf, file);
      break;

    case CONSTANT_NameAndType: 
      fprintf(file, "%s:%s", 
        cp_nameandtype_name(cf, idx),
        cp_get_utf8(cf, entry->info.name_and_type_info.descriptor_index));
      break;

    case CONSTANT_Utf8: 
      print_utf8(file, cp_get_utf8(cf, idx));
      break;
      
    case CONSTANT_String: 
      print_utf8(file, cp_get_utf8(cf, entry->info.string_info.string_index));
      break;

    case CONSTANT_Integer:
      fprintf(file, "%d", entry->info.int_info.bytes);
      break;

    case CONSTANT_Float: {
      float f;
      u4 bits = entry->info.float_info.bytes;
      memcpy(&f, &bits, sizeof(float));
      fprintf(file, "%f", f);
      break;
    }
      
    case CONSTANT_Long: {
      u8 long_hl = ((u8)entry->info.long_info.h_bytes << 32) | 
                   (u8)entry->info.long_info.l_bytes; 
      fprintf(file, "%" PRId64, (int64_t)long_hl);
      break;
    }

    case CONSTANT_Double: {
      double d;
      u8 bits = ((u8)entry->info.double_info.h_bytes << 32) | 
                ((u8)entry->info.double_info.l_bytes);
      memcpy(&d, &bits, sizeof(double));
      fprintf(file, "%f", d);
      break;
    }

    default:
      break;
  }
}

void print_class_constant_pool(const ClassFile *cf, FILE *file) {
  u2 count = cf->constant_pool_count;
  cp_info* entry = NULL;
  char buffer[32]; // Buffer para montar a coluna de índices (ex: #7.#25)

  fprintf(file, "Constant Pool (%d):\n", count);
  for (int i = 1; i < count; i++) {
    fprintf(file, " #%-3d = ", i);
    entry = &cf->constant_pool[i];

    switch (entry->tag) {
      case CONSTANT_Class:
        snprintf(buffer, sizeof(buffer), "#%d", entry->info.class_info.name_index);
        fprintf(file, "%-18s %-15s // ", "Class", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Fieldref:
        snprintf(buffer, sizeof(buffer), "#%d.#%d", 
          entry->info.fieldref_info.class_index, 
          entry->info.fieldref_info.name_and_type_index);
        fprintf(file, "%-18s %-15s // ", "Fieldref", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Methodref:
        snprintf(buffer, sizeof(buffer), "#%d.#%d", 
          entry->info.methodref_info.class_index, 
          entry->info.methodref_info.name_and_type_index);
        fprintf(file, "%-18s %-15s // ", "Methodref", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_InterfaceMethodref:
        snprintf(buffer, sizeof(buffer), "#%d.#%d", 
          entry->info.interface_methodref_info.class_index, 
          entry->info.interface_methodref_info.name_and_type_index);
        fprintf(file, "%-18s %-15s // ", "InterfaceMethodref", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_NameAndType:
        snprintf(buffer, sizeof(buffer), "#%d:#%d", 
          entry->info.name_and_type_info.name_index, 
          entry->info.name_and_type_info.descriptor_index);
        fprintf(file, "%-18s %-15s // ", "NameAndType", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Utf8:
        fprintf(file, "%-18s ", "UTF-8");
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_String:
        snprintf(buffer, sizeof(buffer), "#%d", entry->info.string_info.string_index);
        fprintf(file, "%-18s %-15s // ", "String", buffer);
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Integer:
        fprintf(file, "%-18s ", "Integer");
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Float:
        fprintf(file, "%-18s ", "Float");
        print_cp_value(i, cf, file);
        break;

      case CONSTANT_Long:
        fprintf(file, "%-18s ", "Long");
        print_cp_value(i, cf, file);
        i++; 
        break;

      case CONSTANT_Double:
        fprintf(file, "%-18s ", "Double");
        print_cp_value(i, cf, file);
        i++; 
        break;

      default:
        break;
    }
    putc('\n', file);
  }
}


void print_interfaces(const ClassFile* cf, FILE* file) {
  fprintf(file, "Interfaces: \n");
  for (int i = 0; i < cf->interfaces_count; i++) {
    u2 class_idx = cf->interfaces[i];
    fprintf(file, "  #%2d: (#%d) %s\n", i, class_idx,
        cp_class_name(cf, class_idx));
  }
}
 
void print_attributes(const ClassFile *cf, u2 count, 
    attribute_info *attributes, FILE *file, int indent) {
  
  for (int i = 0; i < count; i++) {
    const char *name = cp_get_utf8(cf, attributes[i].attribute_name_index);

    // identação
    print_indent(indent, file);
    fprintf(file, "[%d] %s: \n", i, name);
   
    // identificando atributo
    if (strcmp(name, "ConstantValue") == 0) {
      print_indent(indent+4, file);
      fprintf(file, "Constant Value index: #%d\n", 
          attributes[i].info.constantvalue_index);
    } 
    
    if (strcmp(name, "Code") == 0) {
      print_indent(indent+4, file);
      fprintf(file, "Bytecode length: %u\n", 
          attributes[i].info.code_attribute->code_length);
      print_code(cf, attributes[i].info.code_attribute, file, indent+6);    
    }

    if (strcmp(name, "Exceptions") == 0) {
      print_indent(indent+4, file);
      fputs("throws: ", file);
      Exceptions_attribute *e = attributes[i].info.exceptions_attribute;
      for (u2 i = 0; i < e->number_of_exceptions; i++) {
        print_cp_value(e->exception_index_table[i], cf, file);
        if (i < e->number_of_exceptions-1) fputs(", ", file);
      }
      fputs("\n", file);
    }

    if (strcmp(name, "SourceFile") == 0) {
      fprintf(file, "SourceFile: \"%s\"", 
          cp_get_utf8(cf, attributes[i].info.sourcefile_index));
    }
  }
}

void print_class_member(const ClassFile* cf,
  u2 index, u2 name_index, 
  u2 descriptor_index, u2 attributes_count,
  attribute_info* attributes,
  access_context_t access_ctx, u2 access_flags,
  FILE* file) {
  
  fprintf(file, "  [%d] Name: %s\n", index, cp_get_utf8(cf, name_index));
  fprintf(file, "      Descriptor: %s\n", cp_get_utf8(cf, descriptor_index));
  

  fputs("      Access Flags: ", file); 
  print_access_flags(access_flags, access_ctx, ACCESS_FMT_DEBUG, file);
  putc('\n', file);
  
  // Exibe a assinatura em caso de ser método
  if (access_ctx == ACCESS_METHOD) {
    fputs("      Definition: ", file);
    print_method_definition(cf, &cf->methods[index], file);
    putc('\n', file);
  }

  if (attributes_count > 0) {
    fprintf(file, "      Attributes:\n");
    print_attributes(cf, attributes_count, attributes, file, 8);
  }
  
  fprintf(file, "\n");
}

void print_fields(const ClassFile *cf, FILE *file) {
  fprintf(file, "Fields (%d):\n", cf->fields_count);
  for (int i = 0; i < cf->fields_count; i++) {
    field_info *f = &cf->fields[i];
    print_class_member(cf, i, f->name_index, 
        f->descriptor_index, 
        f->attributes_count, 
        f->attributes, 
        ACCESS_FIELD, 
        f->access_flags, file);  
  }
}

void print_methods(const ClassFile *cf, FILE *file) {
  fprintf(file, "Methods (%d):\n", cf->methods_count);
  for (int i = 0; i < cf->methods_count; i++) {
    method_info *m = &cf->methods[i];
    print_class_member(cf, i, m->name_index, 
        m->descriptor_index, 
        m->attributes_count, 
        m->attributes, 
        ACCESS_METHOD, 
        m->access_flags, file);  
  }
} 


void printclass(const ClassFile *cf, FILE *file) {
  fprintf(file, "Magic: 0x%X\n", cf->magic);
  fprintf(file, "Version: %d.%d\n", 
      cf->major_version, cf->minor_version);
  
  fputc('\n', file);
  print_class_constant_pool(cf, file);
 
  fputc('\n', file);
  fputs("Access Flags: ", file); 
  print_access_flags(cf->access_flags, ACCESS_CLASS, 
      ACCESS_FMT_DEBUG, file);

  fputc('\n', file);
  // this_class e super class
  fprintf(file, "This class: %s\n", cp_class_name(cf, cf->this_class));
  fprintf(file, "Super class: %s\n", cp_class_name(cf, cf->super_class));

  // interfaces
  fprintf(file, "Interfaces count: %d\n", cf->interfaces_count);
  if (cf->interfaces_count) print_interfaces(cf, file);

  fputc('\n', file);
  print_fields(cf, file);

  putc('\n', file);
  print_methods(cf, file);

  fputc('\n', file);
  print_attributes(cf, cf->attributes_count, cf->attributes, file, 0);
}


void print_field_descriptor(FILE *out, const char *str, int* pos, int dimensions) {
  char base_char = str[*pos];

  switch (base_char) {
    case '\0':
      return;

    case 'B':
      fputs("byte", out);
      break;
    
    case 'C':
      fputs("char", out);
      break;

    case 'D':
      fputs("double", out);
      break;

    case 'F':
      fputs("float", out);
      break;

    case 'I':
      fputs("int", out);
      break;

    case 'J':
      fputs("long", out);
      break;
   
    case 'S':
      fputs("short", out);
      break;
   
    case 'V':
      fputs("void", out);
      break;

    case 'Z':
      fputs("boolean", out);
      break;

    case '[':
      (*pos)++;
      print_field_descriptor(out, 
          str, pos, dimensions + 1);
      return;
      break;

    case 'L': {
      (*pos)++;
      for(; str[*pos] != '\0' && str[*pos] != ';'; (*pos)++) {
        if (str[*pos] == '/') fputc('.', out);
        else fputc(str[*pos], out);
      }
      break;
    }

    default:
      fprintf(out, "<unk '%c'>", str[*pos]);
      break;
  }

  while (dimensions--) fputs("[]", out);
  (*pos)++;
}

void print_method_definition(const ClassFile* cf, method_info* m, FILE* out) {
  print_access_flags(m->access_flags, 
        ACCESS_METHOD, ACCESS_FMT_JAVA, out);
  fputc(' ', out);
  
  const char* nt_str = cp_get_utf8(cf, m->descriptor_index);
  
  int i = 0; 
  while (nt_str[i] && nt_str[i] != ')') i++;
  if (nt_str[i] == ')') i++;

  print_field_descriptor(out, nt_str, &i, 0);
  fputc(' ', out);
  // TODO implementar
  fputs(cp_get_utf8(cf, m->name_index), out);
  
  i = 0; 
  while (nt_str[i] && nt_str[i] != '(') i++;
  if (nt_str[i] == '(') {
    fputc(nt_str[i], out);
    i++;
  }

  while (nt_str[i] != '\0' && nt_str[i] != ')') {
    print_field_descriptor(out, nt_str, &i, 0);
    if (nt_str[i] != ')') fputs(", ", out);
  }

  fputs(");", out); 
}
