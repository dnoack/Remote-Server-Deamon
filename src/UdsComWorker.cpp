/*
 * UdsComWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */


#include "UdsComClient.hpp"
#include "UdsComWorker.hpp"
#include "errno.h"
#include "Utils.h"


UdsComWorker::UdsComWorker(int socket,  UdsComClient* comClient)
{

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

	close(currentSocket);

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();

	deleteReceiveQueue();

}



void UdsComWorker::thread_work(int socket)
{
	RsdMsg* msg = NULL;
	worker_thread_active = true;

	//start the listenerthread and remember the theadId of it
	StartListenerThread(pthread_self(), currentSocket, receiveBuffer);

	configSignals();

	while(worker_thread_active)
	{
		//wait for signals from listenerthread
		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
					msg = receiveQueue.back();
					dyn_print("Routeback: %s\n", msg->getContent()->c_str());
					popReceiveQueueWithoutDelete();
					comClient->routeBack(msg);
				break;

			case SIGUSR2:
				dyn_print("UdsComWorker: SIGUSR2\n");
				break;

			default:
				dyn_print("UdsComWorker: unkown signal \n");
				break;
		}
	}
	close(currentSocket);
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
				dyn_print("UdsCom/Listener: %s\n",content->c_str());
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
		memset(receiveBuffer, '\0', BUFFER_SIZE);
	}
}

