#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;
/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  
}
/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(exe[0], O_RDONLY);
  if (fd < 0) {
    perror("Error opening ELF file");
    exit(1);
}
  off_t file_size = lseek(fd, 0, SEEK_END);

  if (file_size == -1) {
    perror("Error finding file size");
    exit(1);
}

  lseek(fd, 0, SEEK_SET);
  off_t file_size = lseek(fd, 0, SEEK_END);

  if (file_size == -1) {
    perror("Error finding file size");
    exit(1);
}
  ssize_t bytes_read = read(fd, elf_file, file_size);

  if (bytes_read != file_size) {
    perror("Error reading ELF file");
    free(elf_file);
    exit(1);
}
lseek(fd, 0, SEEK_SET);
  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in factorial.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in factorial.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);
}
int main(int argc, char** argv) {
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(&argv[1]);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
