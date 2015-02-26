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


RSD::RSD()
{
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(1234);
	addrlen = sizeof(address);
	optionflag = 1;

	connection_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(connection_socket, (struct sockaddr*)&address, sizeof(address));

	//create Registry Server
	regServer = new UdsRegServer(REGISTRY_PATH, sizeof(REGISTRY_PATH));
	regServer->start();

	pthread_create(&accepter, NULL, accept_connections, NULL);
}



RSD::~RSD()
{
	delete regServer;
	close(connection_socket);

}


void* RSD::accept_connections(void* data)
{
	listen(connection_socket, 5);
	bool accept_thread_active = true;
	int new_socket = 0;
	TcpWorker* worker = NULL;


	while(accept_thread_active)
	{
		new_socket = accept(connection_socket, (struct sockaddr*)&address, &addrlen);
		if(new_socket >= 0)
		{
			printf("Client connected\n");
			worker = new TcpWorker(new_socket);
			//TODO: add to worker list, like in uds server from plugin
		}
	}
	return 0;
}


int main(int argc, char** argv)
{
	RSD* rsd = new RSD();
	while(1)
		sleep(3);

	delete rsd;
}



