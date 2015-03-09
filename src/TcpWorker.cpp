/*
 * UdsWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include <TcpWorker.hpp>

#include "RSD.hpp"
#include "UdsComClient.hpp"
#include "Plugin_Error.h"
#include "errno.h"

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
	this->json = new JsonRPC();

	StartWorkerThread(currentSocket);
}


TcpWorker::~TcpWorker()
{
	listen_thread_active = false;
	worker_thread_active = false;
	pthread_kill(lthread, SIGUSR2);


	WaitForWorkerThreadToExit();
	delete json;
}


void TcpWorker::thread_work(int socket)
{

	memset(receiveBuffer, '\0', BUFFER_SIZE);
	worker_thread_active = true;
	string* request = NULL;
	string* errorResponse = NULL;


	//start the listenerthread and remember the theadId of it
	lthread = StartListenerThread(pthread_self(), currentSocket, receiveBuffer);


	while(worker_thread_active)
	{
		//wait for signals from listenerthread

		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
				while(getReceiveQueueSize() > 0)
				{
					try
					{
						request = receiveQueue.back();
						printf("%s\n", request->c_str());
						handleMsg(request);

					}
					catch(PluginError &e)
					{
						errorResponse = new string(e.get());
						tcp_send(errorResponse);
						delete errorResponse;
					}
					popReceiveQueue();
				}
				break;

			case SIGUSR2:
				printf("TcpComWorker: SIGUSR2\n");
				checkComClientList();
				break;

			case SIGPIPE:
				printf("TcpComWorker: SIGPIPE\n");
				deleteComClientList();
				break;

			default:
				printf("TcpComWorker: unkown signal \n");
				deleteComClientList();
				break;
		}
	}
	close(currentSocket);
	printf("TCP Worker Thread beendet.\n");
	WaitForListenerThreadToExit();
	//mark this whole worker/listener object as deletable
	deletable = true;

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
			pushReceiveQueue(new string(receiveBuffer, recvSize));

			//signal the worker
			pthread_kill(parent_th, SIGUSR1);
		}
		//no data, either udsComClient or plugin invoked a shutdown of this UdsComWorker
		else
		{
			//udsComClient invoked shutdown
			if(errno == EINTR)
			{
				pthread_kill(parent_th, SIGUSR2);
			}
			//client
			else
			{
				worker_thread_active = false;
				listen_thread_active = false;
				pthread_kill(parent_th, SIGPOLL);
				listenerDown = true;
			}
		}

	}
	if(!listenerDown)
	{
		worker_thread_active = false;
		listen_thread_active = false;
		pthread_kill(parent_th, SIGPOLL);
	}
	printf("TCP Listener beendet.\n");
}


int TcpWorker::tcp_send(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}


void TcpWorker::handleMsg(string* request)
{
	char* methodNamespace = NULL;

	// 1) parse to dom with jsonrpc
	json->parse(request);
	methodNamespace = getMethodNamespace();


	lastComClient = findComClient(methodNamespace);

	// 4) use the udsClient to send forward the request
	lastComClient->sendData(receiveQueue.back());

}


UdsComClient* TcpWorker::findComClient(char* pluginName)
{
	UdsComClient* currentComClient = NULL;
	Plugin* currentPlugin = NULL;
	bool clientFound = false;

	/*
	//first check if lastComClient isn't  what we are looking for
	if(lastComClient != NULL && lastComClient->getPluginName()->compare(pluginName) == 0 && !lastComClient->isDeletable())
	{
		currentComClient = lastComClient;
		clientFound = true;
	}
	else*/
	{
		// 3) check if we already have a udsClient for this namespace/pluginName
		for(unsigned int i = 0 ; i < comClientList.size() && !clientFound ; i++)
		{
			currentComClient = comClientList[i];
			if(currentComClient->getPluginName()->compare(pluginName) == 0)
			{
				clientFound = true;
			}
		}

		// 3.1)  IF NOT  -> check RSD plugin list for this namespace and get udsFilePath
		if(!clientFound)
		{
			currentPlugin = RSD::getPlugin(pluginName);

			if(currentPlugin == NULL)
			{
				//TODO: 3.1.1) error, no plugin with this namespace connected
			}

			// 3.2)  create a new udsClient with this udsFilePath and push it to list
			currentComClient = new UdsComClient(this, currentPlugin->getUdsFilePath(), currentPlugin->getName());
			comClientList.push_back(currentComClient);
			printf("Added new ComClient: %s\n", currentComClient->getPluginName()->c_str());
		}
	}

	return currentComClient;
}


char* TcpWorker::getMethodNamespace()
{
	const char* methodName = NULL;
	char* methodNamespace = NULL;
	int namespacePos = 0;

	// 2) (get methodname)get method namespace
	methodName = json->getMethod(true);
	namespacePos = strcspn(methodName, ".");
	methodNamespace = new char[namespacePos+1];
	strncpy(methodNamespace, methodName, namespacePos);
	methodNamespace[namespacePos] = '\0';

	return methodNamespace;

}




void TcpWorker::deleteComClientList()
{
	lastComClient = NULL;
	int size = comClientList.size();

	for(int i = 0; i < size; i++)
	{
		printf("Deleting COmClient: %s\n", comClientList[i]->getPluginName()->c_str());
		delete comClientList[i];
		comClientList[i] = NULL;
	}

	comClientList.clear();
	vector<UdsComClient*>().swap(comClientList);
}


void TcpWorker::checkComClientList()
{
	int size = comClientList.size();

	for(int i = 0; i < size; i++)
	{
		if(comClientList[i]->isDeletable())
		{
			printf("Deleting COmClient: %s\n", comClientList[i]->getPluginName()->c_str());
			delete comClientList[i];
			comClientList.erase(comClientList.begin()+i);
			//TODO: create correct json rpc response or notification for client
			//TODO: also delete plugin from list, else we will always try to connect to it.
			send(currentSocket, "Connection to AardvarkPlugin Aborted!\n", 39, 0);
		}
	}
}
