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
		int get_system_time(char* tmp,int n );
		void set(int m,const char* tmp=NULL);
		log();	
		log(const char* tmp);
		~log();
		void write(const char* fmt, ... );
	};
	const char log::time_template[128]="date +%Y-%m-%d\\ %H:%M:%S";
};
#endif
