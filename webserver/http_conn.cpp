#include "http_conn.h"
#include <iostream>


int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

// 网站的根目录
const char* doc_root = "/home/qiushi/webserver/resources";

int setnonblocking(int fd){
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);
}

void addfd(int epollfd, int fd, bool one_shot){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN || EPOLLRDHUP; //EPOLLRDHUP对链接断开时在内核层处理，并不提交给应用层

	if(one_shot){
		   event.events |= EPOLLONESHOT;
	}
	epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);

	setnonblocking(fd);
}


void removefd(int epollfd, int fd){
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
}


void modfd(int epollfd, int fd, int event){
	epoll_event ev;
	ev.data.fd = fd;
	ev.events = event | EPOLLRDHUP;

	epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev);
}


void http_conn::init(int sockfd, sockaddr_in &addr){
	m_sockfd = sockfd;
	m_address = addr;

	int reuse = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	addfd(m_epollfd, sockfd, true);

	m_user_count++;

	init();
}

void http_conn::init(){
	
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_checked_index = 0;
	m_start_line = 0;
	m_read_index = 0;
	m_version = 0;
	m_linger = 0;
	m_content_length = 0;
		
	bzero(m_read_buf, READ_BUFFER_SIZE);
	
}

void http_conn::close_conn(){
	
	if(m_sockfd !=-1){
		
		removefd(m_epollfd, m_sockfd);
		
		m_sockfd = -1;
		
		m_user_count--;
		
	}

}

bool http_conn::read(){
	
	if(m_read_index >= READ_BUFFER_SIZE)return false;

	int bytes_read = 0;
	
	while(true){
		
		bytes_read = recv(m_sockfd, m_read_buf+m_read_index, READ_BUFFER_SIZE-m_read_index);
		
		if(bytes_read == -1){
			
			if(errno == EAGAIN || errno == EWOULDBLOCK){
				//none data
				break;
			}
			
			return false;
			
		}else if(bytes_read == 0){
			//have closed
			return false;
		}
		
		m_read_index += bytes_read;
		
	}
	
	std::cout<<m_read_buf<<std::endl;
		
	return true;
}


http_conn::HTTP_CODE http_conn::process_read(){
	
	LINE_STATUS line_status = LINE_OK;
	
	HTTP_CODE ret = NO

	char * text = 0;

	while(((m_check_state==CHECK_STATE_CONTENT)&&(line_status==LINE_OK))
			||((line_status=parse_line())==LINE_OK)){
			//解析到了一行数据就往下走
			
		text = get_line();
			
		m_start_line = m_checked_index;
		
		std::cout<<"get a line data:"<<text<<std::endl;

		switch(m_check_state){
			
			case CHECK_STATE_REQUESTLINE:{
				
				ret = parse_request_line(text);
				
				if(ret == BAD_REQUEST){
					
					return BAD_REQUEST;
					
				}
				
				break;
				
			} 
			
			case CHECK_STATE_HEADER:{
				
				ret = parse_headers(text);
				
				if(ret == BAD_REQUEST){
					
					return BAD_REQUEST;
					
				}else if(ret == GET_REQUEST){

					return do_request();
					
				}
			}
			
			case CHECK_STATE_CONTENT:{

				ret = parse_content(text);
				
				if(ret == GET_REQUEST){
					
					return do_request();

				}
				line_status = LINE_OPEN;
				break;
			}
			default:{
				return INTERNAL_ERROR;
			}
		}
		return NO_REQUEST;
	}
}

http_conn::HTTP_CODE http_conn::parse_request_line(char *text){

	m_url = strpbrk(text, " \t");

	*m_url++ = '\0';
	
	char *method = text;
	
	if(strcasecmp(method, "GET") == 0 ){
		
		m_method = GET;
		
	}else{

		return BAD_REQUEST;
		
	}

	m_version = strpbrk(m_url, '\t');
	if(!m_version){
		return BAD_REQUEST;
	}

	*m_version++ = '\0';
	
	if(strcasecmp(m_version, "HTTP/1.1")!=0){
		
		return BAD_REQUEST;
		
	}

	if(strncasecmp(m_url, "http://", 7) == 0){
		
		m_url += 7;
		m_url = strchr(m_url, '/');
		
	}

	if(!m_url || m_url[0] != '/'){

		return BAD_REQUEST;
		
	}

	m_check_state = CHECK_STATE_HEADER;

	return NO_REQUEST;
	
}

http_conn::HTTP_CODE http_conn::parse_headers(char *text){
	
	if( text[0] == '\0' ) {
		
		if ( m_content_length != 0 ) { 
			
			m_check_state = CHECK_STATE_CONTENT;            
			return NO_REQUEST;   
			
		}  
	
		return GET_REQUEST;    
	
	} else if ( strncasecmp( text, "Connection:", 11 ) == 0 ) {       
      
		text += 11;        
		text += strspn( text, " \t" );    
		
		if ( strcasecmp( text, "keep-alive" ) == 0 ) {   
			m_linger = true;     	
		}    
		
	} else if ( strncasecmp( text, "Content-Length:", 15 ) == 0 ) { 
	
		text += 15;        
		text += strspn( text, " \t" );        
		m_content_length = atol(text);  
		
	} else if ( strncasecmp( text, "Host:", 5 ) == 0 ) {        

		text += 5;       
		
		text += strspn( text, " \t" ); 
		
		m_host = text; 

	} else { 
	
		printf( "oop! unknow header %s\n", text );  
		
	}    

	return NO_REQUEST;
	
}

http_conn::HTTP_CODE http_conn::parse_content(char *text){
	
    if (m_read_index >= ( m_content_length + m_checked_idx ) )    {
		
		text[ m_content_length ] = '\0';        
		return GET_REQUEST;  
		
	}    
	
	return NO_REQUEST;
	
}


http_conn::LINE_STATUS http_conn::parse_line(){

	char temp;
	
	for(; m_checked_index<=m_read_index; m_checked_index++){
		
		temp = m_read_buf[m_checked_index];
		
		if(temp == '\r'){
			
			if((m_checked_index+1) == m_read_index)return LINE_OPEN;
			
			else if(m_read_buf[m_checked_index+1]=='\n'){
				
				m_read_buf[m_checked_index++] = '\0';
				m_read_buf[m_checked_index++] = '\0';
				return LINE_OK;

			}
			return LINE_BAD;
			
		}else if(temp == '\n'){
		
			if((m_checked_index > 1) && (m_read_buf[m_checked_index-1] == '\r')){

				m_read_buf[m_checked_index-1] = '\0';
				m_read_buf[m_checked_index++] = '\0';
				return LINE_OK;
				
			}
			
			return LINE_BAD;
			
		}

		return LINE_OPEN;

	}

	return LINE_OK;
	
}

http_conn::LINE_STATUS http_conn::do_request(){
	strcpy(m_real_file);
}


bool http_conn::write(){
	
}


void http_conn::process(){
	HTTP_CODE read_ret = process_read();

	if(read_ret == NO_REQUEST){
		
		
	}
	
	std::cout<<"process......."<<std::endl;

}
