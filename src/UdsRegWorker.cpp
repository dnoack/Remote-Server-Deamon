
#include "RSD.hpp"
#include "UdsRegWorker.hpp"

using namespace rapidjson;


UdsRegWorker::UdsRegWorker(int socket)
{
	this->currentSocket = socket;
	this->logInfo.logName = "UdsRegWorker: ";
	this->logInfoIn.logName = "REG IN: ";
	this->logInfoOut.logName = "REG OUT: ";
	this->registration = new Registration(this);

	StartWorkerThread();

	if(wait_for_listener_up() != 0)
			throw Error(-400, "Timeout for Thread creation");
}


UdsRegWorker::~UdsRegWorker()
{

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();

	delete registration;
	deleteReceiveQueue();
}


string* UdsRegWorker::getPluginName()
{
	return registration->getPluginName();
}


void UdsRegWorker::thread_work()
{
	worker_thread_active = true;
	RsdMsg* msg = NULL;

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
					try
					{
						msg = receiveQueue.back();
						log(logInfoIn, msg->getContent());
						popReceiveQueueWithoutDelete();
						registration->processMsg(msg);
					}
					catch(...)
					{
						dyn_print("Unkown Exception.\n");
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
		retval = select(currentSocket+1, &rfds, NULL, NULL, NULL);

		if(retval < 0)
		{
			//Something is wrong with our socket, maybe plugin closed the connection
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
				listen_thread_active = false;
				deletable = true;
			}
		}

		memset(receiveBuffer, '\0', BUFFER_SIZE);
	}
}



int UdsRegWorker::transmit(const char* data, int size)
{
	log(logInfoOut, data);
	return send(currentSocket, data, size, 0);
}


int UdsRegWorker::transmit(RsdMsg* msg)
{
	log(logInfoOut, msg->getContent());
	return send(currentSocket, msg->getContent()->c_str(), msg->getContent()->size(), 0);
}


