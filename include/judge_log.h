#ifndef JUDGE_LOG
#define JUDGE_LOG

namespace log{
	#include <unistd.h>
	#include <stdio.h>
	#include <string.h>
	#include <stdarg.h>
	#include <cassert>
	class log{
		static const char time_template[128];
		char buffer[2048];
		FILE *fp,*tp;
		int mod;
		public:
		int get_system_time(char *tmp,int n){
			int len;
			tp = popen(time_template,"r");
			len=fread(tmp,sizeof(char),sizeof(char)*n,tp);
			tmp[--len]=' ';
			tmp[++len]=':';
			tmp[++len]=0;
			return len;
		}
		void set(int m,const char* t=NULL){
			assert(m<=1);
			if(m==0||t==NULL){
				mod=0;
				if(fp!=NULL){
					fclose(fp);
					fp=NULL;
				}
			}
			else if(m==1){
				if(fp!=NULL){
					fclose(fp);
					fp=NULL;
				}
				mod=1;
				fp=fopen(t,"a+");
			}
		}
		log():mod(0){//written to the console
		}
		log(const char* tmp):mod(1){//written to the file
			fp=NULL;
			fp=fopen(tmp,"a+");
		}
		~log(){
			if(fp)
				fclose(fp);
			if(tp)
				pclose(tp);
		}
		void write(const char* fmt, ... ){
			char *tmp;
			va_list ap;
			va_start(ap,fmt);
			tmp=buffer;
			tmp+=get_system_time(tmp,2048);
			vsprintf(tmp,fmt,ap);
			va_end(ap);
			tmp+=strlen(tmp);
			*tmp='\n';
			if(mod)
				fprintf(fp,buffer,NULL);
			else
				printf(buffer,NULL);
			return ;
		}
	};
	const char log::time_template[128]="date +%Y-%m-%d\\ %H:%M:%S";
}
#endif
