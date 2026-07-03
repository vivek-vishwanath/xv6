#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"


int
main(int argc, char *argv[]) {
  int invalid = 1;
  if (argc <= 1) {
    printf(1, "Specify a file name");
  } else if (argc == 2) {
    invalid = 0;
    if (chmod(argv[1], 0)) {
      printf(1, "chmod failed");
    }
  } else if(argc == 3) {
    int perms = 0;
    if (argv[1][0] == '+') {
      if (argv[1][1] == 'r') {
        perms |= PROT_R;
        if (argv[1][2] == 'w') perms |= PROT_W;
        invalid--;
      } else if (argv[1][1] == 'w') {
        perms |= PROT_W;
        invalid--;
      }
      if (chmod(argv[2], perms)) {
        printf(1, "chmod failed^");
      }
    } else if (argv[2][0] == '+') {
      if (argv[2][1] == 'r') {
        perms |= PROT_R;
        if (argv[2][2] == 'w') perms |= PROT_W;
        invalid--;
      } else if (argv[2][1] == 'w') {
        perms |= PROT_W;
        invalid--;
      }
      if (chmod(argv[1], perms)) {
        printf(1, "chmod failed%");
      }
    }
  }
  if (invalid) {
    printf(1, "Usage: chmod +[r][w] <file>\n");
  }
  exit();
}

