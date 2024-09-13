#include "backtrace.h"
#include <stdio.h>
#include "asm/x86.h"
#include "stab.h"
#include "defs.h"


void backtrace() {
  cprintf("Backtrace:\n");
  int ebp;
  read_ebp(ebp);
  int *ebp_ptr = (int *) ebp;

  while (ebp_ptr != 0) {
    int return_address = *(ebp_ptr + 1);
    if (!return_address) break;
    struct stab_info *info = (struct stab_info *) kalloc();
    stab_info(return_address, info);
    int offset = return_address - info->eip_fn_addr;
    cprintf("   <0x%x> %.*s+%d\n", return_address, info->eip_fn_namelen, info->eip_fn_name, offset);
    if (!offset) break;
    ebp_ptr = (int *) (*ebp_ptr);
  }
}