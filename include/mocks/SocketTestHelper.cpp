/*
 * SocketTestHelper.cpp
 *
 *  Created on: 03.06.2015
 *      Author: dave2
 */

#include "SocketTestHelper.hpp"

SocketTestHelper::SocketTestHelper(int addressFamily, enum __socket_type socketType , const char* serverAddress)
{
	this->addressFamily = addressFamily;
	this->socketType = socketType;
	this->serverAddress = serverAddress;
	clientSocket = 0;
	serverSocket = 0;
	server_accept_socket = 0;
	addrlen = 0;
	memset(address.sun_path, '\0', 108);

	createServerAcceptSocket();
	createClientSocket();
	connectClientToServer();
}


SocketTestHelper::~SocketTestHelper()
{
	close(clientSocket);
	close(serverSocket);
	close(server_accept_socket);
}


void SocketTestHelper::createServerAcceptSocket()
{
	int optionflag = 1;
	int val = 0;

	server_accept_socket = socket(addressFamily, socketType, 0);

	//manipulate fd for nonblocking mode accept socket
	val = fcntl(server_accept_socket, F_GETFL, 0);
	fcntl(server_accept_socket, F_SETFL, val|O_NONBLOCK);

	if(addressFamily == AF_UNIX)
	{
		//set uds file
		address.sun_family = AF_UNIX;
		strncpy(address.sun_path, serverAddress, strlen(serverAddress));
		unlink(serverAddress);
	}
	else if(addressFamily == AF_INET)
	{
		//TODO:
	}
	addrlen = sizeof(address);

	setsockopt(server_accept_socket, SOL_SOCKET, SO_REUSEADDR, &optionflag, sizeof(optionflag));
	bind(server_accept_socket, (struct sockaddr*)&address, addrlen);
	listen(server_accept_socket, MAX_CLIENTS);
}


void SocketTestHelper::createClientSocket()
{
	clientSocket = socket(addressFamily, socketType, 0);
}


void SocketTestHelper::connectClientToServer()
{
	bool active = true;

	while(active)
	{
		serverSocket = accept(server_accept_socket, (struct sockaddr*)&address, &addrlen);
		if( serverSocket < 0)
		{
			connect(clientSocket, (struct sockaddr*)&address, addrlen);
		}
		else
			active = false;

	}
}
