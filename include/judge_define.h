#ifndef ___judge_define
#define ___judge_define
#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<8)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 256 
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
#define LOCAL_DIR "/mq_test"

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

struct solution_info{
	int run_id;
	int problem_id;
	int lang;
	int contest_id;
	char user_name[BUFFER_SIZE];
	char in_date[BUFFER_SIZE];
	void read(char *buff){
		char t[2][32];
		sscanf(buff,"%s%s%s%d%d%d%d",t[0],t[1],user_name,&run_id,&problem_id,&lang,&contest_id);
		sprintf(in_date,"%s %s",t[0],t[1]);
	}
	void write(char *buff){
		sprintf(buff,"%s %s %d %d %d %d\n",in_date,user_name,run_id,problem_id,lang,contest_id);
	}

}; 

#endif
