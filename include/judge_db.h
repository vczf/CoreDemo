#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <unistd.h>
#include <stdarg.h>
#ifndef JUDGE_DB
#define JUDGE_DB
namespace db{
	class db{
		enum{ 			_BUFFER_SIZE=2048};
		char 			NAME[_BUFFER_SIZE];
		char 			USER[_BUFFER_SIZE];
		char 			PASSWD[_BUFFER_SIZE];
		char 			HOST[_BUFFER_SIZE];
		char 			UNIX_SOCKET[_BUFFER_SIZE];
		char*			STATEMENTS_BUFFER;
		int 			STATEMENTS_BUFFER_SIZE;
		bool			IS_DEFAULT_BUFFER; 
		unsigned int 	PORT;
		unsigned long 	CLIENT_FLAG;
		int 			RETRY;
		int 			RETRY_INTERVALS;
		int 			FIELDS;
		int 			ROWS;
		MYSQL			CONN;
		MYSQL_RES*		RES;
		MYSQL_ROW	    ROW;
		void char_cp(char *t,const char *p);
		const char* conn_parameter(const char *t);
		public:
		db();
		~db();
		void set_retry(int t );
		void check_buffer();
		unsigned int field_count();
		void set_buffer(char *t , int size);
		void set_retry_intervals(int t);
		bool set_character_set(const char *csname);
		char* escape_string(char *to,char* from,unsigned long length);
		void close();
		void init( const char * host, const char * user, const char * passwd, const char * db_name, unsigned int port, const char * unix_socket, unsigned long client_flag);
		const char* get_error();
		bool ping();
		void options(enum mysql_option option, char*t);
		bool connect();
		bool timeout_judge( int t );
		bool retry_conn(int &t);
		void free_result();
		bool has_next();
		bool get_result();
		bool query(const char * fmt, ... );
		bool success();
		int num_rows();
		const char * operator [](unsigned int index);
	};
};
#endif

