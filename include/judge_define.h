#ifndef ___judge_define

#define ___judge_define
#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<8)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 1024 
#define OJ_WT0 0
#define OJ_WT1 1
#define OJ_CI 2
#define OJ_RI 3
#define OJ_AC 4
#define OJ_PE 5
#define OJ_WA 6
#define OJ_TL 7
#define OJ_ML 8
#define OJ_OL 9
#define OJ_RE 10
#define OJ_CE 11
#define OJ_RE_SO 12
//StackOverFLOW
#define OJ_DBZ 13
//DIVIDE_BY_ZERO
#define OJ_AV 14
//ACCESS_VIOLATION

#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi
#endif

#endif
