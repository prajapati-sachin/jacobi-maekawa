#include "syscall.h"
#include "traps.h"

.globl sigret_code_start
.globl sigret_code_end

sigret_code_start:
  movl $SYS_sig_ret, %eax
  int $T_SYSCALL
sigret_code_end:
