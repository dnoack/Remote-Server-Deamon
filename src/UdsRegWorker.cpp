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
#include "Utils.h"


using namespace rapidjson;



UdsRegWorker::UdsRegWorker(int socket)
{
	this->request = 0;
	this->response = 0;
	this->currentSocket = socket;
	this->plugin = NULL;
	this->pluginName = NULL;
	this->state = NOT_ACTIVE;
	this->json = new JsonRPC();

	StartWorkerThread(currentSocket);

	if(wait_for_listener_up() != 0)
			throw PluginError("Creation of Listener/worker threads failed.");
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

	cleanup();

}



void UdsRegWorker::thread_work(int socket)
{
	const char* response = NULL;
	worker_thread_active = true;

	StartListenerThread(pthread_self(), currentSocket, receiveBuffer);

	configSignals();

	pthread_cleanup_push(&UdsRegWorker::cleanupReceiveQueue, this);

	while(worker_thread_active)
	{
		//wait for signals from listenerthread
		sigwait(&sigmask, &currentSig);
		switch(currentSig)
		{
			case SIGUSR1:
				try
				{
					request = receiveQueue.back()->getContent();
					dyn_print("RegWorker Received: %s\n", request->c_str());
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
							plugin = NULL;
							break;

						case ACTIVE:

							break;
					}
					popReceiveQueue();
				}
				catch(PluginError &e)
				{
					popReceiveQueue();
					string* errorString = e.getString();
					state = NOT_ACTIVE;
					cleanup();
					send(currentSocket, errorString->c_str(), errorString->size(), 0);

				}
			break;


			default:
				dyn_print("UdsRegWorker: unkown signal \n");
				break;
		}
	}

	close(currentSocket);
	pthread_cleanup_pop(0);
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
		retval = pselect(socket+1, &rfds, NULL, NULL, NULL, &origmask);

		if(retval < 0)
		{
			//Plugin itself invoked shutdown
			listen_thread_active = false;
			deletable = true;
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

		memset(receiveBuffer, '\0', BUFFER_SIZE);
	}
}


const char* UdsRegWorker::handleAnnounceMsg(string* request)
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
	pluginName = new string(name);

	result.SetString("announceACK");
	id = json->tryTogetId();

	return json->generateResponse(*id, result);

}

bool UdsRegWorker::handleRegisterMsg(string* request)
{
	Document* dom;
	string* methodName = NULL;

	dom = json->parse(request);
	for(Value::ConstMemberIterator i = (*dom)["params"].MemberBegin(); i != (*dom)["params"].MemberEnd(); ++i)
	{
		methodName = new string(i->value.GetString());
		plugin->addMethod(methodName);
	}

	return true;
}


void UdsRegWorker::cleanup()
{

	if(pluginName != NULL)
	{
		delete pluginName;
		pluginName = NULL;
	}

	if(plugin != NULL)
	{
		delete plugin;
		plugin = NULL;
	}
}


const char* UdsRegWorker::createRegisterACKMsg()
{
	Value result;
	Value* id;
	result.SetString("registerACK");
	id = json->getId();
	return json->generateResponse(*id, result);
}


void UdsRegWorker::cleanupReceiveQueue(void* arg)
{
	UdsRegWorker* worker = static_cast<UdsRegWorker*>(arg);
	worker->deleteReceiveQueue();
}


