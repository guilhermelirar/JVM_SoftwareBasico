#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include <stddef.h>

int main(int argc, char** argv) 
{
  if (argc < 2) return 1;

  JVM_Context* ctx = jvm_init();

  extract_class_dir(argv[1], ctx->base_dir, 256);

  char entry_class_name[256];
  extract_class_name_from_path(argv[1], entry_class_name, 
      sizeof(entry_class_name));

  jvm_run(ctx, entry_class_name);

  terminateJVM(ctx);
  return 0;
}
