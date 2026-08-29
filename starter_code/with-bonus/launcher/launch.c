#include "loader.h"

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }

  /* ---- Step 1: Carry out necessary checks on the input ELF file ---- */

  /* Check if the file exists and can be opened */
  int check_fd = open(argv[1], O_RDONLY);
  if (check_fd < 0) {
    printf("Error: Cannot open file '%s'\n", argv[1]);
    exit(1);
  }

  /* Read the first 4 bytes to verify ELF magic number */
  unsigned char magic[4];
  ssize_t n = read(check_fd, magic, 4);
  if (n < 4) {
    printf("Error: File '%s' is too small to be a valid ELF file\n", argv[1]);
    close(check_fd);
    exit(1);
  }

  /* Verify the ELF magic number: 0x7f 'E' 'L' 'F' */
  if (magic[0] != 0x7f || magic[1] != 'E' || magic[2] != 'L' || magic[3] != 'F') {
    printf("Error: '%s' is not a valid ELF file\n", argv[1]);
    close(check_fd);
    exit(1);
  }

  close(check_fd);

  /* ---- Step 2: Pass it to the loader for carrying out loading/execution ---- */

  /*
   * The loader function prototype is: void load_and_run_elf(char** exe)
   * We pass argv[1] (a char*) which the loader receives and casts internally.
   */
  load_and_run_elf((char **)argv[1]);

  /* ---- Step 3: Invoke the cleanup routine inside the loader ---- */

  loader_cleanup();

  return 0;
}
