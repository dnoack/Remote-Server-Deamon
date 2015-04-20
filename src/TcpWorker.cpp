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
#include "Utils.h"


TcpWorker::TcpWorker(ConnectionContext* context, TcpWorker** tcpWorker, int socket)
{
	this->context = context;
	this->currentSocket = socket;
	*tcpWorker = this;

	StartWorkerThread();

	if(wait_for_listener_up() != 0)
		throw PluginError("Creation of Listener/worker threads failed.");
}


TcpWorker::~TcpWorker()
{
	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
}



void TcpWorker::thread_work()
{
	RsdMsg* msg = NULL;
	string* errorResponse = NULL;
	RsdMsg* errorMsg = NULL;
	worker_thread_active = true;

	memset(receiveBuffer, '\0', BUFFER_SIZE);


	//start the listenerthread and remember the theadId of it
	StartListenerThread();

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
							dyn_print("Tcp Queue Received: %s\n", msg->getContent()->c_str());
							popReceiveQueueWithoutDelete();
							context->processMsg(msg);
						}
						catch(PluginError &e)
						{
							errorResponse = new string(e.get());
							errorMsg = new RsdMsg(0, errorResponse);
							transmit(errorMsg);
							delete errorMsg;
						}
						catch(...)
						{
							dyn_print("Unkown Exception.\n");
						}
					}
				}
				break;

			case SIGUSR2:
				dyn_print("TcpComWorker: SIGUSR2\n");
				context->checkUdsConnections();
				break;

			default:
				dyn_print("TcpComWorker: unkown signal \n");
				break;
		}
	}

	close(currentSocket);

}


void TcpWorker::thread_listen()
{
	listen_thread_active = true;
	string* content = NULL;
	int retval;
	fd_set rfds;
	pthread_t worker_thread = getWorker();
	configSignals();

	FD_ZERO(&rfds);
	FD_SET(currentSocket, &rfds);

	while(listen_thread_active)
	{
		memset(receiveBuffer, '\0', BUFFER_SIZE);

		retval = pselect(currentSocket+1, &rfds, NULL, NULL, NULL, &origmask);

		if(retval < 0 )
		{
			//error occured
			context->checkUdsConnections();
		}
		else if(FD_ISSET(currentSocket, &rfds))
		{
			recvSize = recv( currentSocket , receiveBuffer, BUFFER_SIZE, 0);

			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				dyn_print("TcpListener: %s \n", content->c_str());
				pushReceiveQueue(new RsdMsg(0, content));
				//signal the worker
				pthread_kill(worker_thread, SIGUSR1);
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


int TcpWorker::transmit(char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int TcpWorker::transmit(const char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int TcpWorker::transmit(RsdMsg* msg)
{
	return send(currentSocket, msg->getContent()->c_str(), msg->getContent()->size(), 0);
}

