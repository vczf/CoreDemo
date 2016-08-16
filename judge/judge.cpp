#include <unistd.h>
#include <string.h>
#include <ctype.h>
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
#include <fcntl.h>
#include <algorithm>
#include <queue>
#include <mysql/mysql.h>
#include "../include/judge_define.h"
#include "../include/judge_log.h"
#include "../include/judge_db.h"

char DB_NAME[BUFFER_SIZE];
char DB_USER[BUFFER_SIZE];
char DB_PASSWORD[BUFFER_SIZE];
char DB_ADDRESS[BUFFER_SIZE];
char GET_SOME_SOLUTION_INFO[BUFFER_SIZE];
int oj_client_num=1;
int oj_sleep_time=10;
log::log Log;
int DEBUG=0;
db::db conn;

#define JUDGE_CLIENT_NUM 10 
char OJ_HOME[BUFFER_SIZE];
char WORK_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
char PIPE_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
int PIPE_FP[JUDGE_CLIENT_NUM];
int CLIENT_PID[JUDGE_CLIENT_NUM];
int CLIENT_RUN_FOR_ID[JUDGE_CLIENT_NUM];

void call_for_exit(){
	int i;
	printf("stop\n正在关闭子进程....\n");
	Log.write("正在关闭子进程");
	for(i = 0 ; i < oj_client_num ;++i){
		int &tmp = CLIENT_PID[i];
		if(tmp!=-1)
			kill(tmp,SIGKILL);
	}
	Log.write("exit");
}
void call_for_exit(int s){
	int i;
	printf("stop\n正在关闭子进程....\n");
	Log.write("正在关闭子进程");
	for(i = 0 ; i < oj_client_num;++i){
		int &tmp = CLIENT_PID[i];
		if(tmp!=-1)
			kill(tmp,SIGKILL);
	}
	Log.write("exit");
}
void create_client(int id){
	pid_t pid;
	pid = fork();
	CLIENT_PID[id]=-1;
	if(pid==0){
		char tmp[100];
		sprintf(tmp,"%d",id);
		Log.write("正在创建judge%d号",id);
		if(DEBUG){
			if(-1==execl("/usr/bin/judge_client",OJ_HOME,WORK_DIR[id],PIPE_DIR[id],tmp,"DEBUG",(char *)NULL)){
				Log.write("创建judge%d号失败",id);
				exit(0);
			}
		}
		else{
			if(-1==execl("/usr/bin/judge_client",OJ_HOME,WORK_DIR[id],PIPE_DIR[id],tmp,(char *)NULL)){
				Log.write("创建judge%d号失败",id);
				exit(0);
			}

		}
	}
	else{
		CLIENT_PID[id]=pid;
	}
}
void init_client_info(){
	int i,j,k;
	char pipe_tmp[BUFFER_SIZE];
	sprintf(OJ_HOME,"/home/judge/");//最好通过文件来读
	sprintf(pipe_tmp,"%spipe/",OJ_HOME);
	chdir(OJ_HOME);
	if(access(pipe_tmp,F_OK)==-1){
		Log.write("缺少该目录:%s,请使用root权限创建该目录",pipe_tmp);
		exit(0);
	}
	for(i = 0 ; i < oj_client_num;++i){
		sprintf(WORK_DIR[i],"%srun%d/",OJ_HOME,i);
		sprintf(PIPE_DIR[i],"%spipe/%d",OJ_HOME,i);
		if(access(WORK_DIR[i],F_OK)==-1){
			if(mkdir(WORK_DIR[i],0777)==-1){
				Log.write("缺少该目录:%s,请使用root权限创建该目录",WORK_DIR[i]);
				exit(0);
			}
		}
		if(access(PIPE_DIR[i],F_OK)==-1){
			if(mkfifo(PIPE_DIR[i],0777)==-1){
				Log.write("缺少该管道:%s,请使用root权限创建该管道",PIPE_DIR[i]);
				exit(0);
			}
		}
		int tmp_fd;
		create_client(i);
		tmp_fd = open(PIPE_DIR[i],O_NONBLOCK|O_RDONLY);
		PIPE_FP[i]=open(PIPE_DIR[i],O_NONBLOCK|O_WRONLY);
		if(PIPE_FP[i]==-1){
			Log.write("无法打开该管道%s",PIPE_FP[i]);
			exit(0);
		}
		close(tmp_fd);
	}
}
int get_free_client(){
	static int ID=0;
	int status,i ;
	int status2;
	for(i= 0; i < oj_client_num*2;++i){
		int jg1 = waitpid(CLIENT_PID[ID],&status,WNOHANG|WUNTRACED);
		if(CLIENT_PID[ID]==-1){
			Log.write("判题端%d号为-1",ID);
			create_client(ID);
			Log.write("判题端%d号创建完成",ID);
		}/*
		else if(WIFEXITED(status)){
			Log.write("判题端%d号已关闭，重新创建",ID);
			if(CLIENT_RUN_FOR_ID[i]!=-1)
				mysql_rejudge(CLIENT_RUN_FOR_ID[ID]);
			create_client(ID);
			CLIENT_RUN_FOR_ID[ID]=-1;

		}*/
		else if(WIFSTOPPED(status)){
			Log.write("判题端%d号空闲中,返回",ID);
			CLIENT_RUN_FOR_ID[ID]=-1;
			return ID;			
		}
		ID++;
		ID%=oj_client_num;
	}
	return -1;
}
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
void send_solution_info(int id,solution_info &info){
	int i,j,k,rs;
	int status,status2;
	char tmp[BUFFER_SIZE];
	info.write(tmp);
	rs = write(PIPE_FP[id],tmp,strlen(tmp));
	kill(CLIENT_PID[id],SIGCONT);
	CLIENT_RUN_FOR_ID[id]=info.run_id;
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
	fp=fopen("/home/judge/etc/judge.conf","r");
	if(fp==NULL){ 
		Log.write("打开配置文件失败");
		exit(1);
	}
	Log.write("读取配置文件中.....");
	while(fgets(buf,BUFFER_SIZE-1,fp)){
		read_buf(buf,"DB_ADDRESS",DB_ADDRESS);
		read_buf(buf,"DB_NAME",DB_NAME);
		read_buf(buf,"DB_USER_NAME",DB_USER);
		read_buf(buf,"DB_PASSWORD",DB_PASSWORD);
		read_int(buf,"OJ_SLEEP_TIME",&oj_sleep_time);
		read_int(buf,"OJ_CLIENT_NUM",&oj_client_num);
	}
	sprintf(GET_SOME_SOLUTION_INFO,"SELECT in_date,user_name,solution_id,problem_id,language,contest_id FROM solution_info WHERE result<2");
}
void init_mysql(){
	conn.init(DB_ADDRESS,DB_USER,DB_PASSWORD,DB_NAME,3306,NULL,0);
	if(!conn.connect()){
		Log.write("conn fails");
		exit(0);
	}
	if(conn.set_character_set("utf8")){
		Log.write(conn.get_error());
		exit(0);
	}
}
void get_some_solution(std::queue<solution_info> &q){
	char buf[BUFFER_SIZE];
	conn.query(GET_SOME_SOLUTION_INFO);
	while(conn.has_next()){
		sprintf(buf,"%s %s %s %s %s %s",conn[0],conn[1],conn[2],conn[3],conn[4],conn[5]);
		
		solution_info tmp;
		tmp.read(buf);
		q.push(tmp);

	}
}
int daemon_init(){ 
	pid_t pid;
	if((pid = fork()) < 0) return(-1);
	else if(pid != 0) exit(0); /* parent exit */
	setsid(); /* become session leader */
	chdir(OJ_HOME); /* change working directory */
	umask(0); /* clear file mode creation mask */
	close(0); /* close stdin */
	close(1); /* close stdout */
	close(2); /* close stderr */
	return(0); 
}
int main(int args,char **argc){
	int i;
	char tmp[BUFFER_SIZE];
	if(strcmp(argc[args-1],"DEBUG")==0){
		Log.set(0);
		DEBUG=1;
	}
	else{
		sprintf(tmp,"/home/judge/log/judge_");
		Log.get_system_time(tmp+strlen(tmp),BUFFER_SIZE);
		for(i=0;tmp[i];++i)
			if(tmp[i]==' ') tmp[i]='_';
			else if(tmp[i]==':')tmp[i]='_';
		tmp[strlen(tmp)-2]=0;
		Log.set(1,tmp);
		DEBUG=0;
		daemon_init();
	}
	std::queue<solution_info> q;
	for(i=0;i<JUDGE_CLIENT_NUM;++i)
		CLIENT_PID[i]=-1;
	atexit(call_for_exit);//程序结束之前运行该函数
	signal(SIGQUIT,call_for_exit);
	signal(SIGKILL,call_for_exit);
	signal(SIGTERM,call_for_exit);
	init_db_info();
	init_mysql();
	init_client_info();
	while(1){
		Log.write("正在寻找");
		get_some_solution(q);
		while(!q.empty()){
			int id = -1;
			solution_info t = q.front();
			q.pop();
			do{
				id=get_free_client();
			}while(id==-1);
			send_solution_info(id,t);
		}
		sleep(oj_sleep_time);
	}
	return 0;
}
