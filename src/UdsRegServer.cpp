/*
 * Uds.cpp
 *
 *  Created on: 04.02.2015
 *      Author: dnoack
 */

#include <UdsRegServer.hpp>
#include "JsonRPC.hpp"
#include "UdsRegWorker.hpp"


#define EXPECTED_NUM_OF_DEVICES 1

//static symbols
int UdsRegServer::connection_socket;

vector<UdsRegWorker*> UdsRegServer::workerList;
pthread_mutex_t UdsRegServer::wLmutex;

struct sockaddr_un UdsRegServer::address;
socklen_t UdsRegServer::addrlen;



UdsRegServer::UdsRegServer( const char* udsFile, int nameSize)
{

	optionflag = 1;
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
	pthread_mutex_destroy(&wLmutex);
	close(connection_socket);
	workerList.erase(workerList.begin(), workerList.end());
}



int UdsRegServer::call()
{
	connect(connection_socket, (struct sockaddr*) &address, addrlen);
	return 0;
}




void* UdsRegServer::uds_accept(void* param)
{
	int new_socket = 0;
	UdsRegWorker* worker;
	bool accept_thread_active = true;
	listen(connection_socket, 5);

	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket >= 0)
		{
			worker = new UdsRegWorker(new_socket);
			editWorkerList(worker, ADD_WORKER);
			//printf("Client verbunden.\n");
		}

	}
	return 0;
}


void UdsRegServer::editWorkerList(UdsRegWorker* newWorker, bool add)
{
	pthread_mutex_lock(&wLmutex);
	if(add)
	{
		workerList.push_back(newWorker);
	}
	else
	{
		//find worker
		for(unsigned int i = 0; i < workerList.size() ; i++)
		{
			if(workerList[i] == newWorker)
			{
				workerList.erase(workerList.begin()+i);
				break;
			}
		}
	}
	pthread_mutex_unlock(&wLmutex);
}




void UdsRegServer::start()
{
	pthread_t accepter;
	pthread_create(&accepter, NULL, uds_accept, NULL);
}



