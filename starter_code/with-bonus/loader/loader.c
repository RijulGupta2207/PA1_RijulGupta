#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * Global pointers to track allocated resources for cleanup
 */
void *virtual_mem = NULL;    // mmap'd memory for the loaded segment
size_t virtual_mem_size = 0; // size of the mmap'd memory
char *elf_buffer = NULL;     // malloc'd buffer holding the ELF file content

/*
 * release memory and other cleanups
 */
void loader_cleanup() {
  /* Unmap the memory allocated for the loaded segment */
  if (virtual_mem != NULL && virtual_mem != MAP_FAILED) {
    munmap(virtual_mem, virtual_mem_size);
    virtual_mem = NULL;
  }

  /* Free the buffer used to hold the ELF file content */
  if (elf_buffer != NULL) {
    free(elf_buffer);
    elf_buffer = NULL;
  }

  /* Close the file descriptor */
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
}

/*
 * Load and run the ELF executable file
 *
 * Steps:
 * 1. Read entire ELF binary into memory
 * 2. Parse ELF header and program headers
 * 3. Find the PT_LOAD segment containing the entry point
 * 4. mmap memory at the segment's virtual address
 * 5. Copy segment content into the mmap'd region
 * 6. Jump to the entry point and execute
 */
void load_and_run_elf(char** exe) {
  /* Get the filename from the parameter */
  const char *elf_file = (const char *)exe;

  /* ---- Step 1: Open and read the entire ELF file into memory ---- */

  fd = open(elf_file, O_RDONLY);
  if (fd < 0) {
    printf("Error: Cannot open file '%s'\n", elf_file);
    exit(1);
  }

  /* Find the file size using lseek */
  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size < 0) {
    printf("Error: Cannot determine file size\n");
    close(fd);
    exit(1);
  }
  lseek(fd, 0, SEEK_SET); // Seek back to the beginning

  /* Allocate memory and read the file content */
  elf_buffer = (char *)malloc(file_size);
  if (elf_buffer == NULL) {
    printf("Error: Memory allocation failed\n");
    close(fd);
    exit(1);
  }

  ssize_t bytes_read = read(fd, elf_buffer, file_size);
  if (bytes_read != file_size) {
    printf("Error: Could not read entire file\n");
    free(elf_buffer);
    close(fd);
    exit(1);
  }

  /* ---- Step 2: Parse the ELF header ---- */

  ehdr = (Elf32_Ehdr *)elf_buffer;

  /* Validate ELF magic number */
  if (ehdr->e_ident[0] != 0x7f || ehdr->e_ident[1] != 'E' ||
      ehdr->e_ident[2] != 'L'  || ehdr->e_ident[3] != 'F') {
    printf("Error: Not a valid ELF file\n");
    loader_cleanup();
    exit(1);
  }

  /* Verify it is a 32-bit ELF */
  if (ehdr->e_ident[EI_CLASS] != ELFCLASS32) {
    printf("Error: Not a 32-bit ELF file\n");
    loader_cleanup();
    exit(1);
  }

  /* Verify it is an executable */
  if (ehdr->e_type != ET_EXEC) {
    printf("Error: Not an executable ELF file\n");
    loader_cleanup();
    exit(1);
  }

  /* ---- Step 3: Find the PT_LOAD segment containing the entry point ---- */

  Elf32_Addr entry_addr = ehdr->e_entry;
  Elf32_Phdr *target_phdr = NULL;

  for (int i = 0; i < ehdr->e_phnum; i++) {
    phdr = (Elf32_Phdr *)(elf_buffer + ehdr->e_phoff + i * ehdr->e_phentsize);

    if (phdr->p_type == PT_LOAD) {
      /* Check if the entry point falls within this segment's address range */
      if (entry_addr >= phdr->p_vaddr &&
          entry_addr < phdr->p_vaddr + phdr->p_memsz) {
        target_phdr = phdr;
        break;
      }
    }
  }

  if (target_phdr == NULL) {
    printf("Error: Could not find PT_LOAD segment containing entry point\n");
    loader_cleanup();
    exit(1);
  }

  /* ---- Step 4: Allocate memory using mmap and copy segment content ---- */

  /*
   * mmap allocates memory at the exact virtual address specified by p_vaddr.
   * MAP_FIXED ensures the mapping is placed at exactly p_vaddr.
   * MAP_PRIVATE | MAP_ANONYMOUS creates a private, zero-filled mapping.
   * PROT_READ | PROT_WRITE | PROT_EXEC grants full access permissions.
   */
  virtual_mem = mmap(
    (void *)target_phdr->p_vaddr,           // requested virtual address
    target_phdr->p_memsz,                   // size of memory to allocate
    PROT_READ | PROT_WRITE | PROT_EXEC,     // permissions: read, write, execute
    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, // flags
    -1,                                      // no file descriptor (anonymous)
    0                                        // offset (not used for anonymous)
  );

  if (virtual_mem == MAP_FAILED) {
    printf("Error: mmap failed\n");
    loader_cleanup();
    exit(1);
  }

  virtual_mem_size = target_phdr->p_memsz;

  /* Copy the segment content from the ELF file buffer into the mmap'd memory */
  memcpy(virtual_mem, elf_buffer + target_phdr->p_offset, target_phdr->p_filesz);

  /* ---- Step 5: Navigate to the entry point ---- */

  /*
   * The entry point virtual address is e_entry.
   * The segment was loaded at p_vaddr.
   * The offset of the entry point within the loaded segment is:
   *     e_entry - p_vaddr
   * So the actual memory address of the entry point is:
   *     virtual_mem + (e_entry - p_vaddr)
   */
  void *entry_point = (void *)((char *)virtual_mem + (entry_addr - target_phdr->p_vaddr));

  /* ---- Step 6: Typecast to function pointer and call _start ---- */

  /*
   * Cast the entry point address to a function pointer matching the
   * signature of _start() in factorial.c: int _start(void)
   */
  int (*_start)(void) = (int (*)(void))entry_point;

  /* Call _start and capture the return value */
  int result = _start();
  printf("User _start return value = %d\n", result);
}
