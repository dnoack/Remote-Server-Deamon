/*
 * UdsWorker.cpp
 *
 *  Created on: 	09.02.2015
 *  Author: 		dnoack
 */

#include "errno.h"

#include <TcpWorker.hpp>
#include "ConnectionContext.hpp"
#include "UdsComClient.hpp"
#include "Plugin_Error.h"


TcpWorker::TcpWorker(ConnectionContext* context, int socket)
{
	memset(receiveBuffer, '\0', BUFFER_SIZE);
	this->listen_thread_active = false;
	this->worker_thread_active = false;
	this->recvSize = 0;
	this->lthread = 0;
	this->bufferOut = NULL;
	this->context = context;
	this->currentSocket = socket;

	StartWorkerThread(currentSocket);

	if(wait_for_listener_up() != 0)
		throw PluginError("Creation of Listener/worker threads failed.");
}


TcpWorker::~TcpWorker()
{
	listen_thread_active = false;
	worker_thread_active = false;

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
}



void TcpWorker::thread_work(int socket)
{
	RsdMsg* msg = NULL;
	string* errorResponse = NULL;
	RsdMsg* errorMsg = NULL;
	worker_thread_active = true;

	pthread_cleanup_push(&TcpWorker::cleanupWorker, this);
	memset(receiveBuffer, '\0', BUFFER_SIZE);


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
				while(getReceiveQueueSize() > 0)
				{
					//Another request is still in progress, wait...
					if(!context->isRequestInProcess())
					{
						try
						{
							msg = receiveQueue.back();
							printf("Tcp Queue Received: %s\n", msg->getContent()->c_str());
							context->processMsg(msg);
						}
						catch(PluginError &e)
						{
							errorResponse = new string(e.get());
							errorMsg = new RsdMsg(0, errorResponse);
							tcp_send(errorMsg);
							delete errorMsg;
						}
						catch(...)
						{
							printf("Unkown Exception.\n");
						}
						popReceiveQueueWithoutDelete();
					}
				}
				break;

			case SIGUSR2:
				printf("TcpComWorker: SIGUSR2\n");
				context->checkUdsConnections();
				break;

			default:
				printf("TcpComWorker: unkown signal \n");
				break;
		}
	}

	close(currentSocket);
	pthread_cleanup_pop(0);

}


void TcpWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{
	listen_thread_active = true;
	string* content = NULL;
	int retval;
	fd_set rfds;

	configSignals();

	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);

	while(listen_thread_active)
	{
		memset(receiveBuffer, '\0', BUFFER_SIZE);

		retval = pselect(socket+1, &rfds, NULL, NULL, NULL, &origmask);

		if(retval < 0 )
		{
			//error occured
			context->checkUdsConnections();
		}
		else if(FD_ISSET(socket, &rfds))
		{
			recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, MSG_DONTWAIT);

			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				pushReceiveQueue(new RsdMsg(0, content));
				//signal the worker
				pthread_kill(parent_th, SIGUSR1);
			}
			//connection closed
			else
			{
				deletable = true;
				listen_thread_active = false;
			}
		}
	}

}


int TcpWorker::tcp_send(char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int TcpWorker::tcp_send(const char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int TcpWorker::tcp_send(RsdMsg* msg)
{
	string* data = msg->getContent();
	return send(currentSocket, data->c_str(), data->size(), 0);
}


