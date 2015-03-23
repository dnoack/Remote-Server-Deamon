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


#include <pthread.h>
#include "signal.h"

#include "JsonRPC.hpp"
#include "WorkerInterface.hpp"
#include <WorkerThreads.hpp>


class ConnectionContext;
class UdsComClient;



#define BUFFER_SIZE 1024
#define ADD_WORKER true
#define DELETE_WORKER false
#define WORKER_FREE 0
#define WORKER_BUSY 1
#define WORKER_GETSTATUS 2





class TcpWorker : public WorkerInterface, public WorkerThreads{

	public:
		TcpWorker(ConnectionContext* context, int socket);
		~TcpWorker();

		void routeBack(RsdMsg* data);
		void checkComClientList();

		int tcp_send(char* data, int size);


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
		list<UdsComClient*> comClientList;
		int currentSocket;
		ConnectionContext* context;


		void handleMsg(RsdMsg* request);
		char* getMethodNamespace();


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);

		static void cleanupWorker(void* arg)
		{
			TcpWorker* worker = static_cast<TcpWorker*>(arg);
			//TODO::worker->deleteComClientList();
		};



};



#endif /* INCLUDE_UDSWORKER_HPP_ */
