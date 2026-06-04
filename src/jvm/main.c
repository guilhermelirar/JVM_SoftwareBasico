#include "jvm/jvm.h"
#include "jvm/jvmtypes.h"
#include "jvm/utils.h"
#include <stddef.h>

int main(int argc, char** argv) 
{
  if (argc < 2) return 1;


  JVM_Context* ctx = jvm_init();
  extract_class_dir(argv[1], ctx->base_dir, 256);
  load_main_class(ctx, argv[1]);

  jvm_run(ctx);

  terminateJVM(ctx);
  return 0;
}
