#ifndef  _HTTP_CONN_H
#define  _HTTP_CONN_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "locker.h"
#include <sys/uio.h>



class http_conn{
public:
	http_conn(){};
	~http_conn(){};
	
	static int m_epollfd;
	static int m_user_count;
	static const int READ_BUFFER_SIZE = 2048;
	static const int WRITE_BUFFER_SIZE = 1024;

	
	void process();
	void init(int sockfd, sockaddr_in &addr);
	void close_conn();
	bool read();
	bool write();

	// HTTP请求方法，这里只支持GET    
	enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT}; 
	
	/*        
		解析客户端请求时，主状态机的状态        
		CHECK_STATE_REQUESTLINE:当前正在分析请求行 
		CHECK_STATE_HEADER:当前正在分析头部字段
		CHECK_STATE_CONTENT:当前正在解析请求体   
	*/    
	enum CHECK_STATE { 
		CHECK_STATE_REQUESTLINE = 0, 
		CHECK_STATE_HEADER, 
		CHECK_STATE_CONTENT 
	};
	
	/*       
		服务器处理HTTP请求的可能结果，报文解析的结果
		NO_REQUEST          :   请求不完整，需要继续读取客户数据
		GET_REQUEST         :   表示获得了一个完成的客户请求
		BAD_REQUEST         :   表示客户请求语法错误
		NO_RESOURCE         :   表示服务器没有资源
		FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
		FILE_REQUEST        :   文件请求,获取文件成功
		INTERNAL_ERROR      :   表示服务器内部错误
		CLOSED_CONNECTION   :   表示客户端已经关闭连接了    
	*/    
	enum HTTP_CODE { 
		NO_REQUEST, 
		GET_REQUEST, 
		BAD_REQUEST, 
		NO_RESOURCE, 
		FORBIDDEN_REQUEST, 
		FILE_REQUEST, 
		INTERNAL_ERROR, 
		CLOSED_CONNECTION 
	}; 
	
	// 从状态机的三种可能状态，即行的读取状态，分别表示    
	// 1.读取到一个完整的行 2.行出错 3.行数据尚且不完整    
	enum LINE_STATUS { 
		LINE_OK = 0, 
		LINE_BAD, 
		LINE_OPEN 
	};




private:
	int m_sockfd;				//http socket
	sockaddr_in m_address;
	char m_read_buf[READ_BUFFER_SIZE];
	int m_read_index;

	int m_checked_index;
	int m_start_line;

	char * m_url;
	char * m_version;

	METHOD m_method;
	char * m_host;

	int m_content_length;
	
	bool m_linger;
	
	CHECK_STATE m_check_state;

	void init();

	HTTP_CODE process_read();
	HTTP_CODE parse_request_line(char *text);
	HTTP_CODE parse_headers(char *text);
	HTTP_CODE parse_content(char *text);
	LINE_STATUS parse_line();
	LINE_STATUS do_request();

	char * get_line(){return m_read_buf + m_start_line;}
};

#endif
