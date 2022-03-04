#ifndef _LOCKER_H
#define _LOCKER_H

#include <semaphore.h>
#include <exception>
#include <pthread.h>
#include <iostream>



class mutex{
public:
	mutex(){
		if(pthread_mutex_init(&m_mutex, NULL)!=0)
			throw std::exception();
	};

	~mutex(){
		pthread_mutex_destroy(&m_mutex);
	};

	bool lock(){
		return pthread_mutex_lock(&m_mutex);
	};

	bool unlock(){
		return pthread_mutex_unlock(&m_mutex);
	};

private:
	pthread_mutex_t m_mutex;
};

//条件变量
class cond{
public:
	cond(){
	    if(pthread_cond_init(&m_cond, NULL)!=0){
		    throw std::exception();
	    }
	}

	~cond(){
		pthread_cond_destroy(&m_cond);
	}

	bool wait(pthread_mutex_t *mutex){
		return pthread_cond_wait(&m_cond, mutex)==0;
	}

	bool timedwait(pthread_mutex_t *mutex, struct timespec abstime){
		return pthread_cond_timedwait(&m_cond, mutex, &abstime);
	}

	bool signal(){
		return pthread_cond_signal(&m_cond)==0;
	}

	bool broadcast(){
		return pthread_cond_broadcast(&m_cond)==0;
	}
	
private:
	pthread_cond_t m_cond;
};

//信号量
class sem{
public:
	sem(){
		if(sem_init(&m_sem, 0, 0)!=0){
			throw std::exception();
		}
	}
	
	~sem(){
		sem_destroy(&m_sem);
	}

	int wait(){
		return sem_wait(&m_sem)==0;
	}

	int post(){
		return sem_post(&m_sem)==0;
	}
	
	int timedwait(struct timespec *abs_timeout){
		return sem_timedwait(&m_sem, abs_timeout);
	}
	
private:	
	sem_t m_sem;
};

#endif






