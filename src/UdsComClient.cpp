/*
 * UdsClient.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */


#include "UdsComClient.hpp"

struct sockaddr_un UdsComClient::address;
socklen_t UdsComClient::addrlen;


UdsComClient::UdsComClient(TcpWorker* tcpWorker)
{
	optionflag = 1;
	this->tcpWorker = tcpWorker;

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, UDS_COM_PATH, sizeof(UDS_COM_PATH));
	addrlen = sizeof(address);

	comWorker = new UdsComWorker(currentSocket, tcpWorker);
	connect(currentSocket, (struct sockaddr*)&address, addrlen);

}



UdsComClient::~UdsComClient()
{
	delete comWorker;

}



int UdsComClient::sendData(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}




