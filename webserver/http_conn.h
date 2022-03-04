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

	// HTTP���󷽷�������ֻ֧��GET    
	enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT}; 
	
	/*        
		�����ͻ�������ʱ����״̬����״̬        
		CHECK_STATE_REQUESTLINE:��ǰ���ڷ��������� 
		CHECK_STATE_HEADER:��ǰ���ڷ���ͷ���ֶ�
		CHECK_STATE_CONTENT:��ǰ���ڽ���������   
	*/    
	enum CHECK_STATE { 
		CHECK_STATE_REQUESTLINE = 0, 
		CHECK_STATE_HEADER, 
		CHECK_STATE_CONTENT 
	};
	
	/*       
		����������HTTP����Ŀ��ܽ�������Ľ����Ľ��
		NO_REQUEST          :   ������������Ҫ������ȡ�ͻ�����
		GET_REQUEST         :   ��ʾ�����һ����ɵĿͻ�����
		BAD_REQUEST         :   ��ʾ�ͻ������﷨����
		NO_RESOURCE         :   ��ʾ������û����Դ
		FORBIDDEN_REQUEST   :   ��ʾ�ͻ�����Դû���㹻�ķ���Ȩ��
		FILE_REQUEST        :   �ļ�����,��ȡ�ļ��ɹ�
		INTERNAL_ERROR      :   ��ʾ�������ڲ�����
		CLOSED_CONNECTION   :   ��ʾ�ͻ����Ѿ��ر�������    
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
	
	// ��״̬�������ֿ���״̬�����еĶ�ȡ״̬���ֱ��ʾ    
	// 1.��ȡ��һ���������� 2.�г��� 3.���������Ҳ�����    
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
