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
#include <fcntl.h>
#include <algorithm>
#include <mysql/mysql.h>
#include "okcalls.h"
#include "../include/judge_define.h"
#include "../include/judge_db.h"
#include "../include/judge_log.h"
#include "../include/judge_queue.h"
char db_name[BUFFER_SIZE];
char db_user[BUFFER_SIZE];
char db_password[BUFFER_SIZE];
char db_address[BUFFER_SIZE];
db::db conn;
Log::log w_log;

int COMPILE_TIME=60;//秒
int COMPILE_FSIZE=128*STD_MB;//字节
int COMPILE_MSIZE=64*STD_MB;//使用内存 单位字节
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
int SYSTEM_CALLS_COUNT[256];
const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-O2","-Wall", "-lm",
		               "--static", "-std=c99", "-DONLINE_JUDGE", NULL};
const char * CP_CC[] = { "g++", "Main.cpp", "-o", "Main","-O2", "-Wall",
			            "-lm", "--static", "-DONLINE_JUDGE", NULL};
const char LANG_EXT[2][8]={"c","cpp"};
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
		w_log.write("run_id:%d result:%s",run_id,mysql_field[acflag]);
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
		conn.query("SELECT time_limit,memory_limit FROM problem_info WHERE problem_id=%d",problem_id);
		if(!conn.has_next()){
			w_log.write("line %d:select fail",__LINE__);
			return false;
		}
		sprintf(buf,"%s %s",conn[0],conn[1]);
		rs+=sscanf(buf,"%d%d",&time,&memory);	
		return rs==8;
	}
	void get_solution_info(solution_info &sl){
		sl.write(buff);
		w_log.write("read:%s",buff);
		init_real_info();
		if(init_problem_info(buff)){
			w_log.write("read problem parameter success");
		}
		else{
			w_log.write("read problem parameter fail");
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
}
void init_systemcall_count(int lang){
	int i,j,k;
	memset(SYSTEM_CALLS_COUNT,0,sizeof(SYSTEM_CALLS_COUNT));
	for(i = 0 ; SL_SELECT_C[lang][i];++i){
		SYSTEM_CALLS_COUNT[SL_SELECT_V[lang][i]]=SL_SELECT_C[lang][i];
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
int compile(proc_info &std){
	int pid,lang;
	lang=std.lang;
	pid = fork();
	if(pid==0){
		//子进程
		alarm(60);
		//编译超时会发信号
		chdir(std.work_dir);
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
		if(reg.REG_SYSCALL>0&&SYSTEM_CALLS_COUNT[reg.REG_SYSCALL]==0){
			std.acflag=OJ_RE;
			std.kill_proc();
		}
		else if (SYSTEM_CALLS_COUNT[reg.REG_SYSCALL]>0)
			SYSTEM_CALLS_COUNT[reg.REG_SYSCALL]--;
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
bool get_solution(proc_info &std){
	char src_path[BUFFER_SIZE];
	sprintf(src_path,"Main.%s",LANG_EXT[std.lang]);
	conn.query("select source from source_code where solution_id=%d",std.run_id);
	if(!conn.has_next()){
		return false;
	}
	FILE *fp = fopen(src_path,"w");
	fprintf(fp,"%s",conn[0]);
	fclose(fp);
	conn.free_result();
	return true;
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
int read_buf(char *t ,const char * key, char *value){
	if(strncmp(t,key,strlen(key))==0){
		strcpy(value,t+after_equal(t));
		trim(value);
		return 1;
	}
	return 0;
}
int read_int(char *t,const char *key , int *value){
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
	conn.init(db_address,db_user,db_password,db_name,3306,NULL,0);
	if(!conn.connect()){
		w_log.write("conn fails");
		exit(0);
	}
	if(conn.set_character_set("utf8")){
		w_log.write(conn.get_error());
		exit(0);
	}
}
void mysql_update_problem_info(proc_info &std,bool is_ac ){
	conn.query("update problem_info set %s=%s+1 , submit=submit+1 where problem_id=%d",mysql_field[std.acflag],mysql_field[std.acflag],std.problem_id);
	if(!is_ac && std.acflag==OJ_AC){
		conn.query("update problem_info set solve=solve+1 where problem_id=%d",std.problem_id);
	}

}
void mysql_add_problem_info(proc_info &std){
	conn.query("select * from solution_info where user_name='%s' and problem_id=%d and contest_id=0 and result=%d ",std.user_name,std.problem_id,OJ_AC);
	mysql_update_problem_info(std,conn.num_rows()>0);


}
void mysql_update_solution(proc_info &std){
	conn.query("update solution_info set run_time=%d,run_memory=%d,result=%d where solution_id = %d",std.real_time,std.real_memory,std.acflag,std.run_id);
}

void mysql_add_ce_info(proc_info &std){
	char ceinfo[(1<<16)],*cend;
	FILE *fp = fopen("ce.txt","r");
	char tmp[BUFFER_SIZE];
	cend=ceinfo;
	conn.query("delete from compile_info where solution_id=%d",std.run_id);
	while(fgets(cend,1024,fp)){
		cend+=strlen(cend);
		if(cend-ceinfo>2048)break;
	}
	conn.query("INSERT INTO compile_info (solution_id,error) VALUES(%d,'%s')",std.run_id,conn.escape_string(tmp,ceinfo,strlen(ceinfo) ));

}
void mysql_change_result(int run_id,int result){
	conn.query("update solution_info set result=%d where solution_id=%d",result,run_id);
}
bool mysql_insert_contest_rank_user(proc_info &std){
	conn.query("insert into contest_rank_info (in_date,contest_id,problem_id,user_name,submit) values(null,%d,%d,'%s',0)",std.contest_id,std.problem_id,std.user_name);
	return true;
}
bool mysql_check_contest_rank_is_hava(proc_info &std){
	conn.query("select * from contest_rank_info where user_name='%s' and problem_id=%d and contest_id=%d",std.user_name,std.problem_id,std.contest_id);
	return conn.num_rows()>0;
}
void mysql_update_contest_user_rank(proc_info &std){
	conn.query("select problem_id from contest_rank_info where in_date is null and  user_name='%s' and problem_id=%d and contest_id=%d ",std.user_name,std.problem_id,std.contest_id);
	if(!mysql_check_contest_rank_is_hava(std))
		mysql_insert_contest_rank_user(std);
	if(conn.num_rows()!=0){
		if(std.acflag==OJ_AC){
			conn.query("update contest_rank_info SET in_date='%s' where contest_id=%d and problem_id=%d  and user_name='%s';",std.in_date,std.contest_id,std.problem_id,std.user_name);
		}
		else{
			conn.query("update contest_rank_info SET submit=submit+1 where contest_id=%d and problem_id=%d and in_date is null and user_name='%s';",std.contest_id,std.problem_id,std.user_name);
		} 
	}
	
}
void set_log(int client_id){
	int i;
	char tmp[BUFFER_SIZE];
	sprintf(tmp,"/home/judge/log/judge_judge_%d",client_id);
	w_log.get_system_time(tmp+strlen(tmp),BUFFER_SIZE);
	for(i=0;tmp[i];++i)
		if(tmp[i]==' ') tmp[i]='_';
		else if(tmp[i]==':')tmp[i]='_';
	tmp[strlen(tmp)-2]=0;
	w_log.set(1,tmp);
}
int main(int argc , char ** argv){
	int compile_ok;
	int i;
	proc_info std;
	m_queue mq(LOCAL_DIR);
	mq.set_flag(O_RDONLY);
	mq.create(0);
	solution_info sl;
	std.init_client_info(argv);
	if(strcmp(argv[argc-1],"DEBUG")==0)
		w_log.set(0);
	else
		set_log(std.client_id);;
	init_db_info();
	init_mysql();
	init_compile_systemcall_map();
	while(1){
		mq.receive(sl);
		std.get_solution_info(sl);
		execute_cmd("rm %s/*",std.work_dir);
		init_systemcall_count(std.lang);
		pid_t p_id;
		if(!get_solution(std)){
			w_log.write("get run_id %d solution fail",std.run_id);
			exit(0);
		}
		mysql_change_result(std.run_id,OJ_CI);
		compile_ok = compile(std);
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
		}
		std.debug_show_real_info();
		if(std.contest_id){
			mysql_update_contest_user_rank(std);
		}
		else{
			mysql_add_problem_info(std);
		}
		mysql_update_solution(std);
	}
	return 0;
}
