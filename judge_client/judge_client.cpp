#include <unistd.h>
#include <string.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "okcalls.h"
#include <fcntl.h>
#include <algorithm>
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
#define OJ_RE_SO 14
//StackOverFLOW
#define OJ_DBZ 15
//DIVIDE_BY_ZERO
#define OJ_AV 16
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
/*
copy from hustoj
https://github.com/zhblue/hustoj/blob/master/beta/core/judge_client/judge_client.cc
*/
//编译限制 通过读配置文件得到
//暂时写死
int COMPILE_TIME=60;//秒
int COMPILE_FSIZE=128*STD_MB;//字节
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
long get_file_size(const char * filename){
    struct stat f_stat;
	if (stat(filename, &f_stat) == -1)
        return 0;
    return (long) f_stat.st_size;

}
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
	char user_out[BUFFER_SIZE];
	char buff[BUFFER_SIZE];
	int pipe_fd;
	int client_id;
	FILE *fp;
	int real_time;//所耗时间单位毫秒
	int real_memory;//所耗内存单位kb
	void debug_show_real_info(){
		printf("time:%d\nmemory:%d\n",real_time,real_memory);
		printf("result:%d\n",acflag);

	}
	void init_real_info(){
		real_time=real_memory=0;
		acflag=OJ_AC;
	}
	void init_client_info(char **info){
		int i;
		sscanf(info[0],"%s",oj_home);
		sscanf(info[1],"%s",work_dir);
		sscanf(info[2],"%s",pipe_dir);
		sscanf(info[3],"%d",&client_id);
		fp=NULL;
		chdir(work_dir);
		pipe_fd=open(pipe_dir,O_RDONLY|O_NONBLOCK);
	}
	bool init_problem_info(char *buff){
		int rs;
		printf("%d:read:%s\n",client_id,buff);
		rs = sscanf(buff,"%d%d%d%d%d",&runid,&problem_id,&time,&memory,&lang);
		sprintf(data_in,"%sdata/%d/data.in",oj_home,problem_id);
		sprintf(data_out,"%sdata/%d/data.out",oj_home,problem_id);
		sprintf(user_out,"%suser.out",work_dir);
		outputsize=get_file_size(data_out);
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
		init_real_info();
		if(init_problem_info(buff)){
			printf("读取成功\n");
			Debug_ShowInfo();
		}
		else{
			printf("读取失败!\n");
			exit(0);
		}
	}
	bool judge_memory(){
		if(memory<real_memory){
			acflag = OJ_ML;
			real_memory=memory;
			return true;
		}
		return false;
	}
	bool judge_time(){
		if(time<real_time){
			acflag=OJ_TL;
			real_time=time;
			return true;
		}
		return false;
		
	}
	bool judge_outputsize(){
		if(get_file_size("error.out")){
			acflag=OJ_RE;
			return true;
		} 
		if(get_file_size("user.out")>outputsize*2){
			acflag=OJ_OL;
			return true;
		}

		return false;

	}
	void kill_proc(){ptrace(PTRACE_KILL,pid,NULL,NULL);	}
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
void init_systemcall_count(int lang){
	int i,j,k;
	memset(system_calls_count,0,sizeof(system_calls_count));
	for(i = 0 ; SL_SELECT_C[lang][i];++i){
		system_calls_count[SL_SELECT_V[lang][i]]=SL_SELECT_C[lang][i];
	} 

}
int get_flag(int t){
	switch (t){
        case SIGCHLD:
	    case SIGALRM:
	    alarm(0);
	    case SIGKILL:
	    case SIGXCPU:
	    return OJ_TL;
	    case SIGXFSZ:
	    return OJ_OL;
		case SIGFPE:
		return OJ_DBZ;
		break;
		case SIGILL:
		return OJ_RE_SO;
		case SIGSEGV:
		return OJ_AV;
	    default:
	    return OJ_RE;
	}
} 

void set_compile_info(){
	struct rlimit LIM;
	LIM.rlim_max = COMPILE_TIME*2;
	LIM.rlim_cur = COMPILE_TIME;
	setrlimit(RLIMIT_CPU,&LIM);
	LIM.rlim_max = COMPILE_FSIZE*2;
	LIM.rlim_cur = COMPILE_FSIZE;
	setrlimit(RLIMIT_FSIZE,&LIM);
	LIM.rlim_max = COMPILE_MSIZE*2;
	LIM.rlim_cur = COMPILE_MSIZE;
	setrlimit(RLIMIT_AS,&LIM);
}
void set_run_info(proc_info &info){
	struct rlimit LIM;
	int RUN_TIME,RUN_FSIZE,RUN_MSIZE; 
	RUN_TIME=info.time;
	RUN_FSIZE=info.outputsize;
	RUN_MSIZE=info.memory;
	alarm(0);
	alarm(info.time/1000+1);
	printf("time:%d\n",info.time/1000+11);
	LIM.rlim_max = RUN_TIME/1000+1;
	LIM.rlim_cur = RUN_TIME/1000+1;
	setrlimit(RLIMIT_CPU,&LIM);
	//时间
	LIM.rlim_max = STD_F_LIM+STD_MB;
	LIM.rlim_cur = STD_F_LIM;
	setrlimit(RLIMIT_FSIZE,&LIM);
	//文件大小
	LIM.rlim_max = (STD_MB<<6)+RUN_MSIZE;
	LIM.rlim_cur = STD_MB<<6;
	setrlimit(RLIMIT_AS,&LIM);
	//内存使用大小
	LIM.rlim_max = STD_MB<<6;
	LIM.rlim_cur = STD_MB<<6;
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
		alarm(5);
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
int get_proc_status(int pid, const char * mark) {
	    FILE * pf;
	    char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
	    int ret = 0;
	    sprintf(fn, "/proc/%d/status", pid);
	    pf = fopen(fn, "r");
	    int m = strlen(mark);
	    while (pf && fgets(buf, BUFFER_SIZE - 1, pf)){
	        buf[strlen(buf) - 1] = 0;
	        if (strncmp(buf, mark, m) == 0)
	            sscanf(buf + m + 1, "%d", &ret);
		}
	    if (pf)
	        fclose(pf);
	    return ret;
}
int run_solution(proc_info &info){
	int lang = info.lang;
	///////初期我只打算做c++跟c，所以后面的活你们加油！
	chdir(info.work_dir);
	printf("data_in:%s\n",info.data_in);
   
	freopen(info.data_in,"r",stdin);
	freopen("./user.out","w",stdout);
	freopen("./error.out","a+",stderr);
	chmod("./user.out",777);
	chmod("./error.out",777);
	chroot(info.work_dir);
	nice(19);
	
	int tmp;
	while(tmp = setgid(2333),tmp!=0)sleep(1);
	while(tmp = setuid(2333),tmp!=0)sleep(1);
	while(tmp = setresuid(2333,2333,2333),tmp!=0) sleep(1);
	set_run_info(info);

	ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	if(lang<2)
		execl("./Main",(char *)NULL );
	exit(0);
}
int watch_solution(proc_info &std){
	int i,j,k;
	struct user_regs_struct reg;
	struct rusage ruse;
	int status,exitcode;
	std.real_time=0;
	while(1){
		wait4(std.pid,&status,0,&ruse);
		std.real_memory=std::max(std.real_memory,get_proc_status(std.pid,"VmPeak:"));
		std.real_time= (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
		std.real_time += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);
		if(std.judge_memory()){std.kill_proc(); break;}
		if(std.judge_time()){std.kill_proc(); break;}
		if(std.judge_outputsize()){std.kill_proc(); break;}		
		if(WIFEXITED(status)) break;
		if(std.acflag!=OJ_AC){std.kill_proc();break;}
		exitcode = WEXITSTATUS(status);
		if(exitcode==0x05||exitcode==0);
		else{
			std.acflag = get_flag(exitcode);
			std.kill_proc();
			break;
		}
	    if (WIFSIGNALED(status)){
			int sig = WTERMSIG(status);
			std.acflag = get_flag(sig);
		   	std.kill_proc();
			break;
		}
		ptrace(PTRACE_GETREGS,std.pid,NULL,&reg);
		if(reg.REG_SYSCALL>0&&system_calls_count[reg.REG_SYSCALL]==0){
			std.acflag=OJ_RE;
			std.kill_proc();
		}
		else if (system_calls_count[reg.REG_SYSCALL]>0)
			system_calls_count[reg.REG_SYSCALL]--;
		ptrace(PTRACE_SYSCALL,std.pid,NULL,NULL);
	}
}
void find_next_nonspace(int &c1,int &c2,FILE *& f1, FILE *&f2){
	while(isspace(c1)) c1 = fgetc(f1);
	while(isspace(c2)) c2 = fgetc(f2);
	return ;
}
int compare_diff(char *p1, char *p2){
	int c1,c2,ret;
	FILE *f1,*f2;
	f1 = fopen(p1,"r");
	f2 = fopen(p2,"r");
	if(!f1||!f2) return OJ_RE;
	while(1){
		c1=fgetc(f1);
		c2=fgetc(f2);
		if(c1==EOF&&c2==EOF){ret = OJ_AC;break;}
		if(c1!=c2){ret =OJ_WA;break;} 
	}
	if(f1) fclose(f1);
	if(f2) fclose(f2);
	return ret;
}
int compare_pe(char *p1, char *p2){
	int c1,c2,ret,flag=1;
	FILE *f1,*f2;
	f1 = fopen(p1,"r");
	f2 = fopen(p2,"r");
	if(!f1||!f2) return OJ_RE;
	while(flag){
		c1=fgetc(f1);
		c2=fgetc(f2);
		find_next_nonspace(c1,c2,f1,f2);
		if(c1==EOF&&c2==EOF){ret = OJ_PE;break;}
		if(c1==EOF||c2==EOF){ret = OJ_WA;break;}
		while(flag){
			if(c1!=c2){
				ret = OJ_WA;
				flag = 0;
				break;
			} 
			c1=fgetc(f1);
			c2=fgetc(f2);
			if(isspace(c1)&&isspace(c2))
				break;
		}
	}
	if(f1) fclose(f1);
	if(f2) fclose(f2);
	return ret;
}
int judge_solution(proc_info &std){
	int peflag=OJ_AC;
	int ret;
	if(std.judge_memory())return 0;
	if(std.judge_time())return 0;
	if(std.judge_outputsize())return 0;
	if(std.acflag!=OJ_AC) return 0;
	ret = compare_diff(std.data_out,std.user_out);
	if(ret!=OJ_WA){
		std.acflag=ret;
		return 0;
	}
	compare_pe(std.data_out,std.user_out);
}
int get_solution(proc_info &std){
	;
}

int main(int argc , char ** argv){
	int compile_ok;
	int i;
	proc_info std;
	std.init_client_info(argv);
	init_compile_systemcall_map();
	while(1){
		std.get_solution_info();
		init_systemcall_count(std.pid);
		pid_t p_id;
		get_solution(std);
		compile_ok = compile(std.lang);
		if(!compile_ok){
			p_id = fork();
			if(p_id==0)
				run_solution(std);
			else{
				std.pid=p_id;
				watch_solution(std);
				std.debug_show_real_info();
				judge_solution(std);
			}
		}
		else{
			//输出ce信息到数据库
			printf("编译错误\n");
		}
	}
	return 0;
}
