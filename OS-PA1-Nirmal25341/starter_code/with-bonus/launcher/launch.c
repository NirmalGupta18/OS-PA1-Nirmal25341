#include "loader.h"

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  if (access(argv[1], F_OK) != 0) {
    printf("Error: file %s does not exist\n", argv[1]);
    exit(1);
  }
  if (access(argv[1], R_OK) != 0) {
    printf("Error: file %s is not readable\n", argv[1]);
    exit(1);
  }
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv+1);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
