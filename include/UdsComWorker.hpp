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


#include "JsonRPC.hpp"
#include <pthread.h>
#include "signal.h"

#include "TcpWorker.hpp"
#include "WorkerInterface.hpp"
#include "WorkerThreads.hpp"

#define BUFFER_SIZE 1024
#define ADD_WORKER true
#define DELETE_WORKER false
#define WORKER_FREE 0
#define WORKER_BUSY 1
#define WORKER_GETSTATUS 2


class UdsComWorker : public WorkerInterface, WorkerThreads{

	public:
		UdsComWorker(int socket, TcpWorker* tcpWorker);
		~UdsComWorker();


	private:


		//variables for listener
		bool listen_thread_active;
		char receiveBuffer[BUFFER_SIZE];
		int recvSize;


		//variables for worker
		bool worker_thread_active;
		char* bufferOut;
		string* jsonInput;
		string* identity;
		string* jsonReturn;



		//not shared, more common
		pthread_t lthread;
		int currentSocket;
		TcpWorker* tcpWorker;


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);


};



#endif /* INCLUDE_UDSWORKER_HPP_ */
