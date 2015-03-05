/*
 * UdsWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include <TcpWorker.hpp>

#include "UdsComClient.hpp"
#include "errno.h"

TcpWorker::TcpWorker(int socket)
{
	memset(receiveBuffer, '\0', BUFFER_SIZE);
	this->listen_thread_active = false;
	this->worker_thread_active = false;
	this->recvSize = 0;
	this->lthread = 0;
	this->bufferOut = NULL;
	this->jsonReturn = NULL;
	this->jsonInput = NULL;
	this->identity = NULL;
	this->currentSocket = socket;
	this->comClient = new UdsComClient(this);
	this->json = NULL;

	pthread_mutex_init(&rQmutex, NULL);

	configSignals();
	StartWorkerThread(currentSocket);
}




TcpWorker::~TcpWorker()
{
	listen_thread_active = false;
	worker_thread_active = false;
	pthread_kill(lthread, SIGUSR2);

	WaitForWorkerThreadToExit();

	pthread_mutex_destroy(&rQmutex);
}




void TcpWorker::thread_work(int socket)
{

	memset(receiveBuffer, '\0', BUFFER_SIZE);
	worker_thread_active = true;

	//start the listenerthread and remember the theadId of it
	lthread = StartListenerThread(pthread_self(), currentSocket, receiveBuffer);


	while(worker_thread_active)
	{
		//wait for signals from listenerthread

		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
				while(getReceiveQueueSize() > 0)
				{
					printf("%s\n", receiveQueue.back()->c_str());
					//parse to dom with jsonrpc

					//method ? namespace ? send to correct plugin via uds
					//sending will be done by a uds client

					//delete msg from queue
					comClient->sendData(receiveQueue.back());
					popReceiveQueue();
				}
				break;

			case SIGUSR2:
				printf("TcpComWorker: SIGUSR2\n");
				delete comClient;
				break;

			case SIGPIPE:
				printf("TcpComWorker: SIGPIPE\n");
				delete comClient;
				break;

			default:
				printf("TcpComWorker: unkown signal \n");
				delete comClient;
				break;
		}
	}
	close(currentSocket);
	printf("TCP Worker Thread beendet.\n");
	WaitForListenerThreadToExit();
	//mark this whole worker/listener object as deletable
	deletable = true;

}



void TcpWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{
	listen_thread_active = true;

	while(listen_thread_active)
	{
		memset(receiveBuffer, '\0', BUFFER_SIZE);

		recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, 0);
		if(recvSize > 0)
		{
			//add received data in buffer to queue
			pushReceiveQueue(new string(receiveBuffer, recvSize));

			//signal the worker
			pthread_kill(parent_th, SIGUSR1);
		}
		//no data, either udsComClient or plugin invoked a shutdown of this UdsComWorker
		else
		{
			//udsComClient invoked shutdown
			if(errno == EINTR)
			{
				pthread_kill(parent_th, SIGUSR2);
			}
			//plugin invoked shutdown
			else
			{
				worker_thread_active = false;
				listen_thread_active = false;
				pthread_kill(parent_th, SIGUSR2);
			}

			listenerDown = true;
		}

	}
	if(!listenerDown)
	{
		worker_thread_active = false;
		listen_thread_active = false;
		pthread_kill(parent_th, SIGUSR2);
	}
	printf("TCP Listener beendet.\n");
}



int TcpWorker::tcp_send(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}
