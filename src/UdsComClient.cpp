/*
 * UdsClient.cpp
 *
 *  Created on: 11.02.2015
 *      Author: dnoack
 */

#include "TcpWorker.hpp"
#include "UdsComClient.hpp"

struct sockaddr_un UdsComClient::address;
socklen_t UdsComClient::addrlen;


UdsComClient::UdsComClient(TcpWorker* tcpWorker, string* udsFilePath, string* pluginName)
{
	optionflag = 1;
	this->comWorker = NULL;
	this->deletable = false;
	this->tcpWorker = tcpWorker;
	this->udsFilePath = new string(*udsFilePath);
	this->pluginName = new string(*pluginName);

	currentSocket = socket(AF_UNIX, SOCK_STREAM, 0);
	address.sun_family = AF_UNIX;
	strncpy(address.sun_path, udsFilePath->c_str(), udsFilePath->size());
	addrlen = sizeof(address);

}



UdsComClient::~UdsComClient()
{
	if(comWorker != NULL)
		delete comWorker;

	delete udsFilePath;
	delete pluginName;
}

void UdsComClient::markAsDeletable()
{
	deletable = true;
	pthread_kill(tcpWorker->getListener(), SIGUSR2);
}


int UdsComClient::sendData(string* data)
{
	return send(currentSocket, data->c_str(), data->size(), 0);
}


void UdsComClient::tcp_send(string* request)
{
	tcpWorker->tcp_send(request);
}

bool UdsComClient::tryToconnect()
{
	if( connect(currentSocket, (struct sockaddr*)&address, addrlen) < 0)
	{
		printf("Gewünschtes Plugin nicht gefunden.\n");
		return false;
	}
	else
	{
		comWorker = new UdsComWorker(currentSocket, this);
		return true;
	}
}




