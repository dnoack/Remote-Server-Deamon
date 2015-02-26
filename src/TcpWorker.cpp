/*
 * UdsWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include <TcpWorker.hpp>

#include "UdsClient.hpp"

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
	this->udsClient = new UdsClient(this);
	this->json = NULL;

	pthread_mutex_init(&rQmutex, NULL);
	pthread_mutex_init(&wBusyMutex, NULL);

	configSignals();
	StartWorkerThread(currentSocket);
}




TcpWorker::~TcpWorker()
{
	pthread_mutex_destroy(&rQmutex);
	pthread_mutex_destroy(&wBusyMutex);

	delete udsClient;
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
				while(receiveQueue.size() > 0)
				{
					printf("We received something and worker received a signal\n");
					//parse to dom with jsonrpc

					//method ? namespace ? send to correct plugin via uds
					//sending will be done by a uds client

					//delete msg from queue
					udsClient->sendData(receiveQueue.back());
					editReceiveQueue(NULL, false);
				}
				break;

			case SIGUSR2:
				//sigusr2 = time to exit
				worker_thread_active = false;
				listen_thread_active = false;
				break;

			case SIGPIPE:
				worker_thread_active = false;
				listen_thread_active = false;
				break;
			default:
				worker_thread_active = false;
				listen_thread_active = false;
				break;
		}
		workerStatus(WORKER_FREE);
	}
	close(currentSocket);
	printf("Worker Thread beendet.\n");
	WaitForListenerThreadToExit();


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
			editReceiveQueue(new string(receiveBuffer, recvSize), true);

			//worker is doing nothing, wake him up
			if(!workerStatus(WORKER_GETSTATUS))
			{
				workerStatus(WORKER_BUSY);
				//signal the worker
				pthread_kill(parent_th, SIGUSR1);
			}
		}
		else
			pthread_kill(parent_th, SIGPOLL);
	}
	printf("Listener beendet.\n");

}



int TcpWorker::tcp_send(string* data)
{
	send(currentSocket, data->c_str(), data->size(), 0);
}
