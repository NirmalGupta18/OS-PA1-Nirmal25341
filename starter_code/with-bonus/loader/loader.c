#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/* Heap buffer holding a raw copy of the entire ELF file (see step 1) */
static void *elf_buffer = NULL;

/* mmap()-ed region that actually holds the loadable segment and is
 * executed. Kept as globals so loader_cleanup() can release them. */
static void *virtual_mem = NULL;
static size_t virtual_mem_size = 0;

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  if (elf_buffer) {
    free(elf_buffer);
    elf_buffer = NULL;
  }

  if (virtual_mem) {
    if (munmap(virtual_mem, virtual_mem_size) != 0) {
      perror("munmap");
    }
    virtual_mem = NULL;
    virtual_mem_size = 0;
  }

  if (fd > 0) {
    close(fd);
    fd = -1;
  }
}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(1);
  }

  /* --------------------------------------------------------------
   * 1. Load entire binary content into memory from the ELF file.
   *    Find the file size first (needed to size the malloc'd buffer),
   *    then read() the whole file into that heap buffer.
   * ------------------------------------------------------------ */
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size < 0) {
    perror("lseek");
    exit(1);
  }
  if (lseek(fd, 0, SEEK_SET) < 0) {
    perror("lseek");
    exit(1);
  }

  elf_buffer = malloc(file_size);
  if (!elf_buffer) {
    fprintf(stderr, "Error: malloc failed while allocating %ld bytes\n", (long)file_size);
    exit(1);
  }

  ssize_t bytes_read = read(fd, elf_buffer, file_size);
  if (bytes_read < 0 || bytes_read != file_size) {
    fprintf(stderr, "Error: could not read the complete ELF file\n");
    exit(1);
  }

  ehdr = (Elf32_Ehdr *) elf_buffer;

  /* Sanity check: make sure this is really an ELF file and that it is
   * the 32-bit little-endian, executable kind we know how to load. */
  if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG) != 0) {
    fprintf(stderr, "Error: not a valid ELF file\n");
    exit(1);
  }
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    fprintf(stderr, "Error: not a 32-bit ELF file\n");
    exit(1);
  }
  if (ehdr->e_type != ET_EXEC) {
    fprintf(stderr, "Error: SimpleLoader only supports statically linked executables (ET_EXEC)\n");
    exit(1);
  }

  /* --------------------------------------------------------------
   * 2. Iterate through the PHDR table and find the PT_LOAD segment
   *    that contains the entry point address (e_entry).
   * ------------------------------------------------------------ */
  phdr = (Elf32_Phdr *) ((char *) elf_buffer + ehdr->e_phoff);

  Elf32_Phdr *entry_phdr = NULL;
  for (int i = 0; i < ehdr->e_phnum; i++) {
    Elf32_Phdr *cur = &phdr[i];
    if (cur->p_type == PT_LOAD &&
        ehdr->e_entry >= cur->p_vaddr &&
        ehdr->e_entry < cur->p_vaddr + cur->p_memsz) {
      entry_phdr = cur;
      break;
    }
  }

  if (!entry_phdr) {
    fprintf(stderr, "Error: could not find a PT_LOAD segment containing the entry point\n");
    exit(1);
  }

  /* --------------------------------------------------------------
   * 3. Allocate memory of size p_memsz using mmap and copy the
   *    segment content into it.
   * ------------------------------------------------------------ */
  virtual_mem = mmap(NULL, entry_phdr->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
  if (virtual_mem == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  virtual_mem_size = entry_phdr->p_memsz;

  /* Copy only p_filesz bytes from the file - the remaining
   * (p_memsz - p_filesz) bytes (e.g. .bss) are already zero because
   * mmap(MAP_ANONYMOUS) hands back zero-filled pages. */
  memcpy(virtual_mem, (char *) elf_buffer + entry_phdr->p_offset, entry_phdr->p_filesz);

  /* --------------------------------------------------------------
   * 4. Navigate to the entry point address inside the segment just
   *    loaded. e_entry is a *virtual* address, so its offset from
   *    the segment's own virtual address (p_vaddr) tells us how far
   *    into virtual_mem the real entry point lives.
   * ------------------------------------------------------------ */
  unsigned int offset_in_segment = ehdr->e_entry - entry_phdr->p_vaddr;
  void *entry_addr = (char *) virtual_mem + offset_in_segment;

  /* --------------------------------------------------------------
   * 5. Typecast the address to a function pointer matching the
   *    "_start" method's signature in factorial.c (returns int,
   *    takes no arguments).
   * ------------------------------------------------------------ */
  int (*_start)(void) = (int (*)(void)) entry_addr;

  /* --------------------------------------------------------------
   * 6. Call _start and print the value it returns.
   * ------------------------------------------------------------ */
  int result = _start();
  printf("User _start return value = %d\n", result);
}

int main(int argc, char** argv)
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  if (access(argv[1], F_OK) != 0) {
    fprintf(stderr, "Error: file '%s' does not exist\n", argv[1]);
    exit(1);
  }
  if (access(argv[1], R_OK) != 0) {
    fprintf(stderr, "Error: file '%s' is not readable\n", argv[1]);
    exit(1);
  }
  // 2. passing it to the loader for carrying out the loading/execution
  //    (the full argv array is passed, matching the "char** exe"
  //    signature fixed in loader.h; exe[1] is the ELF path)
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();
  return 0;
}
