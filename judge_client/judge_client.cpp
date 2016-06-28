#include <unistd.h>
#include <iostream>
#include <stdarg.h>
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
#include <mysql/mysql.h>
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

//db_info
char db_name[BUFFER_SIZE];
char db_user[BUFFER_SIZE];
char db_password[BUFFER_SIZE];
char db_address[BUFFER_SIZE];
//mysqlpp::Connection conn;
MYSQL *conn;
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

const char * mysql_field[]={
	"","","","",
	"AC","PE","WA",
	"TLE","MLE","OLE",
	"RE","CE","RE",
	"RE","RE"
};


//0GCC 1G++
char const  **CP_SELECT[10];
int const *SL_SELECT_V[10];
int const *SL_SELECT_C[10];
int const SL_SELECT_SIZE=256;
int system_calls_count[256];
const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2","-Wall", "-lm",
		               "--static", "-std=c99", "-DONLINE_JUDGE", NULL};
const char * CP_CC[] = { "g++", "Main.cpp", "-o", "Main","-O2", "-Wall",
			            "-lm", "--static", "-DONLINE_JUDGE", NULL};
const char lang_ext[2][8]={"c","cpp"};
int execute_cmd(const char * fmt, ...){
    char cmd[BUFFER_SIZE];
    int ret = 0;
    va_list ap;
    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system(cmd);
    va_end(ap);
    return ret;
}
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
	int contest_id;
	char user_name[BUFFER_SIZE];
	int run_id;
	int acflag;
	char work_dir[BUFFER_SIZE];//工作目录
	char pipe_dir[BUFFER_SIZE];// 
	char oj_home[BUFFER_SIZE];
	char data_in[BUFFER_SIZE];
	char data_out[BUFFER_SIZE];
	char user_out[BUFFER_SIZE];
	char buff[BUFFER_SIZE];
	char in_date[BUFFER_SIZE];
	int pipe_fd;
	int client_id;
	FILE *fp;
	int real_time;//所耗时间单位毫秒
	int real_memory;//所耗内存单位kb
	void debug_show_real_info(){
		printf("run_id:%d result:%d\n",run_id,acflag);
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
		char in_date_tmp[2][32];
		rs = sscanf(buff,"%s%s%s%d%d%d%d",in_date_tmp[0],in_date_tmp[1],user_name,&run_id,&problem_id,&lang,&contest_id);
		rs--;
		sprintf(in_date,"%s %s",in_date_tmp[0],in_date_tmp[1]);
		sprintf(data_in,"%sdata/%d/data.in",oj_home,problem_id);
		sprintf(data_out,"%sdata/%d/data.out",oj_home,problem_id);
		sprintf(user_out,"%suser.out",work_dir);
		outputsize=get_file_size(data_out);
		char buf[BUFFER_SIZE];
		char tmp[BUFFER_SIZE];
		sprintf(tmp,"SELECT time_limit,memory_limit FROM problem_info WHERE problem_id=%d",problem_id);
		MYSQL_RES *res;
		MYSQL_ROW row;
		mysql_real_query(conn,tmp,strlen(tmp));
		printf("查询失败了?\n");
		res=mysql_store_result(conn);
		row = mysql_fetch_row(res);
		sprintf(buf,"%s %s",row[0],row[1]);
		mysql_free_result(res);
		rs+=sscanf(buf,"%d%d",&time,&memory);	
		return rs==8;
	}
	void Debug_ShowInfo(){
		char tmp[BUFFER_SIZE];
		sprintf(tmp,"%sDebug.out",work_dir);
		if(fp==NULL)
			fp = fopen(tmp,"a+");
		if(fp==NULL){
			printf("fuck!son:%d\n",client_id);
			return ;
		}
		fprintf(fp,"OJ_HOME:%s\n",oj_home);
		fprintf(fp,"work_dir:%s\n",work_dir);
		fprintf(fp,"pipe_dir:%s\n",pipe_dir);
		fprintf(fp,"run_id:%d\nproblem:%d\ntime:%d\nmemory:%d\nlang:%d\n",run_id,problem_id,time,memory,lang);
		fclose(fp);
		fp=NULL;

	}
	void get_solution_info(){
		raise(SIGSTOP);
		if(pipe_fd==-1)
			pipe_fd=open(pipe_dir,O_RDONLY);
		assert(pipe_fd!=-1);
		int len = read(pipe_fd,buff,sizeof(buff)-1);
		printf("son:读取成功!\n内容:%s\n",buff);
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
	freopen(info.data_in,"r",stdin);
	freopen("./user.out","w",stdout);
	freopen("./error.out","a+",stderr);
	chmod("user.out",777);
	chmod("error.out",777);
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
		//if(std.judge_outputsize()){std.kill_proc(); break;}		
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
	std.acflag=compare_pe(std.data_out,std.user_out);
}
int get_solution(proc_info &std){
	char buf[BUFFER_SIZE];
	char tmp[BUFFER_SIZE];
	char src_path[BUFFER_SIZE];
	sprintf(tmp,"SELECT source FROM source_code WHERE solution_id=%d",std.run_id);
	MYSQL_RES *res;
	MYSQL_ROW row;
	mysql_real_query(conn,tmp,strlen(tmp));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	sprintf(src_path,"Main.%s",lang_ext[std.lang]);
	FILE *fp = fopen(src_path,"w");
	fprintf(fp,"%s",row[0]);
	fclose(fp);
	mysql_free_result(res);
	;
}
int after_equal(char *c){
	int i;
	for(i=0;c[i]!='=';++i);
	return ++i;
}
void trim(char * c){
    char buf[BUFFER_SIZE];
    char * start,*end;
	strcpy(buf,c);
    start=buf;
	while(isspace(*start)) start++;
	    end=start;
	while(!isspace(*end)) end++;
	*end='\0';
	strcpy(c,start);
}
int read_buf(char *t , char * key, char *value){
	if(strncmp(t,key,strlen(key))==0){
		strcpy(value,t+after_equal(t));
		trim(value);
		return 1;
	}
	return 0;
}
int read_int(char *t,char *key , int *value){
	char buf2[BUFFER_SIZE];
	if(read_buf(t,key,buf2))
		sscanf(buf2,"%d",value);
}
void init_db_info(){
	FILE *fp=NULL;
	char buf[BUFFER_SIZE];
	fp=fopen("../etc/judge.conf","r");
	if(fp==NULL) exit(1);
	while(fgets(buf,BUFFER_SIZE-1,fp)){
		read_buf(buf,"DB_ADDRESS",db_address);
		read_buf(buf,"DB_NAME",db_name);
		read_buf(buf,"DB_USER_NAME",db_user);
		read_buf(buf,"DB_PASSWORD",db_password);
	}
}
void init_mysql(){
	conn = mysql_init(NULL);
	if(!mysql_real_connect(conn,db_address,db_user,db_password,db_name,3306,NULL,0)){
		printf("connect fail\n");
		exit(0);
	}
    const char * utf8sql = "set names utf8";
	if (mysql_real_query(conn, utf8sql, strlen(utf8sql)))
	{

        exit(0);
	}

	/*conn.set_option(new mysqlpp::SetCharsetNameOption("utf8"));
	conn.connect(db_name,db_address,db_user,db_password);
	conn.query("SET NAMES utf8");
	*/
}
void mysql_update_problem_info(proc_info &std,bool is_ac ){
	char tmp[BUFFER_SIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(tmp,"update problem_info set %s=%s+1 , submit=submit+1 where problem_id=%d",mysql_field[std.acflag],mysql_field[std.acflag],std.problem_id);
	printf("%s\n",tmp);
	mysql_real_query(conn,tmp,strlen(tmp));
	if(!is_ac && std.acflag==OJ_AC){
		printf("gg3\n");
		sprintf(tmp,"update problem_info set solve=solve+1 where problem_id=%d",std.problem_id);
		mysql_real_query(conn,tmp,strlen(tmp));
	}
}
void mysql_add_problem_info(proc_info &std){
	MYSQL_RES *res;
	MYSQL_ROW row;
	char tmp[BUFFER_SIZE];
	sprintf(tmp,"select * from solution_info where user_name='%s' and problem_id=%d and contest_id=0 and result=%d ",std.user_name,std.problem_id,OJ_AC);
	printf("add:语句:%s\n",tmp);
	mysql_real_query(conn,tmp,strlen(tmp));
	res=mysql_store_result(conn);
	mysql_free_result(res);
	mysql_update_problem_info(std,mysql_affected_rows(conn)>0);
}
void mysql_update_solution(proc_info &std){
	MYSQL_RES *res;
	MYSQL_ROW row;
	char tmp[BUFFER_SIZE];
	sprintf(tmp,"update solution_info set run_time=%d,run_memory=%d,result=%d where solution_id = %d",std.real_time,std.real_memory,std.acflag,std.run_id);
	mysql_real_query(conn,tmp,strlen(tmp));
}
void mysql_add_ce_info(proc_info &std){
	char ceinfo[(1<<16)],*cend;
	FILE *fp = fopen("ce.txt","r");
	MYSQL_RES *res;
	MYSQL_ROW row;
	char tmp[BUFFER_SIZE];
	cend=ceinfo;
	sprintf(tmp,"delete from compile_info where solution_id=%d",std.run_id);
	mysql_real_query(conn,tmp,strlen(tmp));
	while(fgets(cend,1024,fp)){
		cend+=strlen(cend);
		if(cend-ceinfo>60000)break;
	}
	sprintf(tmp,"INSERT INTO compile_info (solution_id,error) VALUES(%d,",std.run_id);
	cend=tmp+strlen(tmp);
	*cend++ = '\'';
    cend += mysql_real_escape_string(conn,cend, ceinfo, strlen(ceinfo));
    *cend++ = '\'';
    *cend++ = ')';
	*cend = 0;
	mysql_real_query(conn,tmp,strlen(tmp));
}
void mysql_change_result(int run_id,int result){
	char tmp[BUFFER_SIZE];
	sprintf(tmp,"update solution_info set result=%d where solution_id=%d",result,run_id);
	mysql_real_query(conn,tmp,strlen(tmp));
}
bool mysql_insert_contest_rank_user(proc_info &std){
	MYSQL_RES *res;
	MYSQL_ROW row;
	char tmp[BUFFER_SIZE];
	sprintf(tmp,"insert into contest_rank_info (in_date,contest_id,problem_id,user_name,submit) values(null,%d,%d,'%s',0)",std.contest_id,std.problem_id,std.user_name);
	mysql_real_query(conn,tmp,strlen(tmp));
	res=mysql_store_result(conn);
	mysql_free_result(res);

	return true;

}
bool mysql_check_contest_rank_is_hava(proc_info &std){
	char tmp[BUFFER_SIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(tmp,"select * from contest_rank_info where user_name='%s' and problem_id=%d and contest_id=%d",std.user_name,std.problem_id,std.contest_id);
	mysql_real_query(conn,tmp,strlen(tmp));
	res=mysql_store_result(conn);
	mysql_free_result(res);
	printf("确定有没有:%d\n",mysql_affected_rows(conn));
	return mysql_affected_rows(conn)>0;
}
void mysql_update_contest_user_rank(proc_info &std){
	char tmp[BUFFER_SIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	printf("runid:%d,result:%d\nac.size()=%d\n",std.run_id,std.acflag,0);
	if(!mysql_check_contest_rank_is_hava(std))
		mysql_insert_contest_rank_user(std);
	printf("runid:%d,result:%d\nac.size()=%d\n",std.run_id,std.acflag,1);
	sprintf(tmp,"select problem_id from contest_rank_info where in_date is null and  user_name='%s' and problem_id=%d and contest_id=%d ",std.user_name,std.problem_id,std.contest_id);
	mysql_real_query(conn,tmp,strlen(tmp));
	printf("确定多少被更改:%d\n",mysql_affected_rows(conn));
	printf("%s\n",tmp);
	if(mysql_affected_rows(conn)!=0){
		res=mysql_store_result(conn);
		mysql_free_result(res);
		 //没有通过过
		if(std.acflag==OJ_AC){
			sprintf(tmp,"update contest_rank_info SET in_date='%s' where contest_id=%d and problem_id=%d  and user_name='%s';",std.in_date,std.contest_id,std.problem_id,std.user_name);
			mysql_real_query(conn,tmp,strlen(tmp));
			printf("AC情况可以添加么\n");
		}
		else{
			sprintf(tmp,"update contest_rank_info SET submit=submit+1 where contest_id=%d and problem_id=%d and in_date is null and user_name='%s';",std.contest_id,std.problem_id,std.user_name);
			mysql_real_query(conn,tmp,strlen(tmp));
		} 
	}
	
}
int main(int argc , char ** argv){
	int compile_ok;
	int i;
	proc_info std;
	std.init_client_info(argv);
	init_db_info();
	init_mysql();
	init_compile_systemcall_map();
	while(1){
		std.get_solution_info();
		mysql_change_result(std.run_id,OJ_CI);
		execute_cmd("rm %s/*",std.work_dir);
		init_systemcall_count(std.lang);
		pid_t p_id;
		get_solution(std);
		compile_ok = compile(std.lang);
		if(!compile_ok){
			mysql_change_result(std.run_id,OJ_RI);
			p_id = fork();
			if(p_id==0)
				run_solution(std);
			else{
				std.pid=p_id;
				watch_solution(std);
				judge_solution(std);
			}
		}
		else{
			std.init_real_info();
			std.acflag=OJ_CE;
			mysql_add_ce_info(std);
			printf("ceinfo\n");
			//输出ce信息到数据库
		}
		printf("solution_id:%d\nresult:%d\n",std.run_id,std.acflag);
		if(std.contest_id){
			//比赛里的题
			mysql_update_contest_user_rank(std);
		}
		else{
			//大题库里
			//需要修改problem的AC等信息
			mysql_add_problem_info(std);
		}
		mysql_update_solution(std);
		printf("而处恭\n");
	}
	return 0;
}
