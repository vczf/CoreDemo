#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include "okcalls.h"
#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 512

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
#define OJ_CO 12
#define OJ_TR 13

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
/*
copy from hustoj
https://github.com/zhblue/hustoj/blob/master/beta/core/judge_client/judge_client.cc
*/
int COMPILE_TIME=60;
int COMPILE_FSIZE=10;
int COMPILE_MSIZE=128*STD_MB;

//0GCC 1G++
char const  **CP_SELECT[10];
int const *SL_SELECT_V[10];
int const *SL_SELECT_C[10];
int const SL_SELECT_SIZE=256;

const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2","-Wall", "-lm",

		               "--static", "-std=c99", "-DONLINE_JUDGE", NULL};
const char * CP_CC[] = { "g++", "Main.cc", "-o", "Main","-O2", "-Wall",
			            "-lm", "--static", "-DONLINE_JUDGE", NULL};
int init_compile_systemcall_map(){
	//编译指令限制
	CP_SELECT[0]=CP_C;
	CP_SELECT[1]=CP_CC;


	//系统调用限制
	SL_SELECT_V[0]=LANG_C;
	SL_SELECT_C[0]=LANG_CC;

	SL_SELECT_V[1]=LANG_C;
	SL_SELECT_C[1]=LANG_CC;
	//G++跟GCC是一样的
}

void set_compile_info(){
	struct rlimit LIM;
	LIM.rlim_max = COMPILE_TIME;
	LIM.rlim_cur = COMPILE_TIME;
	setrlimit(RLIMIT_CPU,&LIM);

	LIM.rlim_max = COMPILE_FSIZE;
	LIM.rlim_cur = COMPILE_FSIZE;
	setrlimit(RLIMIT_FSIZE,&LIM);

	LIM.rlim_max = COMPILE_MSIZE;
	LIM.rlim_cur = COMPILE_MSIZE;
	setrlimit(RLIMIT_AS,&LIM);

}


int compile(int lang ){
	int pid;
	pid = fork();
	if(pid==0){
		//子进程
		alarm(10);
		//限制10秒编译时间
		//编译超时会自动发信号
		set_compile_info();
		freopen("ce.txt","w",stderr);
		execvp(CP_SELECT[lang][0],(char * const *)CP_SELECT[lang]);
	}
	else{
		//父进程
		int status = 0;
		waitpid(pid,&status,0);
		printf("%d:%d\n",pid,status);
		return status;
	}
}



int main(int argc , char ** argv){
	init_compile_systemcall_map();
	compile(0);	
	
	return 0;
}
