/*
 * RSD.cpp
 *
 *  Created on: 	11.02.2015
 *  Author: 		dnoack
 */


#include "RSD.hpp"
#include "RsdMsg.h"


int RSD::connection_socket;
struct sockaddr_in RSD::address;
socklen_t RSD::addrlen;

vector<Plugin*> RSD::plugins;
pthread_mutex_t RSD::pLmutex;

list<ConnectionContext*> RSD::connectionContextList;
pthread_mutex_t RSD::connectionContextListMutex;



RSD::RSD()
{
	pthread_mutex_init(&pLmutex, NULL);
	pthread_mutex_init(&connectionContextListMutex, NULL);

	rsdActive = true;
	accepter = 0;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(TCP_PORT);
	addrlen = sizeof(address);
	optionflag = 1;

	connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(connection_socket, (struct sockaddr*)&address, sizeof(address));

	//create Registry Server
	regServer = new UdsRegServer(REGISTRY_PATH, sizeof(REGISTRY_PATH));
}



RSD::~RSD()
{
	delete regServer;
	close(connection_socket);

	pthread_mutex_destroy(&pLmutex);
	pthread_mutex_destroy(&connectionContextListMutex);
}


void* RSD::accept_connections(void* data)
{
	listen(connection_socket, MAX_CLIENTS);
	bool accept_thread_active = true;
	int tcpSocket = 0;
	ConnectionContext* context = NULL;

	while(accept_thread_active)
	{
		tcpSocket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(tcpSocket >= 0)
		{
			printf("Client connected\n");
			context = new ConnectionContext(tcpSocket);
			pushWorkerList(context);
		}
	}

	return 0;
}


bool RSD::addPlugin(char* name, int pluginNumber, char* udsFilePath)
{
	bool result = false;


	if(getPlugin(name) == NULL)
	{
		pthread_mutex_lock(&pLmutex);
		plugins.push_back(new Plugin(name, pluginNumber, udsFilePath));
		result = true;
		pthread_mutex_unlock(&pLmutex);
	}

	return result;
}


bool RSD::addPlugin(Plugin* newPlugin)
{
	bool result = false;
	char* name = (char*)(newPlugin->getName()->c_str());

	if(getPlugin(name) == NULL)
	{
		pthread_mutex_lock(&pLmutex);
		plugins.push_back(newPlugin);
		result = true;
		pthread_mutex_unlock(&pLmutex);
	}

	return result;
}


bool RSD::deletePlugin(string* name)
{
	bool result = false;
	string* currentName = NULL;

	pthread_mutex_lock(&pLmutex);
	for(unsigned int i = 0; i < plugins.size() && result == false; i++)
	{
		currentName = plugins[i]->getName();
		if(currentName->compare(*name) == 0)
		{
			delete plugins[i];
			plugins.erase(plugins.begin()+i);
			result = true;
		}
	}
	pthread_mutex_unlock(&pLmutex);

	return result;
}


Plugin* RSD::getPlugin(char* name)
{
	Plugin* result = NULL;
	string* currentName = NULL;

	pthread_mutex_lock(&pLmutex);
	for(unsigned int i = 0; i < plugins.size() && result == NULL; i++)
	{
		currentName = plugins[i]->getName();
		if(currentName->compare(name) == 0)
		{
			result = plugins[i];
		}
	}
	pthread_mutex_unlock(&pLmutex);

	return result;
}


Plugin* RSD::getPlugin(int pluginNumber)
{
	Plugin* result = NULL;
	int currentNumber = -1;

	pthread_mutex_lock(&pLmutex);
	for(unsigned int i = 0; i < plugins.size() && result == NULL; i++)
	{
		currentNumber = plugins[i]->getPluginNumber();
		if(pluginNumber == currentNumber)
		{
			result = plugins[i];
		}
	}
	pthread_mutex_unlock(&pLmutex);

	return result;
}


void RSD::pushWorkerList(ConnectionContext* context)
{
	pthread_mutex_lock(&connectionContextListMutex);
		connectionContextList.push_back(context);
		printf("Anzahl ConnectionContext: %d\n", connectionContextList.size());
	pthread_mutex_unlock(&connectionContextListMutex);
}


void RSD::checkForDeletableConnections()
{
	pthread_mutex_lock(&connectionContextListMutex);

	list<ConnectionContext*>::iterator connection = connectionContextList.begin();
	while(connection != connectionContextList.end())
	{
		//is a tcp connection down ?
		if((*connection)->isDeletable())
		{
			delete *connection;
			connection = connectionContextList.erase(connection);
			printf("RSD: ConnectionContext deleted from list.Verbleibend: %d\n", connectionContextList.size());
		}
		//maybe we got a working tcp connection but a plugin went down
		else if((*connection)->isUdsCheckEnabled())
		{
			(*connection)->checkUdsConnections();
			++connection;
		}
		else
			++connection;
	}
	pthread_mutex_unlock(&connectionContextListMutex);
}


void RSD::start()
{
	regServer->start();

	//start comListener
	pthread_create(&accepter, NULL, accept_connections, NULL);

	while(rsdActive)
	{
		sleep(MAIN_SLEEP_TIME);
		//check uds registry workers
		regServer->checkForDeletableWorker();
		//check TCP/workers
		this->checkForDeletableConnections();
		RsdMsg::printCounters();
	}
}

#ifndef TESTMODE

int main(int argc, char** argv)
{
	RSD* rsd = new RSD();
	rsd->start();
	delete rsd;
}

#endif



