/*
 * UdsClient.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */


#include "UdsComClient.hpp"

struct sockaddr_un UdsComClient::address;
socklen_t UdsComClient::addrlen;


UdsComClient::UdsComClient(TcpWorker* tcpWorker, string* udsFilePath, string* pluginName)
{
	optionflag = 1;
	this->tcpWorker = tcpWorker;
	this->udsFilePath = new string(*udsFilePath);
	this->pluginName = new string(*pluginName);

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, udsFilePath->c_str(), udsFilePath->size());
	addrlen = sizeof(address);

	comWorker = new UdsComWorker(currentSocket, tcpWorker);
	connect(currentSocket, (struct sockaddr*)&address, addrlen);

}



UdsComClient::~UdsComClient()
{
	delete comWorker;
	delete udsFilePath;
	delete pluginName;
}



int UdsComClient::sendData(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}




