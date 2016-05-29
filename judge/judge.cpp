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

#define JUDGE_CLIENT_NUM 2 
char OJ_HOME[BUFFER_SIZE];
char WORK_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
char PIPE_DIR[JUDGE_CLIENT_NUM][BUFFER_SIZE];
int PIPE_FP[JUDGE_CLIENT_NUM];
int CLIENT_PID[JUDGE_CLIENT_NUM];
int DEBUG;
void call_for_exit(){
	int i;
	printf("stop\n正在关闭子进程....\n");
	for(i = 0 ; i < JUDGE_CLIENT_NUM;++i){
		int &tmp = CLIENT_PID[i];
		if(tmp!=-1)
			kill(tmp,SIGKILL);
	}
}

void call_for_exit(int s){
	int i;
	printf("stop\n正在关闭子进程....\n");
	for(i = 0 ; i < JUDGE_CLIENT_NUM;++i){
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
	if(access(pipe_tmp,F_OK)==-1){
		printf("缺少以下目录,请用管理员创建\n%s\n",pipe_tmp);
		exit(0);
	}
	for(i = 0 ; i < JUDGE_CLIENT_NUM;++i){
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
		for(i = 0 ; i < JUDGE_CLIENT_NUM;++i){
			printf("client_id:%d\nwork_dir:%s\npipe_dir:%s\n",i,WORK_DIR[i],PIPE_DIR[i]);
		}
	}
	
}

int get_free_client(){
	static int ID=0;
	int status,i ;
	int status2;
	for(i= 0; i < JUDGE_CLIENT_NUM*2;++i){
		int jg1 = waitpid(CLIENT_PID[ID],&status,WNOHANG|WUNTRACED);
		if(CLIENT_PID[ID]==-1){
			printf("进到这里了\n");
			create_client(ID);
		}
		else if(WIFSTOPPED(status)){
			if(DEBUG){
				printf("get_id:%d\n",ID);
			}
			return ID;			
		}
		ID++;
		ID%=JUDGE_CLIENT_NUM;
		printf("阻塞了吗?\n");
	}
	return -1;
}
struct solution_info{
	int run_id;
	int problem_id;
	int time;
	int memory;
	int lang;
	void read(char *buff){
		sscanf(buff,"%d%d%d%d%d",&run_id,&problem_id,&time,&memory,&lang);
	}
	void write(char *buff){
		sprintf(buff,"%d %d %d %d %d\n",run_id,problem_id,time,memory,lang);
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
}
int main(){
	int i;
	char tmp[BUFFER_SIZE];
	for(i=0;i<JUDGE_CLIENT_NUM;++i)
		CLIENT_PID[i]=-1;
	atexit(call_for_exit);
	signal(SIGQUIT,call_for_exit);
	signal(SIGKILL,call_for_exit);
	signal(SIGTERM,call_for_exit);
	DEBUG=1;
	init_client_info();
	FILE *fp;
	fp = fopen("data","r");
	std::queue<solution_info> q;
	while(fgets(tmp,sizeof(tmp)-1,fp)!=NULL){
		solution_info tt;
		tt.read(tmp);
		q.push(tt);
	}
	while(!q.empty()){
		int id=-1;
		solution_info t = q.front();
		q.pop();
		do{
			id=get_free_client();
			printf("获得的id:%d\n",id);
		}while(id==-1);
		printf("出来\n");
		send_solution_info(id,t);
		sleep(1);
	}
	printf("好像搞定了的样子\n");
	sleep(10);


//	execl("../judge_client/judge_client_Main");
	
}
