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


#define WORKER_FREE 0
#define WORKER_BUSY 1
#define WORKER_GETSTATUS 2


using namespace std;


class WorkerInterface{

	public:
		WorkerInterface()
		{
			pthread_mutex_init(&rQmutex, NULL);
			pthread_mutex_init(&wBusyMutex, NULL);

			this->currentSig = 0;
			this->workerBusy = false;

			configSignals();
		};


		~WorkerInterface()
		{
			pthread_mutex_destroy(&rQmutex);
			pthread_mutex_destroy(&wBusyMutex);
		};


	protected:

		//receivequeue
		list<string*> receiveQueue;
		pthread_mutex_t rQmutex;


		//signal variables
		struct sigaction action;
		sigset_t sigmask;
		int currentSig;


		//shared worker busy flag
		pthread_mutex_t wBusyMutex;
		bool workerBusy;


		static void dummy_handler(int){};



		//data + add = new msg , data = NULL + add=false = remove oldest
		void editReceiveQueue(string* data, bool add)
		{
			pthread_mutex_lock(&rQmutex);
			if(data != NULL)
			{

				if(add)
				{
					receiveQueue.push_front(data);
				}
			}
			else
			{
				if(!add)
				{
					receiveQueue.pop_back();
				}
			}
			pthread_mutex_unlock(&rQmutex);
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


		bool workerStatus(int status)
		{
			bool rValue = false;
			pthread_mutex_lock(&wBusyMutex);
			switch(status)
			{
				case WORKER_FREE:
					workerBusy = false;
					break;
				case WORKER_BUSY:
					workerBusy = true;
					break;
				case WORKER_GETSTATUS:
					rValue = workerBusy;
					break;
				default:
					break;
			}
			pthread_mutex_unlock(&wBusyMutex);
			return rValue;
		}



};



#endif /* WORKERINTERFACE_HPP_ */
