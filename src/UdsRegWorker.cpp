
#include "errno.h"

#include "RSD.hpp"
#include "UdsRegWorker.hpp"
#include "Registration.hpp"
#include "Plugin_Error.h"
#include "RsdMsg.h"
#include "Utils.h"


using namespace rapidjson;



UdsRegWorker::UdsRegWorker(int socket)
{
	this->msg = NULL;
	this->currentSocket = socket;
	this->registration = new Registration(this);
	this->errorResponse = NULL;

	StartWorkerThread();

	if(wait_for_listener_up() != 0)
			throw PluginError("Creation of Listener/worker threads failed.");
}



UdsRegWorker::~UdsRegWorker()
{

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();

	delete registration;
	cleanupReceiveQueue(this);
}


string* UdsRegWorker::getPluginName()
{
	return registration->getPluginName();
}



void UdsRegWorker::thread_work()
{
	worker_thread_active = true;

	configSignals();
	StartListenerThread();


	while(worker_thread_active)
	{
		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
				while(getReceiveQueueSize() > 0)
				{
					if(!registration->isRequestInProcess())
					{
						try
						{
							msg = receiveQueue.back();
							dyn_print("RegWorker Received: %s\n", msg->getContent()->c_str());
							popReceiveQueueWithoutDelete();
							registration->processMsg(msg);
						}
						catch(...)
						{
							dyn_print("Unkown Exception.\n");
						}
					}
				}
			break;

			default:
				dyn_print("UdsRegWorker: unkown signal \n");
				break;
		}
	}
	close(currentSocket);
}



void UdsRegWorker::thread_listen()
{

	listen_thread_active = true;
	string* content = NULL;
	int retval;
	fd_set rfds;

	pthread_t worker_thread = getWorker();

	FD_ZERO(&rfds);
	FD_SET(currentSocket, &rfds);

	while(listen_thread_active)
	{
		retval = pselect(currentSocket+1, &rfds, NULL, NULL, NULL, &origmask);

		if(retval < 0)
		{
			//Plugin itself invoked shutdown
			listen_thread_active = false;
			deletable = true;
		}
		else if(FD_ISSET(currentSocket, &rfds))
		{
			recvSize = recv(currentSocket , receiveBuffer, BUFFER_SIZE, 0);
			//data received
			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				pushReceiveQueue(new RsdMsg(0, content));
				//signal the worker
				pthread_kill(worker_thread, SIGUSR1);
			}
			else
			{
				deletable = true;
				listen_thread_active = false;
			}
		}

		memset(receiveBuffer, '\0', BUFFER_SIZE);
	}
}




void UdsRegWorker::cleanupReceiveQueue(void* arg)
{
	UdsRegWorker* worker = static_cast<UdsRegWorker*>(arg);
	worker->deleteReceiveQueue();
}


int UdsRegWorker::transmit(char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int UdsRegWorker::transmit(const char* data, int size)
{
	return send(currentSocket, data, size, 0);
}


int UdsRegWorker::transmit(RsdMsg* msg)
{
	string* data = msg->getContent();
	return send(currentSocket, data->c_str(), data->size(), 0);
}


