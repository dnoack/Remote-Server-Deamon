/*
 * WorkerInterface.hpp
 *
 *  Created on: 25.02.2015
 *      Author: Dave
 */

#ifndef WORKERINTERFACE_HPP_
#define WORKERINTERFACE_HPP_

#include <list>
#include <string>
#include <pthread.h>
#include "signal.h"


#define BUFFER_SIZE 1024


using namespace std;


template <class TMsg>
class WorkerInterface{

	public:

		WorkerInterface()
		{
			pthread_mutex_init(&rQmutex, NULL);

			this->currentSig = 0;
			this->ready = false;
			this->deletable = false;
			this->recvSize = 0;
			memset(receiveBuffer, '\0', BUFFER_SIZE);
		};


		~WorkerInterface()
		{
			pthread_mutex_destroy(&rQmutex);
		};

		bool isDeletable(){return deletable;}


	protected:

		//receivequeue
		list<TMsg*> receiveQueue;
		pthread_mutex_t rQmutex;

		char receiveBuffer[BUFFER_SIZE];
		int recvSize;


		//signal variables
		struct sigaction action;
		sigset_t sigmask;
		sigset_t origmask;
		int currentSig;

		bool ready;
		bool deletable;

		static void dummy_handler(int){};

		void deleteReceiveQueue()
		{
			typename list<TMsg*>::iterator i;
			i = receiveQueue.begin();
			while(i != receiveQueue.end())
			{
				delete *i;
				i = receiveQueue.erase(i);
			}
		}

		void popReceiveQueue()
		{
			TMsg* lastElement = NULL;
			pthread_mutex_lock(&rQmutex);
				lastElement = receiveQueue.back();
				receiveQueue.pop_back();
				delete lastElement;
			pthread_mutex_unlock(&rQmutex);
		}



		void popReceiveQueueWithoutDelete()
		{
			pthread_mutex_lock(&rQmutex);
				receiveQueue.pop_back();
			pthread_mutex_unlock(&rQmutex);
		}


		void pushReceiveQueue(TMsg* data)
		{
			pthread_mutex_lock(&rQmutex);
				receiveQueue.push_back(data);
			pthread_mutex_unlock(&rQmutex);
		}


		int getReceiveQueueSize()
		{
			int result = 0;
			pthread_mutex_lock(&rQmutex);
				result = receiveQueue.size();
			pthread_mutex_unlock(&rQmutex);

			return result;
		}


		void configSignals()
		{
			sigemptyset(&origmask);
			sigemptyset(&sigmask);
			sigaddset(&sigmask, SIGUSR1);
			sigaddset(&sigmask, SIGUSR2);

			action.sa_handler = dummy_handler;
			sigemptyset(&action.sa_mask);
			action.sa_flags = 0;

			sigaction(SIGUSR1, &action, NULL);
			sigaction(SIGUSR2, &action, NULL);

			pthread_sigmask(SIG_BLOCK, &sigmask, &origmask);
		}


};



#endif /* WORKERINTERFACE_HPP_ */
