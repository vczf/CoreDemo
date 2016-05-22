/*
fork to hustoj
*/
#include <sys/syscall.h>
#define HOJ_MAX_LIMIT -1
#ifdef __i386

int LANG_C[256]={SYS_time,SYS_read, SYS_uname, SYS_write, SYS_open, SYS_close, SYS_execve, SYS_access, SYS_brk, SYS_munmap, SYS_mprotect, SYS_mmap2, SYS_fstat64, SYS_set_thread_area, 252,0};
int LANG_CC[256]={1,HOJ_MAX_LIMIT,       HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,       HOJ_MAX_LIMIT,        1,          HOJ_MAX_LIMIT,         HOJ_MAX_LIMIT,      HOJ_MAX_LIMIT,         HOJ_MAX_LIMIT,           HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,          HOJ_MAX_LIMIT,                  2,0};

#else

int LANG_C[256]={SYS_time,SYS_read, SYS_uname, SYS_write, SYS_open, SYS_close, SYS_execve, SYS_access, SYS_brk, SYS_munmap, SYS_mprotect, SYS_mmap, SYS_fstat, SYS_set_thread_area, 252,SYS_arch_prctl,231,0};
int LANG_CC[256]={1,HOJ_MAX_LIMIT,       HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,       HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,          HOJ_MAX_LIMIT,         HOJ_MAX_LIMIT,      HOJ_MAX_LIMIT,         HOJ_MAX_LIMIT,           HOJ_MAX_LIMIT,        HOJ_MAX_LIMIT,          HOJ_MAX_LIMIT,                  2,HOJ_MAX_LIMIT,HOJ_MAX_LIMIT,0};


#endif
