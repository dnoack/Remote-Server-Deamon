

#include "UdsComWorker.hpp"
#include "ConnectionContext.hpp"




UdsComWorker::UdsComWorker(ConnectionContext* context, string* udsFilePath, string* pluginName, int pluginNumber)
{
	this->optionflag = 1;
	this->deletable = false;
	this->jsonReturn = NULL;
	this->jsonInput = NULL;
	this->identity = NULL;
	this->logInfoIn.logLevel = LOG_INPUT;
	this->logInfoIn.logName = "UDS IN:";
	this->logInfo.logName = "UdsComWorker:";
	this->logInfoOut.logLevel = LOG_OUTPUT;
	this->logInfoOut.logName = "UDS OUT:";
	this->logInfo.logName = "UdsComWorker:";
	this->pluginNumber = pluginNumber;
	this->context = context;
	this->udsFilePath = new string(*udsFilePath);
	this->pluginName = new string(*pluginName);

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	memset(address.sun_path, '\0', sizeof(address.sun_path));
	strncpy(address.sun_path, udsFilePath->c_str(), udsFilePath->size());
	addrlen = sizeof(address);

}




UdsComWorker::~UdsComWorker()
{
	close(currentSocket);

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();

	delete udsFilePath;
	delete pluginName;

	deleteReceiveQueue();
}



void UdsComWorker::thread_work()
{
	RsdMsg* msg = NULL;
	worker_thread_active = true;

	configSignals();
	StartListenerThread();


	while(worker_thread_active)
	{
		//wait for signals from listenerthread
		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
					msg = receiveQueue.back();
					log(logInfoIn, msg->getContent());
					popReceiveQueueWithoutDelete();
					routeBack(msg);
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




void UdsComWorker::thread_listen()
{
	listen_thread_active = true;
	int retval = 0;
	string* content = NULL;
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
			worker_thread_active = false;
			listen_thread_active = false;
			pthread_kill(worker_thread, SIGUSR2);
		}
		else if(FD_ISSET(currentSocket, &rfds))
		{
			recvSize = recv(currentSocket , receiveBuffer, BUFFER_SIZE, MSG_DONTWAIT);

			//data received
			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				pushReceiveQueue(new RsdMsg(pluginNumber, content));

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


void UdsComWorker::routeBack(RsdMsg* msg)
{
	try
	{
		context->processMsg(msg);
	}
	catch(PluginError &e)
	{
		/*this can happen if a plugin answers with a incorret msg.
		Server will then get a parsing error and throw a PluginError*/
		context->handleIncorrectPluginResponse(msg, e);
	}
}


void UdsComWorker::tryToconnect()
{
	if( connect(currentSocket, (struct sockaddr*)&address, addrlen) < 0)
	{
		dlog(logInfoOut, "Plugin not found, errno: %s ", strerror(errno));
		throw PluginError("Could not connect to plugin.");
	}
	else
	{
		StartWorkerThread();

		if(wait_for_listener_up() != 0)
			throw PluginError("Creation of Listener/worker threads failed.");
	}
}


int UdsComWorker::transmit(char* data, int size)
{
	log(logInfoOut, data);
	return send(currentSocket, data, size, 0);
}


int UdsComWorker::transmit(const char* data, int size)
{

	log(logInfoOut, data);
	return send(currentSocket, data, size, 0);
}


int UdsComWorker::transmit(RsdMsg* msg)
{
	log(logInfoOut, msg->getContent());
	return send(currentSocket, msg->getContent()->c_str(), msg->getContent()->size(), 0);
}

