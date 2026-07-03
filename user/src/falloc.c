#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

int
main(int argc, char *argv[]) {
  int fd;
  if (argc <= 1) {
    printf(1, "Specify a file name\n");
  } else if(argc <= 2) {
    printf(1, "Specify a file length\n");
  } else if (argc < 4) {
    fd = open(argv[1], O_CREATE | O_WRONLY);
    if (fd < 0)
      printf(1, "falloc: cannot open %s\n", argv[1]);
    if (argc == 3) {
      int k = atoi(argv[2]);
      char *data = malloc(k);
      for (int i = 0; i < k; i++) {
        data[i] = 'a' + i % 27;
        if (i % 27 == 26) data[i]='\n';
      }
      int n = 0;
      if ((n = write(fd, data, k)) != k)
        printf(1, "falloc: write error %d, != %d\n", k, n);
      free(data);
    }
  } else {
    printf(1, "Usage: falloc <file> <length>\n");
  }
  exit();
}
