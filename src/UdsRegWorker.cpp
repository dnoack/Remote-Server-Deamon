/*
 * UdsComWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include "RSD.hpp"
#include "UdsRegWorker.hpp"
#include "UdsRegServer.hpp"
#include "Plugin_Error.h"
#include "document.h"

using namespace rapidjson;



UdsRegWorker::UdsRegWorker(int socket)
{
	this->listen_thread_active = false;
	this->worker_thread_active = false;
	this->recvSize = 0;
	this->lthread = 0;
	this->request = 0;
	this->response = 0;
	this->currentSocket = socket;
	this->nextPlugin = NULL;
	this->state = NOT_ACTIVE;
	this->json = new JsonRPC();

	memset(receiveBuffer, '\0', BUFFER_SIZE);
	StartWorkerThread(currentSocket);
}



UdsRegWorker::~UdsRegWorker()
{
	worker_thread_active = false;
	listen_thread_active = false;
	delete json;
	WaitForWorkerThreadToExit();
}




void UdsRegWorker::thread_work(int socket)
{
	char* response = NULL;

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
					request = receiveQueue.back();
					//sigusr1 = there is data for work e.g. parsing json rpc
					printf("Register service received: %s \n", request->c_str());

					switch(state)
					{
						case NOT_ACTIVE:
							//check for announce msg, create a plugin object in RSD central list
							response = handleAnnounceMsg(request);
							send(currentSocket, response,  strlen(response), 0);
							state = ANNOUNCED;
							break;

						case ANNOUNCED:
							//check for register msg, add all method names from msg to plugin object in central RSD list
							if(handleRegisterMsg(request))
							{
								state = REGISTERED;
								response = createRegisterACKMsg();
								send(currentSocket, response, strlen(response), 0);
							}
							//send registerACK
							break;

						case REGISTERED:
							break;

						case ACTIVE:

							break;
						case BROKEN:

							break;
						default:
							//something went completely wrong
							state = BROKEN;
							break;
					}
					editReceiveQueue(NULL, false);

				}
				break;

			case SIGUSR2:
				//sigusr2 = time to exit
				worker_thread_active = false;
				listen_thread_active = false;
				break;

			case SIGPIPE:
				listen_thread_active = false;
				worker_thread_active = false;
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
	//destroy this UdsWorker and delete it from workerList in Uds::Server
	UdsRegServer::editWorkerList(this, DELETE_WORKER);

}



void UdsRegWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
{

	listen_thread_active = true;
	memset(receiveBuffer, '\0', BUFFER_SIZE);

	while(listen_thread_active)
	{

		//TODO:msg_dontwait lets us cancel the client with listen_thread_active flag but not with sigpipe (disco at clientside)
		recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, 0); //MSG_DONTWAIT for nonblocking
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
			memset(receiveBuffer, '\0', BUFFER_SIZE);
		}
		if(recvSize == 0)
			listen_thread_active = false;


	}
	printf("Listener beendet.\n");
	pthread_kill(parent_th, SIGPOLL);
}


char* UdsRegWorker::handleAnnounceMsg(string* request)
{
	Value* currentParam = NULL;
	Value result;
	Value id;
	const char* name = NULL;
	const char* udsFilePath = NULL;

	json->parse(request);
	currentParam = json->getParam("pluginName");
	name = currentParam->GetString();
	currentParam = json->getParam("udsFilePath");
	udsFilePath = currentParam->GetString();
	nextPlugin = new Plugin(name, udsFilePath);

	result.SetString("announceACK");
	id.SetInt(1);//TODO: read id from requestmsg

	return json->generateResponse(id, result);

}

bool UdsRegWorker::handleRegisterMsg(string* request)
{
	Document* dom;
	string* methodName = NULL;
	bool result = false;

	try
	{
		dom = json->parse(request);
		for(Value::ConstMemberIterator i = (*dom)["params"].MemberBegin(); i != (*dom)["params"].MemberEnd(); ++i)
		{
			methodName = new string(i->value.GetString());
			nextPlugin->addMethod(methodName);
		}
		result = true;
	}
	catch(PluginError &e)
	{
		printf("%s\n",e.get());
		result = false;
	}
	return result;
}


char* UdsRegWorker::createRegisterACKMsg()
{
	Value result;
	Value id;
	result.SetString("registerACK");
	id.SetInt(2);//TODO: read id from requestmsg
	return json->generateResponse(id, result);
}




