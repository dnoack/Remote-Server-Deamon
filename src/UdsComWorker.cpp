/*
 * UdsComWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include "UdsComClient.hpp"
#include "UdsComWorker.hpp"
#include "errno.h"


UdsComWorker::UdsComWorker(int socket,  UdsComClient* comClient)
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
	this->comClient = comClient;
	this->currentSocket = socket;
	this->deletable = false;

	StartWorkerThread(currentSocket);
}




UdsComWorker::~UdsComWorker()
{
	worker_thread_active = false;
	listen_thread_active = false;

	close(currentSocket);

	if(pthread_cancel(getListener()) != 0)
		printf("error canceling udsCOMlistener.\n");
	if(pthread_cancel(getWorker()) != 0)
		printf("error canceling udsCOMworker.\n");

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
}



void UdsComWorker::thread_work(int socket)
{

	memset(receiveBuffer, '\0', BUFFER_SIZE);
	worker_thread_active = true;

	pthread_cleanup_push(&UdsComWorker::cleanupWorker, NULL);


	//start the listenerthread and remember the theadId of it
	lthread = StartListenerThread(pthread_self(), currentSocket, receiveBuffer);

	configWorkerSignals();

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
					comClient->tcp_send(receiveQueue.back());
					popReceiveQueue();
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
	pthread_cleanup_pop(NULL);
}




void UdsComWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{

	listen_thread_active = true;
	int retval;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);

	while(listen_thread_active)
	{

		memset(receiveBuffer, '\0', BUFFER_SIZE);

		retval = select(socket+1, &rfds, NULL, NULL, NULL);

		if(retval < 0)
		{

			if(errno == EINTR)
			{
				//Plugin itself invoked shutdown
				worker_thread_active = false;
				listen_thread_active = false;
				pthread_kill(parent_th, SIGUSR2);
			}
			else
			{
				printf("unkown errno\n");
				worker_thread_active = false;
				listen_thread_active = false;
				pthread_kill(parent_th, SIGUSR2);
			}
		}
		else if(FD_ISSET(socket, &rfds))
		{
			recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, MSG_DONTWAIT);

			//data received
			if(recvSize > 0)
			{
				//add received data in buffer to queue
				pushReceiveQueue(new string(receiveBuffer, recvSize));
				//signal the worker
				pthread_kill(parent_th, SIGUSR1);
			}
			else
			{
				listen_thread_active = false;
				deletable = true;
				comClient->markAsDeletable();
			}
		}

	}

	printf("Uds Listener beendet.\n");
}
