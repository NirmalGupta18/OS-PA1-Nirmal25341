#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  if (fd>0){ 
    close(fd); 
  }
}

/*
 * Load and run the ELF executable file
 */

void load_and_run_elf(char** exe) {
  fd = open(exe[0], O_RDONLY);
  if (fd < 0) {
    printf("Error: could not open file %s\n", exe[0]);
    exit(1);
  }

  // 1. Load entire binary content into the memory from the ELF file.
  off_t file_size = lseek(fd, 0, SEEK_END);   // move cursor to end of file, this returns the file's size in bytes
  lseek(fd, 0, SEEK_SET);                     // move cursor back to the start, so read() starts from byte 0

  char *file_buffer = malloc(file_size);      // allocate a chunk of heap memory big enough to hold the whole file
  read(fd, file_buffer, file_size);           // actually read the file's bytes into that memory

  printf("Magic bytes: %x %c %c %c\n", file_buffer[0], file_buffer[1], file_buffer[2], file_buffer[3]);
  // ^ just a sanity check for us — should print "7f E L F" if the read worked correctly

  if (file_buffer[0] != 0x7f || file_buffer[1] != 'E' || file_buffer[2] != 'L' || file_buffer[3] != 'F') {
    printf("Error: not a valid ELF file\n");
    exit(1);
  }

    // 2. Iterate through the PHDR table and find the section of PT_LOAD
  //    type that contains the address of the entrypoint method in factorial.c
  ehdr = (Elf32_Ehdr *) file_buffer;

  printf("Entry point address: %x\n", ehdr->e_entry);
  printf("Program header offset: %d, count: %d\n", ehdr->e_phoff, ehdr->e_phnum);

  phdr = (Elf32_Phdr *) (file_buffer + ehdr->e_phoff);

  Elf32_Phdr *target_phdr = NULL;
  int i;
  for (i = 0; i < ehdr->e_phnum; i++) {
    printf("Segment %d: type = %d, vaddr = %x, memsz = %d\n", i, phdr[i].p_type, phdr[i].p_vaddr, phdr[i].p_memsz);
    if (phdr[i].p_type == PT_LOAD) {
      unsigned int seg_start = phdr[i].p_vaddr;
      unsigned int seg_end = phdr[i].p_vaddr + phdr[i].p_memsz;
      if (ehdr->e_entry >= seg_start && ehdr->e_entry < seg_end) {
        printf("Found the correct PT_LOAD segment at index %d\n", i);
        target_phdr = &phdr[i];
        break;
      }
    }
  }

  if (target_phdr == NULL) {
    printf("Error: could not find a PT_LOAD segment containing the entry point\n");
    exit(1);
  } 

    // 3. Allocate memory of the size "p_memsz" using mmap function
  //    and then copy the segment content
  void *virtual_mem = mmap(NULL, target_phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  if (virtual_mem == MAP_FAILED) {
    printf("Error: mmap failed\n");
    exit(1);
  }

  memcpy(virtual_mem, file_buffer + target_phdr->p_offset, target_phdr->p_memsz);
  // ^ copy p_memsz bytes starting from p_offset in the file, into our freshly mmap'd memory

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  unsigned int offset_into_segment = ehdr->e_entry - target_phdr->p_vaddr;
  // ^ how far the entry point is from the start of this segment

  void *entry_addr = virtual_mem + offset_into_segment;
  // ^ apply that same offset inside our new mmap'd memory, to find where _start actually lives

  // 5. Typecast the address to that of function pointer matching "_start" method in factorial.c.
  typedef int (*start_func_t)();
  start_func_t start_func = (start_func_t) entry_addr;

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = start_func();
  printf("User _start return value = %d\n", result);

}
