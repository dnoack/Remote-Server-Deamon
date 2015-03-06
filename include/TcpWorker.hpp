/*
 * TCPWorker.hpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_TCPWORKER_HPP_
#define INCLUDE_TCPWORKER_HPP_

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
#include "WorkerInterface.hpp"
#include <WorkerThreads.hpp>
#include "signal.h"


class UdsComClient;


#define BUFFER_SIZE 1024
#define ADD_WORKER true
#define DELETE_WORKER false
#define WORKER_FREE 0
#define WORKER_BUSY 1
#define WORKER_GETSTATUS 2


class TcpWorker : public WorkerInterface, WorkerThreads{

	public:
		TcpWorker(int socket);
		~TcpWorker();

		int tcp_send(string* data);
		void checkComClientList();
		pthread_t getLthread(){return this->lthread;}


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
		JsonRPC* json;
		vector<UdsComClient*> comClientList;
		int currentSocket;

		void deleteComClientList();


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);


};



#endif /* INCLUDE_UDSWORKER_HPP_ */
