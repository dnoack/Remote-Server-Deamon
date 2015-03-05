/*
 * UdsComWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include "UdsComWorker.hpp"
#include "errno.h"


UdsComWorker::UdsComWorker(int socket, TcpWorker* tcpworker)
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
	this->tcpWorker = tcpworker;
	this->currentSocket = socket;

	StartWorkerThread(currentSocket);
}




UdsComWorker::~UdsComWorker()
{
	printf("tryn to shutdown udsComworker\n");
	worker_thread_active = false;
	listen_thread_active = false;
	pthread_kill(lthread, SIGUSR2);

	WaitForWorkerThreadToExit();
}



void UdsComWorker::thread_work(int socket)
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
					//remove data from queue
					tcpWorker->tcp_send(receiveQueue.back());
					editReceiveQueue(NULL, false);
				}
				break;

			case SIGUSR2:
				printf("UdsComWorker: SIGUSR2\n");
				break;

			case SIGPIPE:
				printf("UdsComWorker: SIGPIPE\n");
				break;

			default:
				printf("UdsComWorker: unkown signal \n");
				break;
		}

	}
	close(currentSocket);
	printf("Uds Worker Thread beendet.\n");
	WaitForListenerThreadToExit();
	//TODO: tell tcpWorker to delete his udsComClient instance
}



void UdsComWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{
	listen_thread_active = true;

	while(listen_thread_active)
	{
		memset(receiveBuffer, '\0', BUFFER_SIZE);

		recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, 0);
		//data received
		if(recvSize > 0)
		{
			//add received data in buffer to queue
			editReceiveQueue(new string(receiveBuffer, recvSize), true);
			//signal the worker
			pthread_kill(parent_th, SIGUSR1);
		}
		//no data, either udsComClient or plugin invoked a shutdown of this UdsComWorker
		else
		{
			//udsComClient invoked shutdown
			if(errno == EINTR)
			{
				printf("UdsComListener: SIGUSR2\n");
				pthread_kill(parent_th, SIGUSR2);
			}
			//plugin invoked shutdown
			else
			{

				printf("UdsComListener: ???\n");
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
	printf("Uds Listener beendet.\n");
}
