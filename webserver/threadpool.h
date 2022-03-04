#ifndef _THREADPOOL_H
#define _THREADPOOL_H


#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "locker.h"


//�̳߳���
template<typename T>
class threadpool{
public:
	threadpool(int thread_number = 8, int max_requests = 10000);

	~threadpool();		

	bool append(T *request);		//��request���빤������
	
private:
	int m_thread_pool_number;	//�̳߳ص�����

	pthread_t *m_threads;			//�߳�����

	int m_max_request;

	std::list<T*> m_workqueue;   //��������

	mutex m_queuemutex;		//�̰߳�ȫ
	cond m_queuecond;
	sem m_queuesem;

	bool m_stop;
	
private:
	static void* worker(void *arg);
	void run();
};


template<typename T>
threadpool<T>::threadpool(int thread_number, int max_requests):
	m_thread_pool_number(thread_number), m_max_request(max_requests), 
	m_stop(NULL), m_threads(NULL){

	if((thread_number<=0) || (m_max_request<=0)){
		throw std::exception();
	}

	m_threads = new pthread_t[thread_number];
	if(!m_threads){
		throw std::exception();
	}
	
	for(int i=0; i<thread_number; ++i){
		 printf("create the %dth thread\n", i);
		if(pthread_create(m_threads + i, NULL, worker, this)!=0){
			delete [] m_threads;
			throw std::exception();
		}
		if(pthread_detach(m_threads[i])){
			throw std::exception();
		}
	}
}

template<typename T>
threadpool<T>::~threadpool(){
	delete [] m_threads;
	m_stop  = true;
}

template<typename T>
bool threadpool<T>::append(T* request){
	m_queuemutex.lock();
	if(m_workqueue.size() > m_max_request){
		m_queuemutex.unlock();
		return false;
	}
	m_workqueue.push_back(request);
	m_queuemutex.unlock();
	m_queuesem.post();
	return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg){
	threadpool * pool = (threadpool*)arg;

	pool->run();
	return pool;
}

template<typename T>
void threadpool<T>::run(){
	while(!m_stop){
		m_queuesem.wait();
		m_queuemutex.lock();
		if(m_workqueue.empty()){
			m_queuemutex.unlock();
			continue;
		}
		T* request = m_workqueue.front();
		m_workqueue.pop_back();
		m_queuemutex.unlock();
		if(!request){
			continue;
		}
		request->process();
	}
}

#endif
