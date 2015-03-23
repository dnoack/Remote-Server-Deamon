/*
 * UdsComWorker.cpp
 *
 *  Created on: 09.02.2015
 *      Author: dnoack
 */

#include "errno.h"

#include "RSD.hpp"
#include "UdsRegWorker.hpp"
#include "Plugin_Error.h"
#include "document.h"
#include "RsdMsg.h"


using namespace rapidjson;



UdsRegWorker::UdsRegWorker(int socket)
{
	memset(receiveBuffer, '\0', BUFFER_SIZE);
	this->listen_thread_active = false;
	this->worker_thread_active = false;
	this->recvSize = 0;
	this->lthread = 0;
	this->request = 0;
	this->response = 0;
	this->currentSocket = socket;
	this->plugin = NULL;
	this->state = NOT_ACTIVE;
	this->json = new JsonRPC();

	StartWorkerThread(currentSocket);
}



UdsRegWorker::~UdsRegWorker()
{
	worker_thread_active = false;
	listen_thread_active = false;

	pthread_cancel(getListener());
	pthread_cancel(getWorker());


	WaitForListenerThreadToExit();
	WaitForWorkerThreadToExit();
	delete json;
}


string* UdsRegWorker::getPluginName()
{
	return plugin->getName();
}




void UdsRegWorker::thread_work(int socket)
{
	char* response = NULL;

	memset(receiveBuffer, '\0', BUFFER_SIZE);

	worker_thread_active = true;

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
					request = receiveQueue.back()->getContent();
					printf("RegWorker Received: %s\n", request->c_str());
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
							break;

						case REGISTERED:
							RSD::addPlugin(plugin);
							break;

						case ACTIVE:

							break;
						case BROKEN:

							break;
						default:
							//something went completely wrong
							state = BROKEN;
							break;
					//}
					popReceiveQueue();

				}
				break;
			case SIGUSR2:
				printf("UdsRegWorker: SIGUSR2\n");
				break;
			default:
				printf("UdsRegWorker: unkown signal \n");
				break;
		}
	}

	close(currentSocket);

}



void UdsRegWorker::thread_listen(pthread_t parent_th, int socket, char* workerBuffer)
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

		if(retval < 0)
		{
			//Plugin itself invoked shutdown
			worker_thread_active = false;
			listen_thread_active = false;
			pthread_kill(parent_th, SIGUSR2);
		}
		else if(FD_ISSET(socket, &rfds))
		{
			recvSize = recv( socket , receiveBuffer, BUFFER_SIZE, 0); //MSG_DONTWAIT for nonblocking
			//data received
			if(recvSize > 0)
			{
				//add received data in buffer to queue
				content = new string(receiveBuffer, recvSize);
				pushReceiveQueue(new RsdMsg(0, content));
				//signal the worker
				pthread_kill(parent_th, SIGUSR1);
			}
			else
			{
				deletable = true;
				listen_thread_active = false;
			}
		}
	}
}


char* UdsRegWorker::handleAnnounceMsg(string* request)
{
	Value* currentParam = NULL;
	Value result;
	Value* id;
	const char* name = NULL;
	int number;
	const char* udsFilePath = NULL;

	json->parse(request);
	currentParam = json->tryTogetParam("pluginName");
	name = currentParam->GetString();
	currentParam = json->tryTogetParam("udsFilePath");
	udsFilePath = currentParam->GetString();
	currentParam = json->tryTogetParam("pluginNumber");
	number = currentParam->GetInt();
	plugin = new Plugin(name, number, udsFilePath);

	result.SetString("announceACK");
	id = json->tryTogetId();

	return json->generateResponse(*id, result);

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
			plugin->addMethod(methodName);
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
	Value* id;
	result.SetString("registerACK");
	id = json->getId();
	return json->generateResponse(*id, result);
}




