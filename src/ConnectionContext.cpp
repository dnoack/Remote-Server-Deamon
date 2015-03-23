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



ConnectionContext::ConnectionContext(int tcpSocket)
{
	pthread_mutex_init(&rIPMutex, NULL);
	deletable = false;
	udsCheck = false;
	requestInProcess = false;
	lastSender = -1;
	jsonInput = NULL;
	identity = NULL;
	jsonReturn = NULL;
	json = new JsonRPC();
	tcpConnection = new TcpWorker(this, tcpSocket);
}


ConnectionContext::~ConnectionContext()
{
	delete tcpConnection;
	pthread_mutex_destroy(&rIPMutex);
}


void ConnectionContext::processMsg(RsdMsg* msg)
{
	char* methodNamespace = NULL;
	char* error = NULL;
	int lastSender = 0;
	Value* id;
	UdsComClient* currentClient = NULL;
	printf("ProcessMsg()\n");

	try
	{
		setRequestInProcess();
		//parse to dom with jsonrpc
		json->parse(msg->getContent());


		//is it a request ?
		if(json->isRequest())
		{
			methodNamespace = getMethodNamespace();
			currentClient = findUdsConnection(methodNamespace);

			if(currentClient != NULL)
			{
				//TODO: add msg to requestlist
				requests.push_back(msg);
				currentClient->sendData(msg->getContent());
			}
			else
			{
				id = json->tryTogetId();
				error = json->generateResponseError(*id, -33011, "Plugin not found.");
				setRequestNotInProcess();
				throw PluginError(error);
			}
		}
		//or is it a response ?
		else if(json->isResponse())
		{
			//TODO: check the id of the last request !
			lastSender = requests.back()->getSender();
			if(lastSender != 0)
			{
				//TODO: forward msg to sender of request !
				currentClient =  findUdsConnection(lastSender);
				currentClient->sendData(msg->getContent());
				printf("Back to plugin.\n");
			}
			else
			{	printf("Back to Tcp Client.\n");
				tcp_send(msg);
				setRequestNotInProcess();
			}
			requests.pop_back();
		}
	}
	catch(PluginError &e)
	{
		setRequestNotInProcess();
		throw;
	}


}


char* ConnectionContext::getMethodNamespace()
{
	const char* methodName = NULL;
	char* methodNamespace = NULL;
	char* error = NULL;
	unsigned int namespacePos = 0;
	Value* id;

	// 2) (get methodname)get method namespace
	methodName = json->tryTogetMethod()->GetString();
	namespacePos = strcspn(methodName, ".");

	//Not '.' found -> no namespace
	if(namespacePos == strlen(methodName) || namespacePos == 0)
	{
		id = json->tryTogetId();
		error = json->generateResponseError(*id, -32010, "Methodname has no namespace.");
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
			printf("RSD: Tcpworker deleted from list.Verbleibend: %d\n", udsConnections.size());
			++udsConnection;
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
			printf("RSD: Tcpworker deleted from list.Verbleibend: %d\n", udsConnections.size());
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


	// 3) check if we already have a udsClient for this namespace/pluginName
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

	// 3.1)  IF NOT  -> check RSD plugin list for this namespace and get udsFilePath
	if(!clientFound)
	{
		currentComClient = NULL;
		currentPlugin = RSD::getPlugin(pluginName);

		if(currentPlugin != NULL)
		{
			// 3.2)  create a new udsClient with this udsFilePath and push it to list
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



UdsComClient* ConnectionContext::findUdsConnection(int pluginNumber)
{
	UdsComClient* currentComClient = NULL;
	bool clientFound = false;
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
		currentComClient = NULL;

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
