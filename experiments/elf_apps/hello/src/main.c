#include <stdint.h>

uint32_t mia_host_abi_version(void);
void mia_host_log(const char *message);

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (mia_host_abi_version() != 1) {
    mia_host_log("hello.app: unsupported MiaOS host ABI");
    return 1;
  }

  mia_host_log("hello.app: running from SD ELF");
  return 0;
}
