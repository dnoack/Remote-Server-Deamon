/*
 * UdsComWorker.hpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_UDSCOMWORKER_HPP_
#define INCLUDE_UDSCOMWORKER_HPP_

//unix domain socket definition
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>
#include <pthread.h>
#include "signal.h"


#include "JsonRPC.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"


using namespace std;


class UdsComClient;


class UdsComWorker : public WorkerInterface, WorkerThreads{

	public:
		UdsComWorker(int socket, UdsComClient* comClient);
		~UdsComWorker();

		static void cleanupReceiveQueue(void* arg);

	private:

		//variables for worker
		char* bufferOut;
		string* jsonInput;
		string* identity;
		string* jsonReturn;



		//not shared, more common
		int currentSocket;
		UdsComClient* comClient;
		bool deletable;


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);


};



#endif /* INCLUDE_UDSWORKER_HPP_ */
