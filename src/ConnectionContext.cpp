/*
 * ConnectionContext.cpp
 *
 *  Created on: 		23.03.2015
 *  Author: 			dnoack
 */

#include "RSD.hpp"
#include "Plugin.hpp"
#include "ConnectionContext.hpp"


ConnectionContext::ConnectionContext(int tcpSocket)
{
	deletable = false;
	tcpConnection = new TcpWorker(this, tcpSocket);
}


ConnectionContext::~ConnectionContext()
{
	delete tcpConnection;
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


void ConnectionContext::checkUdsConnections()
{
	list<UdsComClient*>::iterator i = udsConnections.begin();

	while(i != udsConnections.end())
	{
		if((*i)->isDeletable())
		{
			delete *i;
			i = udsConnections.erase(i);

			//TODO: create correct json rpc response or notification for client
			//TODO: also delete plugin from list, else we will always try to connect to it.
			tcpConnection->tcp_send("Connection to AardvarkPlugin Aborted!\n", 39);
		}
		else
			++i;
	}
}

