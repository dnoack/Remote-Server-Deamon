/*
 * UdsClient.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */


#include "UdsClient.hpp"

struct sockaddr_un UdsClient::address;
socklen_t UdsClient::addrlen;


UdsClient::UdsClient(TcpWorker* tcpWorker)
{
	optionflag = 1;
	this->tcpWorker = tcpWorker;

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, UDS_COM_PATH, sizeof(UDS_COM_PATH));
	addrlen = sizeof(address);

	udsWorker = new UdsWorker(currentSocket, tcpWorker);
	connect(currentSocket, (struct sockaddr*)&address, addrlen);


}


UdsClient::~UdsClient()
{


}


int UdsClient::sendData(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}




