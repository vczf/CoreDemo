#include <unistd.h>
#include <string.h>
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
#include <mysql++.h>
#include <queue>
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
char get_some_solution_info[BUFFER_SIZE];
int oj_client_num=0;
int oj_sleep_time=10;
mysqlpp::Connection conn;

#define JUDGE_CLIENT_NUM 10 
char OJ_HOME[BUFFER_SIZE];
char WORK_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
char PIPE_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
int PIPE_FP[JUDGE_CLIENT_NUM];
int CLIENT_PID[JUDGE_CLIENT_NUM];
int CLIENT_RUN_FOR_ID[JUDGE_CLIENT_NUM];
int DEBUG;
void mysql_rejudge(int run_id){
	char tmp[1024];
	sprintf(tmp,"update solution_info set result=1 where solution_id=%d",run_id);
	mysqlpp::Query query= conn.query(tmp);
	mysqlpp::SimpleResult ans = query.execute();
	if(!ans){
		printf("更改错误,run_id=%d\n",run_id);
		exit(0);
	}
}

void call_for_exit(){
	int i;
	printf("stop\n正在关闭子进程....\n");
	for(i = 0 ; i < oj_client_num ;++i){
		int &tmp = CLIENT_PID[i];
		if(tmp!=-1)
			kill(tmp,SIGKILL);
	}
}
void call_for_exit(int s){
	int i;
	printf("stop\n正在关闭子进程....\n");
	for(i = 0 ; i < oj_client_num;++i){
		int &tmp = CLIENT_PID[i];
		if(tmp!=-1)
			kill(tmp,SIGKILL);
	}
}
void create_client(int id){
	pid_t pid;
	pid = fork();
	CLIENT_PID[id]=-1;
	if(pid==0){
		char tmp[100];
		sprintf(tmp,"%d",id);
		printf("create:%d tmp=%s",id,tmp);
		if(-1==execl("/usr/bin/judge_client",OJ_HOME,WORK_DIR[id],PIPE_DIR[id],tmp,(char *)NULL)){
			printf("fuck\n");
			exit(0);
		}
	}
	else{
		CLIENT_PID[id]=pid;
		printf("Create_id:%d\n",pid);
	}
}
void init_client_info(){
	int i,j,k;
	char pipe_tmp[BUFFER_SIZE];
	sprintf(OJ_HOME,"/home/judge/");//最好通过文件来读
	sprintf(pipe_tmp,"%spipe/",OJ_HOME);
	chdir(OJ_HOME);
	if(access(pipe_tmp,F_OK)==-1){
		printf("缺少以下目录,请用管理员创建\n%s\n",pipe_tmp);
		exit(0);
	}
	for(i = 0 ; i < oj_client_num;++i){
		sprintf(WORK_DIR[i],"%srun%d/",OJ_HOME,i);
		sprintf(PIPE_DIR[i],"%spipe/%d",OJ_HOME,i);
		if(access(WORK_DIR[i],F_OK)==-1){
			if(mkdir(WORK_DIR[i],0777)==-1){
				printf("无法创建以下目录,请用管理员权限创建\n%s\n",WORK_DIR[i]);
				exit(0);
			}
		}
		if(access(PIPE_DIR[i],F_OK)==-1){
			if(mkfifo(PIPE_DIR[i],0777)==-1){
				printf("无法创建以下管道,请用管理员权限创建\n%s\n",PIPE_DIR[i]);
				exit(0);
			}
		}
		int tmp_fd;
		create_client(i);
		tmp_fd = open(PIPE_DIR[i],O_NONBLOCK|O_RDONLY);
		PIPE_FP[i]=open(PIPE_DIR[i],O_NONBLOCK|O_WRONLY);
		if(PIPE_FP[i]==-1){
			printf("无法打开管道\n凶手就是他:%s\n",PIPE_FP[i]);
			exit(0);
		}
		if(DEBUG){
			printf("now create:%d\n",i);
		}
		close(tmp_fd);
	}
	if(DEBUG){
		printf("OJ_HOME:%s\n",OJ_HOME);
		for(i = 0 ; i < oj_client_num;++i){
			printf("client_id:%d\nwork_dir:%s\npipe_dir:%s\n",i,WORK_DIR[i],PIPE_DIR[i]);
		}
	}
}
int get_free_client(){
	static int ID=0;
	int status,i ;
	int status2;
	for(i= 0; i < oj_client_num*2;++i){
		int jg1 = waitpid(CLIENT_PID[ID],&status,WNOHANG|WUNTRACED);
		if(CLIENT_PID[ID]==-1){
			create_client(ID);
		}
		/*else if(WIFEXITED(status)){
			if(CLIENT_RUN_FOR_ID[i]!=-1)
				mysql_rejudge(CLIENT_RUN_FOR_ID[ID]);
			create_client(ID);
			CLIENT_RUN_FOR_ID[ID]=-1;

		}*/
		else if(WIFSTOPPED(status)){
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
	if(DEBUG){
		printf("send_id:%d\n",id);
	}
	info.write(tmp);
	rs = write(PIPE_FP[id],tmp,strlen(tmp));
	kill(CLIENT_PID[id],SIGCONT);
	printf("wait...\n");
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
		if(DEBUG)printf("%s\n",value);
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
	printf("打开成功\n");
	fp=fopen("/home/judge/etc/judge.conf","r");
	if(fp==NULL) exit(1);
	printf("打开成功\n");
	while(fgets(buf,BUFFER_SIZE-1,fp)){
		read_buf(buf,"DB_ADDRESS",db_address);
		read_buf(buf,"DB_NAME",db_name);
		read_buf(buf,"DB_USER_NAME",db_user);
		read_buf(buf,"DB_PASSWORD",db_password);
		read_int(buf,"OJ_SLEEP_TIME",&oj_sleep_time);
		read_int(buf,"OJ_CLIENT_NUM",&oj_client_num);
	}
	sprintf(get_some_solution_info,"SELECT in_date,user_name,solution_id,problem_id,language,contest_id FROM solution_info WHERE result<2");
}
void init_mysql(){
	printf("开始连接了\n");
	conn.set_option(new mysqlpp::SetCharsetNameOption("utf8"));
	if(!conn.connect(db_name,db_address,db_user,db_password)){
		printf("连接失败啦!\n");
		exit(0);
	}
	conn.query("SET NAMES utf8");
}
void get_some_solution(std::queue<solution_info> &q){
	char buf[BUFFER_SIZE];
	mysqlpp::Query query= conn.query(get_some_solution_info);
	mysqlpp::StoreQueryResult res = query.store();
	mysqlpp::StoreQueryResult::const_iterator it;
	for(it = res.begin(); it != res.end();++it){
		mysqlpp::Row row = *it;
		sprintf(buf,"%s %s %s %s %s %s",row[0].data(),row[1].data(),row[2].data(),row[3].data(),row[4].data(),row[5].data());
		printf("已获得:%s\n",buf);
		solution_info tmp;
		tmp.read(buf);
		q.push(tmp);
	}
}
int main(){
	int i;
	DEBUG=1;
	char tmp[BUFFER_SIZE];
	std::queue<solution_info> q;
	for(i=0;i<JUDGE_CLIENT_NUM;++i)
		CLIENT_PID[i]=-1;
	atexit(call_for_exit);
	signal(SIGQUIT,call_for_exit);
	signal(SIGKILL,call_for_exit);
	signal(SIGTERM,call_for_exit);
	DEBUG=1;
	init_db_info();
	init_mysql();
	init_client_info();
	while(1){
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
