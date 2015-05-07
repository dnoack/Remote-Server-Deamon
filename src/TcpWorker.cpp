
#include "TcpWorker.hpp"
#include "ConnectionContext.hpp"
#include "UdsComClient.hpp"



TcpWorker::TcpWorker(ConnectionContext* context, TcpWorker** tcpWorker, int socket)
{
	this->context = context;
	this->currentSocket = socket;
	this->logInfoIn.logLevel = LOG_INPUT;
	this->logInfoIn.logName = "TCP IN:";
	this->logInfoOut.logLevel = LOG_OUTPUT;
	this->logInfoOut.logName = "TCP OUT:";
	this->logInfo.logName = "TcpWorker:";

	*tcpWorker = this;

	StartWorkerThread();

	if(wait_for_listener_up() != 0)
		throw PluginError("Creation of Listener/worker threads failed.");
	else
		dlog(logInfo, "Created TcpWorker for context %d with socket %d.", context->getContextNumber(), currentSocket);
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
	configSignals();
	StartListenerThread();



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
							log(logInfoIn, msg->getContent());
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
							log(logInfo, "Unkown Exception.");
						}
					}
				}
			break;

			default:
				log(logInfo, "TcpComWorker: unkown signal");
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
	//configSignals();

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
	log(logInfoOut, data);
	return send(currentSocket, data, size, 0);
}


int TcpWorker::transmit(const char* data, int size)
{

	log(logInfoOut, data);
	return send(currentSocket, data, size, 0);
}


int TcpWorker::transmit(RsdMsg* msg)
{
	log(logInfoOut, msg->getContent());
	return send(currentSocket, msg->getContent()->c_str(), msg->getContent()->size(), 0);
}

