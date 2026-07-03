#include "types.h"
#include "stat.h"

#include "user.h"

int main() {
  struct disk_stat stat;
  report_stats(&stat);
  printf(1, "finished with report\n");
  exit();
}