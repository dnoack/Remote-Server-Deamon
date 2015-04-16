/*
 * TCPWorker.hpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#ifndef INCLUDE_TCPWORKER_HPP_
#define INCLUDE_TCPWORKER_HPP_

//unix domain socket definition
#include "errno.h"
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <list>


#include <pthread.h>
#include "signal.h"

#include "WorkerInterface.hpp"
#include <WorkerThreads.hpp>
#include "RsdMsg.h"


class ConnectionContext;
class UdsComClient;



class TcpWorker : public WorkerInterface<RsdMsg>, public WorkerThreads{

	public:
		TcpWorker(ConnectionContext* context, TcpWorker** tcpWorker, int socket);
		~TcpWorker();


		int tcp_send(char* data, int size);
		int tcp_send(const char* data, int size);
		int tcp_send(RsdMsg* data);

	private:
		ConnectionContext* context;
		char* bufferOut;
		int currentSocket;
		int sentBytes;

		void helpme(int sentBytes)
		{
			if(sentBytes < 0)
				printf("TCP send fail: %s\n", strerror(errno));
			else
				printf("Sent %d bytes to Client.", sentBytes);
		}


		virtual void thread_listen(pthread_t partent_th, int socket, char* workerBuffer);

		virtual void thread_work(int socket);


};



#endif /* INCLUDE_UDSWORKER_HPP_ */
