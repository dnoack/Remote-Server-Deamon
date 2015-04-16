/*
 * ConnectionContext.cpp
 *
 *  Created on: 		23.03.2015
 *  Author: 			dnoack
 */

#include "RSD.hpp"
#include "Plugin.hpp"
#include "Plugin_Error.h"
#include "ConnectionContext.hpp"
#include "Utils.h"


unsigned short ConnectionContext::contextCounter;
pthread_mutex_t ConnectionContext::contextCounterMutex;
int ConnectionContext::indexLimit;
int ConnectionContext::currentIndex;



ConnectionContext::ConnectionContext(int tcpSocket)
{
	pthread_mutex_init(&rIPMutex, NULL);
	deletable = false;
	udsCheck = false;
	error = NULL;
	id = NULL;
	currentClient = NULL;
	requestInProcess = false;
	lastSender = -1;
	jsonInput = NULL;
	identity = NULL;
	jsonReturn = NULL;
	contextNumber = getNewContextNumber();
	//TODO: if no number is free, tcpworker has to send an error and close the connection
	json = new JsonRPC();
	new TcpWorker(this, &tcpConnection,tcpSocket);

}



ConnectionContext::~ConnectionContext()
{
	delete tcpConnection;
	delete json;
	pthread_mutex_lock(&contextCounterMutex);
	--contextCounter;
	pthread_mutex_unlock(&contextCounterMutex);
	pthread_mutex_destroy(&rIPMutex);
}


void ConnectionContext::init()
{
	pthread_mutex_init(&contextCounterMutex, NULL);
	contextCounter = 0;
	currentIndex = 1;
	indexLimit = numeric_limits<short>::max();
}


void ConnectionContext::destroy()
{
	pthread_mutex_destroy(&contextCounterMutex);
}




void ConnectionContext::processMsg(RsdMsg* msg)
{

	try
	{
		printf("Stacksize: %d", requests.size());
		//parse to dom with jsonrpc
		json->parse(msg->getContent());

		//is it a request ?
		if(json->isRequest())
		{
			setRequestInProcess();
			handleRequest(msg);
		}
		//or is it a response and is a requestInProcess ?
		else if(json->isResponse() && isRequestInProcess())
		{
			handleResponse(msg);
		}
		//trash or response msg but there is no RequestInProcess
		else
		{
			handleTrash(msg);
		}
	}
	catch(PluginError &e)
	{
		dyn_print("Exception: %s\n", e.get());
		throw;
	}
}


void ConnectionContext::handleRequest(RsdMsg* msg)
{
	char* methodNamespace = NULL;
	id = json->tryTogetId();
	methodNamespace = getMethodNamespace();

	//Msg for RSD
	if(strncmp(methodNamespace, "RSD", strlen(methodNamespace))== 0 )
	{
		handleRSDCommand(msg);
		delete[] methodNamespace;
		delete msg;
		setRequestNotInProcess();
	}
	//Msg for a Plugin
	else
	{
		currentClient = findUdsConnection(methodNamespace);
		delete[] methodNamespace;

		//OK
		if(currentClient != NULL)
		{
			requests.push_back(msg);
			currentClient->sendData(msg->getContent());
		}
		//BAD
		else
		{
			error = json->generateResponseError(*id, -33011, "Plugin not found.");
			setRequestNotInProcess();
			throw PluginError(error);
		}
	}
}


void ConnectionContext::handleResponse(RsdMsg* msg)
{

	RsdMsg* lastMsg = requests.back();

	lastSender = lastMsg->getSender();

	//lastSender == 0 means, send response to tcp client
	if(lastSender != 0)
	{
		currentClient =  findUdsConnection(lastSender);
		//TODO: implement case that plugin went offline, like in handleRequest
		delete lastMsg;
		requests.pop_back();
		currentClient->sendData(msg->getContent());
	}
	else
	{
		delete lastMsg;
		requests.pop_back();
		setRequestNotInProcess();
		tcp_send(msg);
	}
	delete msg;
}


void ConnectionContext::handleTrash(RsdMsg* msg)
{
	//client sent response / plugin send response but without a request
	if(!isRequestInProcess())
	{
		//sender = client -> send back to client (like echo)
		if(msg->getSender() == 0)
			tcp_send(msg);

		//sender = plugin ? -> just drop msg
		delete msg;
	}
	//Request in process but received nonsense ? -> send this to last sender
	else
		handleResponse(msg);
}



void ConnectionContext::handleRSDCommand(RsdMsg* msg)
{
	Value* method = json->tryTogetMethod();
	Value* params = NULL;
	Value* id = json->tryTogetId();
	Value resultValue;
	const char* result = NULL;

	RSD::executeFunction(*method, *params, resultValue);

	//generate json rsponse msg via jsonRPC with resultValue !
	result = json->generateResponse(*id, resultValue);

	//send the generated msg back to client
	tcp_send(result);
}


void ConnectionContext::handleIncorrectPluginResponse(RsdMsg* msg, const char* error)
{
	RsdMsg* lastMsg = requests.back();
	error = json->generateResponseError(*id, -99, "Incorrect Response from plugin.");
	//TODO: insert information from parameter error

	//who was the sender of the last request
	lastSender = lastMsg->getSender();
	//lastSender == 0 means, send response to tcp client
	if(lastSender != 0)
	{
		currentClient =  findUdsConnection(lastSender);
		delete lastMsg;
		requests.pop_back();
		currentClient->sendData(error);
	}
	else
	{
		delete lastMsg;
		requests.pop_back();
		setRequestNotInProcess();
		tcp_send(error);
	}
	delete msg;
}


char* ConnectionContext::getMethodNamespace()
{
	const char* methodName = NULL;
	char* methodNamespace = NULL;
	unsigned int namespacePos = 0;
	Value* id;

	// get method namespace
	methodName = json->tryTogetMethod()->GetString();
	namespacePos = strcspn(methodName, ".");

	//No '.' found -> no namespace
	if(namespacePos == strlen(methodName) || namespacePos == 0)
	{
		id = json->tryTogetId();
		error = json->generateResponseError(*id, -32010, "Methodname has no namespace.");
		setRequestNotInProcess();
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


const char* ConnectionContext::generateIdentificationMsg(int contextNumber)
{
	Value method;
	Value params;
	Value* id = NULL;
	const char* msg = NULL;
	Document* requestDOM = json->getRequestDOM();

	method.SetString("setIdentification", requestDOM->GetAllocator());
	params.SetObject();
	params.AddMember("contextNumber", contextNumber, requestDOM->GetAllocator());


	msg = json->generateRequest(method, params, *id);

	return msg;
}


short ConnectionContext::getNewContextNumber()
{
	short result = 0;
	pthread_mutex_lock(&contextCounterMutex);

	if(contextCounter < NUMBER_OF_CONNECTIONS)
	{
		if(!(currentIndex < indexLimit))
			currentIndex = 1;

		result = currentIndex;
		++currentIndex;
		++contextCounter;
	}
	else
	{
		result = -1;
	}

	pthread_mutex_unlock(&contextCounterMutex);
	return result;
}


bool ConnectionContext::isDeletable()
{
	list<UdsComClient*>::iterator udsConnection;

	if(tcpConnection->isDeletable())
	{
		//close tcpConnection
		deletable = true;
		delete tcpConnection;
		tcpConnection = NULL;

		//close all UdsConnections
		udsConnection = udsConnections.begin();
		while(udsConnection != udsConnections.end())
		{
			delete *udsConnection;
			udsConnection = udsConnections.erase(udsConnection);
			dyn_print("RSD: UdsComworker deleted from list.Verbleibend: %lu \n", udsConnections.size());
		}
	}
	return deletable;
}


void ConnectionContext::checkUdsConnections()
{
	list<UdsComClient*>::iterator udsConnection;

	udsConnection = udsConnections.begin();
	while(udsConnection != udsConnections.end())
	{
		if((*udsConnection)->isDeletable())
		{
			delete *udsConnection;
			udsConnection = udsConnections.erase(udsConnection);
			dyn_print("RSD: UdsComWorker deleted from list.Verbleibend: %d\n", udsConnections.size());
			tcpConnection->tcp_send("Connection to AardvarkPlugin Aborted!\n", 39);
		}
		else
			++udsConnection;
	}

}



UdsComClient* ConnectionContext::findUdsConnection(char* pluginName)
{
	UdsComClient* currentComClient = NULL;
	Plugin* currentPlugin = NULL;
	bool clientFound = false;
	list<UdsComClient*>::iterator i = udsConnections.begin();


	//  check if we already have a udsClient for this namespace/pluginName
	while(i != udsConnections.end() && !clientFound)
	{
		currentComClient = *i;
		if(currentComClient->getPluginName()->compare(pluginName) == 0)
		{
			clientFound = true;
		}
		else
			++i;
	}

	//   IF NOT  -> check RSD plugin list for this namespace and get udsFilePath
	if(!clientFound)
	{
		currentComClient = NULL;
		currentPlugin = RSD::getPlugin(pluginName);

		if(currentPlugin != NULL)
		{
			//   create a new udsClient with this udsFilePath and push it to list
			currentComClient = new UdsComClient(this, currentPlugin->getUdsFilePath(), currentPlugin->getName(), currentPlugin->getPluginNumber());
			if(currentComClient->tryToconnect())
			{
				udsConnections.push_back(currentComClient);
				currentComClient->sendData(generateIdentificationMsg(contextNumber));
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



UdsComClient* ConnectionContext::findUdsConnection(int pluginNumber)
{
	UdsComClient* currentComClient = NULL;
	bool clientFound = false;
	Plugin* currentPlugin = NULL;
	list<UdsComClient*>::iterator i = udsConnections.begin();


	// 3) check if we already have a udsClient for this namespace/pluginName
	while(i != udsConnections.end() && !clientFound)
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
	{
		currentComClient = NULL;
		currentPlugin = RSD::getPlugin(pluginNumber);

		if(currentPlugin != NULL)
		{
			//   create a new udsClient with this udsFilePath and push it to list
			currentComClient = new UdsComClient(this, currentPlugin->getUdsFilePath(), currentPlugin->getName(), currentPlugin->getPluginNumber());
			if(currentComClient->tryToconnect())
			{
				udsConnections.push_back(currentComClient);
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


void ConnectionContext::deleteAllUdsConnections()
{
	list<UdsComClient*>::iterator i = udsConnections.begin();

	while(i != udsConnections.end())
	{
		delete *i;
		i = udsConnections.erase(i);
	}
}


void ConnectionContext::arrangeUdsConnectionCheck()
{
	udsCheck = true;
}


int ConnectionContext::tcp_send(RsdMsg* msg)
{
	return tcpConnection->tcp_send(msg);
}


int ConnectionContext::tcp_send(const char* msg)
{
	return tcpConnection->tcp_send(msg, strlen(msg));
}



bool ConnectionContext::isRequestInProcess()
{
	bool result = false;
	pthread_mutex_lock(&rIPMutex);
		result = requestInProcess;
	pthread_mutex_unlock(&rIPMutex);
	return result;
}


void ConnectionContext::setRequestInProcess()
{
	pthread_mutex_lock(&rIPMutex);
	requestInProcess = true;
	pthread_mutex_unlock(&rIPMutex);
}


void ConnectionContext::setRequestNotInProcess()
{
	pthread_mutex_lock(&rIPMutex);
	requestInProcess = false;
	pthread_mutex_unlock(&rIPMutex);
}
