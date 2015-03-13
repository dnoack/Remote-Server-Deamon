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
			this->deletable = false;
			configSignals();

		};


		~WorkerInterface()
		{
			pthread_mutex_destroy(&rQmutex);
		};

		bool isDeletable(){return deletable;}


	protected:

		//receivequeue
		list<string*> receiveQueue;
		pthread_mutex_t rQmutex;


		//signal variables
		struct sigaction action;
		sigset_t sigmask;
		sigset_t origmask;
		int currentSig;

		bool listenerDown;
		bool deletable;

		static void dummy_handler(int){};



		void popReceiveQueue()
		{
			string* lastElement = NULL;
			pthread_mutex_lock(&rQmutex);
				lastElement = receiveQueue.back();
				receiveQueue.pop_back();
				delete lastElement;
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
			sigemptyset(&origmask);
			sigemptyset(&sigmask);

			action.sa_handler = dummy_handler;
			sigemptyset(&action.sa_mask);
			action.sa_flags = 0;

			sigaction(SIGUSR1, &action, NULL);
			sigaction(SIGUSR2, &action, NULL);
			sigaction(SIGPOLL, &action, NULL);
			sigaction(SIGPIPE, &action, NULL);

			pthread_sigmask(SIG_BLOCK, &sigmask, &origmask);
		}


		void configWorkerSignals()
		{
			sigfillset(&sigmask);

			action.sa_handler = dummy_handler;
			action.sa_flags = 0;

			sigaction(SIGUSR1, &action, NULL);
			sigaction(SIGUSR2, &action, NULL);
			sigaction(SIGPOLL, &action, NULL);
			sigaction(SIGPIPE, &action, NULL);

			pthread_sigmask(SIG_UNBLOCK, &sigmask, NULL);
		}



};



#endif /* WORKERINTERFACE_HPP_ */
