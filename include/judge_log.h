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
		char write_dir[2048];
		char buffer[2048];
		char buff[2049];
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
};
#endif
