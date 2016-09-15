#ifndef JUDGE_LOG_CPP
#define JUDGE_LOG_CPP
#include "judge_log.h"
const char Log::log::time_template[128]="date +%Y-%m-%d\\ %H:%M:%S";
int Log::log::get_system_time(char *tmp,int n){
	int len;
	tp = popen(time_template,"r");
	len=fread(tmp,sizeof(char),sizeof(char)*n,tp);
	tmp[--len]=' ';
	tmp[++len]=':';
	tmp[++len]=0;
	pclose(tp);
	tp=NULL;
	return len;
}
void Log::log::set(int m,const char* t){
	assert(m<=1);
	if(m==0||t==NULL){
		mod=0;
	}
	else if(m==1){
		memset(write_dir,0,sizeof(write_dir));
		sprintf(write_dir,"%s",t);
		fp=fopen(write_dir,"a+");
		setvbuf(fp,buff,_IOLBF,2048);
		mod=1;
	}
}
Log::log::log():mod(0){//written to the console
}
Log::log::log(const char* tmp):mod(1){//written to the file
	fp=NULL;
	fp=fopen(tmp,"a");
}
Log::log::~log(){
	if(fp)
		fclose(fp);
	if(tp)
		pclose(tp);
}
void Log::log::write(const char* fmt, ... ){
	char *tmp;
	va_list ap;
	va_start(ap,fmt);
	tmp=buffer;
	tmp+=get_system_time(tmp,2048);
	vsprintf(tmp,fmt,ap);
	va_end(ap);
	tmp+=strlen(tmp);
	*tmp++='\n';
	*tmp=0;
	if(mod){
		fprintf(fp,buffer);
	}
	else
		printf(buffer);
	return ;
}
#endif	
