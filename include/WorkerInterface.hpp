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


using namespace std;


class WorkerInterface{

	public:
		WorkerInterface()
		{
			pthread_mutex_init(&rQmutex, NULL);

			this->currentSig = 0;
			this->listenerDown = false;
			configSignals();

		};


		~WorkerInterface()
		{
			pthread_mutex_destroy(&rQmutex);
		};


	protected:

		//receivequeue
		list<string*> receiveQueue;
		pthread_mutex_t rQmutex;


		//signal variables
		struct sigaction action;
		sigset_t sigmask;
		int currentSig;

		bool listenerDown;

		static void dummy_handler(int){};



		void popReceiveQueue()
		{
			string* lastElement = NULL;
			pthread_mutex_lock(&rQmutex);
				lastElement = receiveQueue.back();
				delete lastElement;
				receiveQueue.pop_back();
			pthread_mutex_unlock(&rQmutex);
		}


		void pushReceiveQueue(string* data)
		{
			pthread_mutex_lock(&rQmutex);
				receiveQueue.push_front(data);
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
			sigfillset(&sigmask);
			pthread_sigmask(SIG_UNBLOCK, &sigmask, (sigset_t*)0);

			action.sa_flags = 0;
			action.sa_handler = dummy_handler;
			sigaction(SIGUSR1, &action, (struct sigaction*)0);
			sigaction(SIGUSR2, &action, (struct sigaction*)0);
			sigaction(SIGPOLL, &action, (struct sigaction*)0);
			sigaction(SIGPIPE, &action, (struct sigaction*)0);
		}


};



#endif /* WORKERINTERFACE_HPP_ */
