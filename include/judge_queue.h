#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <mqueue.h>
#ifndef JUDGE_QUEUE
#define JUDGE_QUEUE
#include "my_err.h"
#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
class m_queue{
	struct mq_attr attr,tmp_attr;
	mqd_t mqd;
	int length;
	int mode;
	int flags;
	int pathname_size;
	bool close_flag;
//	char *pathname;
	char pathname[4096];
	public:
	~m_queue(){
		mq_close(mqd);
		if(close_flag)
			mq_unlink(pathname);
//		if(pathname)
//			free(pathname);
	}
	void set_unlink_queue(bool t){
		close_flag=t;
	}
	m_queue(){
		pathname_size=sysconf(_PC_PATH_MAX);
	//	pathname=(char *)calloc(pathname_size,sizeof(char));
	//	if(NULL==pathname){
	//		err_sys("calloc error");
	//	}
		flags=O_RDWR|O_CREAT;
		attr.mq_maxmsg=100;
		attr.mq_msgsize=1024;
		attr.mq_flags=flags;
		mode = FILE_MODE;
		close_flag=false;
	}
	m_queue(const char* t):m_queue(){
		int len;
		len=strlen(t);
		if(len>pathname_size-1)
			err_msg("pathname too long");
		strncpy(pathname,t,len);
		close_flag=false;
	}
	void set_mode(int mode){
		this->mode=mode;
	}
	void set_flag(int flag){
		attr.mq_flags=flag;
	}
	void set_msg_size(int size){
		length=size;
		attr.mq_msgsize=length;
	}
	void set_msg_num(int num){
		attr.mq_maxmsg=num;
	}
	long get_msg_num(){
		mq_getattr(mqd,&tmp_attr);
		return tmp_attr.mq_curmsgs;
	}
	long get_msg_size(){
		mq_getattr(mqd,&tmp_attr);
		return tmp_attr.mq_msgsize;
	}

	void create(int m){
		if(m){
			mqd = mq_open(pathname,flags,mode,&attr);
			if(-1!=mqd){
				get_msg_num();
				if(tmp_attr.mq_msgsize!=attr.mq_msgsize){
					mq_close(mqd);
					mq_unlink(pathname);
					mqd = mq_open(pathname,flags,mode,&attr);
				}
			}
		}
		else{
			mqd = mq_open(pathname,flags);
			get_msg_num();
			length=tmp_attr.mq_msgsize;
		}
		if(-1==mqd)
			err_msg("mq open fail");
	}
	template <typename T>
	bool send(const T& t,int prio = 0){
		if(-1==mq_send(mqd,(const char *)&t,length,prio)){
			err_msg("send fail");
			return false;
		}
		return true;
	}
	template <typename T>
	bool receive(T &t,unsigned int *prio=NULL){
		if(-1==mq_receive(mqd,(char *)&t,length,prio)){
			err_msg("receive fail");
			return false;
		}
		return true;
	}
};
#endif


