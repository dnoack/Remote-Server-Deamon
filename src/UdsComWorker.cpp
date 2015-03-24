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

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
}



void UdsComWorker::thread_work(int socket)
{

	worker_thread_active = true;
	memset(receiveBuffer, '\0', BUFFER_SIZE);
	pthread_cleanup_push(&UdsComWorker::cleanupWorker, NULL);


	//start the listenerthread and remember the theadId of it
	lthread = StartListenerThread(pthread_self(), currentSocket, receiveBuffer);

	configSignals();

	while(worker_thread_active)
	{
		//wait for signals from listenerthread

		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
					printf("Routeback: %s\n", receiveQueue.back()->getContent()->c_str());
					comClient->routeBack(receiveQueue.back());
					popReceiveQueueWithoutDelete();

				break;

			case SIGUSR2:
				printf("UdsComWorker: SIGUSR2\n");
				break;

			default:
				printf("UdsComWorker: unkown signal \n");
				break;
		}

	}
	close(currentSocket);
	pthread_cleanup_pop(NULL);
}




void UdsComWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{
	listen_thread_active = true;
	int retval = 0;
	string* content = NULL;
	fd_set rfds;

	configSignals();

	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);

	while(listen_thread_active)
	{

		memset(receiveBuffer, '\0', BUFFER_SIZE);

		retval = pselect(socket+1, &rfds, NULL, NULL, NULL, &origmask);

		if(retval < 0)
		{
			//Plugin itself invoked shutdown
			worker_thread_active = false;
			listen_thread_active = false;
			pthread_kill(parent_th, SIGUSR2);
		}
		else if(FD_ISSET(socket, &rfds))
		{
			recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, MSG_DONTWAIT);

			//data received
			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				pushReceiveQueue(new RsdMsg(comClient->getPluginNumber(), content));

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
}
