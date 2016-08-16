#include "judge_db.h"
void db::db::char_cp(char *t,const char *p){
	if(p!=NULL)
		strcpy(t,p);
	else
		memset(t,0,sizeof(char)*_BUFFER_SIZE);
}
const char* db::db::conn_parameter(const char *t){
	if(t[0])
		return t;
	return NULL;
}
db::db::db(){
	RETRY=10;
	RETRY_INTERVALS=1;
	STATEMENTS_BUFFER=NULL;
	STATEMENTS_BUFFER_SIZE=0;
	IS_DEFAULT_BUFFER=false;
}
db::db::~db(){
	if(IS_DEFAULT_BUFFER&&STATEMENTS_BUFFER!=NULL)
		delete STATEMENTS_BUFFER;
	mysql_close(&CONN);
}
void db::db::set_retry(int t ){
	if(t>0)
		RETRY=t;
}
void db::db::check_buffer(){
	if(NULL==STATEMENTS_BUFFER){
		STATEMENTS_BUFFER=new char[2*_BUFFER_SIZE];
		STATEMENTS_BUFFER_SIZE=2*_BUFFER_SIZE;
		IS_DEFAULT_BUFFER=true;
	}
}
unsigned int db::db::field_count(){
	return mysql_field_count(&CONN);
}
int db::db::num_rows(){
	return ROWS;
}
void db::db::set_buffer(char *t , int size){
	if(STATEMENTS_BUFFER!=NULL && IS_DEFAULT_BUFFER){
		delete(STATEMENTS_BUFFER);
		STATEMENTS_BUFFER=NULL;
	}
	STATEMENTS_BUFFER=t;
	STATEMENTS_BUFFER_SIZE=size;
	IS_DEFAULT_BUFFER=false;
}
void db::db::set_retry_intervals(int t){
	if(t>0)
		RETRY_INTERVALS=t;
}
bool db::db::set_character_set(char *csname){
	return mysql_set_character_set(&CONN,csname);
}
char* db::db::escape_string(char *to,char* from,unsigned long length){
	mysql_real_escape_string(&CONN,to,from, length);
	return to;
}
void db::db::close(){
	mysql_close(&CONN);
}
void db::db::init(
		const char * host,
		const char * user,
		const char * passwd,
		const char * db_name,
		unsigned int port,
		const char * unix_socket,
		unsigned long client_flag
		){
	char_cp(HOST,host);
	char_cp(USER,user);
	char_cp(PASSWD,passwd);
	char_cp(NAME,db_name);
	char_cp(UNIX_SOCKET,unix_socket);
	PORT=port;
	CLIENT_FLAG=client_flag;
	mysql_init(&CONN);
}
const char* db::db::get_error(){
	if(mysql_errno(&CONN))
		return mysql_error(&CONN);
	return NULL;
}
bool db::db::ping(){
	return mysql_ping(&CONN)==0?true:false;
}
void db::db::options(enum mysql_option option, char*t){
	mysql_options(&CONN,option,t);
}
bool db::db::connect(){
	MYSQL* tmp;
	char tt;
	tt=1;
	options(MYSQL_OPT_RECONNECT,&tt);
	if(NULL==mysql_real_connect(&CONN,
				conn_parameter(HOST),
				conn_parameter(USER),
				conn_parameter(PASSWD),
				conn_parameter(NAME),
				PORT,
				conn_parameter(UNIX_SOCKET),
				CLIENT_FLAG
				))     
		return false;
	return true;
}
bool db::db::timeout_judge( int t ){
	if(	CR_CONN_HOST_ERROR 	==t ||
			CR_SERVER_GONE_ERROR==t)
		return true;
	return false;
}
bool db::db::retry_conn(int &t){
	if(t--){
		ping();
		sleep(RETRY_INTERVALS);
		return true;
	}
	return false;
}
void db::db::free_result(){
	if(NULL!=RES){
		mysql_free_result(RES);
		RES=NULL;
		ROWS=0;
		FIELDS=0;
	}
}
bool db::db::has_next(){
	if(!success()) return false;
	if(mysql_errno(&CONN))
		return false;
	ROW=mysql_fetch_row(RES);
	if(NULL==ROW)
		return false;
	return true;
}
bool db::db::get_result(){
	RES=mysql_store_result(&CONN);
	if(NULL!=RES){
		FIELDS=mysql_num_fields(RES);
		ROWS=mysql_num_rows(RES);
	}
	else {
		FIELDS=0;
		ROWS=0;
		return success();
	}
	return true;
}
bool db::db::query(const char * fmt, ... ){
	int length,result;
	int time=RETRY;
	va_list ap;
	check_buffer();
	va_start(ap, fmt);
	length=vsnprintf(STATEMENTS_BUFFER,STATEMENTS_BUFFER_SIZE, fmt, ap);
	va_end(ap);
	free_result();
	if(length<0) return false;
	do{
		result=mysql_real_query(&CONN,STATEMENTS_BUFFER,length);
		result=mysql_errno(&CONN);
	}while(timeout_judge(result)&&retry_conn(time));
	if(result)
		return false;
	return get_result();
}
bool db::db::success(){
	if(0 == mysql_errno(&CONN))
		return true;
	return false;
}
const char * db::db::operator [](unsigned int index){
	if(index<FIELDS)
		return ROW[index];
	else
		throw std::out_of_range("访问越界");
}

