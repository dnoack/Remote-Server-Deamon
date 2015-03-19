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

	pthread_cancel(getListener());
	pthread_cancel(getWorker());

	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
	delete json;
}



void TcpWorker::thread_work(int socket)
{
	RsdMsg* request = NULL;
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
				//while(getReceiveQueueSize() > 0)
				//{
					try
					{
						request = receiveQueue.back();
						printf("Tcp Queue Received: %s\n", request->getContent()->c_str());
						handleMsg(request);
					}
					catch(PluginError &e)
					{
						errorResponse = new string(e.get());
						errorMsg = new RsdMsg(0, errorResponse);
						routeBack(errorMsg);
						delete errorMsg;
					}
					catch(...)
					{
						printf("Unkown Exception.\n");
					}

				//}
				break;

			case SIGUSR2:
				printf("TcpComWorker: SIGUSR2\n");
				checkComClientList();
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
			checkComClientList();
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



void TcpWorker::routeBack(RsdMsg* data)
{
	int lastRequestSender = 0;
	UdsComClient* comClient = NULL;




	try{
		json->parse(data->getContent());
		//check if msg is a response or a request
		if(json->isRequest())
		{
			//request will be queued in the "receiveQueue"
			pushReceiveQueue(new RsdMsg(data)); //copy
			//signal the worker
			pthread_kill(getWorker(), SIGUSR1);
		}
	}
	catch(PluginError &e)
	{
		//if it is a response then
			//check last sender of the latest RsdMsg in "ReceiveQueue"
			lastRequestSender = receiveQueue.back()->getSender();

			//if this is 0, we have to send the response over tcp to the client
			if(lastRequestSender == 0)
			{
				send(currentSocket, data->getContent()->c_str(), data->getContent()->size(), 0);
				popReceiveQueue();
			}
			else
			{
				comClient = findComClient(lastRequestSender);
				popReceiveQueueWithoutDelete();
				//send response to this last sender
				comClient->sendData(data->getContent());
				//delete data; //!!!!!!!!!!!
			}

	}
	//TODO: implement JsonRPC::isResponse() and another else for errors






}


void TcpWorker::handleMsg(RsdMsg* request)
{
	char* methodNamespace = NULL;
	char* error = NULL;
	Value id;
	UdsComClient* currentClient = NULL;

	// 1) parse to dom with jsonrpc
	json->parse(request->getContent());
	methodNamespace = getMethodNamespace();


	currentClient = findComClient(methodNamespace);

	if(currentClient != NULL)
	{
		// 4) use the udsClient to send forward the request
		currentClient->sendData(receiveQueue.back()->getContent());
	}
	else
	{
		id.SetInt(json->getId(true));
		error = json->generateResponseError(id, -33011, "Plugin not found.");
		throw PluginError(error);
	}

}



UdsComClient* TcpWorker::findComClient(char* pluginName)
{
	UdsComClient* currentComClient = NULL;
	Plugin* currentPlugin = NULL;
	bool clientFound = false;
	list<UdsComClient*>::iterator i = comClientList.begin();


	// 3) check if we already have a udsClient for this namespace/pluginName
	while(i != comClientList.end() && !clientFound)
	{
		currentComClient = *i;
		if(currentComClient->getPluginName()->compare(pluginName) == 0)
		{
			clientFound = true;
		}
		else
			++i;
	}

	// 3.1)  IF NOT  -> check RSD plugin list for this namespace and get udsFilePath
	if(!clientFound)
	{
		currentPlugin = RSD::getPlugin(pluginName);

		if(currentPlugin != NULL)
		{
			// 3.2)  create a new udsClient with this udsFilePath and push it to list
			currentComClient = new UdsComClient(this, currentPlugin->getUdsFilePath(), currentPlugin->getName(), currentPlugin->getPluginNumber());
			if(currentComClient->tryToconnect())
			{
				comClientList.push_back(currentComClient);
			}
			else
			{
				delete currentComClient;
				currentComClient = NULL;
			}
		}
	}

	return currentComClient;
}



UdsComClient* TcpWorker::findComClient(int pluginNumber)
{
	UdsComClient* currentComClient = NULL;
	bool clientFound = false;
	list<UdsComClient*>::iterator i = comClientList.begin();


	// 3) check if we already have a udsClient for this namespace/pluginName
	while(i != comClientList.end() && !clientFound)
	{
		currentComClient = *i;
		if(currentComClient->getPluginNumber() == pluginNumber)
		{
			clientFound = true;
		}
		else
			++i;
	}
	if(!clientFound)
		currentComClient = NULL;

	return currentComClient;
}


char* TcpWorker::getMethodNamespace()
{
	const char* methodName = NULL;
	char* methodNamespace = NULL;
	char* error = NULL;
	unsigned int namespacePos = 0;
	Value id;

	// 2) (get methodname)get method namespace
	methodName = json->getMethod(true);
	namespacePos = strcspn(methodName, ".");

	//Not '.' found -> no namespace
	if(namespacePos == strlen(methodName) || namespacePos == 0)
	{
		id.SetInt(json->getId(true));
		error = json->generateResponseError(id, -32010, "Methodname has no namespace.");
		throw PluginError(error);
	}
	else
	{
		methodNamespace = new char[namespacePos+1];
		strncpy(methodNamespace, methodName, namespacePos);
		methodNamespace[namespacePos] = '\0';
	}

	return methodNamespace;

}




void TcpWorker::deleteComClientList()
{
	list<UdsComClient*>::iterator i = comClientList.begin();

	while(i != comClientList.end())
	{
		delete *i;
		i = comClientList.erase(i);
	}

}


void TcpWorker::checkComClientList()
{
	list<UdsComClient*>::iterator i = comClientList.begin();

	while(i != comClientList.end())
	{
		if((*i)->isDeletable())
		{
			delete *i;
			i = comClientList.erase(i);

			//TODO: create correct json rpc response or notification for client
			//TODO: also delete plugin from list, else we will always try to connect to it.
			send(currentSocket, "Connection to AardvarkPlugin Aborted!\n", 39, 0);
		}
		else
			++i;
	}
}
