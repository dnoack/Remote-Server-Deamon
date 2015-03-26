/*
 * Uds.cpp
 *
 *  Created on: 04.02.2015
 *      Author: dnoack
 */


#include "RSD.hpp"
#include "UdsRegServer.hpp"
#include "JsonRPC.hpp"
#include "Plugin_Error.h"



//static symbols
int UdsRegServer::connection_socket;
bool UdsRegServer::accept_thread_active;

list<UdsRegWorker*> UdsRegServer::workerList;
pthread_mutex_t UdsRegServer::wLmutex;

struct sockaddr_un UdsRegServer::address;
socklen_t UdsRegServer::addrlen;



UdsRegServer::UdsRegServer( const char* udsFile, int nameSize)
{
	optionflag = 1;
	accepter = 0;
	accept_thread_active = false;
	connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, udsFile, nameSize);
	addrlen = sizeof(address);

	pthread_mutex_init(&wLmutex, NULL);

	unlink(udsFile);
	setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(connection_socket, (struct sockaddr*)&address, addrlen);

}



UdsRegServer::~UdsRegServer()
{

	if(accepter != 0)
		pthread_cancel(accepter);

	deleteWorkerList();

	close(connection_socket);

	pthread_mutex_destroy(&wLmutex);
}


int UdsRegServer::call()
{
	connect(connection_socket, (struct sockaddr*) &address, addrlen);
	return 0;
}


void* UdsRegServer::uds_accept(void* param)
{
	int new_socket = 0;
	UdsRegWorker* newWorker = NULL;

	listen(connection_socket, 5);
	accept_thread_active = true;

	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket > 0)
		{
			pushWorkerList(new_socket);
		}
	}

	return 0;
}



void UdsRegServer::pushWorkerList(int new_socket)
{
	pthread_mutex_lock(&wLmutex);
		workerList.push_back(new UdsRegWorker(new_socket));
	pthread_mutex_unlock(&wLmutex);
}


void UdsRegServer::checkForDeletableWorker()
{
	pthread_mutex_lock(&wLmutex);
	list<UdsRegWorker*>::iterator i = workerList.begin();
	while(i != workerList.end())
	{
		if((*i)->isDeletable())
		{
			RSD::deletePlugin((*i)->getPluginName());
			delete  *i;
			i = workerList.erase(i);
			printf("UdsRegServer: UdsRegWorker was deleted.\n");
		}
		else
			++i;
	}
	pthread_mutex_unlock(&wLmutex);
}


int UdsRegServer::wait_for_accepter_up()
{
   time_t startTime = time(NULL);
   while(time(NULL) - startTime < TIMEOUT)
   {
	   if(accept_thread_active == true)
		   return 0;
   }
   return -1;
}


void UdsRegServer::deleteWorkerList()
{
	pthread_mutex_lock(&wLmutex);
	list<UdsRegWorker*>::iterator i = workerList.begin();

	while(i != workerList.end())
	{
		delete *i;
		i = workerList.erase(i);
	}

	pthread_mutex_unlock(&wLmutex);
}




void UdsRegServer::start()
{
	pthread_create(&accepter, NULL, uds_accept, NULL);
	if(wait_for_accepter_up() < 0)
		throw PluginError("Couldnt start Accept thread in time.");
}
