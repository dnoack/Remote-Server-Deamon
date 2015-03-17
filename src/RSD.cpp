/*
 * RSD.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */


#include "RSD.hpp"


int RSD::connection_socket;
struct sockaddr_in RSD::address;
socklen_t RSD::addrlen;

vector<Plugin*> RSD::plugins;
pthread_mutex_t RSD::pLmutex;

list<TcpWorker*> RSD::tcpWorkerList;
pthread_mutex_t RSD::tcpWorkerListmutex;



RSD::RSD()
{
	pthread_mutex_init(&pLmutex, NULL);
	pthread_mutex_init(&tcpWorkerListmutex, NULL);

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
	pthread_mutex_destroy(&tcpWorkerListmutex);
}


void* RSD::accept_connections(void* data)
{
	listen(connection_socket, MAX_CLIENTS);
	bool accept_thread_active = true;
	int new_socket = 0;
	TcpWorker* newWorker = NULL;


	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket >= 0)
		{
			printf("Client connected\n");
			newWorker = new TcpWorker(new_socket);
			pushWorkerList(newWorker);
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


void RSD::pushWorkerList(TcpWorker* newWorker)
{
	pthread_mutex_lock(&tcpWorkerListmutex);
		tcpWorkerList.push_back(newWorker);
		printf("Anzahl TcpWorker: %d\n", tcpWorkerList.size());
	pthread_mutex_unlock(&tcpWorkerListmutex);
}


void RSD::checkForDeletableWorker()
{
	//TcpWorker* currentWorker = NULL;
	list<TcpWorker*>::iterator i = tcpWorkerList.begin();
	pthread_mutex_lock(&tcpWorkerListmutex);

	while(i != tcpWorkerList.end())
	{
		if((*i)->isDeletable())
		{
			//currentWorker = *i;
			delete *i;
			i = tcpWorkerList.erase(i);

			printf("RSD: Tcpworker deleted from list.Verbleibend: %d\n", tcpWorkerList.size());
		}
		else
			++i;
	}
	pthread_mutex_unlock(&tcpWorkerListmutex);
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
		this->checkForDeletableWorker();
	}
}


int main(int argc, char** argv)
{
	RSD* rsd = new RSD();
	rsd->start();
	delete rsd;
}



