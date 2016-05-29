#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "okcalls.h"
#include <fcntl.h>
#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
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
//编译限制 通过读配置文件得到
//暂时写死
int COMPILE_TIME=60;//秒
int COMPILE_FSIZE=65535;//字节
int COMPILE_MSIZE=64*STD_MB;//使用内存 单位字节

int Debug=0;

//运行限制通过数据库获得
//通过父进程管道输送,并保存在proc里
//int RUN_TIME  = 1 ;//单位秒
//int RUN_FSIZE = 10*STD_MB;//输出文件大小，单位mb
//int RUN_MSIZE = 32*STD_MB;//使用内存大小，单位mb



//0GCC 1G++
char const  **CP_SELECT[10];
int const *SL_SELECT_V[10];
int const *SL_SELECT_C[10];
int const SL_SELECT_SIZE=256;
int system_calls_count[256];
const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2","-Wall", "-lm",
		               "--static", "-std=c99", "-DONLINE_JUDGE", NULL};
const char * CP_CC[] = { "g++", "Main.cc", "-o", "Main","-O2", "-Wall",
			            "-lm", "--static", "-DONLINE_JUDGE", NULL};
struct proc_info{
	pid_t pid;//进程pid
	int problem_id;//题目数
	int time;//所耗时间单位毫秒
	int memory;//所耗内存单位kb
	int outputsize;//输出文件大小
	int lang;//语言
	int runid;
	int acflag;
	char work_dir[BUFFER_SIZE];//工作目录
	char pipe_dir[BUFFER_SIZE];// 
	char oj_home[BUFFER_SIZE];
	char data_in[BUFFER_SIZE];
	char data_out[BUFFER_SIZE];
	char buff[BUFFER_SIZE];
	int pipe_fd;
	int client_id;
	FILE *fp;
	void init_client_info(char **info){
		int i;
		sscanf(info[0],"%s",oj_home);
		sscanf(info[1],"%s",work_dir);
		sscanf(info[2],"%s",pipe_dir);
		sscanf(info[3],"%d",&client_id);
		fp=NULL;
		pipe_fd=open(pipe_dir,O_RDONLY|O_NONBLOCK);
	}
	bool init_problem_info(char *buff){
		int rs;
		printf("%d:read:%s\n",client_id,buff);
		rs = sscanf(buff,"%d%d%d%d%d",&runid,&problem_id,&time,&memory,&lang);
		sprintf(data_in,"%sdata/%d/data.in",oj_home,problem_id);
		sprintf(data_out,"%sdata/%d/data.out",oj_home,problem_id);
		return rs==5;
	}
	void Debug_ShowInfo(){
		char tmp[BUFFER_SIZE];
		sprintf(tmp,"%sDebug.out",work_dir);
		printf("son:client_id:%d tmp:%s\n",client_id,tmp);
		if(fp==NULL)
			fp = fopen(tmp,"a+");
		if(fp==NULL){
			printf("fuck!son:%d\n",client_id);
			return ;
		}
		fprintf(fp,"OJ_HOME:%s\n",oj_home);
		fprintf(fp,"work_dir:%s\n",work_dir);
		fprintf(fp,"pipe_dir:%s\n",pipe_dir);
		fprintf(fp,"runid:%d\nproblem:%d\ntime:%d\nmemory:%d\nlang:%d\n",runid,problem_id,time,memory,lang);
		fclose(fp);
		fp=NULL;

		/*		
		printf("OJ_HOME:%s\n",oj_home);
		printf("work_dir:%s\n",work_dir);
		printf("pipe_dir:%s\n",pipe_dir);
		printf("runid:%d\nproblem:%d\ntime:%d\nmemory:%d\nlang:%d\n",runid,problem_id,time,memory,lang);
		*/
	}
	void get_solution_info(){
		printf("son:stop\n");
		raise(SIGSTOP);
		if(pipe_fd==-1)
			pipe_fd=open(pipe_dir,O_RDONLY);
		printf("son:pipe_fd:%d\n",pipe_fd);
		assert(pipe_fd!=-1);
		int len = read(pipe_fd,buff,sizeof(buff)-1);
		buff[len]=0;
		if(init_problem_info(buff)){
			printf("读取成功\n");
			Debug_ShowInfo();
		}
		else{
			printf("读取失败!\n");
			exit(0);
			
		}
	}

};

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
void set_run_info(proc_info &info){
	struct rlimit LIM;
	int RUN_TIME,RUN_FSIZE,RUN_MSIZE; 
	RUN_TIME=info.time;
	RUN_FSIZE=info.outputsize;
	RUN_MSIZE=info.memory;
	LIM.rlim_max = RUN_TIME;
	LIM.rlim_cur = RUN_TIME;
	setrlimit(RLIMIT_CPU,&LIM);
	//时间
	LIM.rlim_max = RUN_FSIZE*2;
	LIM.rlim_cur = RUN_FSIZE*3/2;
	setrlimit(RLIMIT_FSIZE,&LIM);
	//文件大小
	LIM.rlim_max = LIM.rlim_cur = 0;
	setrlimit(RLIMIT_NOFILE,&LIM);
	LIM.rlim_max = RUN_MSIZE*2;
	LIM.rlim_cur = RUN_MSIZE;
	setrlimit(RLIMIT_AS,&LIM);
	//内存使用大小
	LIM.rlim_max = 64*STD_MB;
	LIM.rlim_cur = 64*STD_MB;
	setrlimit(RLIMIT_STACK,&LIM);
	//堆栈大小
	LIM.rlim_max = 1;
	LIM.rlim_cur = 1;
	setrlimit(RLIMIT_NPROC,&LIM);
	//进程数目
	///////初期我只打算做c++跟c，所以后面的活你们加油！
}

int compile(int lang ){
	int pid;
	pid = fork();
	if(pid==0){
		//子进程
		alarm(10);
		//限制10秒编译时间
		//编译超时会发信号
		set_compile_info();
		freopen("ce.txt","w",stderr);
		execvp(CP_SELECT[lang][0],(char * const *)CP_SELECT[lang]);
	}
	else{
		//父进程
		int status = 0;
		waitpid(pid,&status,0);
		return status;
	}
}

int run_solution(proc_info info){
	int lang = info.lang;
	set_run_info(info);
	///////初期我只打算做c++跟c，所以后面的活你们加油！
	//chdir(work_dir);
	freopen("data.in","r",stdin);
	freopen("user.out","w",stdout);
	freopen("error.txt","a+",stderr);
	ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	if(lang<2)
		execl("./Main",(char *)NULL );
	exit(0);
	//后面多余，根本不会运行到这里
}
int watch_solution(pid_t pid){

}
int judge_solution(){}

int main(int argc , char ** argv){
	int compile_ok;
	int i;
	for(i=0;i<argc;++i)
		printf("son:%s\n",argv[i]);
	proc_info std;
	std.init_client_info(argv);
	while(1){
		std.get_solution_info();
		sleep(rand()%5);
	}
	pid_t p_id;
	init_compile_systemcall_map();
	compile_ok = compile(0);
	if(!compile_ok){
		//运行代码
		p_id = fork();
		if(p_id==0)
			run_solution(std);
		else{
			watch_solution(p_id);
			judge_solution();
		}
	}
	else{
		//输出ce信息到数据库
	}
	return 0;
}
