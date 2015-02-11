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
	this->listen_thread_active = false;
	this->recvSize = 0;
	this->lthread = 0;
	this->worker_thread_active = false;
	this->bufferOut = NULL;
	this->workerBusy = false;
	this->currentSig = 0;
	this->optionflag = 1;
	this->currentSocket = socket;
	this->udsClient = new UdsClient(this);
	memset(receiveBuffer, '\0', BUFFER_SIZE);


	pthread_mutex_init(&rQmutex, NULL);
	pthread_mutex_init(&wBusyMutex, NULL);
	configSignals();

	StartWorkerThread(currentSocket);
}




TcpWorker::~TcpWorker()
{
	pthread_mutex_destroy(&rQmutex);
	pthread_mutex_destroy(&wBusyMutex);
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




bool TcpWorker::workerStatus(int status)
{
	bool rValue = false;
	pthread_mutex_lock(&wBusyMutex);
	switch(status)
	{
		case WORKER_FREE:
			workerBusy = false;
			break;
		case WORKER_BUSY:
			workerBusy = true;
			break;
		case WORKER_GETSTATUS:
			rValue = workerBusy;
			break;
		default:
			break;
	}
	pthread_mutex_unlock(&wBusyMutex);
	return rValue;
}

int TcpWorker::tcp_send(string* data)
{
	send(currentSocket, data->c_str(), data->size(), 0);
}
